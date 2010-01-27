/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* camel-pop3-folder.c : class for a pop3 folder */

/*
 * Authors:
 *   Dan Winship <danw@ximian.com>
 *   Michael Zucchi <notzed@ximian.com>
 *   Philip Van Hoof <pvanhoof@gnome.org>
 *
 * This is CamelPop3Store for camel-lite that implements CamelDiscoFolder and
 * has support for CamelFolderSummary. Its implementation is significantly
 * different from Camel's upstream version (being used by Evolution): this
 * version supports offline and online modes.
 *
 * Copyright (C) 2002 Ximian, Inc. (www.ximian.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <glib/gstdio.h>

#include <errno.h>
#include <stdlib.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>

#include <glib/gi18n-lib.h>

#include <libedataserver/md5-utils.h>

#include "camel-data-cache.h"
#include "camel-exception.h"
#include "camel-mime-message.h"
#include "camel-operation.h"

#include "camel-pop3-folder.h"
#include "camel-pop3-store.h"
#include "camel-stream-filter.h"
#include "camel-pop3-stream.h"
#include "camel-stream-mem.h"
#include "camel-string-utils.h"

#include "camel-disco-diary.h"

#include "camel-stream-buffer.h"

#define d(x)

#define CF_CLASS(o) (CAMEL_FOLDER_CLASS (CAMEL_OBJECT_GET_CLASS(o)))
static CamelFolderClass *parent_class;
static CamelDiscoFolderClass *disco_folder_class = NULL;

static void pop3_finalize (CamelObject *object);
static void pop3_refresh_info (CamelFolder *folder, CamelException *ex);
static void pop3_sync (CamelFolder *folder, gboolean expunge, CamelException *ex);
static GPtrArray *pop3_get_uids (CamelFolder *folder);
static CamelMimeMessage *pop3_get_message (CamelFolder *folder, const char *uid, CamelFolderReceiveType type, gint param, CamelException *ex);
static gboolean pop3_set_message_flags (CamelFolder *folder, const char *uid, guint32 flags, guint32 set);
static CamelMimeMessage *pop3_get_top (CamelFolder *folder, const char *uid, CamelException *ex);
static int pop3_get_local_size (CamelFolder *folder);

static void pop3_delete_attachments (CamelFolder *folder, const char *uid);
static gboolean pop3_get_allow_external_images (CamelFolder *folder, const char *uid);
static void pop3_set_allow_external_images (CamelFolder *folder, const char *uid, gboolean allow);

static void
check_dir (CamelPOP3Store *store, CamelFolder *folder)
{
	if (!store && !folder)
		return;

	if (!store && folder)
		store = (CamelPOP3Store *) folder->parent_store;

	if (!g_file_test (store->storage_path, G_FILE_TEST_IS_DIR)) 
		g_mkdir_with_parents (store->storage_path, S_IRUSR | S_IWUSR | S_IXUSR);
}

static void
pop3_finalize (CamelObject *object)
{
	CamelFolder *folder = (CamelFolder *) object;

	check_dir (NULL, folder);

	camel_folder_summary_save (folder->summary, NULL);

	return;
}

static void
camel_pop3_summary_set_extra_flags (CamelFolder *folder, CamelMessageInfoBase *mi)
{
	CamelPOP3Store *pop3_store = (CamelPOP3Store *)folder->parent_store;
	camel_data_cache_set_flags (pop3_store->cache, "cache", mi);
}

CamelFolder *
camel_pop3_folder_new (CamelStore *parent, CamelException *ex)
{
	CamelFolder *folder;
	CamelPOP3Store *p3store = (CamelPOP3Store*) parent;
	gchar *summary_file;

	d(printf("opening pop3 INBOX folder\n"));

	folder = CAMEL_FOLDER (camel_object_new (CAMEL_POP3_FOLDER_TYPE));

	camel_folder_construct (folder, parent, "inbox", "inbox");

	check_dir (p3store, NULL);

	summary_file = g_strdup_printf ("%s/summary.mmap", p3store->storage_path);
	folder->summary = camel_folder_summary_new (folder);
	if (folder->summary)
		folder->summary->set_extra_flags_func = camel_pop3_summary_set_extra_flags;

	camel_folder_summary_set_build_content (folder->summary, FALSE);
	camel_folder_summary_set_filename (folder->summary, summary_file);

	if (camel_folder_summary_load (folder->summary) == -1) {
		camel_folder_summary_clear (folder->summary);
		camel_folder_summary_touch (folder->summary);
		camel_folder_summary_save (folder->summary, ex);
		camel_folder_summary_load (folder->summary);
	}

	g_free (summary_file);


	/* mt-ok, since we dont have the folder-lock for new() */
	/* camel_folder_refresh_info (folder, ex);
	if (camel_exception_is_set (ex)) {
		camel_object_unref (CAMEL_OBJECT (folder));
		folder = NULL;
	}*/

	if (!folder->summary) {
		camel_object_unref (CAMEL_OBJECT (folder));
		camel_exception_setv (ex, CAMEL_EXCEPTION_SYSTEM_IO_READ,
				      _("Could not load summary for INBOX"));
		return NULL;
	}

	folder->folder_flags |= CAMEL_FOLDER_HAS_SUMMARY_CAPABILITY;

	return folder;
}

static int
pop3_get_local_size (CamelFolder *folder)
{
	CamelPOP3Store *p3store = CAMEL_POP3_STORE (folder->parent_store);
	int msize = 0;
	check_dir (p3store, NULL);
	camel_du (p3store->storage_path , &msize);
	return msize;
}

/* create a uid from md5 of 'top' output */
static int
cmd_builduid(CamelPOP3Engine *pe, CamelPOP3Stream *stream, void *data)
{
	CamelPOP3FolderInfo *fi = data;
	MD5Context md5;
	unsigned char digest[16];
	struct _camel_header_raw *h;
	CamelMimeParser *mp;

	/* TODO; somehow work out the limit and use that for proper progress reporting
	   We need a pointer to the folder perhaps? */
	camel_operation_progress_count(NULL, fi->id);

	md5_init(&md5);
	mp = camel_mime_parser_new();
	camel_mime_parser_init_with_stream(mp, (CamelStream *)stream);
	switch (camel_mime_parser_step(mp, NULL, NULL)) {
	case CAMEL_MIME_PARSER_STATE_HEADER:
	case CAMEL_MIME_PARSER_STATE_MESSAGE:
	case CAMEL_MIME_PARSER_STATE_MULTIPART:
		h = camel_mime_parser_headers_raw(mp);
		while (h) {
			if (h->name && g_ascii_strcasecmp(h->name, "status") != 0
			    && g_ascii_strcasecmp(h->name, "x-status") != 0) {
				md5_update(&md5, (const guchar*) h->name, strlen(h->name));
				md5_update(&md5, (const guchar*) h->value, strlen(h->value));
			}
			h = h->next;
		}
	default:
		break;
	}
	camel_object_unref(mp);
	md5_final(&md5, digest);
	fi->uid = g_base64_encode(digest, 16);

	d(printf("building uid for id '%d' = '%s'\n", fi->id, fi->uid));

	return 1;
}

static int
cmd_list(CamelPOP3Engine *pe, CamelPOP3Stream *stream, void *data)
{
	int ret;
	unsigned int len, id, size;
	unsigned char *line;
	CamelFolder *folder = data;
	CamelPOP3Store *pop3_store = (CamelPOP3Store*) folder->parent_store;
	CamelPOP3FolderInfo *fi;

	if (!pop3_store)
		return -1;

	do {
		ret = camel_pop3_stream_line(stream, &line, &len);
		if (ret>=0) {
			if (sscanf((char *) line, "%u %u", &id, &size) == 2) {
				fi = g_malloc0 (sizeof(*fi));
				fi->size = size;
				fi->id = id;
				fi->index = pop3_store->uids->len;
				if ((pop3_store->engine && pop3_store->engine->capa & CAMEL_POP3_CAP_UIDL) == 0)
					fi->cmd = camel_pop3_engine_command_new(pe, CAMEL_POP3_COMMAND_MULTI, cmd_builduid, fi, "TOP %u 0\r\n", id);
				g_ptr_array_add(pop3_store->uids, fi);
				g_hash_table_insert(pop3_store->uids_id, GINT_TO_POINTER(id), fi);
			}
		}
	} while (ret>0);

	return 1;
}

