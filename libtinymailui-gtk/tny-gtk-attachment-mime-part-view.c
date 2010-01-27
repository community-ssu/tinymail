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
 * TnyGtkAttachmentMimePartView:
 *
 * A #TnyMimePartView for showing a #TnyMimePart that is also an attachment
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
#include <tny-list.h>

#ifdef GNOME
#include <tny-vfs-stream.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#else
#include <tny-fs-stream.h>
#endif

#include <tny-gtk-attachment-mime-part-view.h>


static GObjectClass *parent_class = NULL;

typedef struct _TnyGtkAttachmentMimePartViewPriv TnyGtkAttachmentMimePartViewPriv;

struct _TnyGtkAttachmentMimePartViewPriv
{
	TnyMimePart *part;
	TnyGtkAttachListModel *imodel;
};

#define TNY_GTK_ATTACHMENT_MIME_PART_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GTK_ATTACHMENT_MIME_PART_VIEW, TnyGtkAttachmentMimePartViewPriv))


static TnyMimePart*
tny_gtk_attachment_mime_part_view_get_part (TnyMimePartView *self)
{
	return TNY_GTK_ATTACHMENT_MIME_PART_VIEW_GET_CLASS (self)->get_part(self);
}

static TnyMimePart*
tny_gtk_attachment_mime_part_view_get_part_default (TnyMimePartView *self)
{
	TnyGtkAttachmentMimePartViewPriv *priv = TNY_GTK_ATTACHMENT_MIME_PART_VIEW_GET_PRIVATE (self);
	return (priv->part)?TNY_MIME_PART (g_object_ref (priv->part)):NULL;
}

static void 
tny_gtk_attachment_mime_part_view_set_part (TnyMimePartView *self, TnyMimePart *part)
{
	TNY_GTK_ATTACHMENT_MIME_PART_VIEW_GET_CLASS (self)->set_part(self, part);
	return;
}

static void 
tny_gtk_attachment_mime_part_view_set_part_default (TnyMimePartView *self, TnyMimePart *part)
{
	TnyGtkAttachmentMimePartViewPriv *priv = TNY_GTK_ATTACHMENT_MIME_PART_VIEW_GET_PRIVATE (self);

	if (G_LIKELY (priv->part))
		g_object_unref (G_OBJECT (priv->part));

	if (part)
	{
		g_assert (TNY_IS_LIST (priv->imodel));
		tny_list_prepend (TNY_LIST (priv->imodel), G_OBJECT (part));
		priv->part = g_object_ref (G_OBJECT (part));
	}

	return;
}

static void
tny_gtk_attachment_mime_part_view_clear (TnyMimePartView *self)
{
	TNY_GTK_ATTACHMENT_MIME_PART_VIEW_GET_CLASS (self)->clear(self);
	return;
}

static void
tny_gtk_attachment_mime_part_view_clear_default (TnyMimePartView *self)
{
	TnyGtkAttachmentMimePartViewPriv *priv = TNY_GTK_ATTACHMENT_MIME_PART_VIEW_GET_PRIVATE (self);

	if (priv->part)
	{
		tny_list_remove (TNY_LIST (priv->imodel), G_OBJECT (priv->part));
		g_object_unref (G_OBJECT (priv->part));
		priv->part = NULL;
	}

	return;
}

/**
 * tny_gtk_attachment_mime_part_view_new:
 * @iview: A #TnyGtkAttachListModel
 * 
 * Create a new #TnyMimePartView for showing MIME parts that are attachments.
 * The @iview parameter that you must pass is the #GtkTreeModel containing the
 * #TnyMimePart instances that are attachments.
 * 
 * returns: (caller-owns): a new #TnyMimePartView for showing MIME parts that are attachments
 * since: 1.0
 * audience: application-developer
 **/
TnyMimePartView*
tny_gtk_attachment_mime_part_view_new (TnyGtkAttachListModel *iview)
{
	TnyGtkAttachmentMimePartView *self = g_object_new (TNY_TYPE_GTK_ATTACHMENT_MIME_PART_VIEW, NULL);

	g_object_ref (G_OBJECT (iview));
	TNY_GTK_ATTACHMENT_MIME_PART_VIEW_GET_PRIVATE (self)->imodel = iview;

	return TNY_MIME_PART_VIEW (self);
}

static void
tny_gtk_attachment_mime_part_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGtkAttachmentMimePartView *self = (TnyGtkAttachmentMimePartView *)instance;
	TnyGtkAttachmentMimePartViewPriv *priv = TNY_GTK_ATTACHMENT_MIME_PART_VIEW_GET_PRIVATE (self);

	priv->part = NULL;
	priv->imodel = NULL;

	return;
}

static void
tny_gtk_attachment_mime_part_view_finalize (GObject *object)
{
	TnyGtkAttachmentMimePartView *self = (TnyGtkAttachmentMimePartView *)object;	
	TnyGtkAttachmentMimePartViewPriv *priv = TNY_GTK_ATTACHMENT_MIME_PART_VIEW_GET_PRIVATE (self);

	if (priv->imodel)
		g_object_unref (G_OBJECT (priv->imodel));
	priv->imodel = NULL;

	if (G_LIKELY (priv->part))
		g_object_unref (G_OBJECT (priv->part));
	priv->part = NULL;

	(*parent_class->finalize) (object);

	return;
}

static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
	TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

	klass->get_part= tny_gtk_attachment_mime_part_view_get_part;
	klass->set_part= tny_gtk_attachment_mime_part_view_set_part;
	klass->clear= tny_gtk_attachment_mime_part_view_clear;

	return;
}

static void 
tny_gtk_attachment_mime_part_view_class_init (TnyGtkAttachmentMimePartViewClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->get_part= tny_gtk_attachment_mime_part_view_get_part_default;
	class->set_part= tny_gtk_attachment_mime_part_view_set_part_default;
	class->clear= tny_gtk_attachment_mime_part_view_clear_default;

	object_class->finalize = tny_gtk_attachment_mime_part_view_finalize;
	g_type_class_add_private (object_class, sizeof (TnyGtkAttachmentMimePartViewPriv));

	return;
}

static gpointer
tny_gtk_attachment_mime_part_view_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkAttachmentMimePartViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_attachment_mime_part_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkAttachmentMimePartView),
		  0,      /* n_preallocs */
		  tny_gtk_attachment_mime_part_view_instance_init,    /* instance_init */
		  NULL
		};

	static const GInterfaceInfo tny_mime_part_view_info = 
		{
		  (GInterfaceInitFunc) tny_mime_part_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyGtkAttachmentMimePartView",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_MIME_PART_VIEW, 
				     &tny_mime_part_view_info);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_gtk_attachment_mime_part_view_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType 
tny_gtk_attachment_mime_part_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_attachment_mime_part_view_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
