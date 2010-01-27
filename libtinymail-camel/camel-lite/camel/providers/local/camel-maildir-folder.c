/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; fill-column: 160 -*-
 *
 * Authors: Michael Zucchi <notzed@ximian.com>
 *
 * Copyright (C) 1999, 2003 Ximian Inc.
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include "camel-data-wrapper.h"
#include "camel-exception.h"
#include "camel-mime-message.h"
#include "camel-stream-fs.h"
#include "camel-file-utils.h"

#include "camel-maildir-folder.h"
#include "camel-maildir-store.h"
#include "camel-maildir-summary.h"

#define d(x) /*(printf("%s(%d): ", __FILE__, __LINE__),(x))*/

static CamelLocalFolderClass *parent_class = NULL;

/* Returns the class for a CamelMaildirFolder */
#define CMAILDIRF_CLASS(so) CAMEL_MAILDIR_FOLDER_CLASS (CAMEL_OBJECT_GET_CLASS(so))
#define CF_CLASS(so) CAMEL_FOLDER_CLASS (CAMEL_OBJECT_GET_CLASS(so))
#define CMAILDIRS_CLASS(so) CAMEL_STORE_CLASS (CAMEL_OBJECT_GET_CLASS(so))

static CamelLocalSummary *maildir_create_summary(CamelLocalFolder *lf, const char *path, const char *folder, CamelIndex *index);

static void maildir_append_message(CamelFolder * folder, CamelMimeMessage * message, const CamelMessageInfo *info, char **appended_uid, CamelException * ex);
static CamelMimeMessage *maildir_get_message(CamelFolder * folder, const gchar * uid, CamelFolderReceiveType type, gint param, CamelException * ex);
static void maildir_rewrite_cache (CamelFolder *folder, const char *uid, CamelMimeMessage *msg);

static void maildir_transfer_messages_to (CamelFolder *source, GPtrArray *uids, CamelFolder *dest, GPtrArray **transferred_uids, gboolean delete_originals, CamelException *ex);

static gboolean maildir_get_allow_external_images (CamelFolder *folder, const char *uid);
static void maildir_set_allow_external_images (CamelFolder *folder, const char *uid, gboolean allow);


static void maildir_finalize(CamelObject * object);

static int
maildir_folder_getv(CamelObject *object, CamelException *ex, CamelArgGetV *args)
{
	CamelFolder *folder = (CamelFolder *)object;
	int i;
	guint32 tag;

	for (i=0;i<args->argc;i++) {
		CamelArgGet *arg = &args->argv[i];

		tag = arg->tag;

		switch (tag & CAMEL_ARG_TAG) {
		case CAMEL_FOLDER_ARG_NAME:
			if (!strcmp(folder->full_name, "."))
				*arg->ca_str = _("Inbox");
			else
				*arg->ca_str = folder->name;
			break;
		default:
			continue;
		}

		arg->tag = (tag & CAMEL_ARG_TYPE) | CAMEL_ARG_IGNORE;
	}

	return ((CamelObjectClass *)parent_class)->getv(object, ex, args);
}

static void camel_maildir_folder_class_init(CamelObjectClass * camel_maildir_folder_class)
{
	CamelFolderClass *camel_folder_class = CAMEL_FOLDER_CLASS(camel_maildir_folder_class);
	CamelLocalFolderClass *lclass = (CamelLocalFolderClass *)camel_maildir_folder_class;

	parent_class = CAMEL_LOCAL_FOLDER_CLASS (camel_type_get_global_classfuncs(camel_local_folder_get_type()));

	/* virtual method definition */

	/* virtual method overload */
	((CamelObjectClass *)camel_folder_class)->getv = maildir_folder_getv;

	camel_folder_class->append_message = maildir_append_message;
	camel_folder_class->get_message = maildir_get_message;
	camel_folder_class->rewrite_cache = maildir_rewrite_cache;
	camel_folder_class->get_allow_external_images = maildir_get_allow_external_images;
	camel_folder_class->set_allow_external_images = maildir_set_allow_external_images;

	//camel_folder_class->transfer_messages_to = maildir_transfer_messages_to;

	lclass->create_summary = maildir_create_summary;
}


