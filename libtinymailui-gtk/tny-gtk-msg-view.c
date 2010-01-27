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
 * TnyGtkMsgView:
 *
 * a #TnyMsgView for showing a message in Gtk+. It's recommended to wrap instances
 * of this type into a #GtkScrolledWindow.
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
#include <tny-simple-list.h>
#include <tny-iterator.h>

#include <tny-gtk-msg-view.h>
#include <tny-gtk-text-buffer-stream.h>
#include <tny-gtk-attach-list-model.h>
#include <tny-header-view.h>
#include <tny-gtk-header-view.h>
#include <tny-gtk-image-mime-part-view.h>
#include <tny-gtk-text-mime-part-view.h>
#include <tny-gtk-attachment-mime-part-view.h>
#include <tny-gtk-expander-mime-part-view.h>
#include <tny-mime-part-saver.h>

#ifdef GNOME
#include <tny-vfs-stream.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#else
#include <tny-fs-stream.h>
#endif

#include "tny-gtk-attach-list-model-priv.h"

static GObjectClass *parent_class = NULL;

typedef struct _TnyGtkMsgViewPriv TnyGtkMsgViewPriv;

struct _TnyGtkMsgViewPriv
{
	TnyMimePart *part;
	TnyHeaderView *headerview;
	GtkIconView *attachview;
	GtkWidget *attachview_sw;
	GList *unattached_views;
	gboolean display_html;
	gboolean display_plain;
	gboolean display_attachments;
	gboolean display_rfc822;
	gboolean first_attachment;
	GtkBox *kid; gboolean in_expander, parented;
	TnyStatusCallback status_callback;
	gpointer status_user_data;
};

typedef struct
{
	gulong signal;
	TnyMimePart *part;
} RealizePriv;

#define TNY_GTK_MSG_VIEW_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GTK_MSG_VIEW, TnyGtkMsgViewPriv))


static void tny_gtk_msg_view_display_parts (TnyMsgView *self, TnyList *parts, gboolean alternatives, const gchar *desc);
static void remove_mime_part_viewer (TnyMimePartView *mpview, GtkContainer *mpviewers);
static gboolean tny_gtk_msg_view_display_part (TnyMsgView *self, TnyMimePart *part, const gchar *desc);


/**
 * tny_gtk_msg_view_get_display_html:
 * @self: a #TnyGtkMsgView
 *
 * Get whether or not to display text/html mime parts
 * 
 * returns: whether or not to display text/html mime parts
 * since: 1.0
 * audience: application-developer
 **/
gboolean 
tny_gtk_msg_view_get_display_html (TnyGtkMsgView *self)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	return priv->display_html;
}

/**
 * tny_gtk_msg_view_get_display_rfc822:
 * @self: a #TnyGtkMsgView
 *
 * Get whether or not to display message/rfc822 mime parts
 *
 * returns: whether or not to display message/rfc822 mime parts
 * since: 1.0
 * audience: application-developer
 **/
gboolean 
tny_gtk_msg_view_get_display_rfc822 (TnyGtkMsgView *self)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	return priv->display_rfc822;
}

/**
 * tny_gtk_msg_view_get_display_attachments:
 * @self: a #TnyGtkMsgView
 *
 * Get whether or not to display attachments
 *
 * returns: whether or not to display attachments
 * since: 1.0
 * audience: application-developer
 **/
gboolean 
tny_gtk_msg_view_get_display_attachments (TnyGtkMsgView *self)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	return priv->display_attachments;
}

/**
 * tny_gtk_msg_view_get_display_plain:
 * @self: a #TnyGtkMsgView
 *
 * Get whether or not to display text/plain mime parts
 * 
 * returns: whether or not to display text/plain mime parts
 * since: 1.0
 * audience: application-developer
 **/
gboolean 
tny_gtk_msg_view_get_display_plain (TnyGtkMsgView *self)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	return priv->display_plain;
}