static int
cmd_uidl(CamelPOP3Engine *pe, CamelPOP3Stream *stream, void *data)
{
	int ret;
	unsigned int len;
	unsigned char *line;
	char uid[1025];
	unsigned int id;
	CamelPOP3FolderInfo *fi;
	CamelPOP3Folder *folder = data;
	CamelPOP3Store *pop3_store = (CamelPOP3Store*) ((CamelFolder *)folder)->parent_store;

	do {
		ret = camel_pop3_stream_line(stream, &line, &len);
		if (ret>=0) {
			if (strlen((char*) line) > 1024)
				line[1024] = 0;
			if (sscanf((char *) line, "%u %s", &id, uid) == 2) {
				fi = g_hash_table_lookup(pop3_store->uids_id, GINT_TO_POINTER(id));
				if (fi) {
					camel_operation_progress(NULL, (fi->index+1) , pop3_store->uids->len);
					fi->uid = g_strdup(uid);
					pop3_debug ("%s added\n", fi->uid);
					g_hash_table_insert(pop3_store->uids_uid, fi->uid, fi);
				} else {
					g_warning("ID %u (uid: %s) not in previous LIST output", id, uid);
				}
			}
		}
	} while (ret>0);

	return 1;
}

static void
pop3_refresh_info (CamelFolder *folder, CamelException *ex)
{
	CamelPOP3Store *pop3_store = CAMEL_POP3_STORE (folder->parent_store);
	CamelPOP3Command *pcl, *pcu = NULL;
	int i, hcnt = 0, lcnt = 0;
	CamelFolderChangeInfo *changes = NULL;
	GList *deleted = NULL, *copy;
	gint max;

	if (camel_disco_store_status (CAMEL_DISCO_STORE (pop3_store)) == CAMEL_DISCO_STORE_OFFLINE)
		return;

	g_static_rec_mutex_lock (pop3_store->eng_lock);
	pop3_store->is_refreshing = TRUE;
	if (pop3_store->engine == NULL)
	{
		camel_service_connect (CAMEL_SERVICE (pop3_store), ex);
		if (camel_exception_is_set (ex)) {
			pop3_store->is_refreshing = FALSE;
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			return;
		}
	}

	camel_operation_start (NULL, _("Fetching summary information for new messages in folder"));

	if (pop3_store->engine == NULL) {
		pop3_store->is_refreshing = FALSE;
		g_static_rec_mutex_unlock (pop3_store->eng_lock);
		goto mfail;
	}

	pcl = camel_pop3_engine_command_new(pop3_store->engine, CAMEL_POP3_COMMAND_MULTI, cmd_list, folder, "LIST\r\n");
	if (pop3_store->engine->capa & CAMEL_POP3_CAP_UIDL)
		pcu = camel_pop3_engine_command_new(pop3_store->engine, CAMEL_POP3_COMMAND_MULTI, cmd_uidl, folder, "UIDL\r\n");
	while ((i = camel_pop3_engine_iterate(pop3_store->engine, NULL)) > 0)
		;

	g_static_rec_mutex_unlock (pop3_store->eng_lock);

	if (i == -1) {
		if (errno == EINTR)
			camel_exception_setv(ex, CAMEL_EXCEPTION_USER_CANCEL, _("User canceled"));
		else
			camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
					      _("Cannot get POP summary: %s"),
					      g_strerror (errno));
	}

	/* TODO: check every id has a uid & commands returned OK too? */

	g_static_rec_mutex_lock (pop3_store->eng_lock);

	if (pop3_store->engine == NULL) {
		pop3_store->is_refreshing = FALSE;
		g_static_rec_mutex_unlock (pop3_store->eng_lock);
		goto mfail;
	}

	camel_pop3_engine_command_free(pop3_store->engine, pcl);

	g_static_rec_mutex_unlock (pop3_store->eng_lock);

	/* Update the summary.mmap file */

	check_dir (pop3_store, NULL);

	camel_folder_summary_prepare_hash (folder->summary);

	camel_pop3_logbook_open (pop3_store->book);

	for (i=0;i<pop3_store->uids->len;i++) {
		CamelPOP3FolderInfo *fi = pop3_store->uids->pdata[i];
		CamelMessageInfoBase *mi = NULL;

		if (!fi || !fi->uid)
			continue;

		mi = (CamelMessageInfoBase*) camel_folder_summary_uid (folder->summary, fi->uid);

		if (!mi && !camel_pop3_logbook_is_registered (pop3_store->book, fi->uid)) {
			CamelMimeMessage *msg = NULL;

			g_static_rec_mutex_lock (pop3_store->eng_lock);

			if (pop3_store->engine == NULL) {
				g_static_rec_mutex_unlock (pop3_store->eng_lock);
				continue;
			}

			if (pop3_store->engine && pop3_store->engine->capa & CAMEL_POP3_CAP_TOP) {
				msg = pop3_get_top (folder, fi->uid, NULL);
				if (!msg && (!(pop3_store->engine && pop3_store->engine->capa & CAMEL_POP3_CAP_TOP)))
					msg = pop3_get_message (folder, fi->uid, CAMEL_FOLDER_RECEIVE_FULL, -1, NULL);
			} else if (pop3_store->engine)
				msg = pop3_get_message (folder, fi->uid, CAMEL_FOLDER_RECEIVE_FULL, -1, NULL);

			g_static_rec_mutex_unlock (pop3_store->eng_lock);

			if (msg)
			{

				mi = (CamelMessageInfoBase*) camel_folder_summary_uid (folder->summary, fi->uid);
				if (mi) {
					mi->size = (fi->size);
					camel_message_info_free (mi);
				}

				struct _camel_header_raw *h;
				
				h = ((CamelMimePart *)msg)->headers;
				if (camel_header_raw_find(&h, "X-MSMail-Priority", NULL) &&
				    !camel_header_raw_find(&h, "X-MS-Has-Attach", NULL)) {
					mi = (CamelMessageInfoBase*) camel_folder_summary_uid (folder->summary, fi->uid);
					if (mi) {
						mi->size = (fi->size);
						((CamelMessageInfoBase *)mi)->flags &= ~CAMEL_MESSAGE_ATTACHMENTS;
						camel_message_info_free (mi);
					}
				} else if (!camel_header_raw_find (&h, "X-MS-Has-Attach", NULL)) {
					
					mi = (CamelMessageInfoBase*) camel_folder_summary_uid (folder->summary, fi->uid);
					if (mi) {
						mi->size = (fi->size);
						/* TNY TODO: This is a hack! But else we need to parse
						 * BODYSTRUCTURE (and I'm lazy). It needs fixing though. */
						if (mi->size > 102400)
							mi->flags |= CAMEL_MESSAGE_ATTACHMENTS;
						/* ... it does */
						camel_message_info_free (mi);
					}
				}


				camel_object_unref (CAMEL_OBJECT (msg));

				if (!changes)
					changes = camel_folder_change_info_new ();
				camel_folder_change_info_add_uid (changes, fi->uid);

				lcnt++;
				if (lcnt > 20)
				{
					if (camel_folder_change_info_changed (changes))
						camel_object_trigger_event (CAMEL_OBJECT (folder), "folder_changed", changes);
					camel_folder_change_info_free (changes);
					changes = NULL;
					lcnt = 0;
				}

				hcnt++;
				if (hcnt > 1000)
				{
					/* Periodically save the summary (this reduces
					   memory usage too) */
					camel_folder_summary_save (folder->summary, ex);
					hcnt = 0;
				}

				camel_pop3_logbook_register (pop3_store->book, fi->uid);

			} if (!pop3_store->engine)
				break;


		} else if (mi) {
			mi->size = fi->size;
			camel_folder_summary_touch (folder->summary);
			camel_message_info_free (mi);
		}

		camel_operation_progress (NULL, i , pop3_store->uids->len);

	}


	

	g_static_rec_mutex_lock (pop3_store->eng_lock);

	if (pop3_store->engine == NULL) {
		pop3_store->is_refreshing = FALSE;
		g_static_rec_mutex_unlock (pop3_store->eng_lock);
		goto mfail;
	}

	if (pop3_store->engine)
	{
		if (pop3_store->engine->capa & CAMEL_POP3_CAP_UIDL) {
			camel_pop3_engine_command_free(pop3_store->engine, pcu);
		} else {
			for (i=0;i<pop3_store->uids->len;i++) {
				CamelPOP3FolderInfo *fi = pop3_store->uids->pdata[i];

				if (fi->cmd) {
					camel_pop3_engine_command_free(pop3_store->engine, fi->cmd);
					fi->cmd = NULL;
				}
				if (fi->uid)
					g_hash_table_insert(pop3_store->uids_uid, fi->uid, fi);
			}
		}
	}

	max = camel_folder_summary_count (folder->summary);
	for (i = 0; i < max; i++) {
		CamelMessageInfoBase *info;
		gboolean found = FALSE;
		gint t=0;

		info = (CamelMessageInfoBase*) camel_folder_summary_index (folder->summary, i);
		if (!info)
			continue;
		for (t=0; t < pop3_store->uids->len; t++)
		{
			CamelPOP3FolderInfo *fi2 = pop3_store->uids->pdata[t];
			if (info && fi2 && fi2->uid && info->uid && !strcmp (fi2->uid, info->uid)) {
				found = TRUE;
				break;
			}
		}

		if (!found) {
			/* Removed remotely (only if not cached locally) */
			if (info && info->uid && !camel_data_cache_exists (pop3_store->cache, "cache", info->uid, NULL)) {

				camel_message_info_ref (info);
				deleted = g_list_prepend (deleted, info);
			}
		}


		camel_message_info_free((CamelMessageInfo *)info);
	}

	copy = deleted;
	while (deleted)
	{
		CamelMessageInfo *info = deleted->data;

		if (!changes)
			changes = camel_folder_change_info_new ();
		camel_folder_change_info_remove_uid (changes, info->uid);
		((CamelMessageInfoBase*)info)->flags |= CAMEL_MESSAGE_EXPUNGED;
		camel_folder_summary_remove (folder->summary, info);
		camel_message_info_free(info);
		deleted = g_list_next (deleted);
	}

	if (copy)
		g_list_free (copy);

	if (changes && camel_folder_change_info_changed (changes)) {
		camel_object_trigger_event (CAMEL_OBJECT (folder), "folder_changed", changes);
		camel_folder_change_info_free (changes);
		changes = NULL;
	}


	camel_pop3_logbook_close (pop3_store->book);

	check_dir (pop3_store, NULL);
	camel_folder_summary_save (folder->summary, ex);
	camel_folder_summary_kill_hash (folder->summary);

	g_static_rec_mutex_unlock (pop3_store->eng_lock);


