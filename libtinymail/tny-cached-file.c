/* libtinymail - The Tiny Mail base library
 * Copyright (C) 2008 Jose Dapena Paz <jdapena@igalia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with self library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * TnyCachedFile:
 *
 * Stream cache storing cached streams in a folder as files.
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <glib/gstdio.h>

#include <tny-fs-stream.h>
#include <tny-fs-stream-cache.h>
#include <tny-cached-file.h>

static GObjectClass *parent_class = NULL;

typedef struct _TnyCachedFilePriv TnyCachedFilePriv;

struct _TnyCachedFilePriv
{
	/* stream cache the file belongs to */
	TnyStreamCache *stream_cache;
	/* id in cache of the file */
	gchar *id;

	/* expected size the file will get */
	gint64 expected_size;
	/* timestamp of last access to stream */
	time_t timestamp;
	
	/* Input stream we use to feed the cache */
	TnyStream *fetch_stream;
	/* Currently fetched size in cache */
	gint64 fetched_size;
	/* is the stream invalid? Due to errors reading, or closing the stream */
	gboolean invalid;
	/* Close is pending when we run remove and we're still fetching the
	 * stream */
	gboolean pending_close;
	/* mutex and gcond for fetched size current size */
	GMutex *fetched_mutex;
	GCond *fetched_cond;

	/* active read operations */
	gint active_ops;
	GMutex *active_ops_mutex;

};

#define TNY_CACHED_FILE_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_CACHED_FILE, TnyCachedFilePriv))

gint64
tny_cached_file_get_expected_size (TnyCachedFile *self)
{
	return TNY_CACHED_FILE_GET_CLASS (self)->get_expected_size (self);
}

static gint64
tny_cached_file_get_expected_size_default (TnyCachedFile *self)
{
	TnyCachedFilePriv *priv = TNY_CACHED_FILE_GET_PRIVATE (self);

	return priv->expected_size;
}

const gchar *
tny_cached_file_get_id (TnyCachedFile *self)
{
	return TNY_CACHED_FILE_GET_CLASS (self)->get_id (self);
}

static const gchar *
tny_cached_file_get_id_default (TnyCachedFile *self)
{
	TnyCachedFilePriv *priv = TNY_CACHED_FILE_GET_PRIVATE (self);

	return priv->id;
}

gboolean
tny_cached_file_is_active (TnyCachedFile *self)
{
	return TNY_CACHED_FILE_GET_CLASS (self)->is_active (self);
}

static gboolean
tny_cached_file_is_active_default (TnyCachedFile *self)
{
	TnyCachedFilePriv *priv = TNY_CACHED_FILE_GET_PRIVATE (self);
	gboolean retval;

	g_mutex_lock (priv->active_ops_mutex);
	retval = (priv->active_ops > 0);
	g_mutex_unlock (priv->active_ops_mutex);
	
	return retval;
}

gboolean
tny_cached_file_is_finished (TnyCachedFile *self)
{
	return TNY_CACHED_FILE_GET_CLASS (self)->is_finished (self);
}

static gboolean
tny_cached_file_is_finished_default (TnyCachedFile *self)
{
	TnyCachedFilePriv *priv = TNY_CACHED_FILE_GET_PRIVATE (self);

	return (priv->fetch_stream == NULL);
}

time_t
tny_cached_file_get_timestamp (TnyCachedFile *self)
{
	return TNY_CACHED_FILE_GET_CLASS (self)->get_timestamp (self);
}

static time_t
tny_cached_file_get_timestamp_default (TnyCachedFile *self)
{
	TnyCachedFilePriv *priv = TNY_CACHED_FILE_GET_PRIVATE (self);

	if (priv->active_ops > 0)
		priv->timestamp = time (NULL);

	return priv->timestamp;
}

void
tny_cached_file_remove (TnyCachedFile *self)
{
	return TNY_CACHED_FILE_GET_CLASS (self)->remove (self);
}

