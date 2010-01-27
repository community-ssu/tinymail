#ifndef TNY_WEBKIT_STREAM_H
#define TNY_WEBKIT_STREAM_H

/* libtinymailui-webkit - The Tiny Mail UI library for Webkit
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
#include <gtk/gtk.h>
#include <glib-object.h>

#include <tny-stream.h>


G_BEGIN_DECLS

#define TNY_TYPE_WEBKIT_STREAM             (tny_webkit_stream_get_type ())
#define TNY_WEBKIT_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_WEBKIT_STREAM, TnyWebkitStream))
#define TNY_WEBKIT_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_WEBKIT_STREAM, TnyWebkitStreamClass))
#define TNY_IS_WEBKIT_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_WEBKIT_STREAM))
#define TNY_IS_WEBKIT_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_WEBKIT_STREAM))
#define TNY_WEBKIT_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_WEBKIT_STREAM, TnyWebkitStreamClass))

typedef struct _TnyWebkitStream TnyWebkitStream;
typedef struct _TnyWebkitStreamClass TnyWebkitStreamClass;
typedef struct _TnyWebkitStreamPriv TnyWebkitStreamPriv;

struct _TnyWebkitStream
{
	GObject parent;
	TnyWebkitStreamPriv *priv;
};

struct _TnyWebkitStreamClass 
{
	GObjectClass parent;
};

GType tny_webkit_stream_get_type (void);
TnyStream* tny_webkit_stream_new (TnyMimePartView *part_view);


G_END_DECLS

#endif