mfail:

	pop3_store->is_refreshing = FALSE;

	/* dont need this anymore */
	camel_operation_end (NULL);

	return;
}


static void
pop3_sync (CamelFolder *folder, gboolean expunge, CamelException *ex)
{
	CamelPOP3Store *pop3_store;
	CamelPOP3Command *pcl, *pcu = NULL;
	int i, max;
	CamelMessageInfoBase *info;
	GList *deleted = NULL;
	CamelFolderChangeInfo *changes = NULL;

	pop3_store = CAMEL_POP3_STORE (folder->parent_store);

	check_dir (pop3_store, NULL);

	g_static_rec_mutex_lock (pop3_store->eng_lock);
	pop3_store->is_refreshing = TRUE;
	if (pop3_store->engine == NULL)
	{
		camel_service_connect (CAMEL_SERVICE (pop3_store), ex);
		if (camel_exception_is_set (ex)) {
			pop3_store->is_refreshing = FALSE;
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			return;
		}
	}

	if(pop3_store->delete_after && !expunge)
	{
		camel_operation_start(NULL, _("Expunging old messages"));
		camel_pop3_delete_old(folder, pop3_store->delete_after,ex);
		camel_operation_end (NULL);
	}

	g_static_rec_mutex_unlock (pop3_store->eng_lock);

	if (!expunge)
		return;

	camel_operation_start(NULL, _("Expunging deleted messages"));

	max = camel_folder_summary_count (folder->summary);
	for (i = 0; i < max; i++) {

		info = (CamelMessageInfoBase*) camel_folder_summary_index (folder->summary, i);

		if (!info)
			continue;

		if (info->flags & CAMEL_MESSAGE_DELETED)
		{
			struct _CamelPOP3Command *cmd;

			g_static_rec_mutex_lock (pop3_store->eng_lock);

			if (pop3_store->cache && info->uid)
				camel_data_cache_remove(pop3_store->cache, "cache", info->uid, NULL);

			if (expunge) {
				CamelPOP3FolderInfo *fi = NULL;

				if (pop3_store->engine == NULL) {
					g_static_rec_mutex_unlock (pop3_store->eng_lock);
					return;
				}

				fi = g_hash_table_lookup (pop3_store->uids_uid, info->uid);
				if (fi) {
					cmd = camel_pop3_engine_command_new(pop3_store->engine,
									    0, NULL, NULL, "DELE %u\r\n",
									    fi->id);
					while (camel_pop3_engine_iterate(pop3_store->engine, cmd) > 0);
					camel_pop3_engine_command_free(pop3_store->engine, cmd);
				}
			}
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
		}

		if (info->flags & CAMEL_MESSAGE_DELETED) {
			camel_message_info_ref (info);
			deleted = g_list_prepend (deleted, info);
		} else {
			gboolean found = FALSE;
			gint t=0;
			for (t=0; t < pop3_store->uids->len; t++)
			{
				CamelPOP3FolderInfo *fi2 = pop3_store->uids->pdata[t];
				if (info && fi2 && fi2->uid && info->uid && !strcmp (fi2->uid, info->uid)) {
					found = TRUE;
					break;
				}
			}

			if (!found) {
				/* Removed remotely (only if not cached locally) */
				if (info && info->uid && !camel_data_cache_exists (pop3_store->cache, "cache", info->uid, NULL)) {
					camel_message_info_ref (info);
					deleted = g_list_prepend (deleted, info);
				}
			}
		}

		camel_message_info_free((CamelMessageInfo *)info);
	}

	while (deleted) {
		CamelMessageInfo *info = deleted->data;

		if (!changes)
			changes = camel_folder_change_info_new ();
		camel_folder_change_info_remove_uid (changes, info->uid);
		((CamelMessageInfoBase*)info)->flags |= CAMEL_MESSAGE_EXPUNGED;
		camel_folder_summary_remove (folder->summary, info);
		camel_message_info_free(info);
		deleted = g_list_next (deleted);
	}


	if (changes) {
		camel_object_trigger_event (CAMEL_OBJECT (folder), "folder_changed", changes);
		camel_folder_change_info_free (changes);
	}

	camel_operation_end(NULL);

	g_static_rec_mutex_lock (pop3_store->eng_lock);
	camel_pop3_store_expunge (pop3_store, ex);
	g_static_rec_mutex_unlock (pop3_store->eng_lock);

	camel_folder_summary_save (folder->summary, ex);

	return;

mfail:

	pop3_store->is_refreshing = FALSE;

	/* dont need this anymore */
	camel_operation_end (NULL);

	return;

}


static gboolean
pop3_get_message_time_from_cache (CamelFolder *folder, const char *uid, time_t *message_time)
{
	CamelPOP3Store *pop3_store;
	CamelStream *stream = NULL;
	char buffer[1];
	gboolean res = FALSE;

	g_return_val_if_fail (folder != NULL, FALSE);
	g_return_val_if_fail (uid != NULL, FALSE);
	g_return_val_if_fail (message_time != NULL, FALSE);

	pop3_store = CAMEL_POP3_STORE (folder->parent_store);

	g_return_val_if_fail (pop3_store->cache != NULL, FALSE);

	if ((stream = camel_data_cache_get (pop3_store->cache, "cache", uid, NULL)) != NULL
	    && camel_stream_read (stream, buffer, 1) == 1
	    && buffer[0] == '#') {
		CamelMimeMessage *message;

		camel_object_ref ((CamelObject *)stream);

		message = camel_mime_message_new ();
		if (camel_data_wrapper_construct_from_stream ((CamelDataWrapper *)message, stream) == -1) {
			g_warning (_("Cannot get message %s: %s"), uid, g_strerror (errno));
			camel_object_unref ((CamelObject *)message);
			message = NULL;
		}

		if (message) {
			res = TRUE;
			*message_time = message->date + message->date_offset;

			camel_object_unref ((CamelObject *)message);
		}

		camel_object_unref ((CamelObject *)stream);
	}

	return res;
}