static void
tny_cached_file_remove_default (TnyCachedFile *self)
{
	TnyCachedFilePriv *priv = TNY_CACHED_FILE_GET_PRIVATE (self);
	
	g_return_if_fail (priv->active_ops == 0);

	priv->invalid = TRUE;
	if (priv->fetch_stream) {
		priv->pending_close = TRUE;
	} else {
		gchar *fullname;

		fullname = g_build_filename (tny_fs_stream_cache_get_path (TNY_FS_STREAM_CACHE (priv->stream_cache)), priv->id, NULL);
		g_unlink (fullname);
		g_free (fullname);
		priv->invalid = TRUE;
	}
}

static gint
get_new_fd_of_stream (TnyCachedFile *self, gboolean creation, gboolean temp)
{
	TnyCachedFilePriv *priv;
	gint new_fd;
	gint flags;
	gchar *filename;
	gchar *fullpath;

	priv = TNY_CACHED_FILE_GET_PRIVATE (self);
	flags = O_RDWR;
	if (creation) {
		flags = O_CREAT | O_RDWR | O_SYNC;
	} else {
		flags |= O_RDONLY;
	}

	if (temp) {
		filename = g_strconcat (priv->id, ".tmp", NULL);
	} else {
		filename = g_strdup (priv->id);
	}
	fullpath = g_build_filename (tny_fs_stream_cache_get_path (TNY_FS_STREAM_CACHE (priv->stream_cache)),
				     filename, NULL);
	g_free (filename);

	new_fd = g_open (fullpath, flags, 0660);
	g_free (fullpath);

	return new_fd;
}

TnyStream *
tny_cached_file_get_stream (TnyCachedFile *self)
{
	return TNY_CACHED_FILE_GET_CLASS (self)->get_stream (self);
}

static TnyStream *
tny_cached_file_get_stream_default (TnyCachedFile *self)
{
	TnyStream *stream;
	TnyCachedFilePriv *priv;
	gint new_fd;

	priv = TNY_CACHED_FILE_GET_PRIVATE (self);

	if (priv->invalid)
		return NULL;

	new_fd = get_new_fd_of_stream (self, FALSE, priv->fetch_stream != FALSE);
	lseek (new_fd, 0, SEEK_SET);
	g_mutex_lock (priv->active_ops_mutex);
	priv->active_ops ++;
	g_mutex_unlock (priv->active_ops_mutex);

	stream = tny_cached_file_stream_new (self, new_fd);

	return stream;
}

gint64
tny_cached_file_wait_fetchable (TnyCachedFile *self, gint64 offset)
{
	return TNY_CACHED_FILE_GET_CLASS (self)->wait_fetchable (self, offset);
}

static gint64
tny_cached_file_wait_fetchable_default (TnyCachedFile *self, gint64 offset)
{
	TnyCachedFilePriv *priv;
	gint64 retval;

	priv = TNY_CACHED_FILE_GET_PRIVATE (self);

	g_mutex_lock (priv->fetched_mutex);

	while ((!priv->invalid) && (offset > priv->fetched_size)) {
		g_cond_wait(priv->fetched_cond, priv->fetched_mutex);
	}

	if (priv->invalid) {
		retval = -1;
	} else {
		retval = priv->fetched_size;
	}

	g_mutex_unlock (priv->fetched_mutex);

	return retval;
}

void
tny_cached_file_unregister_stream (TnyCachedFile *self, TnyCachedFileStream *stream)
{
	TNY_CACHED_FILE_GET_CLASS (self)->unregister_stream (self, stream);	
}

static void
tny_cached_file_unregister_stream_default (TnyCachedFile *self, TnyCachedFileStream *stream)
{
	TnyCachedFilePriv *priv;

	priv = TNY_CACHED_FILE_GET_PRIVATE (self);

	g_mutex_lock (priv->active_ops_mutex);
	priv->active_ops --;
	g_mutex_unlock (priv->active_ops_mutex);
}


typedef struct {
	TnyCachedFile *self;
	gint write_fd;
} AsyncFetchStreamData;