/**
 * tny_gtk_msg_view_set_display_html:
 * @self: a #TnyGtkMsgView
 * @setting: whether or not to display text/html mime parts
 *
 * With this setting will the default implementation of #TnyGtkMsgView display
 * the HTML source code of text/html mime parts. Default is FALSE.
 *
 * Note that these settings only affect the instance in case an overridden
 * implementation of tny_msg_view_create_mime_part_view_for() doesn't handle
 * creating a viewer for a mime part.
 * 
 * So for example in case a more advanced implementation that inherits this
 * type implements viewing a text/html mime part, and will therefore not call
 * this types original tny_msg_view_create_mime_part_view_for() method for
 * the mime part anymore, the setting isn't used.
 *
 * The effect, by default, of this setting is showing the HTML source code.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_gtk_msg_view_set_display_html (TnyGtkMsgView *self, gboolean setting)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	priv->display_html = setting;
	return;
}

/**
 * tny_gtk_msg_view_set_display_rfc822:
 * @self: a #TnyGtkMsgView
 * @setting: whether or not to display message/rfc822 mime parts
 *
 * With this setting will the default implementation of #TnyGtkMsgView display
 * RFC822 inline message mime parts (forwards). Default is FALSE.
 *
 * Note that these settings only affect the instance in case an overridden
 * implementation of tny_msg_view_create_mime_part_view_for() doesn't handle
 * creating a viewer for a mime part.
 * 
 * So for example in case a more advanced implementation that inherits this
 * type implements viewing a text/html mime part, and will therefore not call
 * this types original tny_msg_view_create_mime_part_view_for() method for
 * the mime part anymore, the setting isn't used.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_gtk_msg_view_set_display_rfc822 (TnyGtkMsgView *self, gboolean setting)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	priv->display_rfc822 = setting;
	return;
}


/**
 * tny_gtk_msg_view_set_display_attachments:
 * @self: a #TnyGtkMsgView
 * @setting: whether or not to display attachment mime parts
 *
 * With this setting will the default implementation of #TnyGtkMsgView display
 * attachments using a #GtkIconList and the #TnyGtkAttachListModel at the bottom
 * of the #TnyGtkMsgView's scrollwindow. Default is TRUE.
 *
 * Note that these settings only affect the instance in case an overridden
 * implementation of tny_msg_view_create_mime_part_view_for() doesn't handle
 * creating a viewer for a mime part.
 * 
 * So for example in case a more advanced implementation that inherits this
 * type implements viewing a text/html mime part, and will therefore not call
 * this types original tny_msg_view_create_mime_part_view_for() method for
 * the mime part anymore, the setting isn't used.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_gtk_msg_view_set_display_attachments (TnyGtkMsgView *self, gboolean setting)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	priv->display_attachments = setting;
	return;
}

/**
 * tny_gtk_msg_view_set_display_plain:
 * @self: A #TnyGtkMsgView instance
 * @setting: whether or not to display text/plain mime parts
 *
 * With this setting will the default implementation of #TnyGtkMsgView display
 * text/plain mime parts. Default is TRUE.
 *
 * Note that these settings only affect the instance in case an overridden
 * implementation of tny_msg_view_create_mime_part_view_for() doesn't handle
 * creating a viewer for a mime part.
 * 
 * So for example in case a more advanced implementation that inherits this
 * type implements viewing a text/html mime part, and will therefore not call
 * this types original tny_msg_view_create_mime_part_view_for() method for
 * the mime part anymore, the setting isn't used.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_gtk_msg_view_set_display_plain (TnyGtkMsgView *self, gboolean setting)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	priv->display_plain = setting;
	return;
}



static void
tny_gtk_msg_view_set_unavailable (TnyMsgView *self)
{
	TNY_GTK_MSG_VIEW_GET_CLASS (self)->set_unavailable(self);
}

static void
tny_gtk_msg_view_set_unavailable_default (TnyMsgView *self)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);

	tny_msg_view_clear (self);

	tny_header_view_clear (priv->headerview);
	gtk_widget_hide (GTK_WIDGET (priv->headerview));

	return;
}


static TnyMsg*
tny_gtk_msg_view_get_msg (TnyMsgView *self)
{
	return TNY_GTK_MSG_VIEW_GET_CLASS (self)->get_msg(self);
}

static TnyMsg* 
tny_gtk_msg_view_get_msg_default (TnyMsgView *self)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);

	return (priv->part && TNY_IS_MSG (priv->part))?TNY_MSG (g_object_ref (priv->part)):NULL;
}


static TnyMsgView*
tny_gtk_msg_view_create_new_inline_viewer (TnyMsgView *self)
{
	return TNY_GTK_MSG_VIEW_GET_CLASS (self)->create_new_inline_viewer(self);
}

/**
 * tny_gtk_msg_view_set_parented:
 * @self: a #TnyGtkMsgView
 * @parented: parented or not
 *
 * Set @self as parented. Usually internally used.
 *
 * since: 1.0
 * audience: type-implementer, tinymail-developer
 **/
