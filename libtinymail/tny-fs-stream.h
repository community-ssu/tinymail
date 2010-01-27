#ifndef TNY_FS_STREAM_H
#define TNY_FS_STREAM_H

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

#include <tny-stream.h>
#include <tny-seekable.h>

G_BEGIN_DECLS

#define TNY_TYPE_FS_STREAM             (tny_fs_stream_get_type ())
#define TNY_FS_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_FS_STREAM, TnyFsStream))
#define TNY_FS_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_FS_STREAM, TnyFsStreamClass))
#define TNY_IS_FS_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_FS_STREAM))
#define TNY_IS_FS_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_FS_STREAM))
#define TNY_FS_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_FS_STREAM, TnyFsStreamClass))

#ifndef TNY_SHARED_H
typedef struct _TnyFsStream TnyFsStream;
typedef struct _TnyFsStreamClass TnyFsStreamClass;
#endif

struct _TnyFsStream
{
	GObject parent;
};

struct _TnyFsStreamClass 
{
	GObjectClass parent;
};

GType  tny_fs_stream_get_type (void);
TnyStream* tny_fs_stream_new (int fd);
void tny_fs_stream_set_fd (TnyFsStream *self, int fd);

G_END_DECLS

#endif

