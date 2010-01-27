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
 * TnyMsgRemoveStrategy:
 *
 * A strategy for removing messages
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-msg-remove-strategy.h>
#include <tny-folder.h>
#include <tny-header.h>

/**
 * tny_msg_remove_strategy_perform_remove:
 * @self: a #TnyMsgRemoveStrategy 
 * @folder: a #TnyFolder from which the message will be removed
 * @header: a #TnyHeader of the message that must be removed
 * @err: (null-ok): a #GError or NULL
 *
 * Performs the removal of a message from @folder.
 *
 * This doesn't remove it from a #TnyList that holds the headers (for example 
 * for a header summary view) if the tny_folder_get_headers() method happened 
 * before the deletion. You are responsible for refreshing your own lists.
 * But take a look at #TnyFolderMonitor which serves this purpose.
 *
 * This method also doesn't have to wipe it immediately from @folder. 
 * Depending on the implementation it might only mark it as removed (it for 
 * example sets the TNY_HEADER_FLAG_DELETED). In such a case only after
 * performing the tny_folder_sync() method on @folder, it will really be
 * removed.
 *
 * In such a case this means that a tny_folder_get_headers() method call will
 * still prepend the removed message to the list. It will do this until the
 * expunge happened. You are advised to hide messages that have been marked
 * as being deleted from your summary view. In Gtk+, for the #GtkTreeView
 * component, you can do this using the #GtkTreeModelFilter tree model filtering
 * model.
 *
 * The #TnyCamelMsgRemoveStrategy implementation works this way. This 
 * implementation is also the default implementation for most #TnyFolder 
 * implementations in libtinymail-camel
 * 
 * Note that it's possible that another implementation works differently. You
 * could, for example, inherit or decorate the #TnyCamelMsgRemoveStrategy 
 * implementation by adding code that also permanently removes the message
 * in your inherited special type.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_msg_remove_strategy_perform_remove (TnyMsgRemoveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MSG_REMOVE_STRATEGY (self));
	g_assert (folder);
	g_assert (TNY_IS_FOLDER (folder));
	g_assert (header);
	g_assert (TNY_IS_HEADER (header));
	g_assert (TNY_MSG_REMOVE_STRATEGY_GET_IFACE (self)->perform_remove!= NULL);
#endif

	TNY_MSG_REMOVE_STRATEGY_GET_IFACE (self)->perform_remove(self, folder, header, err);
	return;
}

static void
tny_msg_remove_strategy_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) 
		initialized = TRUE;
}

static gpointer
tny_msg_remove_strategy_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyMsgRemoveStrategyIface),
			tny_msg_remove_strategy_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL    /* instance_init */
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyMsgRemoveStrategy", &info, 0);
	return GUINT_TO_POINTER (type);
}

GType
tny_msg_remove_strategy_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_msg_remove_strategy_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
