#ifndef TNY_CACHED_FILE_STREAM_H
#define TNY_CACHED_FILE_STREAM_H

/* libtinymail- The Tiny Mail base library
 * Copyright (C) 2006-2007 Philip Van Hoof <pvanhoof@gnome.org>
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

#include <tny-fs-stream.h>
#include <tny-cached-file.h>

G_BEGIN_DECLS

#define TNY_TYPE_CACHED_FILE_STREAM             (tny_cached_file_stream_get_type ())
#define TNY_CACHED_FILE_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CACHED_FILE_STREAM, TnyCachedFileStream))
#define TNY_CACHED_FILE_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CACHED_FILE_STREAM, TnyCachedFileStreamClass))
#define TNY_IS_CACHED_FILE_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CACHED_FILE_STREAM))
#define TNY_IS_CACHED_FILE_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CACHED_FILE_STREAM))
#define TNY_CACHED_FILE_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CACHED_FILE_STREAM, TnyCachedFileStreamClass))

#ifndef TNY_SHARED_H
typedef struct _TnyCachedFileStream TnyCachedFileStream;
typedef struct _TnyCachedFileStreamClass TnyCachedFileStreamClass;
#endif

struct _TnyCachedFileStream
{
	TnyFsStream parent;
};

struct _TnyCachedFileStreamClass 
{
	TnyFsStreamClass parent;
};

GType  tny_cached_file_stream_get_type (void);
TnyStream* tny_cached_file_stream_new (TnyCachedFile *cached_file, int fd);

G_END_DECLS

#endif