void 
tny_gtk_msg_view_set_parented (TnyGtkMsgView *self, gboolean parented)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	priv->parented = TRUE;
	return;
}

/**
 * tny_gtk_msg_view_set_status_callback:
 * @self: a #TnyGtkMsgView
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @status_user_data: (null-ok): user data for @status_callback
 *
 * Set the status callback info. This callback can be NULL and will be called
 * when status information happens. You can for example set a progress bar's
 * position here (for for example when downloading of a message takes place).
 *
 * since: 1.0
 * audience: application-developer, type-implementer, tinymail-developer
 **/
void 
tny_gtk_msg_view_set_status_callback (TnyGtkMsgView *self, TnyStatusCallback status_callback, gpointer status_user_data)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	priv->status_callback = status_callback;
	priv->status_user_data = status_user_data;
	return;
}

/**
 * tny_gtk_msg_view_get_status_callback:
 * @self: a #TnyGtkMsgView
 * @status_callback: (out): byref a #TnyStatusCallback 
 * @status_user_data: (out): byref user data for @status_callback
 *
 * Get the status callback info. Usually internally used.
 *
 * since: 1.0
 * audience: type-implementer, tinymail-developer
 **/
void 
tny_gtk_msg_view_get_status_callback (TnyGtkMsgView *self, TnyStatusCallback *status_callback, gpointer *status_user_data)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	*status_callback = priv->status_callback;
	*status_user_data = priv->status_user_data;
}

static TnyMsgView*
tny_gtk_msg_view_create_new_inline_viewer_default (TnyMsgView *self)
{
	TnyMsgView *retval = tny_gtk_msg_view_new ();
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);

	tny_gtk_msg_view_set_parented (TNY_GTK_MSG_VIEW (retval), TRUE);
	tny_gtk_msg_view_set_status_callback (TNY_GTK_MSG_VIEW (retval), 
		priv->status_callback, priv->status_user_data);

	return retval;
}

static TnyMimePartView*
tny_gtk_msg_view_create_mime_part_view_for (TnyMsgView *self, TnyMimePart *part)
{
	return TNY_GTK_MSG_VIEW_GET_CLASS (self)->create_mime_part_view_for(self, part);
}


/**
 * tny_gtk_msg_view_create_mime_part_view_for_default:
 * @self: a #TnyGtkMsgView
 * @part: a #TnyMimePart
 *
 * This is non-public API documentation
 *
 * Default implementation for tny_msg_view_create_mime_part_view_for. You will
 * usually call this method from overridden implementations that inherit this
 * type. A.k.a. calling the super method.
 * 
 * The text/plain and message/rfc822 content types and attachment mime parts
 * are typically handled by this implementation. The text/html one is typically
 * handled by an implementation that inherits this implementation. However, if
 * this implementation handles the text/html mime parts, then the HTML source
 * code will be displayed to the user. 
 *
 * ps. A #TnyMimePartView implementation that displays the HTML source code as
 * plain text would be welcomed for this situation. For example links or 
 * lynx - like.
 *
 * This is non-public API documentation
 **/
