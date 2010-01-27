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
 * TnyGtkImageMimePartView:
 *
 * A #TnyMimePartView that can render a #TnyMimePart that is an image. It's
 * recommended to use this type together with a #TnyGtkExpanderMimePartView.
 *
 * free-function: g_object_unref
 **/

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

#include <tny-gtk-image-mime-part-view.h>
#include <tny-gtk-pixbuf-stream.h>

#ifdef GNOME
#include <tny-vfs-stream.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#else
#include <tny-fs-stream.h>
#endif


static GObjectClass *parent_class = NULL;

typedef struct _TnyGtkImageMimePartViewPriv TnyGtkImageMimePartViewPriv;

struct _TnyGtkImageMimePartViewPriv
{
	TnyMimePart *part;
	TnyStatusCallback status_callback;
	gpointer status_user_data;
};

#define TNY_GTK_IMAGE_MIME_PART_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GTK_IMAGE_MIME_PART_VIEW, TnyGtkImageMimePartViewPriv))


static TnyMimePart*
tny_gtk_image_mime_part_view_get_part (TnyMimePartView *self)
{
	return TNY_GTK_IMAGE_MIME_PART_VIEW_GET_CLASS (self)->get_part(self);
}

static TnyMimePart*
tny_gtk_image_mime_part_view_get_part_default (TnyMimePartView *self)
{
	TnyGtkImageMimePartViewPriv *priv = TNY_GTK_IMAGE_MIME_PART_VIEW_GET_PRIVATE (self);
	return (priv->part)?TNY_MIME_PART (g_object_ref (priv->part)):NULL;
}

static void 
tny_gtk_image_mime_part_view_set_part (TnyMimePartView *self, TnyMimePart *part)
{
	TNY_GTK_IMAGE_MIME_PART_VIEW_GET_CLASS (self)->set_part(self, part);
	return;
}

static void 
on_mime_part_decoded (TnyMimePart *part, gboolean canceled, TnyStream *dest, GError *err, gpointer user_data)
{
	TnyMimePartView *self = (TnyMimePartView *) user_data;
	GdkPixbuf *pixbuf;
	tny_stream_reset (dest);
	pixbuf = tny_gtk_pixbuf_stream_get_pixbuf (TNY_GTK_PIXBUF_STREAM (dest));
	gtk_image_set_from_pixbuf (GTK_IMAGE (self), pixbuf);
	g_object_unref (self);
}

static void 
on_status (GObject *part, TnyStatus *status, gpointer user_data)
{
	TnyMimePartView *self = (TnyMimePartView *) user_data;
	TnyGtkImageMimePartViewPriv *priv = TNY_GTK_IMAGE_MIME_PART_VIEW_GET_PRIVATE (self);
	if (priv->status_callback)
		priv->status_callback ((GObject *) self, status, priv->status_user_data);
	return;
}

static void 
tny_gtk_image_mime_part_view_set_part_default (TnyMimePartView *self, TnyMimePart *part)
{
	TnyGtkImageMimePartViewPriv *priv = TNY_GTK_IMAGE_MIME_PART_VIEW_GET_PRIVATE (self);

	g_assert (TNY_IS_MIME_PART (part));

	if (priv->part)
		g_object_unref (priv->part);

	if (part) {
		TnyStream *dest = tny_gtk_pixbuf_stream_new (tny_mime_part_get_content_type (part));
		tny_stream_reset (dest);
		tny_mime_part_decode_to_stream_async (part, dest, on_mime_part_decoded, 
				on_status, g_object_ref (self));
		g_object_unref (dest);
		priv->part = g_object_ref (part);
	}

	return;
}

static void
tny_gtk_image_mime_part_view_clear (TnyMimePartView *self)
{
	TNY_GTK_IMAGE_MIME_PART_VIEW_GET_CLASS (self)->clear(self);
	return;
}

static void
tny_gtk_image_mime_part_view_clear_default (TnyMimePartView *self)
{
	return;
}

/**
 * tny_gtk_image_mime_part_view_new:
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @status_user_data: (null-ok): user data for @status_callback
 *
 * Create a new #TnyMimePartView for displaying image #TnyMimePartView instances.
 * The returned value inherits #GtkImage. It's recommended to use a
 * #TnyGtkExpanderMimePartView to wrap this type.
 *
 * Whenever data must be retrieved or takes long to load, @status_callback will
 * be called to let the outside world know about what this compenent is doing.
 *
 * returns: (caller-owns): a new #TnyMimePartView for images
 * since: 1.0
 * audience: application-developer
 **/
TnyMimePartView*
tny_gtk_image_mime_part_view_new (TnyStatusCallback status_callback, gpointer status_user_data)
{
	TnyGtkImageMimePartView *self = g_object_new (TNY_TYPE_GTK_IMAGE_MIME_PART_VIEW, NULL);
	TnyGtkImageMimePartViewPriv *priv = TNY_GTK_IMAGE_MIME_PART_VIEW_GET_PRIVATE (self);

	priv->status_callback = status_callback;
	priv->status_user_data = status_user_data;

	return TNY_MIME_PART_VIEW (self);
}

static void
tny_gtk_image_mime_part_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGtkImageMimePartView *self = (TnyGtkImageMimePartView *)instance;
	TnyGtkImageMimePartViewPriv *priv = TNY_GTK_IMAGE_MIME_PART_VIEW_GET_PRIVATE (self);

	priv->status_callback = NULL;
	priv->status_user_data = NULL;

	return;
}

static void
tny_gtk_image_mime_part_view_finalize (GObject *object)
{
	TnyGtkImageMimePartView *self = (TnyGtkImageMimePartView *)object;
	TnyGtkImageMimePartViewPriv *priv = TNY_GTK_IMAGE_MIME_PART_VIEW_GET_PRIVATE (self);

	if (priv->part)
		g_object_unref (priv->part);

	(*parent_class->finalize) (object);

	return;
}

static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
	TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

	klass->get_part= tny_gtk_image_mime_part_view_get_part;
	klass->set_part= tny_gtk_image_mime_part_view_set_part;
	klass->clear= tny_gtk_image_mime_part_view_clear;

	return;
}

static void 
tny_gtk_image_mime_part_view_class_init (TnyGtkImageMimePartViewClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->get_part= tny_gtk_image_mime_part_view_get_part_default;
	class->set_part= tny_gtk_image_mime_part_view_set_part_default;
	class->clear= tny_gtk_image_mime_part_view_clear_default;

	object_class->finalize = tny_gtk_image_mime_part_view_finalize;

	g_type_class_add_private (object_class, sizeof (TnyGtkImageMimePartViewPriv));

	return;
}

static gpointer 
tny_gtk_image_mime_part_view_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkImageMimePartViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_image_mime_part_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkImageMimePartView),
		  0,      /* n_preallocs */
		  tny_gtk_image_mime_part_view_instance_init,    /* instance_init */
		  NULL
		};

	static const GInterfaceInfo tny_mime_part_view_info = 
		{
		  (GInterfaceInitFunc) tny_mime_part_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (GTK_TYPE_IMAGE,
				       "TnyGtkImageMimePartView",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_MIME_PART_VIEW, 
				     &tny_mime_part_view_info);

	return GUINT_TO_POINTER (type);
}

GType 
tny_gtk_image_mime_part_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_image_mime_part_view_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
