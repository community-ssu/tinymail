/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* camel-imap-message-cache.c: Class for an IMAP message cache */

/*
 * Author:
 *   Dan Winship <danw@ximian.com>
 *
 * Copyright (C) 2001 Ximian, Inc. (www.ximian.com)
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

#include <config.h>

#include <ctype.h>
#include <errno.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include "camel-data-wrapper.h"
#include "camel-exception.h"
#include "camel-stream-fs.h"

#include "camel-string-utils.h"
#include "camel-imap-message-cache.h"
#include "camel-stream-buffer.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

static void finalize (CamelImapMessageCache *cache);
static void stream_finalize (CamelObject *stream, gpointer event_data, gpointer user_data);


CamelType
camel_imap_message_cache_get_type (void)
{
	static CamelType camel_imap_message_cache_type = CAMEL_INVALID_TYPE;

	if (camel_imap_message_cache_type == CAMEL_INVALID_TYPE) {
		camel_imap_message_cache_type = camel_type_register (
			CAMEL_OBJECT_TYPE, "CamelImapMessageCache",
			sizeof (CamelImapMessageCache),
			sizeof (CamelImapMessageCacheClass),
			NULL,
			NULL,
			NULL,
			(CamelObjectFinalizeFunc) finalize);
	}

	return camel_imap_message_cache_type;
}

static void
free_part (gpointer key, gpointer value, gpointer data)
{
	if (value) {
		if (strchr (key, '.')) {
			camel_object_unhook_event (value, "finalize",
						   stream_finalize, data);
			camel_object_unref (value);
		} else
			g_ptr_array_free (value, TRUE);
	}
	g_free (key);
}

static void
finalize (CamelImapMessageCache *cache)
{
	if (cache->path)
		g_free (cache->path);
	if (cache->parts) {
		g_hash_table_foreach (cache->parts, free_part, cache);
		g_hash_table_destroy (cache->parts);
	}
	if (cache->cached)
		g_hash_table_destroy (cache->cached);
}

static void
cache_put (CamelImapMessageCache *cache, const char *uid, const char *key,
	   CamelStream *stream)
{
	char *hash_key;
	GPtrArray *subparts;
	gpointer okey, ostream;
	guint32 uidval;

	uidval = strtoul (uid, NULL, 10);
	if (uidval > cache->max_uid)
		cache->max_uid = uidval;

	subparts = g_hash_table_lookup (cache->parts, uid);
	if (!subparts) {
		subparts = g_ptr_array_new ();
		g_hash_table_insert (cache->parts, g_strdup (uid), subparts);
	}

	if (g_hash_table_lookup_extended (cache->parts, key, &okey, &ostream)) {
		if (ostream) {
			camel_object_unhook_event (ostream, "finalize",
						   stream_finalize, cache);
			g_hash_table_remove (cache->cached, ostream);
			camel_object_unref (ostream);
		}
		hash_key = okey;
	} else {
		hash_key = g_strdup (key);
		g_ptr_array_add (subparts, hash_key);
	}

	g_hash_table_insert (cache->parts, hash_key, stream);
	g_hash_table_insert (cache->cached, stream, hash_key);

	if (stream) {
		camel_object_hook_event (CAMEL_OBJECT (stream), "finalize",
					 stream_finalize, cache);
	}
}

/**
 * camel_imap_message_cache_new:
 * @path: directory to use for storage
 * @summary: CamelFolderSummary for the folder we are caching
 * @ex: a CamelException
 *
 * Return value: a new CamelImapMessageCache object using @path for
 * storage. If cache files already exist in @path, then any that do not
 * correspond to messages in @summary will be deleted.
 **/
