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
 * TnyConnectionPolicy:
 * 
 * A connection policy. For example to attempt to reconnect or to fail
 * in case of connectivity problems.
 *
 * free-function: g_object_unref
 */

#include <config.h>

#include <tny-account.h>
#include <tny-folder.h>
#include <tny-connection-policy.h>
#include <tny-list.h>


/**
 * tny_connection_policy_set_current:
 * @self: a #TnyConnectionPolicy
 * @account: a #TnyAccount
 * @folder: a #TnyFolder
 *
 * Set the current situation of @self (in case @self wants to restore the
 * situation when @account reconnected).
 *
 * Tinymail's internal administration will call this function whenever
 * tny_folder_refresh or tny_folder_refresh_async get called. It's highly
 * recommended to use g_object_weak_ref when setting the current and 
 * g_object_weak_unref when unsetting the current or when finalising @self.
 *
 * since: 1.0
 * audience: platform-developer, application-developer, type-implementer
 **/
void 
tny_connection_policy_set_current (TnyConnectionPolicy *self, TnyAccount *account, TnyFolder *folder)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_CONNECTION_POLICY (self));
	g_assert (TNY_IS_ACCOUNT (account));
	g_assert (TNY_IS_FOLDER (folder));

	g_assert (TNY_CONNECTION_POLICY_GET_IFACE (self)->set_current!= NULL);
#endif

	TNY_CONNECTION_POLICY_GET_IFACE (self)->set_current(self, account, folder);

	return;
}

/**
 * tny_connection_policy_on_connect:
 * @self: A #TnyConnectionPolicy
 * @account: a #TnyAccount
 *
 * Called when @account got connected.
 *
 * since: 1.0
 * audience: platform-developer, application-developer, type-implementer
 **/
void 
tny_connection_policy_on_connect (TnyConnectionPolicy *self, TnyAccount *account)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_CONNECTION_POLICY (self));
	g_assert (TNY_IS_ACCOUNT (account));

	g_assert (TNY_CONNECTION_POLICY_GET_IFACE (self)->on_connect!= NULL);
#endif

	TNY_CONNECTION_POLICY_GET_IFACE (self)->on_connect(self, account);

	return;
}

/**
 * tny_connection_policy_on_disconnect:
 * @self: a #TnyConnectionPolicy
 * @account: a #TnyAccount
 *
 * Called when @account got disconnected.
 *
 * since: 1.0
 * audience: platform-developer, application-developer, type-implementer
 **/
void 
tny_connection_policy_on_disconnect (TnyConnectionPolicy *self, TnyAccount *account)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_CONNECTION_POLICY (self));
	g_assert (TNY_IS_ACCOUNT (account));

	g_assert (TNY_CONNECTION_POLICY_GET_IFACE (self)->on_disconnect!= NULL);
#endif

	TNY_CONNECTION_POLICY_GET_IFACE (self)->on_disconnect(self, account);

	return;
}

/**
 * tny_connection_policy_on_connection_broken:
 * @self: A #TnyConnectionPolicy
 * @account: a #TnyAccount
 *
 * Called when @account's connection broke. For example when a connection
 * timeout occurred.
 *
 * since: 1.0
 * audience: platform-developer, application-developer, type-implementer
 **/
void 
tny_connection_policy_on_connection_broken (TnyConnectionPolicy *self, TnyAccount *account)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_CONNECTION_POLICY (self));
	g_assert (TNY_IS_ACCOUNT (account));

	g_assert (TNY_CONNECTION_POLICY_GET_IFACE (self)->on_connection_broken!= NULL);
#endif

	TNY_CONNECTION_POLICY_GET_IFACE (self)->on_connection_broken(self, account);

	return;
}


static void
tny_connection_policy_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_connection_policy_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyConnectionPolicyIface),
			tny_connection_policy_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,    /* instance_init */
			NULL
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyConnectionPolicy", &info, 0);
	return GSIZE_TO_POINTER (type);
}

GType
tny_connection_policy_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	g_once (&once, tny_connection_policy_register_type, NULL);

	return GPOINTER_TO_SIZE (once.retval);
}