static TnyMimePartView*
tny_gtk_msg_view_create_mime_part_view_for_default (TnyMsgView *self, TnyMimePart *part)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	TnyMimePartView *retval = NULL;

	g_assert (TNY_IS_MIME_PART (part));

	/* PLAIN mime part */
	if (priv->display_plain && tny_mime_part_content_type_is (part, "text/plain"))
	{
		retval = tny_gtk_text_mime_part_view_new (priv->status_callback, priv->status_user_data);

	/* HTML mime part (shows HTML source code) (should be overridden in case there's
	   support for a HTML TnyMsgView (like the TnyMozEmbedMsgView) */
	} else if (priv->display_html && tny_mime_part_content_type_is (part, "text/html"))
	{
		retval = tny_gtk_text_mime_part_view_new (priv->status_callback, priv->status_user_data);

	/* Inline message RFC822 */
	} else if (tny_mime_part_content_type_is (part, "image/*"))
	{
		gboolean nf = FALSE;
		TnyMimePartView *image_view = tny_gtk_image_mime_part_view_new (priv->status_callback, priv->status_user_data);
		gchar *desc = (gchar *) tny_mime_part_get_description (part);

		retval = tny_gtk_expander_mime_part_view_new (image_view);
		if (!desc) {
			const gchar *filen = tny_mime_part_get_filename (part);
			if (filen) {
				desc = g_strdup_printf (_("Attached image: %s"), filen);
				nf = TRUE;
			} else
				desc = _("Attached image");
		}
		gtk_expander_set_label (GTK_EXPANDER (retval), desc);
		if (nf)
			g_free (desc);
	} else if (tny_mime_part_content_type_is (part, "multipart/*"))
	{
		retval = TNY_MIME_PART_VIEW (tny_msg_view_create_new_inline_viewer (self));

	/* Attachments */
	} else if ((priv->display_attachments && tny_mime_part_is_attachment (part)) || 
		(priv->display_rfc822 && (tny_mime_part_content_type_is (part, "message/rfc822"))))
	{
		GtkTreeModel *model;

		gtk_widget_show (priv->attachview_sw);
		gtk_widget_show (GTK_WIDGET (priv->attachview));

		if (priv->first_attachment)
		{
			model = tny_gtk_attach_list_model_new ();
			gtk_icon_view_set_model (priv->attachview, model);
			priv->first_attachment = FALSE;
		} else {

			model = gtk_icon_view_get_model (priv->attachview);
			if (!model || !TNY_IS_LIST (model))
			{
				model = tny_gtk_attach_list_model_new ();
				gtk_icon_view_set_model (priv->attachview, model);
			}
		}

		retval = tny_gtk_attachment_mime_part_view_new (TNY_GTK_ATTACH_LIST_MODEL (model));
	}

	return retval;
}



static void
on_mpview_realize (GtkWidget *widget, gpointer user_data)
{
	RealizePriv *prv = user_data;

	tny_mime_part_view_set_part (TNY_MIME_PART_VIEW (widget), prv->part);
	g_signal_handler_disconnect (widget, prv->signal);
	g_object_unref (prv->part);
	g_slice_free (RealizePriv, prv);
}


/**
 * tny_gtk_msg_view_display_part:
 * @self: a #TnyGtkMsgView
 * @part: a #TnyMimePart
 *
 * This is non-public API documentation
 * 
 * This method will display one mime part. 
 *
 * The method will use the tny_msg_view_create_mime_part_view_for on self to get
 * a suitable #TnyMimePartView for part. It will attach it to the viewers GtkBox
 * and it will show it in case it's a widget. In case the widget isn't realized,
 * it will delay setting the mime part on the viewer until the widget is 
 * effectively realized (using the realize signal).
 *
 * In case it's not a widget nor a known attachment mime part viewer, the 
 * instance is added to a list of unattached views. Upon finalization of self,
 * will that list be unreferenced.
 **/
