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

#include <config.h>

#include <glib.h>

#include <string.h>

#include <tny-stream.h>
#include <tny-camel-stream.h>
#include <tny-folder.h>
#include <tny-camel-folder.h>

#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-store.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <tny-session-camel.h>

#include <tny-camel-shared.h>
#include "tny-camel-stream-priv.h"

static GObjectClass *parent_class = NULL;


static gssize
tny_camel_stream_write_to_stream (TnyStream *self, TnyStream *output)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);
	CamelStream *stream = priv->stream;
	char tmp_buf[4096];
	gssize total = 0;
	gssize nb_read;
	gssize nb_written;

	g_return_val_if_fail (CAMEL_IS_STREAM (stream), -1);
	g_return_val_if_fail (TNY_IS_STREAM (output), -1);

	while (G_LIKELY (!camel_stream_eos (stream))) 
	{
		nb_read = camel_stream_read (stream, tmp_buf, sizeof (tmp_buf));
		if (G_UNLIKELY (nb_read < 0))
			return -1;
		else if (G_LIKELY (nb_read > 0)) 
		{
			nb_written = 0;
	
			while (G_UNLIKELY (nb_written < nb_read)) 
			{
				gssize len = tny_stream_write (output, tmp_buf + nb_written,
								  nb_read - nb_written);
				if (G_UNLIKELY (len < 0))
					return -1;
				nb_written += len;
			}
			total += nb_written;
		}
	}
	return total;
}

static gssize
tny_camel_stream_read  (TnyStream *self, char *buffer, gsize n)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	return camel_stream_read (priv->stream, buffer, n);
}

static gssize
tny_camel_stream_write (TnyStream *self, const char *buffer, gsize n)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	return camel_stream_write (priv->stream, buffer, n);
}

static gint
tny_camel_stream_flush (TnyStream *self)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	return camel_stream_flush (priv->stream);
}

static gint
tny_camel_stream_close (TnyStream *self)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	return camel_stream_close (priv->stream);
}

static gboolean
tny_camel_stream_is_eos   (TnyStream *self)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	return camel_stream_eos (priv->stream);
}

static gint
tny_camel_stream_reset (TnyStream *self)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	return camel_stream_reset (priv->stream);
}

static off_t 
tny_camel_stream_seek (TnySeekable *self, off_t offset, int policy)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);
	if (CAMEL_IS_SEEKABLE_STREAM (priv->stream))
		return camel_seekable_stream_seek ((CamelSeekableStream *) priv->stream, offset, (CamelStreamSeekPolicy) policy);
	return -1;
}


static off_t
tny_camel_stream_tell (TnySeekable *self)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);
	if (CAMEL_IS_SEEKABLE_STREAM (priv->stream))
		return camel_seekable_stream_tell ((CamelSeekableStream *) priv->stream);
	return -1;
}

static gint 
tny_camel_stream_set_bounds (TnySeekable *self, off_t start, off_t end)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);
	if (CAMEL_IS_SEEKABLE_STREAM (priv->stream))
		return camel_seekable_stream_set_bounds ((CamelSeekableStream *) priv->stream, start, end);
	return -1;
}


/**
 * tny_camel_stream_get_stream:
 * @self: A #TnyCamelStream object
 *
 * The returned value must be unreferenced 
 *
 * Return value: the stream to play proxy for
 **/
CamelStream *
tny_camel_stream_get_stream (TnyCamelStream *self)
{
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	if (priv->stream)
		camel_object_ref (priv->stream);

	return priv->stream;
}


/**
 * tny_camel_stream_new:
 * @stream: A #CamelStream stream to play proxy for
 *
 * Create a new #TnyStream instance implemented as a proxy
 * for a #CamelStream
 * 
 * Return value: A new #TnyStream instance implemented as a proxy
 * for a #CamelStream
 **/
TnyStream*
tny_camel_stream_new (CamelStream *stream)
{
	TnyCamelStream *self = g_object_new (TNY_TYPE_CAMEL_STREAM, NULL);
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	camel_object_ref (CAMEL_OBJECT (stream));
	priv->stream = stream;

	return TNY_STREAM (self);
}

static void
tny_camel_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelStream *self = (TnyCamelStream *)instance;
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	priv->stream = NULL;

	return;
}

static void
tny_camel_stream_finalize (GObject *object)
{
	TnyCamelStream *self = (TnyCamelStream *)object;
	TnyCamelStreamPriv *priv = TNY_CAMEL_STREAM_GET_PRIVATE (self);

	if (priv->stream)
		camel_object_unref (priv->stream);

	(*parent_class->finalize) (object);

	return;
}

static void
tny_stream_init (gpointer g, gpointer iface_data)
{
	TnyStreamIface *klass = (TnyStreamIface *)g;

	klass->read= tny_camel_stream_read;
	klass->write= tny_camel_stream_write;
	klass->flush= tny_camel_stream_flush;
	klass->close= tny_camel_stream_close;
	klass->is_eos= tny_camel_stream_is_eos;
	klass->reset= tny_camel_stream_reset;
	klass->write_to_stream= tny_camel_stream_write_to_stream;

	return;
}



static void
tny_seekable_init (gpointer g, gpointer iface_data)
{
	TnySeekableIface *klass = (TnySeekableIface *)g;

	klass->seek= tny_camel_stream_seek;
	klass->tell= tny_camel_stream_tell;
	klass->set_bounds= tny_camel_stream_set_bounds;

	return;
}

static void 
tny_camel_stream_class_init (TnyCamelStreamClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_camel_stream_finalize;

	g_type_class_add_private (object_class, sizeof (TnyCamelStreamPriv));

	return;
}

static gpointer
tny_camel_stream_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelStreamClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_stream_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelStream),
			0,      /* n_preallocs */
			tny_camel_stream_instance_init    /* instance_init */
		};
	
	static const GInterfaceInfo tny_stream_info = 
		{
			(GInterfaceInitFunc) tny_stream_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	static const GInterfaceInfo tny_seekable_info = 
		{
			(GInterfaceInitFunc) tny_seekable_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelStream",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_STREAM, 
				     &tny_stream_info);
	
	g_type_add_interface_static (type, TNY_TYPE_SEEKABLE, 
				     &tny_seekable_info);
	

	return GSIZE_TO_POINTER (type);
}

/**
 * tny_camel_stream_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_stream_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_stream_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
