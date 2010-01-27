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

#include <config.h>
#include <glib/gi18n-lib.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include <tny-stream.h>
#include <tny-webkit-stream.h>

static GObjectClass *parent_class = NULL;


struct _TnyWebkitStreamPriv {
	TnyMimePartView *part_view;
	GString *buffer;
};

#define TNY_WEBKIT_STREAM_GET_PRIVATE(o) ((TnyWebkitStream*)(o))->priv

static ssize_t
tny_webkit_write_to_stream (TnyStream *self, TnyStream *output)
{
	char tmp_buf[4096];
	ssize_t total = 0;
	ssize_t nb_read;
	ssize_t nb_written;

	g_assert (TNY_IS_STREAM (output));

	while (G_LIKELY (!tny_stream_is_eos (self))) 
	{
		nb_read = tny_stream_read (self, tmp_buf, sizeof (tmp_buf));
		if (G_UNLIKELY (nb_read < 0))
			return -1;
		else if (G_LIKELY (nb_read > 0)) {
			nb_written = 0;
	
			while (G_LIKELY (nb_written < nb_read))
			{
				ssize_t len = tny_stream_write (output, tmp_buf + nb_written,
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

static ssize_t
tny_webkit_stream_read  (TnyStream *self, char *data, size_t n)
{
	ssize_t retval = -1;
	return retval;
}

static gint
tny_webkit_stream_reset (TnyStream *self)
{
	g_string_free (priv->buffer, TRUE);
	priv->buffer = g_string_new ();
	return 0;
}

static ssize_t
tny_webkit_stream_write (TnyStream *self, const char *data, size_t n)
{
	TnyWebkitStreamPriv *priv = TNY_WEBKIT_STREAM_GET_PRIVATE (self);
	g_string_append (priv->buffer, data);
	webkit_web_view_load_html_string (WEBKIT_WEB_VIEW (priv->part_view), 
		(const gchar *) priv->buffer->str, "");
	return (ssize_t) n;
}

static gint
tny_webkit_stream_flush (TnyStream *self)
{
	return 0;
}

static gint
tny_webkit_stream_close (TnyStream *self)
{
	TnyWebkitStreamPriv *priv = TNY_WEBKIT_STREAM_GET_PRIVATE (self);

	return 0;
}

static gboolean
tny_webkit_stream_is_eos (TnyStream *self)
{
	return TRUE;
}



/**
 * tny_webkit_stream_new:
 *
 * Create an adaptor instance between #TnyStream and #GtkWebkit
 *
 * Return value: a new #TnyStream instance
 **/
TnyStream*
tny_webkit_stream_new (TnyMimePartView *part_view)
{
	TnyWebkitStream *self = g_object_new (TNY_TYPE_WEBKIT_STREAM, NULL);
	TnyWebkitStreamPriv *priv = TNY_WEBKIT_STREAM_GET_PRIVATE (self);
	priv->part_view = (TnyMimePartView *) g_object_ref (part_view);
	return TNY_STREAM (self);
}

static void
tny_webkit_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyWebkitStream *self = (TnyWebkitStream *)instance;
	TnyWebkitStreamPriv *priv = g_slice_new (TnyWebkitStreamPriv);
	self->priv = priv;
	priv->buffer = g_string_new ();
	return;
}

static void
tny_webkit_stream_finalize (GObject *object)
{
	TnyWebkitStream *self = (TnyWebkitStream *)object;
	TnyWebkitStreamPriv *priv = TNY_WEBKIT_STREAM_GET_PRIVATE (self);
	if (priv->part_view)
		g_object_unref (priv->part_view);
	g_string_free (priv->buffer, TRUE);
	g_slice_free (TnyWebkitStreamPriv, priv);
	(*parent_class->finalize) (object);
	return;
}

static void
tny_stream_init (gpointer g, gpointer iface_data)
{
	TnyStreamIface *klass = (TnyStreamIface *)g;

	klass->read= tny_webkit_stream_read;
	klass->write= tny_webkit_stream_write;
	klass->flush= tny_webkit_stream_flush;
	klass->close= tny_webkit_stream_close;
	klass->is_eos= tny_webkit_stream_is_eos;
	klass->reset= tny_webkit_stream_reset;
	klass->write_to_stream= tny_webkit_write_to_stream;

	return;
}

static void 
tny_webkit_stream_class_init (TnyWebkitStreamClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_webkit_stream_finalize;

	return;
}

static gpointer 
tny_webkit_stream_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyWebkitStreamClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_webkit_stream_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyWebkitStream),
		  0,      /* n_preallocs */
		  tny_webkit_stream_instance_init    /* instance_init */
		};

	static const GInterfaceInfo tny_stream_info = 
		{
		  (GInterfaceInitFunc) tny_stream_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyWebkitStream",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_STREAM, 
				     &tny_stream_info);

	return GUINT_TO_POINTER (type);
}

GType 
tny_webkit_stream_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_webkit_stream_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
