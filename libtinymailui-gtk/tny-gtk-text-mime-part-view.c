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
 * TnyGtkTextMimePartView:
 *
 * A #TnyMimePartView to show a plain text #TnyMimePart.
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

#include <tny-gtk-text-mime-part-view.h>
#include <tny-gtk-text-buffer-stream.h>

#ifdef GNOME
#include <tny-vfs-stream.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#else
#include <tny-fs-stream.h>
#endif


static GObjectClass *parent_class = NULL;

typedef struct _TnyGtkTextMimePartViewPriv TnyGtkTextMimePartViewPriv;

struct _TnyGtkTextMimePartViewPriv
{
	TnyMimePart *part;
	TnyStatusCallback status_callback;
	gpointer status_user_data;
};

#define TNY_GTK_TEXT_MIME_PART_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GTK_TEXT_MIME_PART_VIEW, TnyGtkTextMimePartViewPriv))


static TnyMimePart*
tny_gtk_text_mime_part_view_get_part (TnyMimePartView *self)
{
	return TNY_GTK_TEXT_MIME_PART_VIEW_GET_CLASS (self)->get_part(self);
}

static TnyMimePart*
tny_gtk_text_mime_part_view_get_part_default (TnyMimePartView *self)
{
	TnyGtkTextMimePartViewPriv *priv = TNY_GTK_TEXT_MIME_PART_VIEW_GET_PRIVATE (self);
	return (priv->part)?TNY_MIME_PART (g_object_ref (priv->part)):NULL;
}

static void 
tny_gtk_text_mime_part_view_set_part (TnyMimePartView *self, TnyMimePart *part)
{
	TNY_GTK_TEXT_MIME_PART_VIEW_GET_CLASS (self)->set_part(self, part);
	return;
}

static void 
tny_gtk_text_mime_part_view_set_part_default (TnyMimePartView *self, TnyMimePart *part)
{
	TnyGtkTextMimePartViewPriv *priv = TNY_GTK_TEXT_MIME_PART_VIEW_GET_PRIVATE (self);

	g_assert (TNY_IS_MIME_PART (part));

	if (priv->part)
		g_object_unref (priv->part);

	if (part) {
		GtkTextBuffer *buffer;
		TnyStream *dest;

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self));
		if (buffer && GTK_IS_TEXT_BUFFER (buffer))
			gtk_text_buffer_set_text (buffer, "", 0);

		dest = tny_gtk_text_buffer_stream_new (buffer);

		tny_stream_reset (dest);
		tny_mime_part_decode_to_stream_async (part, dest, NULL, 
			priv->status_callback, priv->status_user_data);
		g_object_unref (dest);

		priv->part = g_object_ref (part);
	}

	return;
}

static void
tny_gtk_text_mime_part_view_clear (TnyMimePartView *self)
{
	TNY_GTK_TEXT_MIME_PART_VIEW_GET_CLASS (self)->clear(self);
	return;
}

static void
tny_gtk_text_mime_part_view_clear_default (TnyMimePartView *self)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self));

	if (buffer && GTK_IS_TEXT_BUFFER (buffer))
		gtk_text_buffer_set_text (buffer, "", 0);

	return;
}

/**
 * tny_gtk_text_mime_part_view_new:
 * @status_callback: (null-ok): a #TnyStatusCallback for when status information happens or NULL
 * @status_user_data: (null-ok): user data for @status_callback
 *
 * Create a new #TnyMimePartView for showing plain text MIME parts.
 *
 * Whenever data must be retrieved or takes long to load, @status_callback will
 * be called to let the outside world know about what this compenent is doing.
 *
 * returns: (caller-owns): a new #TnyMimePartView 
 **/
TnyMimePartView*
tny_gtk_text_mime_part_view_new (TnyStatusCallback status_callback, gpointer status_user_data)
{
	TnyGtkTextMimePartView *self = g_object_new (TNY_TYPE_GTK_TEXT_MIME_PART_VIEW, NULL);
	TnyGtkTextMimePartViewPriv *priv = TNY_GTK_TEXT_MIME_PART_VIEW_GET_PRIVATE (self);

	priv->status_callback = status_callback;
	priv->status_user_data = status_user_data;

	return TNY_MIME_PART_VIEW (self);
}

static void
tny_gtk_text_mime_part_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGtkTextMimePartView *self = (TnyGtkTextMimePartView *)instance;
	TnyGtkTextMimePartViewPriv *priv = TNY_GTK_TEXT_MIME_PART_VIEW_GET_PRIVATE (self);

	priv->part = NULL;
	priv->status_callback = NULL;
	priv->status_user_data = NULL;

	gtk_text_view_set_editable (GTK_TEXT_VIEW (self), FALSE);

	return;
}

static void
tny_gtk_text_mime_part_view_finalize (GObject *object)
{
	TnyGtkTextMimePartView *self = (TnyGtkTextMimePartView *)object;	
	TnyGtkTextMimePartViewPriv *priv = TNY_GTK_TEXT_MIME_PART_VIEW_GET_PRIVATE (self);

	if (G_LIKELY (priv->part))
		g_object_unref (G_OBJECT (priv->part));

	(*parent_class->finalize) (object);

	return;
}

static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
	TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

	klass->get_part= tny_gtk_text_mime_part_view_get_part;
	klass->set_part= tny_gtk_text_mime_part_view_set_part;
	klass->clear= tny_gtk_text_mime_part_view_clear;

	return;
}

static void 
tny_gtk_text_mime_part_view_class_init (TnyGtkTextMimePartViewClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->get_part= tny_gtk_text_mime_part_view_get_part_default;
	class->set_part= tny_gtk_text_mime_part_view_set_part_default;
	class->clear= tny_gtk_text_mime_part_view_clear_default;

	object_class->finalize = tny_gtk_text_mime_part_view_finalize;

	g_type_class_add_private (object_class, sizeof (TnyGtkTextMimePartViewPriv));

	return;
}

static gpointer
tny_gtk_text_mime_part_view_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkTextMimePartViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_text_mime_part_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkTextMimePartView),
		  0,      /* n_preallocs */
		  tny_gtk_text_mime_part_view_instance_init,    /* instance_init */
		  NULL
		};

	static const GInterfaceInfo tny_mime_part_view_info = 
		{
		  (GInterfaceInitFunc) tny_mime_part_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (GTK_TYPE_TEXT_VIEW,
				       "TnyGtkTextMimePartView",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_MIME_PART_VIEW, 
				     &tny_mime_part_view_info);

	return GUINT_TO_POINTER (type);
}

GType 
tny_gtk_text_mime_part_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_text_mime_part_view_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