static gboolean
tny_gtk_msg_view_display_part (TnyMsgView *self, TnyMimePart *part, const gchar *desc)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	TnyMimePartView *mpview = NULL;

	mpview = tny_msg_view_create_mime_part_view_for (TNY_MSG_VIEW (self), part);

	if (mpview) 
	{
		if (GTK_IS_WIDGET (mpview))
		{
			if (TNY_IS_GTK_MSG_VIEW (mpview) && !GTK_IS_WINDOW (mpview) && !priv->in_expander)
			{
				TnyGtkMsgViewPriv *mppriv = TNY_GTK_MSG_VIEW_GET_PRIVATE (mpview);
				const gchar *label = tny_mime_part_get_description (part);
				GtkWidget *expander;

				mppriv->in_expander = TRUE;
				if (label == NULL || strlen (label) <= 0)
					label = _("Email message attachment");
				expander = gtk_expander_new (label);
				gtk_expander_set_expanded (GTK_EXPANDER (expander), 
					tny_mime_part_content_type_is (part, "text/*"));
				gtk_expander_set_spacing (GTK_EXPANDER (expander), 7);
				gtk_container_add (GTK_CONTAINER (expander), GTK_WIDGET (mpview));
				gtk_widget_show (GTK_WIDGET (expander));
				gtk_box_pack_end (GTK_BOX (TNY_GTK_MSG_VIEW (self)->viewers), 
					GTK_WIDGET (expander), TRUE, TRUE, 0);
			} else if (!GTK_IS_WINDOW (mpview))
				gtk_box_pack_end (GTK_BOX (TNY_GTK_MSG_VIEW (self)->viewers), 
						GTK_WIDGET (mpview), TRUE, TRUE, 0);

			/* If it's a GtkWindow, we don't pack it (but this is unusual and
			 probably a mistake by the developer who created the mime part view) */

			gtk_widget_show (GTK_WIDGET (mpview));

			if (!GTK_WIDGET_REALIZED (mpview))
			{
				RealizePriv *prv = g_slice_new (RealizePriv);
				prv->part = g_object_ref (part);
				prv->signal = g_signal_connect (G_OBJECT (mpview),
					"realize", G_CALLBACK (on_mpview_realize), prv);
			} else
				tny_mime_part_view_set_part (mpview, part);

		} else if (TNY_IS_GTK_ATTACHMENT_MIME_PART_VIEW (mpview)) 
			tny_mime_part_view_set_part (mpview, part);
		else if (!TNY_IS_GTK_ATTACHMENT_MIME_PART_VIEW (mpview)) 
		{
			priv->unattached_views = g_list_prepend (priv->unattached_views, mpview);
			tny_mime_part_view_set_part (mpview, part);
		}
	} else if (!tny_mime_part_content_type_is (part, "multipart/*") &&
		!tny_mime_part_content_type_is (part, "message/*"))
	{
		g_warning (_("I don't have a mime part viewer for %s\n"),
			tny_mime_part_get_content_type (part));

		return FALSE;
	}

	return TRUE;
}



/**
 * tny_gtk_msg_view_display_parts:
 * @self: a #TnyGtkMsgView
 * @parts: a #TnyList instance containing #TnyMimePart instances
 * @desc: description of the parent part (if any)
 *
 * This is non-public API documentation
 *
 * Walks all items in parts and performs tny_gtk_msg_view_display_part on each.
 **/