int
camel_pop3_delete_old(CamelFolder *folder, int days_to_delete,	CamelException *ex)
{
	CamelPOP3Folder *pop3_folder;
	CamelPOP3FolderInfo *fi;
	int i;
	CamelPOP3Store *pop3_store;
	time_t temp, message_time;

	pop3_folder = CAMEL_POP3_FOLDER (folder);
	pop3_store = CAMEL_POP3_STORE (CAMEL_FOLDER(pop3_folder)->parent_store);
	temp = time(&temp);

	if (camel_disco_store_status (CAMEL_DISCO_STORE (pop3_store)) == CAMEL_DISCO_STORE_OFFLINE)
		return -1;

	g_static_rec_mutex_lock (pop3_store->eng_lock);
	if (pop3_store->engine == NULL)
	{
		camel_service_connect (CAMEL_SERVICE (pop3_store), ex);
		if (camel_exception_is_set (ex)) {
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			return -1;
		}
	}

	d(printf("%s(%d): pop3_folder->uids->len=[%d]\n", __FILE__, __LINE__, pop3_folder->uids->len));
	for (i = 0; i < pop3_store->uids->len; i++) {
		fi = pop3_store->uids->pdata[i];

		d(printf("%s(%d): fi->uid=[%s]\n", __FILE__, __LINE__, fi->uid));
		if (pop3_get_message_time_from_cache (folder, fi->uid, &message_time)) {
			double time_diff = difftime(temp,message_time);
			int day_lag = time_diff/(60*60*24);

			d(printf("%s(%d): message_time= [%ld]\n", __FILE__, __LINE__, message_time));
			d(printf("%s(%d): day_lag=[%d] \t days_to_delete=[%d]\n",
				 __FILE__, __LINE__, day_lag, days_to_delete));

			if( day_lag > days_to_delete)
			{
				if (fi->cmd) {
					while (camel_pop3_engine_iterate(pop3_store->engine, fi->cmd) > 0) {
						; /* do nothing - iterating until end */
					}

					camel_pop3_engine_command_free(pop3_store->engine, fi->cmd);
					fi->cmd = NULL;
				}
				d(printf("%s(%d): Deleting old messages\n", __FILE__, __LINE__));
				fi->cmd = camel_pop3_engine_command_new(pop3_store->engine,
									0,
									NULL,
									NULL,
									"DELE %u\r\n",
									fi->id);
				/* also remove from cache */
				if (pop3_store->cache && fi->uid) {
					camel_data_cache_remove(pop3_store->cache, "cache", fi->uid, NULL);
				}
			}
		}
	}

	for (i = 0; i < pop3_store->uids->len; i++) {
		fi = pop3_store->uids->pdata[i];
		/* wait for delete commands to finish */
		if (fi->cmd) {
			while (camel_pop3_engine_iterate(pop3_store->engine, fi->cmd) > 0)
				;
			camel_pop3_engine_command_free(pop3_store->engine, fi->cmd);
			fi->cmd = NULL;
		}
		camel_operation_progress(NULL, (i+1) , pop3_store->uids->len);
	}

	camel_operation_end(NULL);

	camel_pop3_store_expunge (pop3_store, ex);

	g_static_rec_mutex_unlock (pop3_store->eng_lock);

	return 0;
}


static int
cmd_tocache(CamelPOP3Engine *pe, CamelPOP3Stream *stream, void *data)
{
	CamelPOP3Store *tstore = (CamelPOP3Store *) pe->store;
	CamelPOP3FolderInfo *fi = data;
	char buffer[2049];
	int w = 0, n;
	int retval = 1;

	/* What if it fails? */
	/* We write an '*' to the start of the stream to say its not complete yet */
	/* This should probably be part of the cache code */

	if ((n = camel_stream_write(fi->stream, "*", 1)) == -1)
		goto done;

	while ((n = camel_stream_read((CamelStream *)stream, buffer, sizeof(buffer) - 1)) > 0) {
		n = camel_stream_write(fi->stream, buffer, n);
		if (n == -1)
			break;

		buffer[n] = '\0';
		/* heuristics */
		if (camel_strstrcase (buffer, "Content-Disposition: attachment") != NULL)
			fi->has_attachments = TRUE;
		else if (camel_strstrcase (buffer, "filename=") != NULL &&
			 strchr (buffer, '.'))
			fi->has_attachments = TRUE;
		else if (camel_strstrcase (buffer, "Content-Type: message/rfc822") != NULL)
			fi->has_attachments = TRUE;

		w += n;
		if (w > fi->size)
			w = fi->size;
		if (fi->size != 0)
			camel_operation_progress(NULL, w , fi->size);
	}

	/* it all worked, output a '#' to say we're a-ok */
	if (n != -1) {
		camel_stream_reset(fi->stream);
		n = camel_stream_write(fi->stream, "#", 1);

		camel_data_cache_set_partial (tstore->cache, "cache", fi->uid, FALSE);
	}
done:
	if (n == -1) {
		fi->err = errno;
		retval = 0;
		g_warning("POP3 retrieval failed: %s", strerror(errno));
	} else {
		fi->err = 0;
	}

	camel_object_unref((CamelObject *)fi->stream);
	fi->stream = NULL;

	return retval;
}




static int
cmd_totop(CamelPOP3Engine *pe, CamelPOP3Stream *stream, void *data)
{
	CamelPOP3Store *tstore = (CamelPOP3Store *) pe->store;
	CamelPOP3FolderInfo *fi = data;
	char buffer[2049];
	int w = 0, n;
	int retval = 1;

	/* What if it fails? */
	/* We write an '*' to the start of the stream to say its not complete yet */
	/* This should probably be part of the cache code */

	if ((n = camel_stream_write(fi->stream, "*", 1)) == -1)
		goto done;

	while ((n = camel_stream_read((CamelStream *)stream, buffer, sizeof(buffer) - 1)) > 0) {
		n = camel_stream_write(fi->stream, buffer, n);
		if (n == -1)
			break;

		buffer[n] = '\0';
		/* heuristics */
		if (camel_strstrcase (buffer, "Content-Disposition: attachment") != NULL)
			fi->has_attachments = TRUE;
		else if (camel_strstrcase (buffer, "filename=") != NULL &&
			 strchr (buffer, '.'))
			fi->has_attachments = TRUE;
		else if (camel_strstrcase (buffer, "Content-Type: message/rfc822") != NULL)
			fi->has_attachments = TRUE;

		w += n;
		if (w > fi->size)
			w = fi->size;
/*		if (fi->size != 0)
			camel_operation_progress(NULL, w , fi->size);*/
	}

	/* it all worked, output a '#' to say we're a-ok */
	if (n != -1) {
		camel_stream_reset(fi->stream);
		n = camel_stream_write(fi->stream, "#", 1);

		camel_data_cache_set_partial (tstore->cache, "cache", fi->uid, FALSE);
	}
done:
	if (n == -1) {
		fi->err = errno;
		retval = 0;
		g_warning("POP3 retrieval failed: %s", strerror(errno));
	} else {
		fi->err = 0;
	}

	camel_object_unref((CamelObject *)fi->stream);
	fi->stream = NULL;

	return retval;
}