static void 
maildir_transfer_messages_to (CamelFolder *source, GPtrArray *uids, CamelFolder *dest, GPtrArray **transferred_uids, gboolean delete_originals, CamelException *ex)
{
	gboolean fallback = FALSE;

	if (delete_originals && CAMEL_IS_MAILDIR_FOLDER (source) && CAMEL_IS_MAILDIR_FOLDER (dest)) {
		gint i;
		CamelLocalFolder *lf = (CamelLocalFolder *) source;
		CamelLocalFolder *df = (CamelLocalFolder *) dest;

		camel_operation_start(NULL, _("Moving messages"));

		camel_folder_freeze(dest);
		camel_folder_freeze(source);

		for (i = 0; i < uids->len; i++) {
			char *uid = (char *) uids->pdata[i];
			char *s_filename, *d_filename, *tmp; 
			CamelMaildirMessageInfo *mdi;
			CamelMessageInfo *mi, *info;

			if ((info = camel_folder_summary_uid (source->summary, uid)) == NULL) {
				camel_exception_setv(ex, CAMEL_EXCEPTION_FOLDER_INVALID_UID,
						     _("Cannot get message: %s from folder %s\n  %s"),
						     uid, lf->folder_path, _("No such message"));
				return;
			}

			mdi = (CamelMaildirMessageInfo *) info;
			tmp = camel_maildir_summary_info_to_name (mdi);

			d_filename = g_strdup_printf("%s/cur/%s", df->folder_path, tmp);
			g_free (tmp);
			s_filename = camel_maildir_get_filename (lf->folder_path, mdi, uid);
			camel_message_info_free (info);

			if (g_rename (s_filename, d_filename) != 0)
				if (errno == EXDEV) {
					i = uids->len + 1;
					fallback = TRUE;
				} else
					camel_exception_setv(ex, CAMEL_EXCEPTION_SYSTEM_IO_WRITE,
						     _("Cannot transfer message to destination folder"));
			else {
				camel_folder_set_message_flags (source, uid, 
					CAMEL_MESSAGE_EXPUNGED|CAMEL_MESSAGE_DELETED|CAMEL_MESSAGE_SEEN, ~0);
				((CamelMessageInfoBase *)info)->flags |= CAMEL_MESSAGE_EXPUNGED;
				camel_folder_summary_remove (source->summary, info);
			}

			g_free (s_filename);
			g_free (d_filename);
		}

		camel_folder_thaw(dest);
		camel_folder_thaw(source);

		camel_operation_end(NULL);

	} else
		fallback = TRUE;

	if (fallback)
		((CamelFolderClass *)parent_class)->transfer_messages_to (source, uids, dest, transferred_uids, delete_originals, ex);

	return;
}

static void maildir_init(gpointer object, gpointer klass)
{
	/*CamelFolder *folder = object;
	  CamelMaildirFolder *maildir_folder = object;*/
}

static void maildir_finalize(CamelObject * object)
{
	/*CamelMaildirFolder *maildir_folder = CAMEL_MAILDIR_FOLDER(object);*/
}

CamelType camel_maildir_folder_get_type(void)
{
	static CamelType camel_maildir_folder_type = CAMEL_INVALID_TYPE;

	if (camel_maildir_folder_type == CAMEL_INVALID_TYPE) {
		camel_maildir_folder_type = camel_type_register(CAMEL_LOCAL_FOLDER_TYPE, "CamelMaildirFolder",
							   sizeof(CamelMaildirFolder),
							   sizeof(CamelMaildirFolderClass),
							   (CamelObjectClassInitFunc) camel_maildir_folder_class_init,
							   NULL,
							   (CamelObjectInitFunc) maildir_init,
							   (CamelObjectFinalizeFunc) maildir_finalize);
	}

	return camel_maildir_folder_type;
}

