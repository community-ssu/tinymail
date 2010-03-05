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
 * TnyFsStreamCache:
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

typedef struct _TnyFsStreamCachePriv TnyFsStreamCachePriv;

struct _TnyFsStreamCachePriv
{
	gchar *path;
	gint64 max_size;
	gint64 current_size;
	GHashTable *cached_files;
	GStaticMutex *cache_lock;
};

#define TNY_FS_STREAM_CACHE_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_FS_STREAM_CACHE, TnyFsStreamCachePriv))

static void
count_active_files_size (gchar *id, TnyCachedFile *cached_file, gint64* count)
{
	if (tny_cached_file_is_active (cached_file))
		*count += tny_cached_file_get_expected_size (cached_file);
}

static void
count_files_size (gchar *id, TnyCachedFile *cached_file, gint64* count)
{
	*count += tny_cached_file_get_expected_size (cached_file);
}

static gint64
get_available_size (TnyFsStreamCache *self)
{
	TnyFsStreamCachePriv *priv;
	guint64 active_files_size = 0;
	priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);

	g_static_mutex_lock (priv->cache_lock);
	g_hash_table_foreach (priv->cached_files, (GHFunc) count_active_files_size, &active_files_size);
	g_static_mutex_unlock (priv->cache_lock);

	/* if available size < 0 it means we shrinked cache and we need to drop files */
	return priv->max_size - active_files_size;
}

static gint64
get_free_space (TnyFsStreamCache *self)
{
	TnyFsStreamCachePriv *priv;
	guint64 files_size = 0;
	priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);

	g_static_mutex_lock (priv->cache_lock);
	g_hash_table_foreach (priv->cached_files, (GHFunc) count_files_size, &files_size);
	g_static_mutex_unlock (priv->cache_lock);

	/* if available size < 0 it means we shrinked cache and we need to drop files */
	return priv->max_size - files_size;
}

static void
get_inactive_files_list (gchar *id, TnyCachedFile *cached_file, GList **list)
{
	if (!tny_cached_file_is_active (cached_file)) {
		*list = g_list_prepend (*list, g_object_ref (cached_file));
	}
}

static gint 
remove_priority (TnyCachedFile *cf1, TnyCachedFile *cf2)
{
	gint result = 0;

	/* IMPORTANT: priority is lower = before */

	/* non finished has priority to be removed */
	result = ((gint) (!tny_cached_file_is_finished (cf2))) - ((gint) (!tny_cached_file_is_finished (cf1)));
	
	if (result == 0) {
		/* older has priority to be removed */
		result = tny_cached_file_get_timestamp (cf1) - tny_cached_file_get_timestamp (cf2);
	}

	return result;
}

static void
remove_old_files (TnyFsStreamCache *self, gint64 required_size)
{

	/* 1. we obtain a list of non active files
	 * 2. we sort the list (first uncomplete, then old)
	 * 3. we get items in list and remove them until we have the required size
	 */
	GList *cached_files_list;
	TnyFsStreamCachePriv *priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);
	gint64 available_size;

	/* 1. we obtain a list of non active files */
	cached_files_list = NULL;
	g_static_mutex_lock (priv->cache_lock);
	g_hash_table_foreach (priv->cached_files, (GHFunc) get_inactive_files_list, &cached_files_list);
	g_static_mutex_unlock (priv->cache_lock);

	/* 2. we sort the list (first uncomplete, then old) */
	cached_files_list = g_list_sort (cached_files_list, (GCompareFunc) remove_priority);

	/* 3. we get items in list and remove them until we have the required size */
	available_size = get_free_space (self);
	while (available_size < required_size) {
		TnyCachedFile *cached_file = (TnyCachedFile *) cached_files_list->data;
		available_size += tny_cached_file_get_expected_size (cached_file);
		g_static_mutex_lock (priv->cache_lock);
		g_hash_table_remove (priv->cached_files, tny_cached_file_get_id (cached_file));
		g_static_mutex_unlock (priv->cache_lock);
		tny_cached_file_remove (cached_file);
		cached_files_list = g_list_delete_link (cached_files_list, cached_files_list);
		g_object_unref (cached_file);
	}

	g_list_foreach (cached_files_list, (GFunc) g_object_unref, NULL);
	g_list_free (cached_files_list);
	
}

