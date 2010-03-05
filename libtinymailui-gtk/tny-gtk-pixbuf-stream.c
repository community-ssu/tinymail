/* libtinymailui-gtk - The Tiny Mail UI library for Gtk+
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


/**
 * TnyGtkPixbufStream:
 *
 * A #TnyStream for building a #GdkPixbuf from a stream in Tinymail, like when
 * streaming a #TnyMimePart that is an image to a #GdkPixbuf that will be used
 * by a #GtkImage.
 *
 * free-function: g_object_unref
 **/

#include <config.h>


#include <string.h>
#include <glib.h>

#include <gtk/gtk.h>

#include <tny-stream.h>
#include <tny-gtk-pixbuf-stream.h>

static GObjectClass *parent_class = NULL;


typedef struct _TnyGtkPixbufStreamPriv TnyGtkPixbufStreamPriv;

struct _TnyGtkPixbufStreamPriv
{
	GdkPixbufLoader *loader;
};

#define TNY_GTK_PIXBUF_STREAM_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GTK_PIXBUF_STREAM, TnyGtkPixbufStreamPriv))


static gssize
tny_pixbuf_write_to_stream (TnyStream *self, TnyStream *output)
{
	return TNY_GTK_PIXBUF_STREAM_GET_CLASS (self)->write_to_stream(self, output);
}

