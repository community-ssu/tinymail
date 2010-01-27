/* libtinymailui-webkit - The Tiny Mail UI library for Webkit
 * Copyright (C) 2006-2007 Philip Van Hoof <pvanhoof@gnome.org>
 *
 * This component was developed based on Modest's ModestTnyStreamGtkHtml
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
#include <tny-gtk-html-stream.h>

static GObjectClass *parent_class = NULL;


struct _TnyGtkHtmlStreamPriv {
	GtkHTMLStream *stream;
};

#define TNY_GTK_HTML_STREAM_GET_PRIVATE(o) ((TnyGtkHtmlStream*)(o))->priv

static ssize_t
tny_gtk_html_stream_write_to_stream (TnyStream *self, TnyStream *output)
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
tny_gtk_html_stream_read  (TnyStream *self, char *data, size_t n)
{
	return -1;
}

static gint
tny_gtk_html_stream_reset (TnyStream *self)
{
	return 0;
}

static ssize_t
tny_gtk_html_stream_write (TnyStream *self, const char *data, size_t n)
{
	TnyGtkHtmlStreamPriv *priv = TNY_GTK_HTML_STREAM_GET_PRIVATE (self);

	if (!priv->stream) {
		g_print ("modest: cannot write to closed stream\n");
		return 0;
	}

	if (n == 0 || !data)
		return 0;
		
	gtk_html_stream_write (priv->stream, data, n);

	return (ssize_t) n;
}

static gint
tny_gtk_html_stream_flush (TnyStream *self)
{
	return 0;
}

static gint
tny_gtk_html_stream_close (TnyStream *self)
{
	TnyGtkHtmlStreamPriv *priv = TNY_GTK_HTML_STREAM_GET_PRIVATE (self);

	gtk_html_stream_close (priv->stream, GTK_HTML_STREAM_OK);
	priv->stream = NULL;

	return 0;
}

static gboolean
tny_gtk_html_stream_is_eos (TnyStream *self)
{
	return TRUE;
}



/**
 * tny_gtk_html_stream_new:
 * @stream: a #GtkHTMLStream
 *
 * Create an adaptor instance between #TnyStream and #GtkHtmlStream
 *
 * Return value: a new #TnyStream instance
 **/
TnyStream*
tny_gtk_html_stream_new (GtkHTMLStream *stream)
{
	TnyGtkHtmlStream *self = g_object_new (TNY_TYPE_GTK_HTML_STREAM, NULL);
	TnyGtkHtmlStreamPriv *priv = TNY_GTK_HTML_STREAM_GET_PRIVATE (self);
	priv->stream = stream;
	return TNY_STREAM (self);
}

static void
tny_gtk_html_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGtkHtmlStream *self = (TnyGtkHtmlStream *)instance;
	TnyGtkHtmlStreamPriv *priv = g_slice_new (TnyGtkHtmlStreamPriv);
	priv->stream = NULL;
	self->priv = priv;
	return;
}

static void
tny_gtk_html_stream_finalize (GObject *object)
{
	TnyGtkHtmlStream *self = (TnyGtkHtmlStream *)object;
	TnyGtkHtmlStreamPriv *priv = TNY_GTK_HTML_STREAM_GET_PRIVATE (self);
	priv->stream = NULL;
	g_slice_free (TnyGtkHtmlStreamPriv, priv);
	(*parent_class->finalize) (object);
	return;
}

static void
tny_stream_init (gpointer g, gpointer iface_data)
{
	TnyStreamIface *klass = (TnyStreamIface *)g;

	klass->read= tny_gtk_html_stream_read;
	klass->write= tny_gtk_html_stream_write;
	klass->flush= tny_gtk_html_stream_flush;
	klass->close= tny_gtk_html_stream_close;
	klass->is_eos= tny_gtk_html_stream_is_eos;
	klass->reset= tny_gtk_html_stream_reset;
	klass->write_to_stream= tny_gtk_html_stream_write_to_stream;

	return;
}

static void 
tny_gtk_html_stream_class_init (TnyGtkHtmlStreamClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_gtk_html_stream_finalize;

	return;
}

static gpointer 
tny_gtk_html_stream_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkHtmlStreamClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_html_stream_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkHtmlStream),
		  0,      /* n_preallocs */
		  tny_gtk_html_stream_instance_init    /* instance_init */
		};

	static const GInterfaceInfo tny_stream_info = 
		{
		  (GInterfaceInitFunc) tny_stream_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyGtkHtmlStream",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_STREAM, 
				     &tny_stream_info);

	return GUINT_TO_POINTER (type);
}

GType 
tny_gtk_html_stream_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_html_stream_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
