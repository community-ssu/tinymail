/* libtinymailui-webkit - The Tiny Mail UI library for Webkit
 * Copyright (C) 2006-2007 Philip Van Hoof <pvanhoof@gnome.org>
 *
 * This component was developed based on Modest's ModestGtkHtmlMimePartView
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string.h>
#include <gtk/gtk.h>

#include <tny-gtk-html-mime-part-view.h>
#include <tny-gtk-html-stream.h>

static GObjectClass *parent_class = NULL;

typedef struct _TnyGtkHtmlMimePartViewPriv TnyGtkHtmlMimePartViewPriv;

struct _TnyGtkHtmlMimePartViewPriv {
	TnyMimePart *part;
	TnyStatusCallback status_callback;
	gpointer status_user_data;
};

#define TNY_GTK_HTML_MIME_PART_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GTK_HTML_MIME_PART_VIEW, TnyGtkHtmlMimePartViewPriv))


static TnyMimePart*
tny_gtk_html_mime_part_view_get_part (TnyMimePartView *self)
{
	TnyGtkHtmlMimePartViewPriv *priv = TNY_GTK_HTML_MIME_PART_VIEW_GET_PRIVATE (self);
	return (priv->part)?TNY_MIME_PART (g_object_ref (priv->part)):NULL;
}


typedef struct {
	TnyGtkHtmlMimePartView *self;
	GtkHTMLStream *gtkhtml_stream;
	TnyStream *tny_stream;
} SomeData;


static void 
on_status (GObject *self, TnyStatus *status, gpointer user_data)
{
	SomeData *info = (SomeData *) user_data;
	TnyGtkHtmlMimePartViewPriv *priv = TNY_GTK_HTML_MIME_PART_VIEW_GET_PRIVATE (info->self);
	if (priv->status_callback)
		priv->status_callback (self, status, priv->status_user_data);
}

static void
on_set_stream_finished (TnyMimePart *self, gboolean cancelled, TnyStream *stream, GError *err, gpointer user_data)
{
	SomeData *info = (SomeData *) user_data;

	g_object_unref (info->tny_stream);
	gtk_html_stream_destroy (info->gtkhtml_stream);
	g_object_unref (info->self);

	g_slice_free (SomeData, user_data);
}

static void
set_html_part (TnyGtkHtmlMimePartView *self, TnyMimePart *part)
{
	SomeData *info = g_slice_new (SomeData);

	info->self = (TnyGtkHtmlMimePartView *) g_object_ref (self);
	info->gtkhtml_stream = gtk_html_begin(GTK_HTML(self));
	info->tny_stream = TNY_STREAM (tny_gtk_html_stream_new (info->gtkhtml_stream));
	tny_stream_reset (info->tny_stream);

	tny_mime_part_decode_to_stream_async ((TnyMimePart*)part, info->tny_stream, 
			on_set_stream_finished, on_status, info);

	return;
}


/**
 * tny_gtk_html_mime_part_view_set_part:
 * @self: a #TnyWebkitMimePartView instance
 * @part: a #TnyMimePart instance
 *
 * This is non-public API documentation
 *
 * The implementation simply decodes the part to a stream that is implemented in
 * such a way that it will forward the stream to the GtkWebkit stream API.
 **/
static void 
tny_gtk_html_mime_part_view_set_part (TnyMimePartView *self, TnyMimePart *part)
{
	TnyGtkHtmlMimePartViewPriv *priv = TNY_GTK_HTML_MIME_PART_VIEW_GET_PRIVATE (self);

	if (priv->part)
		g_object_unref (priv->part);

	if (part) {
		set_html_part (TNY_GTK_HTML_MIME_PART_VIEW (self), part);
		priv->part = (TnyMimePart *) g_object_ref (part);
	}

	return;
}

static void
tny_gtk_html_mime_part_view_clear (TnyMimePartView *self)
{
	return;
}

