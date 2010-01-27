#ifndef TNY_CAMEL_MEM_STREAM_H
#define TNY_CAMEL_MEM_STREAM_H

/* libtinymail-camel The Tiny Mail base library for Camel
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

#include <tny-camel-stream.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_MEM_STREAM             (tny_camel_mem_stream_get_type ())
#define TNY_CAMEL_MEM_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_MEM_STREAM, TnyCamelMemStream))
#define TNY_CAMEL_MEM_STREAM_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_MEM_STREAM, TnyCamelMemStreamClass))
#define TNY_IS_CAMEL_MEM_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_MEM_STREAM))
#define TNY_IS_CAMEL_MEM_STREAM_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_MEM_STREAM))
#define TNY_CAMEL_MEM_STREAM_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_MEM_STREAM, TnyCamelMemStreamClass))

typedef struct _TnyCamelMemStream TnyCamelMemStream;
typedef struct _TnyCamelMemStreamClass TnyCamelMemStreamClass;

struct _TnyCamelMemStream
{
	TnyCamelStream parent;
};

struct _TnyCamelMemStreamClass 
{
	TnyCamelStreamClass parent;
};

GType  tny_camel_mem_stream_get_type (void);
TnyStream* tny_camel_mem_stream_new (void);
TnyStream* tny_camel_mem_stream_new_with_buffer (const char *buffer, size_t len);

G_END_DECLS

#endif