static int
cmd_tocache_partial (CamelPOP3Engine *pe, CamelPOP3Stream *stream, void *data)
{
	CamelPOP3Store *tstore = (CamelPOP3Store *) pe->store;
	CamelPOP3FolderInfo *fi = data;
	unsigned char *buffer;
	int w = 0, n;
	gchar *boundary = NULL;
	gboolean occurred = FALSE, theend = FALSE;
	unsigned int len;
	int retval = 1;

	/* We write an '*' to the start of the stream to say its not complete yet */
	if ((n = camel_stream_write(fi->stream, "*", 1)) == -1)
		goto done;


	while (!theend && camel_pop3_stream_line (stream, &buffer, &len) > 0)
	{
		if (!buffer)
			continue;

		/* heuristics */
		if (buffer && camel_strstrcase ((const char *) buffer, "Content-Disposition: attachment") != NULL)
			fi->has_attachments = TRUE;
		else if (buffer && camel_strstrcase ((const char *) buffer, "filename=") != NULL &&
			 strchr ((const char *) buffer, '.'))
			fi->has_attachments = TRUE;
		else if (buffer && camel_strstrcase ((const char *) buffer, "Content-Type: message/rfc822") != NULL)
			fi->has_attachments = TRUE;

		if (boundary == NULL && buffer)
		{
			   CamelContentType *ct = NULL;
			   const char *bound=NULL;
			   char *pstr = (char*)camel_strstrcase ((const char *) buffer, "Content-Type:");

			   if (pstr)
			   {
				pstr = strchr (pstr, ':');
				if (pstr) { pstr++;
				ct = camel_content_type_decode(pstr); }
			   }

			   if (ct)
			   {
				bound = camel_content_type_param(ct, "boundary");
				if (bound && strlen (bound) > 0)
					boundary = g_strdup (bound);
			   }
		} else if (buffer && strstr ((const char*) buffer, (const char*) boundary))
		{
			if (occurred)
			{
				CamelException myex = CAMEL_EXCEPTION_INITIALISER;
				camel_service_disconnect (CAMEL_SERVICE (tstore), FALSE, &myex);
				camel_service_connect (CAMEL_SERVICE (tstore), &myex);
				pe->partial_happening = TRUE;
				theend = TRUE;
			}

			occurred = TRUE;
		}

		if (!theend && buffer)
		{
		    n = camel_stream_write(fi->stream, (const char*) buffer, len);
		    if (n == -1 || camel_stream_write(fi->stream, "\n", 1) == -1)
			break;
		    w += n+1;
		} else if (boundary != NULL)
		{
		    gchar *nb = g_strdup_printf ("\n--%s\n", boundary);
		    n = camel_stream_write(fi->stream, nb, strlen (nb));
		    g_free (nb);
		}

		if (w > fi->size)
			w = fi->size;
		if (fi->size != 0)
			camel_operation_progress(NULL, w , fi->size);
	}

	/* it all worked, output a '#' to say we're a-ok */
	if (n != -1 || theend) {
		camel_stream_reset(fi->stream);
		n = camel_stream_write(fi->stream, "#", 1);
		if (theend)
			camel_data_cache_set_partial (tstore->cache, "cache", fi->uid, TRUE);
		else
 			camel_data_cache_set_partial (tstore->cache, "cache", fi->uid, FALSE);

	}
done:
	if (n == -1 && !theend) {
		fi->err = errno;
		retval = 0;
		g_warning("POP3 retrieval failed: %s", strerror(errno));
	} else {
		fi->err = 0;
	}

	camel_object_unref((CamelObject *)fi->stream);
	fi->stream = NULL;

	if (boundary)
		g_free (boundary);

	return retval;
}


static void
pop3_delete_attachments (CamelFolder *folder, const char *uid)
{
	CamelPOP3Store *pop3_store = CAMEL_POP3_STORE (folder->parent_store);
	camel_data_cache_delete_attachments (pop3_store->cache, "cache", uid);
	return;
}

static gboolean
pop3_get_allow_external_images (CamelFolder *folder, const char *uid)
{
	gboolean retval;
	CamelPOP3Store *pop3_store = CAMEL_POP3_STORE (folder->parent_store);
	retval = camel_data_cache_get_allow_external_images (pop3_store->cache, "cache", uid);
	return retval;
}

static void
pop3_set_allow_external_images (CamelFolder *folder, const char *uid, gboolean allow)
{
	gboolean retval;
	CamelPOP3Store *pop3_store = CAMEL_POP3_STORE (folder->parent_store);
	camel_data_cache_set_allow_external_images (pop3_store->cache, "cache", uid, allow);
	return;
}