CamelFolder *
camel_maildir_folder_new(CamelStore *parent_store, const char *full_name, guint32 flags, CamelException *ex)
{
	CamelFolder *folder;

	d(printf("Creating maildir folder: %s\n", full_name));

	folder = (CamelFolder *)camel_object_new(CAMEL_MAILDIR_FOLDER_TYPE);

	if (parent_store->flags & CAMEL_STORE_FILTER_INBOX
	    && strcmp(full_name, ".") == 0)
		folder->folder_flags |= CAMEL_FOLDER_FILTER_RECENT;

	folder = (CamelFolder *)camel_local_folder_construct((CamelLocalFolder *)folder,
							     parent_store, full_name, flags, ex);

	return folder;
}

static CamelLocalSummary *maildir_create_summary(CamelLocalFolder *lf, const char *path, const char *folder, CamelIndex *index)
{
	return (CamelLocalSummary *)camel_maildir_summary_new((CamelFolder *)lf, path, folder, index);
}

static void
maildir_append_message (CamelFolder *folder, CamelMimeMessage *message, const CamelMessageInfo *info, char **appended_uid, CamelException *ex)
{
	CamelLocalFolder *lf = (CamelLocalFolder *)folder;
	CamelStream *output_stream;
	CamelMessageInfo *mi;
	CamelMaildirMessageInfo *mdi;
	char *name, *dest = NULL;

	d(printf("Appending message\n"));

	/* add it to the summary/assign the uid, etc */
	mi = camel_local_summary_add((CamelLocalSummary *)folder->summary, message, info, lf->changes, ex);
	if (camel_exception_is_set (ex))
		return;

	mdi = (CamelMaildirMessageInfo *)mi;

	d(printf("Appending message: uid is %s filename is %s\n", camel_message_info_uid(mi), mdi->filename));

	/* write it out to tmp, use the uid we got from the summary */
	name = g_strdup_printf ("%s/tmp/%s", lf->folder_path, camel_message_info_uid(mi));
	output_stream = camel_stream_fs_new_with_name (name, O_WRONLY|O_CREAT, 0600);
	if (output_stream == NULL)
		goto fail_write;

	if (camel_data_wrapper_write_to_stream ((CamelDataWrapper *)message, output_stream) == -1
	    || camel_stream_close (output_stream) == -1)
		goto fail_write;

	/* now move from tmp to cur (bypass new, does it matter?) */
	dest = camel_maildir_get_filename (lf->folder_path, mdi, mi->uid);
	if (rename (name, dest) == -1)
		goto fail_write;

	g_free (dest);
	g_free (name);

	/*camel_object_trigger_event (CAMEL_OBJECT (folder), "folder_changed",
				    ((CamelLocalFolder *)maildir_folder)->changes);
	camel_folder_change_info_clear (((CamelLocalFolder *)maildir_folder)->changes);*/

	if (appended_uid)
		*appended_uid = g_strdup(camel_message_info_uid(mi));

	if (output_stream)
		camel_object_unref (output_stream);

	return;

 fail_write:


	if (errno == EINTR)
		camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL,
				     _("Maildir append message canceled"));
	else /* Most likely a IO error */
		camel_exception_setv (ex, CAMEL_EXCEPTION_SYSTEM_IO_WRITE,
				      _("Cannot append message to maildir folder: %s: %s"),
				      name, g_strerror (errno));

	/* remove the summary info so we are not out-of-sync with the mh folder */
	camel_folder_summary_remove_uid (CAMEL_FOLDER_SUMMARY (folder->summary),
					 camel_message_info_uid (mi));

	if (output_stream) {
		camel_object_unref (CAMEL_OBJECT (output_stream));
		unlink (name);
	}

	g_free (name);
	g_free (dest);
}