static gssize
tny_pixbuf_write_to_stream_default (TnyStream *self, TnyStream *output)
{
	char tmp_buf[4096];
	gssize total = 0;
	gssize nb_read;
	gssize nb_written;

	g_assert (TNY_IS_STREAM (output));

	while (G_LIKELY (!tny_stream_is_eos (self))) 
	{
		nb_read = tny_stream_read (self, tmp_buf, sizeof (tmp_buf));
		if (G_UNLIKELY (nb_read < 0))
			return -1;
		else if (G_LIKELY (nb_read > 0)) {
			const gchar *end;
			if (!g_utf8_validate (tmp_buf, nb_read, &end)) 
				g_warning ("utf8 invalid: %d of %d", (gint)nb_read,
					   (gint)(end - tmp_buf));
				
			nb_written = 0;
	
			while (G_LIKELY (nb_written < nb_read))
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
tny_gtk_pixbuf_stream_read (TnyStream *self, char *buffer, gsize n)
{
	return TNY_GTK_PIXBUF_STREAM_GET_CLASS (self)->read(self, buffer, n);
}

static gssize
tny_gtk_pixbuf_stream_read_default (TnyStream *self, char *buffer, gsize n)
{
	return (gssize) n;
}

static gssize
tny_gtk_pixbuf_stream_write (TnyStream *self, const char *buffer, gsize n)
{
	return TNY_GTK_PIXBUF_STREAM_GET_CLASS (self)->write(self, buffer, n);
}

static gssize
tny_gtk_pixbuf_stream_write_default (TnyStream *self, const char *buffer, gsize n)
{
	TnyGtkPixbufStreamPriv *priv = TNY_GTK_PIXBUF_STREAM_GET_PRIVATE (self);
	gssize wr = (gssize) n;

	if (!gdk_pixbuf_loader_write (priv->loader, (const guchar *) buffer, n, NULL))
		wr = -1;

	return (gssize) wr;
}

static gint
tny_gtk_pixbuf_stream_flush (TnyStream *self)
{
	return TNY_GTK_PIXBUF_STREAM_GET_CLASS (self)->flush(self);
}

static gint
tny_gtk_pixbuf_stream_flush_default (TnyStream *self)
{
	return 0;
}

static gint
tny_gtk_pixbuf_stream_close (TnyStream *self)
{
	return TNY_GTK_PIXBUF_STREAM_GET_CLASS (self)->close(self);
}

static gint
tny_gtk_pixbuf_stream_close_default (TnyStream *self)
{
	return 0;
}

static gboolean
tny_gtk_pixbuf_stream_is_eos (TnyStream *self)
{
	return TNY_GTK_PIXBUF_STREAM_GET_CLASS (self)->is_eos(self);
}

static gboolean
tny_gtk_pixbuf_stream_is_eos_default (TnyStream *self)
{
	return TRUE;
}

static gint
tny_gtk_pixbuf_stream_reset (TnyStream *self)
{
	return TNY_GTK_PIXBUF_STREAM_GET_CLASS (self)->reset(self);
}

static gint
tny_gtk_pixbuf_stream_reset_default (TnyStream *self)
{
	return 0;
}


/**
 * tny_gtk_pixbuf_stream_new:
 * @mime_type: the MIME type, for example image/jpeg
 *
 * Create an adaptor instance between #TnyStream and #GdkPixbuf
 *
 * returns: (caller-owns): a new #TnyStream instance
 * since: 1.0
 * audience: application-developer
 **/
TnyStream*
tny_gtk_pixbuf_stream_new (const gchar *mime_type)
{
	TnyGtkPixbufStream *self = g_object_new (TNY_TYPE_GTK_PIXBUF_STREAM, NULL);
	TnyGtkPixbufStreamPriv *priv = TNY_GTK_PIXBUF_STREAM_GET_PRIVATE (self);

	/* TODO: Handle errors */
	priv->loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, NULL);

	return TNY_STREAM (self);
}

/**
 * tny_gtk_pixbuf_stream_get_pixbuf:
 * @self: a #TnyGtkPixbufStream
 *
 * Get the #GfkPixbuf that got created when we streamed data to @self.
 *
 * returns: (caller-owns): a #GdkPixbuf instance
 * since: 1.0
 * audience: application-developer
 **/
GdkPixbuf *
tny_gtk_pixbuf_stream_get_pixbuf (TnyGtkPixbufStream *self)
{
	TnyGtkPixbufStreamPriv *priv = TNY_GTK_PIXBUF_STREAM_GET_PRIVATE (self);
	return gdk_pixbuf_loader_get_pixbuf (priv->loader);
}

static void
tny_gtk_pixbuf_stream_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGtkPixbufStream *self = (TnyGtkPixbufStream *)instance;
	TnyGtkPixbufStreamPriv *priv = TNY_GTK_PIXBUF_STREAM_GET_PRIVATE (self);

	priv->loader = NULL;

	return;
}

static void
tny_gtk_pixbuf_stream_finalize (GObject *object)
{
	TnyGtkPixbufStream *self = (TnyGtkPixbufStream *)object;
	TnyGtkPixbufStreamPriv *priv = TNY_GTK_PIXBUF_STREAM_GET_PRIVATE (self);

	if (priv->loader) {
		gdk_pixbuf_loader_close (priv->loader, NULL);
		g_object_unref (priv->loader);
	}

	(*parent_class->finalize) (object);

	return;
}

static void
tny_stream_init (gpointer g, gpointer iface_data)
{
	TnyStreamIface *klass = (TnyStreamIface *)g;

	klass->read= tny_gtk_pixbuf_stream_read;
	klass->write= tny_gtk_pixbuf_stream_write;
	klass->flush= tny_gtk_pixbuf_stream_flush;
	klass->close= tny_gtk_pixbuf_stream_close;
	klass->is_eos= tny_gtk_pixbuf_stream_is_eos;
	klass->reset= tny_gtk_pixbuf_stream_reset;
	klass->write_to_stream= tny_pixbuf_write_to_stream;

	return;
}

static void 
tny_gtk_pixbuf_stream_class_init (TnyGtkPixbufStreamClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->read= tny_gtk_pixbuf_stream_read_default;
	class->write= tny_gtk_pixbuf_stream_write_default;
	class->flush= tny_gtk_pixbuf_stream_flush_default;
	class->close= tny_gtk_pixbuf_stream_close_default;
	class->is_eos= tny_gtk_pixbuf_stream_is_eos_default;
	class->reset= tny_gtk_pixbuf_stream_reset_default;
	class->write_to_stream= tny_pixbuf_write_to_stream_default;

	object_class->finalize = tny_gtk_pixbuf_stream_finalize;

	g_type_class_add_private (object_class, sizeof (TnyGtkPixbufStreamPriv));

	return;
}

static gpointer 
tny_gtk_pixbuf_stream_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkPixbufStreamClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_pixbuf_stream_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkPixbufStream),
		  0,      /* n_preallocs */
		  tny_gtk_pixbuf_stream_instance_init    /* instance_init */
		};

	static const GInterfaceInfo tny_stream_info = 
		{
		  (GInterfaceInitFunc) tny_stream_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyGtkPixbufStream",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_STREAM, 
				     &tny_stream_info);

	return GSIZE_TO_POINTER (type);
}

GType 
tny_gtk_pixbuf_stream_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_pixbuf_stream_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