static void
tny_gtk_msg_view_display_parts (TnyMsgView *self, TnyList *parts, gboolean alternatives, const gchar *desc)
{
	TnyIterator *iterator = tny_list_create_iterator (parts);

	while (!tny_iterator_is_done (iterator))
	{
		TnyMimePart *part = (TnyMimePart*)tny_iterator_get_current (iterator);
		/* gboolean displayed =  */

		tny_gtk_msg_view_display_part (self, part, desc);

		g_object_unref (G_OBJECT (part));

		/* TNY TODO: partial message retrieval: temporarily disabled 
			alternatives detection */

		/* if (alternatives && displayed)
			break; */
		tny_iterator_next (iterator);
	}

	g_object_unref (iterator);

}

static void
tny_gtk_msg_view_set_msg (TnyMsgView *self, TnyMsg *msg)
{
	TNY_GTK_MSG_VIEW_GET_CLASS (self)->set_msg(self, msg);
}

static void 
tny_gtk_msg_view_set_msg_default (TnyMsgView *self, TnyMsg *msg)
{

	tny_mime_part_view_set_part (TNY_MIME_PART_VIEW (self), TNY_MIME_PART (msg));

	return;
}

static void
remove_mime_part_viewer (TnyMimePartView *mpview, GtkContainer *mpviewers)
{
	gtk_container_remove (mpviewers, GTK_WIDGET (mpview));
	return;
}

static void
tny_gtk_msg_view_clear (TnyMsgView *self)
{
	TNY_GTK_MSG_VIEW_GET_CLASS (self)->clear(self);
}

static void
clear_prv (TnyGtkMsgViewPriv *priv)
{
	g_list_foreach (priv->unattached_views, (GFunc)g_object_unref, NULL);
	g_list_free (priv->unattached_views);
	priv->unattached_views = NULL;

	if (G_LIKELY (priv->part))
		g_object_unref (G_OBJECT (priv->part));
	priv->part = NULL;
}

static void
tny_gtk_msg_view_clear_default (TnyMsgView *self)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	GtkContainer *viewers = GTK_CONTAINER (TNY_GTK_MSG_VIEW (self)->viewers);
	GList *kids = gtk_container_get_children (viewers);

	g_list_foreach (kids, (GFunc)remove_mime_part_viewer, viewers);
	g_list_free (kids);

	clear_prv (priv);

	gtk_icon_view_set_model (priv->attachview, tny_gtk_attach_list_model_new ());
	gtk_widget_hide (priv->attachview_sw);
	tny_header_view_set_header (priv->headerview, NULL);
	gtk_widget_hide (GTK_WIDGET (priv->headerview));

	return;
}



static void 
tny_gtk_msg_view_mp_set_part (TnyMimePartView *self, TnyMimePart *part)
{
	TNY_GTK_MSG_VIEW_GET_CLASS (self)->set_part(self, part);
	return;
}

/**
 * tny_gtk_msg_view_mp_set_part_default:
 * @self: a #TnyGtkMsgView
 * @part: a #TnyMimePart
 *
 * This is non-public API documentation
 *
 * In case part is a TnyMsg, first show the header, then the part itself
 * (#TnyMsg inherits from #TnyMimePart) and then dance on the 
 * tny_gtk_msg_view_display_parts song for the parts that are in the message.
 **/
static void 
tny_gtk_msg_view_mp_set_part_default (TnyMimePartView *self, TnyMimePart *part)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);

	tny_msg_view_clear (TNY_MSG_VIEW (self));

	if (part)
	{
		g_assert (TNY_IS_MIME_PART (part));

		if (TNY_IS_MSG (part))
		{
			TnyHeader *header = (TnyHeader*) tny_msg_get_header (TNY_MSG (part));
			if (header && TNY_IS_HEADER (header))
			{
				tny_header_view_set_header (priv->headerview, header);
				g_object_unref (G_OBJECT (header));
				gtk_widget_show (GTK_WIDGET (priv->headerview));
			}
		}

		priv->part = g_object_ref (G_OBJECT (part));

		if (!tny_mime_part_content_type_is (part, "multipart/*")) {

			tny_gtk_msg_view_display_part (TNY_MSG_VIEW (self), part, NULL);

		} else {
			TnyList *list;
			list = tny_simple_list_new ();

			tny_mime_part_get_parts (part, list);
			tny_gtk_msg_view_display_parts (TNY_MSG_VIEW (self), list, 
				tny_mime_part_content_type_is (part, "multipart/alternative"),
				tny_mime_part_get_description (part));
			g_object_unref (G_OBJECT (list));
		}
	}

	return;
}