static CamelMimeMessage *
maildir_get_message(CamelFolder * folder, const gchar * uid, CamelFolderReceiveType type, gint param, CamelException * ex)
{
	CamelLocalFolder *lf = (CamelLocalFolder *)folder;
	CamelStream *message_stream = NULL;
	CamelMimeMessage *message = NULL;
	CamelMessageInfo *info;
	char *name; 
	CamelMaildirMessageInfo *mdi;

	d(printf("getting message: %s\n", uid));

	/* TNY TODO: Implement partial message retrieval if full==TRUE
	   maybe remove the attachments if it happens to be FALSE in this case?
	 */

	/* get the message summary info */
	if ((info = camel_folder_summary_uid(folder->summary, uid)) == NULL) {
		camel_exception_setv(ex, CAMEL_EXCEPTION_FOLDER_INVALID_UID,
				     _("Cannot get message: %s from folder %s\n  %s"),
				     uid, lf->folder_path, _("No such message"));
		return NULL;
	}

	mdi = (CamelMaildirMessageInfo *)info;

	name = camel_maildir_get_filename (lf->folder_path, mdi, uid);

	camel_message_info_free(info);

	if ((message_stream = camel_stream_fs_new_with_name(name, O_RDONLY, 0)) == NULL) {
		camel_exception_setv(ex, CAMEL_EXCEPTION_SYSTEM_IO_READ,
				     _("Cannot get message: %s from folder %s\n  %s"),
				     uid, lf->folder_path, g_strerror(errno));
		g_free(name);
		return NULL;
	}

	message = camel_mime_message_new();
	if (camel_data_wrapper_construct_from_stream((CamelDataWrapper *)message, message_stream) == -1) {
		camel_exception_setv(ex, (errno==EINTR)?CAMEL_EXCEPTION_USER_CANCEL:CAMEL_EXCEPTION_SYSTEM_IO_READ,
				     _("Cannot get message: %s from folder %s\n  %s"),
				     uid, lf->folder_path, _("Invalid message contents"));
		g_free(name);
		camel_object_unref((CamelObject *)message_stream);
		camel_object_unref((CamelObject *)message);
		return NULL;

	}
	camel_object_unref((CamelObject *)message_stream);
	g_free(name);

	return message;
}

static void
maildir_rewrite_cache (CamelFolder *folder, const char *uid, CamelMimeMessage *msg)
{
	CamelLocalFolder *lf = (CamelLocalFolder *) folder;
	char *name = NULL;
	CamelStream *output_stream = NULL;
	char *dest = NULL;
	CamelMessageInfo *info;
	CamelMaildirMessageInfo *mdi;

	/* get the message summary info */
	if ((info = camel_folder_summary_uid(folder->summary, uid)) == NULL) {
		return;
	}

	mdi = (CamelMaildirMessageInfo *)info;

	/* write it out to tmp, use the uid we got from the summary */
	name = g_strdup_printf("%s/tmp/%s", lf->folder_path, info->uid);
	output_stream = camel_stream_fs_new_with_name (name, O_WRONLY|O_CREAT, 0600);
	if (output_stream == NULL)
		goto fail_write;

	if (camel_data_wrapper_write_to_stream ((CamelDataWrapper *)msg, output_stream) == -1
	    || camel_stream_close (output_stream) == -1)
		goto fail_write;

	/* now move from tmp to cur (bypass new, does it matter?) */
	dest = camel_maildir_get_filename (lf->folder_path, mdi, info->uid);
	if (rename (name, dest) == -1)
		goto fail_write;

	g_free (dest);
	g_free (name);
	camel_message_info_free(info);
	
	return;

 fail_write:
	camel_message_info_free(info);
	g_free (name);
	g_free (dest);
}

static gboolean
maildir_get_allow_external_images (CamelFolder *folder, const char *uid)
{
	CamelLocalFolder *lf = (CamelLocalFolder *) folder;
	char *name = NULL;
	gboolean retval;

	/* write it out to tmp, use the uid we got from the summary */
	name = g_strdup_printf("%s/%s.getimages", lf->folder_path, uid);
	retval = g_file_test (name, G_FILE_TEST_IS_REGULAR);
	g_free (name);
	return retval;
}

static void
maildir_set_allow_external_images (CamelFolder *folder, const char *uid, gboolean allow)
{
	CamelLocalFolder *lf = (CamelLocalFolder *) folder;
	char *name = NULL;
	int fd;

	/* write it out to tmp, use the uid we got from the summary */
	name = g_strdup_printf("%s/%s.getimages", lf->folder_path, uid);

	if (!allow)
	{
		if (g_file_test (name, G_FILE_TEST_IS_REGULAR))
			g_unlink (name);
	} else {
		if (!g_file_test (name, G_FILE_TEST_IS_REGULAR))
		{
		    fd = g_open (name, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0600);
		    if (fd != -1)
			close (fd);
		}
	}

	g_free (name);	
}

