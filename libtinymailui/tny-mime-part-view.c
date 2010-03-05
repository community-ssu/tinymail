/* libtinymailui - The Tiny Mail UI library
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
 * TnyMimePartView:
 * 
 * A type that can view a #TnyMimePart
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-mime-part-view.h>


/**
 * tny_mime_part_view_clear:
 * @self: a #TnyMimePartView
 *
 * Clear @self, show nothing
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void
tny_mime_part_view_clear (TnyMimePartView *self)
{
#ifdef DEBUG
	if (!TNY_MIME_PART_VIEW_GET_IFACE (self)->clear)
		g_critical ("You must implement tny_mime_part_view_clear\n");
#endif

	TNY_MIME_PART_VIEW_GET_IFACE (self)->clear(self);
	return;
}


/**
 * tny_mime_part_view_get_part:
 * @self: a #TnyMimePartView
 *
 * Get the current mime part of @self. If @self is not displaying any mime part,
 * NULL will be returned. Else the return value must be unreferenced after use. 
 *
 * Example:
 * <informalexample><programlisting>
 * static TnyMimePart* 
 * tny_gtk_text_mime_part_view_get_part (TnyMimePartView *self)
 * {
 *      TnyGtkTextMimePartViewPriv *priv = TNY_GTK_TEXT_MIME_PART_VIEW_GET_PRIV (self);
 *      return priv->part?TNY_MIME_PART (g_object_ref (priv->part)):NULL;
 * }
 * </programlisting></informalexample>
 *
 * returns: (null-ok) (caller-owns): a #TnyMimePart instance or NULL
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyMimePart*
tny_mime_part_view_get_part (TnyMimePartView *self)
{
#ifdef DEBUG
	if (!TNY_MIME_PART_VIEW_GET_IFACE (self)->get_part)
		g_critical ("You must implement tny_mime_part_view_get_part\n");
#endif

	return TNY_MIME_PART_VIEW_GET_IFACE (self)->get_part(self);
}

/**
 * tny_mime_part_view_set_part:
 * @self: a #TnyMimePartView
 * @mime_part: a #TnyMimePart
 *
 * Set the MIME part which @self should display. Note that if possible, try to
 * wait for as long as possible to call this API. As soon as this API is used,
 * and the part's data is not available locally, will the part's data be
 * requested from the service.
 *
 * If you can delay the need for calling this API, for example by offering the
 * user a feature to make it visible rather than making the part always visible,
 * then this is a smart thing to do to save bandwidth consumption in your final
 * application.
 *
 * For example #GtkExpander and setting it when the child widget gets realized
 * in case you are using libtinymailui-gtk. A convenient #TnyGtkExpanderMimePartView
 * is available in libtinymailui-gtk that does exactly this. It will call for the
 * API when you expand the expander (the user presses the expand-arrow).
 *
 * Note that it's recommended to use tny_mime_part_decode_to_stream_async() over
 * tny_mime_part_decode_to_stream() if you want your user interface to remain 
 * responsive in case Tinymail's engine needs to pull data from a service.
 *
 * Example:
 * <informalexample><programlisting>
 * static void
 * on_mime_part_decoded (TnyMimePart *part, TnyStream *dest, gboolean cancelled, GError *err, gpointer user_data)
 * {
 *        TnyMimePartView *self = (TnyMimePartView *) user_data;
 *        if (!cancelled && !err)
 *             make_part_really_visible_now (self, part);
 *        g_object_unref (self);
 * }
 * static void
 * on_status (GObject *part, TnyStatus *status, gpointer user_data)
 * {
 *        TnyMimePartView *self = (TnyMimePartView *) user_data;
 *        move_progress_bar (self, tny_status_get_fraction (status));
 * }
 * static void 
 * tny_gtk_text_mime_part_view_set_part (TnyMimePartView *self, TnyMimePart *part)
 * {
 *      if (part) {
 *           GtkTextBuffer *buffer;
 *           TnyStream *dest;
 *           buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self));
 *           if (buffer &amp;&amp; GTK_IS_TEXT_BUFFER (buffer))
 *                gtk_text_buffer_set_text (buffer, "", 0);
 *           dest = tny_gtk_text_buffer_stream_new (buffer);
 *           tny_stream_reset (dest);
 *           tny_mime_part_decode_to_stream_async (part, dest, on_mime_part_decoded, 
 *                   on_status, g_object_ref (self));
 *           tny_stream_reset (dest);
 *           g_object_unref (dest);
 *           priv->part = TNY_MIME_PART (g_object_ref (part));
 *      }
 * }
 * static void
 * tny_gtk_text_mime_part_view_finalize (TnyGtkTextMimePartView *self)
 * {
 *      if (priv->part))
 *          g_object_unref (priv->part);
 * }
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void
tny_mime_part_view_set_part (TnyMimePartView *self, TnyMimePart *mime_part)
{
#ifdef DEBUG
	if (!TNY_MIME_PART_VIEW_GET_IFACE (self)->set_part)
		g_critical ("You must implement tny_mime_part_view_set_part\n");
#endif

	TNY_MIME_PART_VIEW_GET_IFACE (self)->set_part(self, mime_part);
	return;
}

static void
tny_mime_part_view_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_mime_part_view_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyMimePartViewIface),
		  tny_mime_part_view_base_init,   /* base_init */
		  NULL,   /* base_finalize */
		  NULL,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  0,
		  0,      /* n_preallocs */
		  NULL    /* instance_init */
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyMimePartView", &info, 0);

	return GSIZE_TO_POINTER (type);
}

GType
tny_mime_part_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_mime_part_view_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
