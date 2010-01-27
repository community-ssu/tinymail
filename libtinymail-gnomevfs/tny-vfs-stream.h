#ifndef TNY_VFS_STREAM_H
#define TNY_VFS_STREAM_H

/* libtinymail-gnomevfs - The Tiny Mail base library for GnomeVFS
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
#include <libgnomevfs/gnome-vfs.h>
#include <glib-object.h>

#include <tny-stream.h>
#include <tny-seekable.h>

G_BEGIN_DECLS

#define TNY_TYPE_VFS_STREAM             (tny_vfs_stream_get_type ())
#define TNY_VFS_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_VFS_STREAM, TnyVfsStream))
#define TNY_VFS_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_VFS_STREAM, TnyVfsStreamClass))
#define TNY_IS_VFS_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_VFS_STREAM))
#define TNY_IS_VFS_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_VFS_STREAM))
#define TNY_VFS_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_VFS_STREAM, TnyVfsStreamClass))

typedef struct _TnyVfsStream TnyVfsStream;
typedef struct _TnyVfsStreamClass TnyVfsStreamClass;

struct _TnyVfsStream
{
	GObject parent;
};

struct _TnyVfsStreamClass 
{
	GObjectClass parent;
};

GType tny_vfs_stream_get_type (void);
TnyStream* tny_vfs_stream_new (GnomeVFSHandle *handle);

void tny_vfs_stream_set_handle (TnyVfsStream *self, GnomeVFSHandle *handle);

G_END_DECLS

#endif

