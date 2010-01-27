#ifndef TNY_FS_STREAM_CACHE_H
#define TNY_FS_STREAM_CACHE_H

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

#include <tny-stream-cache.h>

G_BEGIN_DECLS

#define TNY_TYPE_FS_STREAM_CACHE             (tny_fs_stream_cache_get_type ())
#define TNY_FS_STREAM_CACHE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_FS_STREAM_CACHE, TnyFsStreamCache))
#define TNY_FS_STREAM_CACHE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_FS_STREAM_CACHE, TnyFsStreamCacheClass))
#define TNY_IS_FS_STREAM_CACHE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_FS_STREAM_CACHE))
#define TNY_IS_FS_STREAM_CACHE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_FS_STREAM_CACHE))
#define TNY_FS_STREAM_CACHE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_FS_STREAM_CACHE, TnyFsStreamCacheClass))

#ifndef TNY_SHARED_H
typedef struct _TnyFsStreamCache TnyFsStreamCache;
typedef struct _TnyFsStreamCacheClass TnyFsStreamCacheClass;
#endif

struct _TnyFsStreamCache
{
	GObject parent;
};

struct _TnyFsStreamCacheClass 
{
	GObjectClass parent;
};

GType  tny_fs_stream_cache_get_type (void);
TnyStreamCache* tny_fs_stream_cache_new (const gchar *path, guint64 max_size);
const gchar *tny_fs_stream_cache_get_path (TnyFsStreamCache *path);

G_END_DECLS

#endif