static TnyStream *
tny_fs_stream_cache_get_stream (TnyStreamCache *self, const gchar *id,
				TnyStreamCacheOpenStreamFetcher fetcher, gpointer userdata)
{
	TnyStream *result = NULL;
	TnyFsStreamCachePriv *priv;
	TnyCachedFile *cached_file;

	g_return_val_if_fail (TNY_IS_FS_STREAM_CACHE (self), NULL);
	g_return_val_if_fail (id, NULL);
	priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);

	g_static_mutex_lock (priv->cache_lock);
	cached_file = g_hash_table_lookup (priv->cached_files, id);
	g_static_mutex_unlock (priv->cache_lock);
	if (cached_file) {
		result = tny_cached_file_get_stream (cached_file);
	} else {
		TnyStream *input_stream;
		gint64 expected_size = 0;

		input_stream = fetcher (self, &expected_size, userdata);

		if (input_stream == NULL)
			return NULL;

		if (expected_size <= get_available_size (TNY_FS_STREAM_CACHE (self))) {
			remove_old_files (TNY_FS_STREAM_CACHE (self), expected_size);
			cached_file = tny_cached_file_new (TNY_FS_STREAM_CACHE (self), id, expected_size, input_stream);
			g_static_mutex_lock (priv->cache_lock);
			g_hash_table_insert (priv->cached_files, g_strdup (id), cached_file);
			g_static_mutex_unlock (priv->cache_lock);
			result = tny_cached_file_get_stream (cached_file);
		} else {
			result = input_stream;
		}
	}

	return result;
}

static gint64
tny_fs_stream_cache_get_max_size (TnyStreamCache *self)
{
	TnyFsStreamCachePriv *priv;
	
	g_return_val_if_fail (TNY_IS_FS_STREAM_CACHE (self), 0);
	priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);

	return priv->max_size;
}

static void
tny_fs_stream_cache_set_max_size (TnyStreamCache *self, gint64 max_size)
{
	TnyFsStreamCachePriv *priv;

	g_return_if_fail (TNY_IS_FS_STREAM_CACHE (self));
	priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);

	priv->max_size = max_size;
	
	/* if se shrink the cache, then we have to drop old files */
	remove_old_files (TNY_FS_STREAM_CACHE (self), 0);
}

typedef struct {
	TnyFsStreamCache *self;
	TnyStreamCacheRemoveFilter filter;
	gpointer userdata;
} RemoveFilterData;

static gboolean
remove_filter (gchar *id, TnyCachedFile *cached_file, RemoveFilterData *remove_filter_data)
{
	gboolean result;

	result = remove_filter_data->filter (TNY_STREAM_CACHE (remove_filter_data->self), id, remove_filter_data->userdata);
	if (result) {
		tny_cached_file_remove (cached_file);
	}
	return result;
}

static void
tny_fs_stream_cache_remove (TnyStreamCache *self, TnyStreamCacheRemoveFilter filter, gpointer userdata)
{
	TnyFsStreamCachePriv *priv;
	RemoveFilterData *remove_filter_data;

	g_return_if_fail (TNY_IS_FS_STREAM_CACHE (self));
	priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);

	remove_filter_data = g_slice_new (RemoveFilterData);
	remove_filter_data->self = TNY_FS_STREAM_CACHE (self);
	remove_filter_data->filter = filter;
	remove_filter_data->userdata = userdata;

	g_static_mutex_lock (priv->cache_lock);
	g_hash_table_foreach_remove (priv->cached_files, (GHRFunc) remove_filter, (gpointer) remove_filter_data);
	g_static_mutex_unlock (priv->cache_lock);

	g_slice_free (RemoveFilterData, remove_filter_data);
}