static gpointer
async_fetch_stream (gpointer userdata)
{
	AsyncFetchStreamData *afs_data = (AsyncFetchStreamData *) userdata;
	TnyCachedFile *self;
	TnyCachedFilePriv *priv;
	TnyStream *write_stream;

	self = afs_data->self;
	priv = TNY_CACHED_FILE_GET_PRIVATE (self);

	/* First we dump the read stream contents to file cache */
	write_stream = tny_fs_stream_new (afs_data->write_fd);
	while (!tny_stream_is_eos (priv->fetch_stream)) {
		gssize readed, written;
		char buffer[1024];

		readed = tny_stream_read (priv->fetch_stream, buffer, 1024);
		if (readed == -1 || priv->pending_close) {
			g_mutex_lock (priv->fetched_mutex);
			priv->invalid = TRUE;
			g_cond_broadcast (priv->fetched_cond);
			g_mutex_unlock (priv->fetched_mutex);
			break;
		}
		written = 0;
		while (written < readed) {
			gssize written_now;

			written_now = tny_stream_write (write_stream, buffer + written, readed - written);
			if (written_now == -1 || priv->pending_close) {
				g_mutex_lock (priv->fetched_mutex);
				priv->invalid = TRUE;
				g_cond_broadcast (priv->fetched_cond);
				g_mutex_unlock (priv->fetched_mutex);
				break;
			}
			written += written_now;
		}
		tny_stream_flush (write_stream);
		g_mutex_lock (priv->fetched_mutex);
		priv->fetched_size += written;
		g_cond_broadcast (priv->fetched_cond);
		g_mutex_unlock (priv->fetched_mutex);
	}

	tny_stream_close (priv->fetch_stream);
	g_object_unref (priv->fetch_stream);
	priv->fetch_stream = NULL;

	if (!priv->invalid) {
		gchar *tmp_filename;
		gchar *old_name;
		gchar *new_name;

		tmp_filename = g_strconcat (priv->id, ".tmp", NULL);
		old_name = g_build_filename (tny_fs_stream_cache_get_path (TNY_FS_STREAM_CACHE (priv->stream_cache)), tmp_filename, NULL);
		new_name = g_build_filename (tny_fs_stream_cache_get_path (TNY_FS_STREAM_CACHE (priv->stream_cache)), priv->id, NULL);
		g_free (tmp_filename);

		if (g_rename (old_name, new_name) == -1) {
			g_mutex_lock (priv->fetched_mutex);
			priv->invalid = TRUE;
			g_cond_broadcast (priv->fetched_cond);
			g_mutex_unlock (priv->fetched_mutex);
		}
	}

	tny_stream_close (write_stream);
	g_object_unref (write_stream);

	if (priv->invalid) {
		gchar *tmp_filename;
		gchar *fullname;

		tmp_filename = g_strconcat (priv->id, ".tmp", NULL);
		fullname = g_build_filename (tny_fs_stream_cache_get_path (TNY_FS_STREAM_CACHE (priv->stream_cache)), tmp_filename, NULL);
		g_unlink (fullname);
	} else {
		priv->expected_size = priv->fetched_size;
	}

	g_slice_free (AsyncFetchStreamData, afs_data);

	return NULL;
}

/**
 * tny_cached_file_new:
 * @stream_cache: the cache the file will be added to
 * @id: the stream unique-id
 * @fetcher: a #TnyStreamCacheOpenStreamFetcher
 * @userdata: a #gpointer
 *
 * Creates a cached file information object. This object maintains the stream
 * that fetches the file @id into @stream_cache, and gives valid TnySeekable
 * streams for users.
 *
 * If @fetcher and @userdata are %NULL, then it assumes the file is already in 
 * @stream_cache
 *
 * returns: (caller-owns): a new #TnyCachedFile instance
 * since: 1.0
 * audience: application-developer
 **/