CamelImapMessageCache *
camel_imap_message_cache_new (const char *path, CamelFolderSummary *summary,
			      CamelException *ex)
{
	CamelImapMessageCache *cache;
	GDir *dir;
	const char *dname;
	char *uid, *p;
	GPtrArray *deletes;
	CamelMessageInfo *info;
	GError *error = NULL;

	dir = g_dir_open (path, 0, &error);
	if (!dir) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SYSTEM_IO_WRITE,
				      _("Could not open cache directory: %s"),
				      error->message);
		g_error_free (error);
		return NULL;
	}

	cache = (CamelImapMessageCache *)camel_object_new (CAMEL_IMAP_MESSAGE_CACHE_TYPE);
	cache->path = g_strdup (path);

	cache->parts = g_hash_table_new (g_str_hash, g_str_equal);
	cache->cached = g_hash_table_new (NULL, NULL);
	deletes = g_ptr_array_new ();

	camel_folder_summary_prepare_hash (summary);

	while ((dname = g_dir_read_name (dir))) {
		if (!isdigit (dname[0]))
			continue;
		p = strchr (dname, '_');
		if (p)
			uid = g_strndup (dname, p - dname);
		else if ((p = strchr (dname, '.')))
			uid = g_strndup (dname, p - dname);
		else {
			g_warning("Cache file name Invalid\n");
			continue;
		}
		info = camel_folder_summary_uid (summary, uid);
		if (info) {
			camel_message_info_free(info);
			cache_put (cache, uid, dname, NULL);
		} else 
			g_ptr_array_add (deletes, g_strdup_printf ("%s/%s", cache->path, dname)); 
		g_free (uid);
	}
	g_dir_close (dir);
	camel_folder_summary_kill_hash (summary);

	while (deletes->len) {
		g_unlink (deletes->pdata[0]);
		g_free (deletes->pdata[0]);
		g_ptr_array_remove_index_fast (deletes, 0);
	}
	g_ptr_array_free (deletes, TRUE);

	return cache;
}

/**
 * camel_imap_message_cache_max_uid:
 * @cache: the cache
 *
 * Return value: the largest (real) UID in the cache.
 **/
guint32
camel_imap_message_cache_max_uid (CamelImapMessageCache *cache)
{
	return cache->max_uid;
}

/**
 * camel_imap_message_cache_set_path:
 * @cache:
 * @path:
 *
 * Set the path used for the message cache.
 **/
void
camel_imap_message_cache_set_path (CamelImapMessageCache *cache, const char *path)
{
	g_free(cache->path);
	cache->path = g_strdup(path);
}

static void
stream_finalize (CamelObject *stream, gpointer event_data, gpointer user_data)
{
	CamelImapMessageCache *cache = user_data;
	char *key;

	key = g_hash_table_lookup (cache->cached, stream);
	if (!key)
		return;
	g_hash_table_remove (cache->cached, stream);
	g_hash_table_insert (cache->parts, key, NULL);
}


static CamelStream *
insert_setup (CamelImapMessageCache *cache, const char *uid, const char *part_spec,
	      char **path, char **key, CamelException *ex)
{
	CamelStream *stream;
	int fd;

	/* Trailing periods in file names are silently dropped on
	 * Win32, argh. The code in this file requires the period to
	 * be there. So in case part_spec is empty, use a tilde (just
	 * a random choice) instead.
	 */
	if (!*part_spec)
		part_spec = "~";
	*path = g_strdup_printf ("%s/%s.%s", cache->path, uid, part_spec);
	*key = strrchr (*path, '/') + 1;
	stream = g_hash_table_lookup (cache->parts, *key);
	if (stream)
		camel_object_unref (CAMEL_OBJECT (stream));

	fd = g_open (*path, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0600);
	if (fd == -1) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SYSTEM_IO_WRITE,
				      _("Failed to cache message %s: %s"),
				      uid, g_strerror (errno));
		g_free (*path);
		return NULL;
	}

	return camel_stream_fs_new_with_fd (fd);
}

static CamelStream *
insert_abort (char *path, CamelStream *stream)
{
	g_unlink (path);
	g_free (path);
	camel_object_unref (CAMEL_OBJECT (stream));
	return NULL;
}

static CamelStream *
insert_finish (CamelImapMessageCache *cache, const char *uid, char *path,
	       char *key, CamelStream *stream)
{
	camel_stream_flush (stream);
	camel_stream_reset (stream);
	cache_put (cache, uid, key, stream);
	g_free (path);

	return stream;
}

/**
 * camel_imap_message_cache_insert:
 * @cache: the cache
 * @uid: UID of the message data to cache
 * @part_spec: the IMAP part_spec of the data
 * @data: the data
 * @len: length of @data
 *
 * Caches the provided data into @cache.
 *
 * Return value: a CamelStream containing the cached data, which the
 * caller must unref.
 **/