static CamelMimeMessage *
pop3_get_message (CamelFolder *folder, const char *uid, CamelFolderReceiveType type, gint param, CamelException *ex)
{
	CamelMimeMessage *message = NULL;
	CamelPOP3Store *pop3_store = CAMEL_POP3_STORE (folder->parent_store);
	CamelPOP3Folder *pop3_folder = (CamelPOP3Folder *)folder;
	CamelPOP3Command *pcr=NULL;
	CamelPOP3FolderInfo *fi;
	char buffer[1]; int i;
	CamelStream *stream = NULL;
	CamelFolderSummary *summary = folder->summary;
	CamelMessageInfoBase *mi; gboolean im_certain=FALSE;
	gint retry = 0;
	gboolean had_attachment = FALSE;
	gboolean free_ex = FALSE;

	if (!ex) {
		free_ex = TRUE;
		ex = camel_exception_new ();
	}		

	if (!uid)
		goto do_free_ex;

	pop3_debug ("%s requested\n", uid);

	check_dir (pop3_store, NULL);

	stream = camel_data_cache_get (pop3_store->cache, "cache", uid, NULL);
	if (stream)
	{
		message = camel_mime_message_new ();
		if (camel_data_wrapper_construct_from_stream((CamelDataWrapper *)message, stream) == -1) {
			if (errno == EINTR)
				camel_exception_setv(ex, CAMEL_EXCEPTION_USER_CANCEL, "User canceled");
			else
				camel_exception_setv (ex, CAMEL_EXCEPTION_FOLDER_INVALID_UID,
						      "Cannot get message %s: %s",
						      uid, g_strerror (errno));
			if (message)
				camel_object_unref (message);
			message = NULL;
		}

		camel_object_unref (CAMEL_OBJECT (stream));

		goto do_free_ex;
	}


	if (camel_disco_store_status (CAMEL_DISCO_STORE (pop3_store)) == CAMEL_DISCO_STORE_OFFLINE) {
		camel_exception_set (ex, CAMEL_EXCEPTION_FOLDER_UID_NOT_AVAILABLE,
			     _("This message is not currently available"));
		goto do_free_ex;
	}

	g_static_rec_mutex_lock (pop3_store->uidl_lock);

	if (pop3_store->uids_uid && uid)
		fi = g_hash_table_lookup(pop3_store->uids_uid, uid);
	else 
		fi = NULL;


	if (fi == NULL) 
	{
		CamelPOP3Command *pcl, *pcu = NULL;

		g_static_rec_mutex_lock (pop3_store->eng_lock);
		if (!pop3_store->is_refreshing) 
		{
			if (pop3_store->engine == NULL)
			{
				camel_service_connect (CAMEL_SERVICE (pop3_store), ex);
				if (camel_exception_is_set (ex)) {
					g_static_rec_mutex_unlock (pop3_store->eng_lock);
					goto rfail;
				}
			}

			camel_pop3_store_destroy_lists (pop3_store);
			pcl = camel_pop3_engine_command_new(pop3_store->engine, CAMEL_POP3_COMMAND_MULTI, cmd_list, folder, "LIST\r\n");
			if (pop3_store->engine && pop3_store->engine->capa & CAMEL_POP3_CAP_UIDL)
				pcu = camel_pop3_engine_command_new(pop3_store->engine, CAMEL_POP3_COMMAND_MULTI, cmd_uidl, folder, "UIDL\r\n");
			if (pop3_store->engine) {
			  while ((i = camel_pop3_engine_iterate(pop3_store->engine, NULL)) > 0)
				;
			}
			fi = g_hash_table_lookup(pop3_store->uids_uid, uid);
		}
		g_static_rec_mutex_unlock (pop3_store->eng_lock);

rfail:
		if (fi == NULL) {

			/* This means that we have a UIDL locally, that we no longer
			 * have remotely (POP servers sometimes do this kind of
			 * funny things, especially if they have web access and
			 * the user has a delete-mail finger  ... ) */

			if (camel_operation_cancel_check (NULL)) {
				camel_exception_setv(ex, CAMEL_EXCEPTION_USER_CANCEL,
					"User canceled message retrieval");
			} else {
				CamelFolder *folder = (CamelFolder *) pop3_folder;
				CamelMessageInfo *mi = camel_folder_summary_uid (folder->summary, uid);
				if (mi) {
					CamelFolderChangeInfo *changes = camel_folder_change_info_new ();
					((CamelMessageInfoBase*)mi)->flags |= CAMEL_MESSAGE_EXPUNGED;
					if (mi->uid) {
						camel_folder_change_info_remove_uid (changes, mi->uid);
						camel_object_trigger_event (CAMEL_OBJECT (folder), "folder_changed", changes);
					}
					camel_folder_summary_remove (folder->summary, mi);
					camel_folder_change_info_free (changes);
					camel_message_info_free (mi);
				}

				camel_exception_setv (ex, CAMEL_EXCEPTION_FOLDER_UID_NOT_AVAILABLE,
				      "Message with UID %s is not currently available on the POP server. "
				      "If you have a web E-mail account with POP access, make sure "
				      "that you select to export all E-mails over POP for mail, "
				      "the ones that have already been downloaded too. Also verify "
				      "whether other E-mail clients that use the account are not "
				      "configured to automatically delete E-mails from the POP server." ,
				      uid);
			}

			g_static_rec_mutex_unlock (pop3_store->uidl_lock);
			goto do_free_ex;
		}
	}

	/* Sigh, most of the crap in this function is so that the cancel button
	   returns the proper exception code.  Sigh. */

	if (camel_disco_store_status (CAMEL_DISCO_STORE (pop3_store)) == CAMEL_DISCO_STORE_OFFLINE)
	{
		camel_exception_setv (ex, CAMEL_EXCEPTION_FOLDER_UID_NOT_AVAILABLE,
		  "Message with UID %s is not currently available", uid);

		g_static_rec_mutex_unlock (pop3_store->uidl_lock);
		goto do_free_ex;
	}

   while (retry < 2)
   {

	g_static_rec_mutex_lock (pop3_store->eng_lock);
	if (pop3_store->engine == NULL)
	{
		camel_service_connect (CAMEL_SERVICE (pop3_store), ex);
		if (camel_exception_is_set (ex)) {
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			g_static_rec_mutex_unlock (pop3_store->uidl_lock);
			goto do_free_ex;
		}
	}
	g_static_rec_mutex_unlock (pop3_store->eng_lock);

	camel_operation_start (NULL, _("Retrieving message"));

	/* If we have an oustanding retrieve message running, wait for that to complete
	   & then retrieve from cache, otherwise, start a new one, and similar */

	if (fi->cmd != NULL) {

		g_static_rec_mutex_lock (pop3_store->eng_lock);

		if (pop3_store->engine == NULL) {
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			goto fail;
		}

		while ((i = camel_pop3_engine_iterate(pop3_store->engine, fi->cmd)) > 0)
			;

		g_static_rec_mutex_unlock (pop3_store->eng_lock);

		if (i == -1)
			fi->err = errno;

		/* getting error code? */
		/* g_assert (fi->cmd->state == CAMEL_POP3_COMMAND_DATA); */

		g_static_rec_mutex_lock (pop3_store->eng_lock);
		if (pop3_store->engine == NULL) {
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			goto fail;
		}
		camel_pop3_engine_command_free(pop3_store->engine, fi->cmd);
		g_static_rec_mutex_unlock (pop3_store->eng_lock);

		fi->cmd = NULL;

		if (fi->err != 0) {
			if (fi->err == EINTR)
				camel_exception_setv(ex, CAMEL_EXCEPTION_USER_CANCEL,
					"User canceled message retrieval");
			else
				camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
					"Failure while retrieving message %s from POP server: %s",
					uid, g_strerror (fi->err));
			goto fail;
		}
	}

	if (pop3_store->cache != NULL)
	{
		CamelException tex = CAMEL_EXCEPTION_INITIALISER;

		if ((type & CAMEL_FOLDER_RECEIVE_FULL) && camel_data_cache_is_partial (pop3_store->cache, "cache", fi->uid))
		{

			camel_data_cache_remove (pop3_store->cache, "cache", fi->uid, &tex);
			im_certain = TRUE;

		} else if ((type & CAMEL_FOLDER_RECEIVE_PARTIAL)
			&& !camel_data_cache_is_partial (pop3_store->cache, "cache", fi->uid))
		{
			camel_data_cache_remove (pop3_store->cache, "cache", fi->uid, &tex);
			im_certain = TRUE;
		}
	}

	/* check to see if we have safely written flag set */
	if (im_certain || pop3_store->cache == NULL
	    || (stream = camel_data_cache_get(pop3_store->cache, "cache", fi->uid, NULL)) == NULL
	    || camel_stream_read(stream, buffer, 1) != 1
	    || buffer[0] != '#') {

		/* Initiate retrieval, if disk backing fails, use a memory backing */
		if (pop3_store->cache == NULL
		    || (stream = camel_data_cache_add(pop3_store->cache, "cache", fi->uid, ex)) == NULL) {
			/* stream = camel_stream_mem_new(); */
			g_static_rec_mutex_unlock (pop3_store->uidl_lock);
			goto do_free_ex;
		}

		/* ref it, the cache storage routine unref's when done */
		camel_object_ref((CamelObject *)stream);
		fi->stream = stream;
		fi->err = EIO;


		g_static_rec_mutex_lock (pop3_store->eng_lock);

		if (pop3_store->engine == NULL) {
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			goto done;
		}

		pop3_store->engine->type = type;
		pop3_store->engine->param = param;
		had_attachment = FALSE;
		fi->has_attachments = FALSE;

		if (type & CAMEL_FOLDER_RECEIVE_FULL || type & CAMEL_FOLDER_RECEIVE_ANY_OR_FULL) {

			pcr = camel_pop3_engine_command_new(pop3_store->engine, CAMEL_POP3_COMMAND_MULTI,
				cmd_tocache, fi, "RETR %u\r\n", fi->id);
		}
		else if (type & CAMEL_FOLDER_RECEIVE_PARTIAL || type & CAMEL_FOLDER_RECEIVE_ANY_OR_PARTIAL) {
			pcr = camel_pop3_engine_command_new(pop3_store->engine, CAMEL_POP3_COMMAND_MULTI,
				cmd_tocache_partial, fi, "RETR %u\r\n", fi->id);
		}

		while ((i = camel_pop3_engine_iterate(pop3_store->engine, pcr)) > 0)
			;
		if (i == -1)
			fi->err = errno;

		had_attachment = fi->has_attachments;

		/* getting error code? */
		/*g_assert (pcr->state == CAMEL_POP3_COMMAND_DATA);*/

		camel_pop3_engine_command_free(pop3_store->engine, pcr);

		g_static_rec_mutex_unlock (pop3_store->eng_lock);

		camel_stream_reset(stream);

		/* Check to see we have safely written flag set */
		if (fi->err != 0) {
			CamelException iex = CAMEL_EXCEPTION_INITIALISER;
			if (fi->err == EINTR)
				camel_exception_setv (ex, CAMEL_EXCEPTION_USER_CANCEL,
					"User canceled message retrieval");
			else
				camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
					"Failure while retrieving message %s from POP server: %s",
					uid, g_strerror (fi->err));

			camel_data_cache_remove (pop3_store->cache, "cache", fi->uid, &iex);
			camel_exception_clear (&iex);
			message = NULL;

			goto done;
		}

		if (camel_stream_read(stream, buffer, 1) != 1 || buffer[0] != '#') {
			camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
				"Failure while retrieving message %s from POP server.", uid);
			goto done;
		}
	}

	message = camel_mime_message_new ();
	if (camel_data_wrapper_construct_from_stream((CamelDataWrapper *)message, stream) == -1) {
		if (errno == EINTR)
			camel_exception_setv(ex, CAMEL_EXCEPTION_USER_CANCEL,
				"User canceled message retrieval");
		else
			camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
				"Failure while retrieving message %s from POP server: %s",
				uid, g_strerror (errno));
		camel_object_unref((CamelObject *)message);
		message = NULL;
	} else {
		if (type & CAMEL_FOLDER_RECEIVE_FULL && pop3_store->immediate_delete_after)
		{
			struct _CamelPOP3Command *cmd = NULL;

			g_static_rec_mutex_lock (pop3_store->eng_lock);

			if (pop3_store->engine == NULL) {
				g_static_rec_mutex_unlock (pop3_store->eng_lock);
				goto done;
			}

			cmd = camel_pop3_engine_command_new(pop3_store->engine, 0, NULL, NULL, "DELE %u\r\n", uid);
			while (camel_pop3_engine_iterate(pop3_store->engine, cmd) > 0);
			camel_pop3_engine_command_free(pop3_store->engine, cmd);

			g_static_rec_mutex_unlock (pop3_store->eng_lock);
		}

		/* No more retry */
		retry=3;

	}

	mi = (CamelMessageInfoBase *) camel_folder_summary_uid (summary, uid);
	if (!mi && !camel_pop3_logbook_is_registered (pop3_store->book, uid) && message) {
		mi = (CamelMessageInfoBase *) camel_folder_summary_info_new_from_message (summary, message);
		if (mi->uid)
			g_free (mi->uid);
		mi->uid = g_strdup (uid);
		if (had_attachment)
			camel_message_info_set_flags((CamelMessageInfo *) mi, CAMEL_MESSAGE_ATTACHMENTS, CAMEL_MESSAGE_ATTACHMENTS);
		camel_folder_summary_add (summary, (CamelMessageInfo *)mi);
	} else if (mi) {
		if (had_attachment)
			camel_message_info_set_flags((CamelMessageInfo *) mi, CAMEL_MESSAGE_ATTACHMENTS, CAMEL_MESSAGE_ATTACHMENTS);
		camel_message_info_free (mi);
	}

