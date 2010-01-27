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

#include <tny-camel-mem-stream.h>
#include "tny-camel-stream-priv.h"


#include <camel/camel.h>

static GObjectClass *parent_class = NULL;

/**
 * tny_camel_mem_stream_new:
 *
 * Create a new in-memory stream
 *
 * Return value: a new #TnyStream instance
 **/
TnyStream*
tny_camel_mem_stream_new (void)
{
	TnyCamelMemStream *self = g_object_new (TNY_TYPE_CAMEL_MEM_STREAM, NULL);
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	priv->stream = camel_stream_mem_new ();

	return TNY_STREAM (self);
}

/**
 * tny_camel_mem_stream_new_with_buffer:
 * @buffer: a memory buffer to use as the stream data
 * @len: length of @buffer
 *
 * Create a new in-memory stream
 *
 * returns: (caller-owns): a new #TnyStream instance
 * since: 1.0
 * audience: application-developer
 **/
TnyStream*
tny_camel_mem_stream_new_with_buffer (const char *buffer, size_t len)
{
	TnyCamelMemStream *self = g_object_new (TNY_TYPE_CAMEL_MEM_STREAM, NULL);
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	priv->stream = camel_stream_mem_new_with_buffer (buffer, len);

	return TNY_STREAM (self);
}

static void
tny_camel_mem_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	return;
}

static void
tny_camel_mem_stream_finalize (GObject *object)
{
	(*parent_class->finalize) (object);
	return;
}


static void 
tny_camel_mem_stream_class_init (TnyFsStreamClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = tny_camel_mem_stream_finalize;

	return;
}

static gpointer 
tny_camel_mem_stream_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelMemStreamClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_mem_stream_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelMemStream),
			0,      /* n_preallocs */
			tny_camel_mem_stream_instance_init,   /* instance_init */
			NULL
		};
	
	
	type = g_type_register_static (TNY_TYPE_CAMEL_STREAM,
				       "TnyCamelMemStream",
				       &info, 0);

	return GUINT_TO_POINTER (type);
}

GType 
tny_camel_mem_stream_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_camel_mem_stream_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
