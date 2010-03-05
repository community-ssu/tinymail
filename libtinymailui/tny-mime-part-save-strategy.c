/* libtinymail - The Tiny Mail base library
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
 * TnyMimePartSaveStrategy:
 * 
 * A strategy for saving a #TnyMimePart
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-mime-part-save-strategy.h>

/**
 * tny_mime_part_save_strategy_perform_save:
 * @self: a #TnyMimePartSaveStrategy
 * @part: a #TnyMimePart that must be saved
 *
 * With @self being a delegate of a #TnyMimePartSaver, this method performs the
 * saving of @part.
 *
 * A save strategy for a mime part is used with a type that implements the 
 * #TnyMimePartSaver interface. Very often are such types also implementing the
 * #TnyMsgView and/or #TnyMimePartView interfaces (although it's not a
 * requirement). When implementing #TnyMimePartSaver you say that the view has
 * functionality for saving mime parts.
 *
 * Example:
 * <informalexample><programlisting>
 * static void 
 * tny_my_msg_view_save (TnyMimePartView *self_i, TnyMimePart *attachment)
 * {
 *     TnyMyMsgView *self = TNY_MY_MSG_VIEW (self_i);
 *     tny_mime_part_save_strategy_perform_save (self->mime_part_save_strategy, 
 *              attachment);
 * }
 * </programlisting></informalexample>
 *
 * Devices can have specific strategies that are changed at runtime. For example
 * a save-strategy that sends the content of the mime part it to another computer
 * and/or a save-strategy that saves it to a flash disk. Configurable at runtime
 * by simply switching the save-strategy property of a #TnyMimePartSaver.
 *
 * Example:
 * <informalexample><programlisting>
 * static void
 * tny_gtk_mime_part_save_strategy_perform_save (TnyMimePartSaveStrategy *self, TnyMimePart *part)
 * {
 *      GtkFileChooserDialog *dialog;
 *      dialog = GTK_FILE_CHOOSER_DIALOG 
 *            (gtk_file_chooser_dialog_new (_("Save attachment"), NULL,
 *            GTK_FILE_CHOOSER_ACTION_MIME_PART_SAVE,
 *            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_MIME_PART_SAVE, 
 *            GTK_RESPONSE_ACCEPT, NULL));
 *      gtk_file_chooser_set_current_name (dialog, 
 *                    tny_mime_part_get_filename (part));
 *      if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
 *            gchar *uri; int fd;
 *            uri = gtk_file_chooser_get_filename (dialog);
 *            fd = open (uri, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
 *            if (fd != -1) {
 *                      TnyStream *stream = tny_fs_stream_new (fd);
 *                      tny_mime_part_decode_to_stream (part, stream, NULL);
 *                      g_object_unref (stream);
 *            }
 *      }
 *      gtk_widget_destroy (GTK_WIDGET (dialog));
 * }
 * </programlisting></informalexample>
 *
 * An example when to use this method is in a clicked handler of a popup menu
 * of a attachment #TnyMimePartView in your #TnyMsgView.
 * 
 * Note that a mime part can mean both the entire message (without its headers)
 * and one individual mime part in such a message or a message in a message (in 
 * case of a messge/rfc822 mime part).
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void
tny_mime_part_save_strategy_perform_save (TnyMimePartSaveStrategy *self, TnyMimePart *part)
{
#ifdef DEBUG
	if (!TNY_MIME_PART_SAVE_STRATEGY_GET_IFACE (self)->perform_save)
		g_critical ("You must implement tny_mime_part_save_strategy_perform_save\n");
#endif

	TNY_MIME_PART_SAVE_STRATEGY_GET_IFACE (self)->perform_save(self, part);
	return;
}

static void
tny_mime_part_save_strategy_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) 
		initialized = TRUE;
}

static gpointer
tny_mime_part_save_strategy_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyMimePartSaveStrategyIface),
		  tny_mime_part_save_strategy_base_init,   /* base_init */
		  NULL,   /* base_finalize */
		  NULL,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  0,
		  0,      /* n_preallocs */
		  NULL    /* instance_init */
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyMimePartSaveStrategy", &info, 0);

	g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);

	return GSIZE_TO_POINTER (type);
}

GType
tny_mime_part_save_strategy_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_mime_part_save_strategy_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