CamelStream *
camel_imap_message_cache_insert (CamelImapMessageCache *cache, const char *uid,
				 const char *part_spec, const char *data,
				 int len, CamelException *ex)
{
	char *path, *key;
	CamelStream *stream;

	stream = insert_setup (cache, uid, part_spec, &path, &key, ex);
	if (!stream)
		return NULL;

	if (camel_stream_write (stream, data, len) == -1) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SYSTEM_IO_WRITE,
				      _("Failed to cache message %s: %s"),
				      uid, g_strerror (errno));
		return insert_abort (path, stream);
	}

	return insert_finish (cache, uid, path, key, stream);
}

static gchar*
cachefile_get(const char *path, const char *uid, const char *part_spec)
{
	gchar *file;
	if (part_spec && *part_spec) {
		file = g_build_filename(path, uid, part_spec, NULL);
	} else {
		char tmp [512];

		snprintf(tmp, 512, "%s_bodystructure", uid);
		file = g_build_filename (path, tmp, NULL);
		if (!g_file_test (file, G_FILE_TEST_IS_REGULAR)) {
			g_free (file);
			snprintf(tmp, 512, "%s.~", uid);
			file = g_build_filename(path, tmp, NULL);
			if (!g_file_test(file, G_FILE_TEST_IS_REGULAR)) {
				/*  Test if old cache file exists (like "uid."*/
				int len = strlen(file);
				file [len -1] = '\0';
				if (!g_file_test(file, G_FILE_TEST_IS_REGULAR)) {
					file [len -1] = '~';
				}
			}
		}
	}
	return file;
}

void
camel_imap_message_cache_set_flags (const gchar *folder_dir, CamelMessageInfoBase *mi)
{
	gchar *cachefile;

	if( !folder_dir ){
		return;
	}
	cachefile = cachefile_get(folder_dir, mi->uid, NULL);
	if (g_file_test (cachefile, G_FILE_TEST_IS_REGULAR))
	{
		char mystring [512];

		mi->flags |= CAMEL_MESSAGE_CACHED;
		snprintf (mystring, 512, "%s/%s.partial", folder_dir, mi->uid);
		if (g_file_test (mystring, G_FILE_TEST_IS_REGULAR))
			mi->flags |= CAMEL_MESSAGE_PARTIAL;
		else
			mi->flags &= ~CAMEL_MESSAGE_PARTIAL;
	} else {
		mi->flags &= ~CAMEL_MESSAGE_CACHED;
		mi->flags &= ~CAMEL_MESSAGE_PARTIAL;
	}
	g_free(cachefile);
}

gboolean
camel_imap_message_cache_is_partial (CamelImapMessageCache *cache, const char *uid)
{
	gchar *path = g_strdup_printf ("%s/%s.ispartial", cache->path, uid);
	gboolean retval = FALSE;

	retval = g_file_test (path, G_FILE_TEST_IS_REGULAR);

	g_free (path);

	return retval;
}

void
camel_imap_message_cache_set_partial (CamelImapMessageCache *cache, const char *uid, gboolean partial)
{
	gchar *path = g_strdup_printf ("%s/%s.ispartial", cache->path, uid);
	int fd;

	if (!partial)
	{
		if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
			g_unlink (path);
	} else {
		if (!g_file_test (path, G_FILE_TEST_IS_REGULAR))
		{
		    fd = g_open (path, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0600);
		    if (fd != -1)
			close (fd);
		}
	}

	g_free (path);
}

gboolean
camel_imap_message_cache_get_allow_external_images (CamelImapMessageCache *cache, const char *uid)
{
	gchar *path = g_strdup_printf ("%s/%s.getimages", cache->path, uid);
	gboolean retval = FALSE;

	retval = g_file_test (path, G_FILE_TEST_IS_REGULAR);

	g_free (path);

	return retval;
}

void
camel_imap_message_cache_set_allow_external_images (CamelImapMessageCache *cache, const char *uid, gboolean allow)
{
	gchar *path = g_strdup_printf ("%s/%s.getimages", cache->path, uid);
	int fd;

	if (!allow)
	{
		if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
			g_unlink (path);
	} else {
		if (!g_file_test (path, G_FILE_TEST_IS_REGULAR))
		{
		    fd = g_open (path, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0600);
		    if (fd != -1)
			close (fd);
		}
	}

	g_free (path);	
}

