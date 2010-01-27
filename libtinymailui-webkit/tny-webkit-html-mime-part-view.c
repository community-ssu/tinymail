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

#include "mozilla-preferences.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string.h>
#include <gtk/gtk.h>

#include <tny-webkit-html-mime-part-view.h>
#include <tny-webkit-stream.h>

#ifdef GNOME
#include <tny-vfs-stream.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#else
#include <tny-fs-stream.h>
#endif

static GObjectClass *parent_class = NULL;

typedef struct _TnyWebkitHtmlMimePartViewPriv TnyWebkitHtmlMimePartViewPriv;

struct _TnyWebkitHtmlMimePartViewPriv {
	TnyMimePart *part;
	TnyStatusCallback status_callback;
	gpointer status_user_data;
};

#define TNY_WEBKIT_HTML_MIME_PART_VIEW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_WEBKIT_HTML_MIME_PART_VIEW, TnyWebkitHtmlMimePartViewPriv))


static TnyMimePart*
tny_webkit_html_mime_part_view_get_part (TnyMimePartView *self)
{
	TnyWebkitHtmlMimePartViewPriv *priv = TNY_WEBKIT_HTML_MIME_PART_VIEW_GET_PRIVATE (self);
	return (priv->part)?TNY_MIME_PART (g_object_ref (priv->part)):NULL;
}

/**
 * tny_webkit_html_mime_part_view_set_part:
 * @self: a #TnyWebkitMimePartView instance
 * @part: a #TnyMimePart instance
 *
 * This is non-public API documentation
 *
 * The implementation simply decodes the part to a stream that is implemented in
 * such a way that it will forward the stream to the GtkWebkit stream API.
 **/
static void 
tny_webkit_html_mime_part_view_set_part (TnyMimePartView *self, TnyMimePart *part)
{
	TnyWebkitHtmlMimePartViewPriv *priv = TNY_WEBKIT_HTML_MIME_PART_VIEW_GET_PRIVATE (self);

	if (priv->part)
		g_object_unref (priv->part);

	if (part) {
		TnyStream *dest;

		dest = tny_webkit_stream_new (self);
		tny_stream_reset (dest);
		tny_mime_part_decode_to_stream_async (part, dest, NULL, 
			priv->status_callback, priv->status_user_data);
		g_object_unref (dest);
		priv->part = (TnyMimePart *) g_object_ref (part);
	}

	return;
}

static void
tny_webkit_html_mime_part_view_clear (TnyMimePartView *self)
{

	webkit_web_view_load_html_string (WEBKIT_WEB_VIEW (self), "", "");

	return;
}

/**
 * tny_webkit_html_mime_part_view_new:
 *
 * Create a #TnyMimePartView that can display HTML mime parts
 *
 * Return value: a new #TnyMimePartView instance implemented for Gtk+
 **/
TnyMimePartView*
tny_webkit_html_mime_part_view_new (TnyStatusCallback status_callback, gpointer status_user_data)
{
	TnyWebkitHtmlMimePartView *self = g_object_new (TNY_TYPE_WEBKIT_HTML_MIME_PART_VIEW, NULL);
	TnyWebkitHtmlMimePartViewPriv *priv = TNY_WEBKIT_HTML_MIME_PART_VIEW_GET_PRIVATE (self);

	priv->status_callback = status_callback;
	priv->status_user_data = status_user_data;

	return TNY_MIME_PART_VIEW (self);
}

static void
tny_webkit_html_mime_part_view_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyWebkitHtmlMimePartView *self  = (TnyWebkitHtmlMimePartView*) instance;
	TnyWebkitHtmlMimePartViewPriv *priv = TNY_WEBKIT_HTML_MIME_PART_VIEW_GET_PRIVATE (self);
	priv->part = NULL;
	return;
}

static void
tny_webkit_html_mime_part_view_finalize (GObject *object)
{
	TnyWebkitHtmlMimePartView *self = (TnyWebkitHtmlMimePartView *)object;	
	TnyWebkitHtmlMimePartViewPriv *priv = TNY_WEBKIT_HTML_MIME_PART_VIEW_GET_PRIVATE (self);
	if (priv->part)
		g_object_unref (priv->part);
	(*parent_class->finalize) (object);
	return;
}

static void
tny_mime_part_view_init (gpointer g, gpointer iface_data)
{
	TnyMimePartViewIface *klass = (TnyMimePartViewIface *)g;

	klass->get_part= tny_webkit_html_mime_part_view_get_part;
	klass->set_part= tny_webkit_html_mime_part_view_set_part;
	klass->clear= tny_webkit_html_mime_part_view_clear;

	return;
}

static void 
tny_webkit_html_mime_part_view_class_init (TnyWebkitHtmlMimePartViewClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_webkit_html_mime_part_view_finalize;

	g_type_class_add_private (object_class, sizeof (TnyWebkitHtmlMimePartViewPriv));

	return;
}

static gpointer
tny_webkit_html_mime_part_view_get_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyWebkitHtmlMimePartViewClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_webkit_html_mime_part_view_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyWebkitHtmlMimePartView),
		  0,      /* n_preallocs */
		  tny_webkit_html_mime_part_view_instance_init,    /* instance_init */
		  NULL
		};

	static const GInterfaceInfo tny_mime_part_view_info = 
		{
		  (GInterfaceInitFunc) tny_mime_part_view_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (GTK_TYPE_WEBKIT,
				       "TnyWebkitHtmlMimePartView",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_MIME_PART_VIEW, 
				     &tny_mime_part_view_info);

	return GUINT_TO_POINTER (type);
}

GType 
tny_webkit_html_mime_part_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_webkit_html_mime_part_view_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