done:
	if (stream)
		camel_object_unref((CamelObject *)stream);
fail:
	camel_operation_end(NULL);

    retry++;
  }

	g_static_rec_mutex_unlock (pop3_store->uidl_lock);

 do_free_ex:
	if (free_ex)
		camel_exception_free (ex);
	return message;
}



static CamelMimeMessage *
pop3_get_top (CamelFolder *folder, const char *uid, CamelException *ex)
{
	CamelMimeMessage *message = NULL;
	CamelPOP3Store *pop3_store = CAMEL_POP3_STORE (folder->parent_store);
	CamelPOP3Command *pcr;
	CamelPOP3FolderInfo *fi;
	char buffer[1]; int i;
	CamelStream *stream = NULL, *old;
	CamelFolderSummary *summary = folder->summary;
	CamelMessageInfoBase *mi;
	CamelPOP3Engine *mengine = NULL;

	if (camel_disco_store_status (CAMEL_DISCO_STORE (pop3_store)) == CAMEL_DISCO_STORE_OFFLINE)
	{
		camel_exception_setv (ex, CAMEL_EXCEPTION_FOLDER_UID_NOT_AVAILABLE,
			"No message with UID %s and not online while retrieving summary info", uid);
		return NULL;
	}

	g_static_rec_mutex_lock (pop3_store->eng_lock);

	if (pop3_store->engine == NULL) {
		camel_service_connect (CAMEL_SERVICE (pop3_store), ex);
		if (camel_exception_is_set (ex)) {
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			return NULL;
		}
	}


	fi = g_hash_table_lookup(pop3_store->uids_uid, uid);

	if (fi == NULL) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_FOLDER_UID_NOT_AVAILABLE,
			"No message with UID %s while retrieving summary info", uid);
		g_static_rec_mutex_unlock (pop3_store->eng_lock);
		return NULL;
	}

	g_static_rec_mutex_unlock (pop3_store->eng_lock);

	old = fi->stream;

	/* Sigh, most of the crap in this function is so that the cancel button
	   returns the proper exception code.  Sigh. */

	/*camel_operation_start_transient(NULL, "Retrieving summary info for %d", fi->id);*/

	/* If we have an oustanding retrieve message running, wait for that to complete
	   & then retrieve from cache, otherwise, start a new one, and similar */

	if (fi->cmd != NULL) {

		g_static_rec_mutex_lock (pop3_store->eng_lock);

		if (pop3_store->engine == NULL) {
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			goto fail;
		}

		mengine = pop3_store->engine;
		camel_object_ref (mengine);

		g_static_rec_mutex_unlock (pop3_store->eng_lock);

		while ((i = camel_pop3_engine_iterate(mengine, fi->cmd)) > 0)
			;

		if (i == -1)
			fi->err = errno;

		/* getting error code? */
		/* g_assert (fi->cmd->state == CAMEL_POP3_COMMAND_DATA); */

		g_static_rec_mutex_lock (pop3_store->eng_lock);

		camel_object_unref (mengine);

		if (pop3_store->engine == NULL) {
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			goto fail;
		}

		camel_pop3_engine_command_free(pop3_store->engine, fi->cmd);
		g_static_rec_mutex_unlock (pop3_store->eng_lock);

		fi->cmd = NULL;

		if (fi->err != 0) {
			if (fi->err == EINTR)
				camel_exception_setv(ex, CAMEL_EXCEPTION_USER_CANCEL,
					"User canceled summary retreival");
			else
				camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
					"Cannot get summary info for %s: %s",
					uid, g_strerror (fi->err));
			goto fail;
		}
	}

	/* check to see if we have safely written flag set */
	if (pop3_store->cache == NULL
	    || (stream = camel_data_cache_get(pop3_store->cache, "cache", fi->uid, NULL)) == NULL
	    || camel_stream_read(stream, buffer, 1) != 1
	    || buffer[0] != '#') {


		stream = camel_stream_mem_new();

		/* the cmd_tocache thing unrefs it, therefore this is to keep it
		   compatible with existing code */
		camel_object_ref (CAMEL_OBJECT (stream));

		fi->stream = stream;
		fi->err = EIO;

		/* TOP %s 1 only returns the headers of a message and the first
		   line. Which is fine and to make sure broken POP servers also
		   return something (in case TOP %s 0 would otherwise be
		   misinterpreted by the POP server) */


		g_static_rec_mutex_lock (pop3_store->eng_lock);

		if (pop3_store->engine == NULL) {
			g_static_rec_mutex_unlock (pop3_store->eng_lock);
			goto done;
		}

		pcr = camel_pop3_engine_command_new(pop3_store->engine, CAMEL_POP3_COMMAND_MULTI,
			cmd_totop, fi, "TOP %u 0\r\n", fi->id);
		while ((i = camel_pop3_engine_iterate(pop3_store->engine, pcr)) > 0)
			;
		if (i == -1)
			fi->err = errno;

		if (pop3_store->engine && pcr->state == CAMEL_POP3_COMMAND_ERR) {
			fi->err = -1;
			pop3_store->engine->capa &= ~CAMEL_POP3_CAP_TOP;
		}

		camel_pop3_engine_command_free(pop3_store->engine, pcr);

		g_static_rec_mutex_unlock (pop3_store->eng_lock);


		camel_stream_reset(stream);

		fi->stream = old;

		/* Check to see we have safely written flag set */
		if (fi->err != 0) {
			if (fi->err == EINTR)
				camel_exception_setv (ex, CAMEL_EXCEPTION_USER_CANCEL,
					"User canceled summary retrieval");
			else
				camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
					"Cannot get summary info for %s: %s",
					uid, g_strerror (fi->err));
			goto done;
		}

		if (camel_stream_read(stream, buffer, 1) != 1 || buffer[0] != '#') {
			camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
					"Cannot get summary info for %s.", uid);
			goto done;
		}
	}

	message = camel_mime_message_new ();
	if (camel_data_wrapper_construct_from_stream((CamelDataWrapper *)message, stream) == -1) {
		if (errno == EINTR)
			camel_exception_setv(ex, CAMEL_EXCEPTION_USER_CANCEL,
				"User canceled summary retrieval");
		else
			camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
				"Cannot get summary info for %s: %s",
				uid, g_strerror (errno));
		camel_object_unref((CamelObject *)message);
		message = NULL;
	}

	check_dir (pop3_store, NULL);

	mi = (CamelMessageInfoBase *) camel_folder_summary_uid (summary, uid);
	if (!mi && !camel_pop3_logbook_is_registered (pop3_store->book, uid) && message) {
		mi = (CamelMessageInfoBase *) camel_folder_summary_info_new_from_message (summary, message);
		if (mi->uid)
			g_free (mi->uid);
		mi->uid = g_strdup(uid);
		camel_folder_summary_add (summary, (CamelMessageInfo *)mi);
	} else if (mi)
		camel_message_info_free (mi);