/**
 * camel_imap_message_cache_insert_stream:
 * @cache: the cache
 * @uid: UID of the message data to cache
 * @part_spec: the IMAP part_spec of the data
 * @data_stream: the stream to cache
 *
 * Caches the provided data into @cache.
 **/
void
camel_imap_message_cache_insert_stream (CamelImapMessageCache *cache,
					const char *uid, const char *part_spec,
					CamelStream *data_stream, CamelException *ex)
{
	char *path, *key;
	CamelStream *stream;

	stream = insert_setup (cache, uid, part_spec, &path, &key, ex);
	if (!stream)
		return;

	if (camel_stream_write_to_stream (data_stream, stream) == -1) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SYSTEM_IO_WRITE,
				      _("Failed to cache message %s: %s"),
				      uid, g_strerror (errno));
		insert_abort (path, stream);
	} else {
		insert_finish (cache, uid, path, key, stream);
		camel_object_unref (CAMEL_OBJECT (stream));
	}
}

/**
 * camel_imap_message_cache_insert_wrapper:
 * @cache: the cache
 * @uid: UID of the message data to cache
 * @part_spec: the IMAP part_spec of the data
 * @wrapper: the wrapper to cache
 *
 * Caches the provided data into @cache.
 **/
void
camel_imap_message_cache_insert_wrapper (CamelImapMessageCache *cache,
					 const char *uid, const char *part_spec,
					 CamelDataWrapper *wrapper, CamelException *ex)
{
	char *path, *key;
	CamelStream *stream;

	stream = insert_setup (cache, uid, part_spec, &path, &key, ex);
	if (!stream)
		return;

	if (camel_data_wrapper_write_to_stream (wrapper, stream) == -1) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SYSTEM_IO_WRITE,
				      _("Failed to cache message %s: %s"),
				      uid, g_strerror (errno));
		insert_abort (path, stream);
	} else {
		insert_finish (cache, uid, path, key, stream);
		camel_object_unref (CAMEL_OBJECT (stream));
	}
}

/**
 * camel_imap_message_cache_replace_with_wrapper:
 * @cache: the cache
 * @uid: UID of the message data to cache
 * @part_spec: the IMAP part_spec of the data
 * @wrapper: the wrapper to cache
 *
 * Caches the provided data into @cache.
 **/
void
camel_imap_message_cache_replace_with_wrapper (CamelImapMessageCache *cache,
					       const char *uid,
					       CamelDataWrapper *wrapper, CamelException *ex)
{
	/* char *path, *key; */
	CamelStream *stream;
	gchar *real = g_strdup_printf("%s/%s.purgetmp", cache->path, uid);

	stream = camel_stream_fs_new_with_name (real, O_RDWR|O_CREAT|O_TRUNC, 0600);
	g_free (real);
	if (!stream)
		return;

	if (camel_data_wrapper_write_to_stream (wrapper, stream) == -1) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SYSTEM_IO_WRITE,
				      _("Failed to cache message %s: %s"),
				      uid, g_strerror (errno));
	} else {
		camel_object_unref (CAMEL_OBJECT (stream));
		camel_imap_message_cache_remove (cache, uid);
		camel_imap_message_cache_replace_cache (cache, uid, NULL, uid, "purgetmp");
		camel_imap_message_cache_set_partial (cache, uid, TRUE);
	}
}


/**
 * camel_imap_message_cache_get:
 * @cache: the cache
 * @uid: the UID of the data to get
 * @part_spec: the part_spec of the data to get
 * @ex: exception
 *
 * Return value: a CamelStream containing the cached data (which the
 * caller must unref), or %NULL if that data is not cached.
 **/
CamelStream *
camel_imap_message_cache_get (CamelImapMessageCache *cache, const char *uid,
			      const char *part_spec, CamelException *ex)
{
	CamelStream *stream;
	char *path, *key;

	if (uid[0] == 0)
		return NULL;

	path = cachefile_get(cache->path, uid, part_spec);
	key = strrchr (path, '/') + 1;

	stream = g_hash_table_lookup (cache->parts, key);
	if (stream) {
		camel_stream_reset (CAMEL_STREAM (stream));
		camel_object_ref (CAMEL_OBJECT (stream));
		g_free (path);
		return stream;
	}

	if (!g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
		g_free (path);
		return NULL;
	}

	stream = camel_stream_fs_new_with_name (path, O_RDONLY, 0);
	if (stream) {
		cache_put (cache, uid, key, stream);
	} else {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SYSTEM_IO_WRITE,
				      _("Failed to cache %s: %s"),
				      part_spec, g_strerror (errno));
	}

	g_free (path);

	return stream;
}


