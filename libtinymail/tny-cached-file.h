#ifndef TNY_FS_CACHED_FILE_H
#define TNY_FS_CACHED_FILE_H

/* libtinymail- The Tiny Mail base library
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

#include <glib.h>
#include <glib-object.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <tny-fs-stream-cache.h>
#include <tny-cached-file-stream.h>

G_BEGIN_DECLS

#define TNY_TYPE_CACHED_FILE                 (tny_cached_file_get_type ())
#define TNY_CACHED_FILE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CACHED_FILE, TnyCachedFile))
#define TNY_CACHED_FILE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CACHED_FILE, TnyCachedFileClass))
#define TNY_IS_CACHED_FILE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CACHED_FILE))
#define TNY_IS_CACHED_FILE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CACHED_FILE))
#define TNY_CACHED_FILE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CACHED_FILE, TnyCachedFileClass))

#ifndef TNY_SHARED_H
typedef struct _TnyCachedFile TnyCachedFile;
typedef struct _TnyCachedFileClass TnyCachedFileClass;
#endif

struct _TnyCachedFile
{
	GObject parent;
};

struct _TnyCachedFileClass 
{
	GObjectClass parent;

	/* virtuals */
	gint64 (*get_expected_size) (TnyCachedFile *self);
	const gchar * (*get_id) (TnyCachedFile *self);
	void (*remove) (TnyCachedFile *self);
	gboolean (*is_active) (TnyCachedFile *self);
	gboolean (*is_finished) (TnyCachedFile *self);
	time_t (*get_timestamp) (TnyCachedFile *self);
	TnyStream * (*get_stream) (TnyCachedFile *self);
	gint64 (*wait_fetchable) (TnyCachedFile *self, gint64 offset);
	void (*unregister_stream) (TnyCachedFile *self, TnyCachedFileStream *stream);
};

GType  tny_cached_file_get_type (void);
TnyCachedFile* tny_cached_file_new (TnyFsStreamCache *stream_cache, const gchar *id, gint64 expected_size, TnyStream *stream);

gint64 tny_cached_file_get_expected_size (TnyCachedFile *self);
const gchar *tny_cached_file_get_id (TnyCachedFile *self);
void tny_cached_file_remove (TnyCachedFile *self);
gboolean tny_cached_file_is_active (TnyCachedFile *self);
gboolean tny_cached_file_is_finished (TnyCachedFile *self);
TnyStream *tny_cached_file_get_stream (TnyCachedFile *self);
time_t tny_cached_file_get_timestamp (TnyCachedFile *self);
gint64 tny_cached_file_wait_fetchable (TnyCachedFile *self, gint64 offset);
void tny_cached_file_unregister_stream (TnyCachedFile *self, TnyCachedFileStream *stream);

G_END_DECLS

#endif