done:
	camel_object_unref((CamelObject *)stream);
fail:

	/*camel_operation_end(NULL);*/


	return message;
}

static gboolean
pop3_set_message_flags (CamelFolder *folder, const char *uid, guint32 flags, guint32 set)
{
	CamelPOP3FolderInfo *fi;
	gboolean res = FALSE;
	CamelMessageInfo *info;
	CamelPOP3Store *pop3_store = (CamelPOP3Store*) ((CamelFolder *)folder)->parent_store;

	fi = g_hash_table_lookup(pop3_store->uids_uid, uid);
	if (fi) {
		guint32 new = (fi->flags & ~flags) | (set & flags);

		if (fi->flags != new) {
			fi->flags = new;
			res = TRUE;
		}
	}

	info = camel_folder_summary_uid(folder->summary, uid);
	if (info == NULL)
		return FALSE;

	res = camel_message_info_set_flags(info, flags, set);
	camel_message_info_free(info);

	return res;
}


static GPtrArray *
pop3_get_uids (CamelFolder *folder)
{
	CamelPOP3Folder *pop3_folder = CAMEL_POP3_FOLDER (folder);
	CamelPOP3Store *pop3_store = (CamelPOP3Store*) ((CamelFolder *) pop3_folder)->parent_store;
	GPtrArray *uids = g_ptr_array_new();
	CamelPOP3FolderInfo **fi = (CamelPOP3FolderInfo **)pop3_store->uids->pdata;
	int i;

	for (i=0;i<pop3_store->uids->len;i++,fi++) {
		if (fi[0]->uid)
			g_ptr_array_add(uids, fi[0]->uid);
	}

	return uids;
}


static void
pop3_sync_offline (CamelFolder *folder, CamelException *ex)
{
	camel_folder_summary_save (folder->summary, ex);
}

static void
pop3_sync_online (CamelFolder *folder, CamelException *ex)
{
	camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_NOT_SUPPORTED, _("Not supported"));

	pop3_sync_offline (folder, ex);

	return;
}

static void
pop3_expunge_uids_online (CamelFolder *folder, GPtrArray *uids, CamelException *ex)
{
	camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_NOT_SUPPORTED, _("Not supported"));

	return;
}


static int
uid_compar (const void *va, const void *vb)
{
	const char **sa = (const char **)va, **sb = (const char **)vb;
	unsigned long a, b;

	a = strtoul (*sa, NULL, 10);
	b = strtoul (*sb, NULL, 10);
	if (a < b)
		return -1;
	else if (a == b)
		return 0;
	else
		return 1;
}

static void
pop3_expunge_uids_offline (CamelFolder *folder, GPtrArray *uids, CamelException *ex)
{
	CamelFolderChangeInfo *changes;
	int i;

	qsort (uids->pdata, uids->len, sizeof (void *), uid_compar);

	changes = camel_folder_change_info_new ();

	for (i = 0; i < uids->len; i++) {
		camel_folder_summary_remove_uid (folder->summary, uids->pdata[i]);
		camel_folder_change_info_remove_uid (changes, uids->pdata[i]);
		/* We intentionally don't remove it from the cache because
		 * the cached data may be useful in replaying a COPY later. */
	}
	camel_folder_summary_save (folder->summary, ex);

	camel_disco_diary_log (CAMEL_DISCO_STORE (folder->parent_store)->diary,
			       CAMEL_DISCO_DIARY_FOLDER_EXPUNGE, folder, uids);

	camel_object_trigger_event (CAMEL_OBJECT (folder), "folder_changed", changes);
	camel_folder_change_info_free (changes);

	return;
}

static void
pop3_expunge_uids_resyncing (CamelFolder *folder, GPtrArray *uids, CamelException *ex)
{
	camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_NOT_SUPPORTED, _("Not supported"));

	return;
}


static void
pop3_append_offline (CamelFolder *folder, CamelMimeMessage *message,
		     const CamelMessageInfo *info, char **appended_uid,
		     CamelException *ex)
{
	camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_NOT_SUPPORTED, _("Not supported"));

	return;
}

static void
pop3_transfer_offline (CamelFolder *source, GPtrArray *uids,
		       CamelFolder *dest, GPtrArray **transferred_uids,
		       gboolean delete_originals, CamelException *ex)
{
	camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_NOT_SUPPORTED, _("Not supported"));
	return;
}

static void
pop3_cache_message (CamelDiscoFolder *disco_folder, const char *uid,
		    CamelException *ex)
{
	CamelMimeMessage *msg = pop3_get_message (CAMEL_FOLDER (disco_folder), uid,
		 CAMEL_FOLDER_RECEIVE_FULL, -1, ex);

	if (msg)
		camel_object_unref (CAMEL_OBJECT (msg));

}

static void
camel_pop3_folder_class_init (CamelPOP3FolderClass *camel_pop3_folder_class)
{
	CamelFolderClass *camel_folder_class = CAMEL_FOLDER_CLASS(camel_pop3_folder_class);
	CamelDiscoFolderClass *camel_disco_folder_class = CAMEL_DISCO_FOLDER_CLASS (camel_pop3_folder_class);

	disco_folder_class = CAMEL_DISCO_FOLDER_CLASS (camel_type_get_global_classfuncs (camel_disco_folder_get_type ()));

	parent_class = CAMEL_FOLDER_CLASS(camel_folder_get_type());

	/* virtual method overload */
	camel_folder_class->get_local_size = pop3_get_local_size;
	camel_folder_class->refresh_info = pop3_refresh_info;
	camel_folder_class->sync = pop3_sync;

	camel_folder_class->get_uids = pop3_get_uids;
	camel_folder_class->free_uids = camel_folder_free_shallow;
	camel_folder_class->get_message = pop3_get_message;
	camel_folder_class->set_message_flags = pop3_set_message_flags;
	camel_folder_class->delete_attachments = pop3_delete_attachments;
	camel_folder_class->get_allow_external_images = pop3_get_allow_external_images;
	camel_folder_class->set_allow_external_images = pop3_set_allow_external_images;

	camel_disco_folder_class->refresh_info_online = pop3_refresh_info;
	camel_disco_folder_class->sync_online = pop3_sync_online;
	camel_disco_folder_class->sync_offline = pop3_sync_offline;

	camel_disco_folder_class->sync_resyncing = pop3_sync_offline;

	camel_disco_folder_class->expunge_uids_online = pop3_expunge_uids_online;
	camel_disco_folder_class->expunge_uids_offline = pop3_expunge_uids_offline;
	camel_disco_folder_class->expunge_uids_resyncing = pop3_expunge_uids_resyncing;

	camel_disco_folder_class->append_online = pop3_append_offline;
	camel_disco_folder_class->append_offline = pop3_append_offline;
	camel_disco_folder_class->append_resyncing = pop3_append_offline;

	camel_disco_folder_class->transfer_online = pop3_transfer_offline;
	camel_disco_folder_class->transfer_offline = pop3_transfer_offline;
	camel_disco_folder_class->transfer_resyncing = pop3_transfer_offline;

	camel_disco_folder_class->cache_message = pop3_cache_message;
}

CamelType
camel_pop3_folder_get_type (void)
{
	static CamelType camel_pop3_folder_type = CAMEL_INVALID_TYPE;

	if (!camel_pop3_folder_type) {
		camel_pop3_folder_type = camel_type_register (CAMEL_DISCO_FOLDER_TYPE, "CamelPOP3Folder",
							      sizeof (CamelPOP3Folder),
							      sizeof (CamelPOP3FolderClass),
							      (CamelObjectClassInitFunc) camel_pop3_folder_class_init,
							      NULL,
							      NULL,
							      (CamelObjectFinalizeFunc) pop3_finalize);
	}

	return camel_pop3_folder_type;
}