TnyCachedFile*
tny_cached_file_new (TnyFsStreamCache *stream_cache, const char *id, gint64 expected_size, TnyStream *input_stream)
{
	TnyCachedFile *result;
	TnyCachedFilePriv *priv;

	g_return_val_if_fail (id, NULL);
	g_return_val_if_fail (expected_size >= 0, NULL);
	g_return_val_if_fail (TNY_IS_FS_STREAM_CACHE (stream_cache), NULL);

	result = g_object_new (TNY_TYPE_CACHED_FILE, NULL);
	priv = TNY_CACHED_FILE_GET_PRIVATE (result);

	priv->expected_size = expected_size;
	priv->id = g_strdup (id);
	priv->stream_cache = g_object_ref (stream_cache);

	if (input_stream) {
		AsyncFetchStreamData *afs_data;

		priv->fetch_stream = g_object_ref (input_stream);

		/* we begin retrieval of the stream to disk. This should be cancellable */
		afs_data = g_slice_new0 (AsyncFetchStreamData);
		afs_data->self = g_object_ref (result);
		afs_data->write_fd = get_new_fd_of_stream (result, TRUE, TRUE);
		lseek (afs_data->write_fd, 0, SEEK_SET);
		g_thread_create (async_fetch_stream, afs_data, FALSE, NULL);
	} else {
		gchar *fullpath;
		struct stat filestat;
		fullpath = g_build_filename (tny_fs_stream_cache_get_path (TNY_FS_STREAM_CACHE (priv->stream_cache)),
					     priv->id, NULL);
		if (g_stat (fullpath, &filestat)== -1) {
			g_free (fullpath);
			g_object_unref (result);
			return NULL;
		}
		priv->timestamp = filestat.st_atime;
		priv->expected_size = priv->fetched_size = filestat.st_size;
		g_free (fullpath);
	}

	return result;
}

static void
tny_cached_file_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCachedFile *self = (TnyCachedFile *)instance;
	TnyCachedFilePriv *priv = TNY_CACHED_FILE_GET_PRIVATE (self);

	priv->expected_size = 0;
	priv->id = NULL;
	priv->active_ops = 0;
	priv->active_ops_mutex = g_mutex_new ();
	priv->fetch_stream = NULL;
	priv->stream_cache = NULL;
	priv->timestamp = 0;
	priv->pending_close = FALSE;
	priv->invalid = FALSE;
	priv->fetched_size = 0;
	priv->fetched_mutex = g_mutex_new ();
	priv->fetched_cond = g_cond_new ();

	return;
}

static void
tny_cached_file_finalize (GObject *object)
{
	TnyCachedFile *self = (TnyCachedFile *)object;
	TnyCachedFilePriv *priv = TNY_CACHED_FILE_GET_PRIVATE (self);

	if (priv->fetch_stream) {
		g_object_unref (priv->fetch_stream);
		priv->fetch_stream = NULL;
	}

	if (priv->id) {
		g_free (priv->id);
		priv->id = NULL;
	}

	if (priv->stream_cache) {
		g_object_unref (priv->stream_cache);
		priv->stream_cache = NULL;
	}

	if (priv->active_ops_mutex) {
		g_mutex_free (priv->active_ops_mutex);
		priv->active_ops_mutex = NULL;
	}

	if (priv->fetched_mutex) {
		g_mutex_free (priv->fetched_mutex);
		priv->fetched_mutex = NULL;
	}

	if (priv->fetched_cond) {
		g_cond_free (priv->fetched_cond);
		priv->fetched_cond = NULL;
	}

	(*parent_class->finalize) (object);
	return;
}


static void 
tny_cached_file_class_init (TnyCachedFileClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = tny_cached_file_finalize;
	class->get_expected_size = tny_cached_file_get_expected_size_default;
	class->get_id = tny_cached_file_get_id_default;
	class->remove = tny_cached_file_remove_default;
	class->get_stream = tny_cached_file_get_stream_default;
	class->is_finished = tny_cached_file_is_finished_default;
	class->is_active = tny_cached_file_is_active_default;
	class->wait_fetchable = tny_cached_file_wait_fetchable_default;
	class->get_timestamp = tny_cached_file_get_timestamp_default;
	class->unregister_stream = tny_cached_file_unregister_stream_default;
	g_type_class_add_private (object_class, sizeof (TnyCachedFilePriv));

	return;
}

static gpointer
tny_cached_file_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCachedFileClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_cached_file_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCachedFile),
			0,      /* n_preallocs */
			tny_cached_file_instance_init,   /* instance_init */
			NULL
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCachedFile",
				       &info, 0);

	return GUINT_TO_POINTER (type);
}

GType 
tny_cached_file_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_cached_file_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