static int
recursive_insanity (CamelStreamBuffer *stream, CamelStream *to, gchar *boundary_in, gchar *root_boundary)
{
	char *buffer = NULL;
	int w = 0, n = -1;
	gchar *boundary = boundary_in;
	gboolean open = FALSE, do_write = TRUE, first_write = FALSE;
	unsigned int len;
	GList *keep = NULL;
	gboolean theend = FALSE;

	while (!theend)
	{
		buffer = camel_stream_buffer_read_line (stream);

		if (!buffer) {
			theend = TRUE;
			continue;
		} else if (n == -1)
			n = 0;

		len = strlen (buffer);

		if (boundary == NULL)
		{
			   char *pstr = (char*) camel_strstrcase ((const char *) buffer, "boundary");

			   if (pstr)
			   {

				char *end;
				pstr = strchr (pstr, '"');
				if (pstr) {
					pstr++;
					end = strchr (pstr, '"');
					if (end) {
						*end='\0';
						boundary = g_strdup (pstr);
						root_boundary = boundary;
					}
				}
			   }

		} else if (strstr ((const char*) buffer, (const char*) boundary))
		{
			do_write = FALSE;

			g_list_foreach (keep, (GFunc) g_free, NULL);
			g_list_free (keep);
			keep = NULL;

			if (!open)
				open = TRUE;
			else if (open)
				open = FALSE;
		}

		if (open)
		{
			if (camel_strstrcase ((const char*) buffer, (const char*) "Content-Type") &&
					camel_strstrcase ((const char*) buffer, (const char*) "multipart/alternative") &&
					camel_strstrcase ((const char*) buffer, (const char*) "boundary"))
			{
				char *nboundary = NULL;
				char *pstr = (char*)camel_strstrcase ((const char *) buffer, "boundary");

				if (pstr)
				{
					char *end;
					pstr = strchr (pstr, '"');
					if (pstr) {
						pstr++;
						end = strchr (pstr, '"');
						if (end) {
							*end='\0';
							nboundary = g_strdup (pstr);
						}
					}
				}
				if (nboundary) {
					n = recursive_insanity (stream, to, nboundary, root_boundary);
					if (n == -1)
						break;
				}
			}

			if (strstr ((const char*) buffer, (const char*) "Content-Type") &&
					strstr ((const char*) buffer, (const char*) "text/"))
			{

				do_write = TRUE;
				first_write = TRUE;
			} else {
				if (strchr (buffer, ':'))
					keep = g_list_append (keep, g_strdup (buffer));
			}
		}

		if (do_write)
		{

		    if (first_write)
		    {
			GList *copy = keep;

			if (root_boundary != NULL)
			{
				gchar *nb = g_strdup_printf ("\n--%s\n", root_boundary);
				n = camel_stream_write(to, nb, strlen (nb));
				g_free (nb);
			}

			while (copy) {
				char *buf = copy->data;
				n = camel_stream_write(to, (const char*) buf, len);
				 if (n == -1 || camel_stream_write(to, "\n", 1) == -1)
					break;
				g_free (copy->data);
				copy = g_list_next (copy);
			}
			g_list_free (keep);
			keep = NULL;
		    }
		    first_write = FALSE;

		    n = camel_stream_write(to, (const char*) buffer, len);
		    if (n == -1 || camel_stream_write(to, "\n", 1) == -1)
			break;
		    w += n+1;

		}
	}

	if (boundary)
		g_free (boundary);

	return n;
}

