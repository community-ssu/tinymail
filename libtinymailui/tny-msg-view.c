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
 * TnyMsgView:
 * 
 * A type that can view a #TnyMsg and usually inherits from #TnyMimePartView
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-msg-view.h>

/**
 * tny_msg_view_create_new_inline_viewer:
 * @self: a #TnyMsgView
 *
 * Create a new #TnyMsgView that can be used to display an inline message, 
 * like a message/rfc822 MIME part. Usually it will return a new instance of
 * the same type as @self. The returned instance must be unreferenced after
 * use.
 *
 * Example:
 * <informalexample><programlisting>
 * static TnyMsgView*
 * tny_my_html_msg_view_create_new_inline_viewer (TnyMsgView *self)
 * {
 *    return tny_my_html_msg_view_new ();
 * }
 * </programlisting></informalexample>
 *
 * returns: (caller-owns): a #TnyMsgView instance
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyMsgView* 
tny_msg_view_create_new_inline_viewer (TnyMsgView *self)
{
#ifdef DEBUG
	if (!TNY_MSG_VIEW_GET_IFACE (self)->create_new_inline_viewer)
		g_critical ("You must implement tny_msg_view_create_new_inline_viewer\n");
#endif

	return TNY_MSG_VIEW_GET_IFACE (self)->create_new_inline_viewer(self);
}

/**
 * tny_msg_view_create_mime_part_view_for:
 * @self: a #TnyMsgView
 * @part: a #TnyMimePart
 *
 * Create a #TnyMimePartView instance for viewing @part. The returned instance
 * must be unreferenced after use. It's recommended to return the result of
 * calling the function on the super of @self (like in the example below) in
 * case your type's implementation can't display @part.
 *
 * Example:
 * <informalexample><programlisting>
 * static TnyMimePartView*
 * tny_my_html_msg_view_create_mime_part_view_for (TnyMsgView *self, TnyMimePart *part)
 * {
 *    TnyMimePartView *retval = NULL;
 *    if (tny_mime_part_content_type_is (part, "text/html")) {
 *        GtkWidget *widget = (GtkWidget *) self;
 *        retval = tny_my_html_mime_part_view_new ();
 *    } else
 *        retval = TNY_GTK_MSG_VIEW_CLASS (parent_class)->create_mime_part_view_for(self, part);
 *    return retval;
 * }
 * </programlisting></informalexample>
 *
 * ps. For a real and complete working example take a look at the implementation of 
 * #TnyMozEmbedMsgView in libtinymailui-mozembed.
 *
 * returns: (caller-owns): a #TnyMimePartView instance
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyMimePartView*
tny_msg_view_create_mime_part_view_for (TnyMsgView *self, TnyMimePart *part)
{
#ifdef DEBUG
	if (!TNY_MSG_VIEW_GET_IFACE (self)->create_mime_part_view_for)
		g_critical ("You must implement tny_msg_view_create_mime_part_view_for\n");
#endif

	return TNY_MSG_VIEW_GET_IFACE (self)->create_mime_part_view_for(self, part);
}

/**
 * tny_msg_view_clear:
 * @self: A #TnyMsgView
 *
 * Clear @self, show nothing
 * 
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void
tny_msg_view_clear (TnyMsgView *self)
{
#ifdef DEBUG
	if (!TNY_MSG_VIEW_GET_IFACE (self)->clear)
		g_critical ("You must implement tny_msg_view_clear\n");
#endif

	TNY_MSG_VIEW_GET_IFACE (self)->clear(self);
	return;
}


/**
 * tny_msg_view_set_unavailable:
 * @self: a #TnyMsgView
 *
 * Set @self to display that a message was unavailable. You can for example
 * implement this method by simply calling tny_msg_view_clear().
 * 
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void
tny_msg_view_set_unavailable (TnyMsgView *self)
{
#ifdef DEBUG
	if (!TNY_MSG_VIEW_GET_IFACE (self)->set_unavailable)
		g_critical ("You must implement tny_msg_view_set_unavailable\n");
#endif

	TNY_MSG_VIEW_GET_IFACE (self)->set_unavailable(self);
	return;
}


/**
 * tny_msg_view_get_msg:
 * @self: a #TnyMsgView
 *
 * Get the current message of @self. If @self is not displaying any message,
 * NULL will be returned. Else the return value must be unreferenced after use.
 *
 * When inheriting from a #TnyMimePartView, this method is most likely going to
 * be an alias for tny_mime_part_view_get_part(), with the returned #TnyMimePart
 * casted to a #TnyMsg (in this case, the method in the #TnyMimePartView should
 * return a #TnyMsg, indeed).
 *
 * returns: (null-ok) (caller-owns): A #TnyMsg instance or NULL
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyMsg* 
tny_msg_view_get_msg (TnyMsgView *self)
{
#ifdef DEBUG
	if (!TNY_MSG_VIEW_GET_IFACE (self)->get_msg)
		g_critical ("You must implement tny_msg_view_get_msg\n");
#endif

	return TNY_MSG_VIEW_GET_IFACE (self)->get_msg(self);
}

/**
 * tny_msg_view_set_msg:
 * @self: a #TnyMsgView
 * @msg: a #TnyMsg
 *
 * Set the message which view @self must display.
 * 
 * Note that you can get a list of mime parts using the tny_mime_part_get_parts()
 * API of the #TnyMimePart type. You can use the tny_msg_view_create_mime_part_view_for()
 * API to get a #TnyMimePartView that can view the mime part.
 * 
 * When inheriting from a #TnyMimePartView this method is most likely going to
 * decorate or alias tny_mime_part_view_set_part(), with the passed #TnyMsg
 * casted to a #TnyMimePart.
 *
 * Example:
 * <informalexample><programlisting>
 * static void 
 * tny_my_msg_view_set_msg (TnyMsgView *self, TnyMsg *msg)
 * {
 *     TnyIterator *iterator;
 *     TnyList *list = tny_simple_list_new ();
 *     tny_msg_view_clear (self);
 *     header = tny_msg_get_header (msg);
 *     tny_header_view_set_header (priv->headerview, header);
 *     g_object_unref (header);
 *     tny_mime_part_view_set_part (TNY_MIME_PART_VIEW (self),
 *                TNY_MIME_PART (msg));
 *     tny_mime_part_get_parts (TNY_MIME_PART (msg), list);
 *     iterator = tny_list_create_iterator (list);
 *     while (!tny_iterator_is_done (iterator)) {
 *         TnyMimePart *part = tny_iterator_get_current (iterator);
 *         TnyMimePartView *mpview;
 *         mpview = tny_msg_view_create_mime_part_view_for (self, part);
 *         if (mpview)
 *             tny_mime_part_view_set_part (mpview, part);
 *         g_object_unref (part);
 *         tny_iterator_next (iterator);
 *     }
 *     g_object_unref (iterator);
 *     g_object_unref (list);
 * }
 * </programlisting></informalexample>
 *
 * ps. For a real and complete working example take a look at the implementation of 
 * #TnyGtkMsgView in libtinymailui-gtk.
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void
tny_msg_view_set_msg (TnyMsgView *self, TnyMsg *msg)
{
#ifdef DEBUG
	if (!TNY_MSG_VIEW_GET_IFACE (self)->set_msg)
		g_critical ("You must implement tny_msg_view_set_msg\n");
#endif

	TNY_MSG_VIEW_GET_IFACE (self)->set_msg(self, msg);
	return;
}

static void
tny_msg_view_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_msg_view_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyMsgViewIface),
		  tny_msg_view_base_init,   /* base_init */
		  NULL,   /* base_finalize */
		  NULL,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  0,
		  0,      /* n_preallocs */
		  NULL    /* instance_init */
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyMsgView", &info, 0);

	g_type_interface_add_prerequisite (type, TNY_TYPE_MIME_PART_VIEW);

	return GSIZE_TO_POINTER (type);
}

GType
tny_msg_view_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_msg_view_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