static void
fill_from_cache (TnyFsStreamCache *self)
{
	TnyFsStreamCachePriv *priv;
	GDir *dir;
	const gchar *filename;

	priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);
	dir = g_dir_open (priv->path, 0, NULL);

	while ((filename = g_dir_read_name (dir)) != NULL) {
		gchar *fullname;

		fullname = g_build_filename (priv->path, filename, NULL);
		if (g_str_has_suffix (filename, ".tmp")) {
			g_unlink (fullname);
		} else {
			TnyCachedFile *cached_file;

			cached_file = tny_cached_file_new (self, filename, 0, NULL);
			priv->current_size += tny_cached_file_get_expected_size (cached_file);
			g_static_mutex_lock (priv->cache_lock);
			g_hash_table_insert (priv->cached_files, g_strdup (filename), cached_file);
			g_static_mutex_unlock (priv->cache_lock);
		}
		g_free (fullname);
	}

	g_dir_close (dir);
}

/**
 * tny_fs_stream_cache_new:
 * @path: a folder path
 *
 * Creates a stream cache that stores cached streams as files in a filesystem
 * folder specified by @path.
 *
 * returns: (caller-owns): a new #TnyStream instance
 * since: 1.0
 * audience: application-developer
 **/
TnyStreamCache*
tny_fs_stream_cache_new (const char *path, guint64 max_size)
{
	TnyFsStreamCachePriv *priv;
	TnyFsStreamCache *self;

	self = g_object_new (TNY_TYPE_FS_STREAM_CACHE, NULL);
	priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);

	priv->path = g_strdup (path);
	priv->current_size = 0;
	priv->max_size = max_size;
	if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
		g_mkdir_with_parents (path, 0777);
	}
	fill_from_cache (TNY_FS_STREAM_CACHE (self));
	return TNY_STREAM_CACHE (self);
}

const gchar *
tny_fs_stream_cache_get_path (TnyFsStreamCache *self)
{
	TnyFsStreamCachePriv *priv;

	g_return_val_if_fail (TNY_IS_FS_STREAM_CACHE (self), NULL);
	priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);

	return priv->path;
	
}

static void
tny_fs_stream_cache_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyFsStreamCache *self = (TnyFsStreamCache *)instance;
	TnyFsStreamCachePriv *priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);

	priv->path = NULL;
	priv->max_size = 0;
	priv->cached_files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
	priv->cache_lock = g_new0 (GStaticMutex, 1);

	return;
}

static void
tny_fs_stream_cache_finalize (GObject *object)
{
	TnyFsStreamCache *self = (TnyFsStreamCache *)object;
	TnyFsStreamCachePriv *priv = TNY_FS_STREAM_CACHE_GET_PRIVATE (self);

	if (priv->cached_files) {
		if (priv->cache_lock)
			g_static_mutex_lock (priv->cache_lock);
		g_hash_table_unref (priv->cached_files);
		priv->cached_files = NULL;
		if (priv->cache_lock)
			g_static_mutex_unlock (priv->cache_lock);
	}

	if (priv->cache_lock) {
		g_free (priv->cache_lock);
		priv->cache_lock = NULL;
	}

	if (priv->path) {
		g_free (priv->path);
		priv->path = NULL;
	}

	(*parent_class->finalize) (object);
	return;
}


static void
tny_stream_cache_init (gpointer g, gpointer iface_data)
{
	TnyStreamCacheIface *klass = (TnyStreamCacheIface *)g;

	klass->get_stream = tny_fs_stream_cache_get_stream;
	klass->remove = tny_fs_stream_cache_remove;
	klass->set_max_size = tny_fs_stream_cache_set_max_size;
	klass->get_max_size = tny_fs_stream_cache_get_max_size;

	return;
}

static void 
tny_fs_stream_cache_class_init (TnyFsStreamCacheClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = tny_fs_stream_cache_finalize;
	g_type_class_add_private (object_class, sizeof (TnyFsStreamCachePriv));

	return;
}

static gpointer
tny_fs_stream_cache_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyFsStreamCacheClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_fs_stream_cache_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyFsStreamCache),
			0,      /* n_preallocs */
			tny_fs_stream_cache_instance_init,   /* instance_init */
			NULL
		};
	
	static const GInterfaceInfo tny_stream_cache_info = 
		{
			(GInterfaceInitFunc) tny_stream_cache_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyFsStreamCache",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_STREAM_CACHE, 
				     &tny_stream_cache_info);
	
	return GSIZE_TO_POINTER (type);
}

GType 
tny_fs_stream_cache_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_fs_stream_cache_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
