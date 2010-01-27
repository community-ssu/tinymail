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
 * TnyMsgReceiveStrategy:
 *
 * A strategy for receiving messages
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-msg-receive-strategy.h>
#include <tny-folder.h>
#include <tny-header.h>
#include <tny-msg.h>

/* Possible future API changes:
 * tny_msg_receive_strategy_perform_get_msg will get a status callback handler.
 * Also take a look at the possible API changes for TnyFolder's get_msg_async 
 * as this would affect that API too. */

/**
 * tny_msg_receive_strategy_perform_get_msg:
 * @self: A #TnyMsgReceiveStrategy
 * @folder: a #TnyFolder from which the message will be received
 * @header: a #TnyHeader of the message that must be received
 * @err: (null-ok): A #GError or NULL
 *
 * Performs the receiving of a message from @folder. If not NULL, the returned
 * value must be unreferenced after use.
 *
 * returns: (null-ok) (caller-owns): the received message or NULL
 * since: 1.0
 * audience: application-developer
 **/
TnyMsg *
tny_msg_receive_strategy_perform_get_msg (TnyMsgReceiveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err)
{
	TnyMsg *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MSG_RECEIVE_STRATEGY (self));
	g_assert (folder);
	g_assert (TNY_IS_FOLDER (folder));
	g_assert (header);
	g_assert (TNY_IS_HEADER (header));
	g_assert (TNY_MSG_RECEIVE_STRATEGY_GET_IFACE (self)->perform_get_msg!= NULL);
#endif

	retval = TNY_MSG_RECEIVE_STRATEGY_GET_IFACE (self)->perform_get_msg(self, folder, header, err);

#ifdef DBC /* ensure */
	if (retval)
		g_assert (TNY_IS_MSG (retval));
#endif

	return retval;
}

static void
tny_msg_receive_strategy_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) 
		initialized = TRUE;
}

static gpointer
tny_msg_receive_strategy_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyMsgReceiveStrategyIface),
			tny_msg_receive_strategy_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL    /* instance_init */
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyMsgReceiveStrategy", &info, 0);
	return GUINT_TO_POINTER (type);
}

GType
tny_msg_receive_strategy_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_msg_receive_strategy_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
