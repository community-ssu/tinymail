#ifndef TNY_CAMEL_STREAM_H
#define TNY_CAMEL_STREAM_H

/* libtinymail-camel - The Tiny Mail base library for Camel
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
#include <tny-stream.h>
#include <tny-seekable.h>

#include <camel/camel-stream.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_STREAM             (tny_camel_stream_get_type ())
#define TNY_CAMEL_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_STREAM, TnyCamelStream))
#define TNY_CAMEL_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_STREAM, TnyCamelStreamClass))
#define TNY_IS_CAMEL_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_STREAM))
#define TNY_IS_CAMEL_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_STREAM))
#define TNY_CAMEL_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_STREAM, TnyCamelStreamClass))

typedef struct _TnyCamelStream TnyCamelStream;
typedef struct _TnyCamelStreamClass TnyCamelStreamClass;

struct _TnyCamelStream
{
	GObject parent;
};

struct _TnyCamelStreamClass 
{
	GObjectClass parent;
};

GType tny_camel_stream_get_type (void);
TnyStream* tny_camel_stream_new (CamelStream *stream);

CamelStream *tny_camel_stream_get_stream (TnyCamelStream *self);

G_END_DECLS

#endif