void camel_imap_message_cache_replace_cache (CamelImapMessageCache *cache, const char *uid, const char *part_spec,
					  const char *dest_uid, const char *dest_part_spec)
{
	gchar *real = cachefile_get(cache->path, uid, part_spec);
	gchar *dest_real = cachefile_get(cache->path, dest_uid, dest_part_spec);

	rename (dest_real, real);

	g_free (real);
	g_free (dest_real);

}
void
camel_imap_message_cache_delete_attachments (CamelImapMessageCache *cache, const char *uid)
{
  char filename [512];

  CamelStream *in = camel_imap_message_cache_get (cache, uid, "", NULL);
  gchar *real1 = cachefile_get(cache->path, uid, NULL);

  snprintf(filename, 512, "%s.tmp", uid);
  gchar *real = g_build_filename (cache->path, filename, NULL);
  CamelStream *to = camel_stream_fs_new_with_name (real, O_RDWR|O_CREAT|O_TRUNC, 0600);
  int n;

  if (in && to)
  {
	CamelStreamBuffer *stream = (CamelStreamBuffer *) camel_stream_buffer_new (in, CAMEL_STREAM_BUFFER_READ);

	n = recursive_insanity (stream, to, NULL, NULL);

	/*if (boundary != NULL)
	{
	    gchar *nb = g_strdup_printf ("\n--%s\n", boundary);
	    n = camel_stream_write(to, nb, strlen (nb));
	    g_free (nb);
	}*/

	if (n != -1) {
		camel_stream_reset(to);
		/*if (theend)
			camel_imap_message_cache_set_partial (cache, uid, TRUE);
		else*/
 			camel_imap_message_cache_set_partial (cache, uid, TRUE);
	}

	camel_object_unref (stream);
	camel_object_unref (in);
	/*if (boundary)
		g_free (boundary);*/
	camel_object_unref (to);

	camel_imap_message_cache_remove (cache, uid);
	rename (real, real1);
  }

  g_free (real);
  g_free (real1);

}

/**
 * camel_imap_message_cache_remove:
 * @cache: the cache
 * @uid: UID of the data to remove
 *
 * Removes all data associated with @uid from @cache.
 **/
void
camel_imap_message_cache_remove (CamelImapMessageCache *cache, const char *uid)
{
	GPtrArray *subparts;
	char *key, *path;
	CamelObject *stream;
	int i;

	subparts = g_hash_table_lookup (cache->parts, uid);
	if (!subparts)
		return;
	for (i = 0; i < subparts->len; i++) {
		key = subparts->pdata[i];
		path = g_strdup_printf ("%s/%s", cache->path, key);
		g_unlink (path);
		g_free (path);
		stream = g_hash_table_lookup (cache->parts, key);
		if (stream) {
			camel_object_unhook_event (stream, "finalize",
						   stream_finalize, cache);
			camel_object_unref (stream);
			g_hash_table_remove (cache->cached, stream);
		}
		g_hash_table_remove (cache->parts, key);
		g_free (key);
	}
	g_hash_table_remove (cache->parts, uid);
	g_ptr_array_free (subparts, TRUE);
}

static void
add_uids (gpointer key, gpointer value, gpointer data)
{
	if (!strchr (key, '.'))
		g_ptr_array_add (data, key);
}

/**
 * camel_imap_message_cache_clear:
 * @cache: the cache
 *
 * Removes all cached data from @cache.
 **/
void
camel_imap_message_cache_clear (CamelImapMessageCache *cache)
{
	GPtrArray *uids;
	int i;

	uids = g_ptr_array_new ();
	g_hash_table_foreach (cache->parts, add_uids, uids);

	for (i = 0; i < uids->len; i++)
		camel_imap_message_cache_remove (cache, uids->pdata[i]);
	g_ptr_array_free (uids, TRUE);
}


/**
 * camel_imap_message_cache_copy:
 * @source: the source message cache
 * @source_uid: UID of a message in @source
 * @dest: the destination message cache
 * @dest_uid: UID of the message in @dest
 *
 * Copies all cached parts from @source_uid in @source to @dest_uid in
 * @destination.
 **/
void
camel_imap_message_cache_copy (CamelImapMessageCache *source,
			       const char *source_uid,
			       CamelImapMessageCache *dest,
			       const char *dest_uid,
			       CamelException *ex)
{
	GPtrArray *subparts;
	CamelStream *stream;
	char *part;
	int i;

	subparts = g_hash_table_lookup (source->parts, source_uid);
	if (!subparts || !subparts->len)
		return;

	for (i = 0; i < subparts->len; i++) {
		part = strchr (subparts->pdata[i], '.');
		if (!part++)
			continue;

		if ((stream = camel_imap_message_cache_get (source, source_uid, part, ex))) {
			camel_imap_message_cache_insert_stream (dest, dest_uid, part, stream, ex);
			camel_object_unref (CAMEL_OBJECT (stream));
		}
	}
}