static TnyMimePart* 
tny_gtk_msg_view_mp_get_part (TnyMimePartView *self)
{
	return TNY_GTK_MSG_VIEW_GET_CLASS (self)->get_part(self);
}


static TnyMimePart* 
tny_gtk_msg_view_mp_get_part_default (TnyMimePartView *self)
{
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	return TNY_MIME_PART (g_object_ref (priv->part));
}

static void 
tny_gtk_msg_view_mp_clear (TnyMimePartView *self)
{
	tny_msg_view_clear (TNY_MSG_VIEW (self));
}


/**
 * tny_gtk_msg_view_new:
 *
 * Create a new #TnyMsgView
 *
 * returns: (caller-owns): a new #TnyMsgView 
 * since: 1.0
 * audience: application-developer
 **/
TnyMsgView*
tny_gtk_msg_view_new (void)
{
	TnyGtkMsgView *self = g_object_new (TNY_TYPE_GTK_MSG_VIEW, NULL);
	return TNY_MSG_VIEW (self);
}


static TnyHeaderView * 
tny_gtk_msg_view_create_header_view_default (TnyGtkMsgView *self)
{
	return tny_gtk_header_view_new ();
}

static void
tny_gtk_msg_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGtkMsgView *self = (TnyGtkMsgView *)instance;
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);
	GtkBox *vbox;

	priv->kid = GTK_BOX (gtk_vbox_new (FALSE, 0));
	vbox = priv->kid;

	priv->parented = FALSE;
	priv->in_expander = FALSE;

	/* Defaults */
	priv->display_html = FALSE;
	priv->display_plain = TRUE;
	priv->display_attachments = TRUE;
	priv->display_rfc822 = TRUE;
	priv->first_attachment = TRUE;
	priv->unattached_views = NULL;
	priv->part = NULL;


	priv->headerview = TNY_GTK_MSG_VIEW_GET_CLASS (self)->create_header_view(self);

	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (priv->headerview), FALSE, FALSE, 0);

	TNY_GTK_MSG_VIEW (self)->viewers = GTK_CONTAINER (gtk_vbox_new (FALSE, 0));
	gtk_box_pack_start (GTK_BOX (vbox), 
		GTK_WIDGET (TNY_GTK_MSG_VIEW (self)->viewers), FALSE, FALSE, 0);
	gtk_widget_show (GTK_WIDGET (TNY_GTK_MSG_VIEW (self)->viewers));

	priv->attachview_sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (priv->attachview_sw),
					GTK_SHADOW_NONE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->attachview_sw),
					GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	priv->attachview = GTK_ICON_VIEW (gtk_icon_view_new ());
	gtk_icon_view_set_selection_mode (priv->attachview, GTK_SELECTION_SINGLE);
	gtk_icon_view_set_text_column (priv->attachview,
			TNY_GTK_ATTACH_LIST_MODEL_FILENAME_COLUMN);
	gtk_icon_view_set_pixbuf_column (priv->attachview,
			TNY_GTK_ATTACH_LIST_MODEL_PIXBUF_COLUMN);
	gtk_icon_view_set_columns (priv->attachview, -1);
	gtk_icon_view_set_item_width (priv->attachview, 100);
	gtk_icon_view_set_column_spacing (priv->attachview, 10);

	gtk_box_pack_start (GTK_BOX (vbox), priv->attachview_sw, FALSE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (priv->attachview_sw), GTK_WIDGET (priv->attachview));

	/* Default is a non-online viewer */
	gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (vbox));

	gtk_widget_show (GTK_WIDGET (vbox));
	gtk_widget_hide (GTK_WIDGET (priv->headerview));
	gtk_widget_show (GTK_WIDGET (priv->attachview));

	return;
}

