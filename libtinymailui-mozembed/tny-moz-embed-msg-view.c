/* libtinymailui-mozembed - The Tiny Mail UI library for Gtk+
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
#include <gtk/gtk.h>

#include <tny-moz-embed-msg-view.h>
#include <tny-moz-embed-html-mime-part-view.h>

static GObjectClass *parent_class = NULL;

/**
 * tny_moz_embed_msg_view_new:
 *
 * Create a new #TnyMsgView that can display HTML messages.
 *
 * Return value: a new #TnyMsgView instance implemented for Gtk+
 **/
TnyMsgView*
tny_moz_embed_msg_view_new (void)
{
	TnyMozEmbedMsgView *self = g_object_new (TNY_TYPE_MOZ_EMBED_MSG_VIEW, NULL);
	
	return TNY_MSG_VIEW (self);
}

static void
tny_moz_embed_msg_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	return;
}

static void
tny_moz_embed_msg_view_finalize (GObject *object)
{
	(*parent_class->finalize) (object);

	return;
}


/**
 * tny_moz_embed_msg_view_create_mime_part_view_for_default:
 * @self: a #TnyMozEmbedMsgView instance
 * @part: a #TnyMimePart instance
 *
 * This is non-public API documentation
 *
 * This implementation will return a #TnyMozEmbedHtmlMimePartView in case part
 * is of content type text/html. Else it will call its super implementation.
 **/
static TnyMimePartView*
tny_moz_embed_msg_view_create_mime_part_view_for_default (TnyMsgView *self, TnyMimePart *part)
{
	TnyMimePartView *retval = NULL;

	g_assert (TNY_IS_MIME_PART (part));

	/* HTML mime part (shows HTML using GtkMozEmbed) */
	if (tny_mime_part_content_type_is (part, "text/html")) {
		TnyStatusCallback status_callback;
		gpointer status_user_data;

		tny_gtk_msg_view_get_status_callback (TNY_GTK_MSG_VIEW (self), &status_callback, &status_user_data);

		GtkWidget *widget = (GtkWidget *) self;
		retval = tny_moz_embed_html_mime_part_view_new (status_callback, status_user_data);

		gtk_widget_set_size_request (GTK_WIDGET (retval),
					     widget->allocation.width>11?widget->allocation.width-10:1,
					     2500);
	} else
		retval = TNY_GTK_MSG_VIEW_CLASS (parent_class)->create_mime_part_view_for(self, part);

	return retval;
}

static TnyMimePartView*
tny_moz_embed_msg_view_create_mime_part_view_for (TnyMsgView *self, TnyMimePart *part)
{
	return TNY_MOZ_EMBED_MSG_VIEW_GET_CLASS (self)->create_mime_part_view_for(self, part);
}



static TnyMsgView*
tny_moz_embed_msg_view_create_new_inline_viewer (TnyMsgView *self)
{
	return TNY_MOZ_EMBED_MSG_VIEW_GET_CLASS (self)->create_new_inline_viewer(self);
}

/**
 * tny_moz_embed_msg_view_create_new_inline_viewer_default:
 * @self: a #TnyMozEmbedMsgView instance
 *
 * This is non-public API documentation
 *
 * This implementation returns a new #TnyMozEmbedMsgView instance suitable for
 * displaying HTML messages.
 **/
static TnyMsgView*
tny_moz_embed_msg_view_create_new_inline_viewer_default (TnyMsgView *self)
{
	TnyMsgView *retval = tny_moz_embed_msg_view_new ();
	TnyStatusCallback status_callback;
	gpointer status_user_data;

	tny_gtk_msg_view_set_parented (TNY_GTK_MSG_VIEW (retval), TRUE);
	tny_gtk_msg_view_get_status_callback (TNY_GTK_MSG_VIEW (self), &status_callback, &status_user_data);
	tny_gtk_msg_view_set_status_callback (TNY_GTK_MSG_VIEW (retval), status_callback, status_user_data);

	return retval;
}


static void 
tny_moz_embed_msg_view_class_init (TnyMozEmbedMsgViewClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->create_mime_part_view_for= tny_moz_embed_msg_view_create_mime_part_view_for_default;
	class->create_new_inline_viewer= tny_moz_embed_msg_view_create_new_inline_viewer_default;

	object_class->finalize = tny_moz_embed_msg_view_finalize;

	TNY_GTK_MSG_VIEW_CLASS (class)->create_mime_part_view_for= tny_moz_embed_msg_view_create_mime_part_view_for;
	TNY_GTK_MSG_VIEW_CLASS (class)->create_new_inline_viewer= tny_moz_embed_msg_view_create_new_inline_viewer;

	return;
}

static gpointer
tny_moz_embed_msg_view_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyMozEmbedMsgViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_moz_embed_msg_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyMozEmbedMsgView),
		  0,      /* n_preallocs */
		  tny_moz_embed_msg_view_instance_init    /* instance_init */
		};

	type = g_type_register_static (TNY_TYPE_GTK_MSG_VIEW,
				       "TnyMozEmbedMsgView",
				       &info, 0);

	return GSIZE_TO_POINTER (type);
}

GType 
tny_moz_embed_msg_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_moz_embed_msg_view_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