static gboolean
on_url_requested (GtkWidget *widget, const gchar *uri, GtkHTMLStream *stream, TnyMimePartView *self)
{
	gboolean result;
	TnyStream *tny_stream;

	tny_stream = TNY_STREAM (tny_gtk_html_stream_new (stream));
	g_signal_emit_by_name (self, "fetch-url", uri, tny_stream, &result);
	gtk_html_stream_close (stream, result?GTK_HTML_STREAM_OK:GTK_HTML_STREAM_ERROR);
	g_object_unref (tny_stream);
	return result;
}



/**
 * tny_gtk_html_mime_part_view_new:
 *
 * Create a #TnyMimePartView that can display HTML mime parts
 *
 * Return value: a new #TnyMimePartView instance implemented for Gtk+
 **/
TnyMimePartView*
tny_gtk_html_mime_part_view_new (TnyStatusCallback status_callback, gpointer status_user_data)
{
	TnyGtkHtmlMimePartView *self = g_object_new (TNY_TYPE_GTK_HTML_MIME_PART_VIEW, NULL);
	TnyGtkHtmlMimePartViewPriv *priv = TNY_GTK_HTML_MIME_PART_VIEW_GET_PRIVATE (self);

	priv->status_callback = status_callback;
	priv->status_user_data = status_user_data;

	return TNY_MIME_PART_VIEW (self);
}



static void
tny_gtk_html_mime_part_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGtkHtmlMimePartView *self  = (TnyGtkHtmlMimePartView*) instance;
	TnyGtkHtmlMimePartViewPriv *priv = TNY_GTK_HTML_MIME_PART_VIEW_GET_PRIVATE (self);

	gtk_html_set_editable (GTK_HTML(self), FALSE);
	gtk_html_allow_selection (GTK_HTML(self), TRUE);
	gtk_html_set_caret_mode (GTK_HTML(self), FALSE);
	gtk_html_set_blocking (GTK_HTML(self), FALSE);
	gtk_html_set_images_blocking (GTK_HTML(self), FALSE);

	g_signal_connect (G_OBJECT(self), "url_requested",
			G_CALLBACK(on_url_requested), self);

	priv->part = NULL;

	return;
}

static void
tny_gtk_html_mime_part_view_finalize (GObject *object)
{
	TnyGtkHtmlMimePartView *self = (TnyGtkHtmlMimePartView *)object;	
	TnyGtkHtmlMimePartViewPriv *priv = TNY_GTK_HTML_MIME_PART_VIEW_GET_PRIVATE (self);
	if (priv->part)
		g_object_unref (priv->part);
	(*parent_class->finalize) (object);
	return;
}

static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
	TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

	klass->get_part= tny_gtk_html_mime_part_view_get_part;
	klass->set_part= tny_gtk_html_mime_part_view_set_part;
	klass->clear= tny_gtk_html_mime_part_view_clear;

	return;
}

static void 
tny_gtk_html_mime_part_view_class_init (TnyGtkHtmlMimePartViewClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_gtk_html_mime_part_view_finalize;

	g_type_class_add_private (object_class, sizeof (TnyGtkHtmlMimePartViewPriv));

	return;
}

static gpointer 
tny_gtk_html_mime_part_view_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkHtmlMimePartViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_html_mime_part_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkHtmlMimePartView),
		  0,      /* n_preallocs */
		  tny_gtk_html_mime_part_view_instance_init,    /* instance_init */
		  NULL
		};

	static const GInterfaceInfo tny_mime_part_view_info = 
		{
		  (GInterfaceInitFunc) tny_mime_part_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (GTK_TYPE_HTML,
				       "TnyGtkHtmlMimePartView",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_MIME_PART_VIEW, 
				     &tny_mime_part_view_info);

	return GSIZE_TO_POINTER (type);
}

GType 
tny_gtk_html_mime_part_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_html_mime_part_view_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