static void
widget_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	if (((GtkBin *) widget)->child)
		gtk_widget_size_request (((GtkBin *) widget)->child, requisition);
}

static void
widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	widget->allocation = *allocation;

	if (((GtkBin *) widget)->child)
		gtk_widget_size_allocate (((GtkBin *) widget)->child, allocation);
}


static void
tny_gtk_msg_view_finalize (GObject *object)
{
	TnyGtkMsgView *self = (TnyGtkMsgView *)object;	
	TnyGtkMsgViewPriv *priv = TNY_GTK_MSG_VIEW_GET_PRIVATE (self);

	clear_prv (priv);

	if (G_LIKELY (priv->part))
		g_object_unref (G_OBJECT (priv->part));

	(*parent_class->finalize) (object);

	return;
}

static void
tny_msg_view_init (gpointer g, gpointer iface_data)
{
	TnyMsgViewIface *klass = (TnyMsgViewIface *)g;

	klass->get_msg= tny_gtk_msg_view_get_msg;
	klass->set_msg= tny_gtk_msg_view_set_msg;
	klass->set_unavailable= tny_gtk_msg_view_set_unavailable;
	klass->clear= tny_gtk_msg_view_clear;
	klass->create_mime_part_view_for= tny_gtk_msg_view_create_mime_part_view_for;
	klass->create_new_inline_viewer= tny_gtk_msg_view_create_new_inline_viewer;

	return;
}


static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
	TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

	klass->get_part= tny_gtk_msg_view_mp_get_part;
	klass->set_part= tny_gtk_msg_view_mp_set_part;
	klass->clear= tny_gtk_msg_view_mp_clear;

	return;
}


static void 
tny_gtk_msg_view_class_init (TnyGtkMsgViewClass *class)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	widget_class = (GtkWidgetClass *) class;
	widget_class->size_request  = widget_size_request;
	widget_class->size_allocate = widget_size_allocate;

	object_class->finalize = tny_gtk_msg_view_finalize;

	class->get_part= tny_gtk_msg_view_mp_get_part_default;
	class->set_part= tny_gtk_msg_view_mp_set_part_default;
	class->get_msg= tny_gtk_msg_view_get_msg_default;
	class->set_msg= tny_gtk_msg_view_set_msg_default;
	class->set_unavailable= tny_gtk_msg_view_set_unavailable_default;
	class->clear= tny_gtk_msg_view_clear_default;
	class->create_mime_part_view_for= tny_gtk_msg_view_create_mime_part_view_for_default;
	class->create_new_inline_viewer= tny_gtk_msg_view_create_new_inline_viewer_default;

	class->create_header_view= tny_gtk_msg_view_create_header_view_default;

	g_type_class_add_private (object_class, sizeof (TnyGtkMsgViewPriv));


	return;
}

static gpointer 
tny_gtk_msg_view_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkMsgViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_msg_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkMsgView),
		  0,      /* n_preallocs */
		  tny_gtk_msg_view_instance_init,    /* instance_init */
		  NULL
		};

	static const GInterfaceInfo tny_msg_view_info = 
		{
		  (GInterfaceInitFunc) tny_msg_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	static const GInterfaceInfo tny_mime_part_view_info = 
		{
		  (GInterfaceInitFunc) tny_mime_part_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (GTK_TYPE_BIN,
				       "TnyGtkMsgView",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_MIME_PART_VIEW, 
				     &tny_mime_part_view_info);

	g_type_add_interface_static (type, TNY_TYPE_MSG_VIEW, 
				     &tny_msg_view_info);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_gtk_msg_view_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType 
tny_gtk_msg_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_msg_view_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
