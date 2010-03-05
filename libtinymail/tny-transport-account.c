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
 * TnyTransportAccount:
 *
 * A account to send E-mails with
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-transport-account.h>
#include <tny-msg.h>


/**
 * tny_transport_account_send:
 * @self: a #TnyTransportAccount
 * @msg: a #TnyMsg
 * @err: (null-ok): a #GError or NULL
 *
 * Send @msg. Note that @msg must be a correct #TnyMsg instance with a correct
 * #TnyHeader, which will be used as the envelope while sending.
 * 
 * since: 1.0
 * audience: application-developer
 **/
void
tny_transport_account_send (TnyTransportAccount *self, TnyMsg *msg, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_TRANSPORT_ACCOUNT (self));
	g_assert (msg);
	g_assert (TNY_IS_MSG (msg));
	g_assert (TNY_TRANSPORT_ACCOUNT_GET_IFACE (self)->send != NULL);
#endif

	TNY_TRANSPORT_ACCOUNT_GET_IFACE (self)->send(self, msg, err);

	return;
}


static void
tny_transport_account_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_transport_account_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyTransportAccountIface),
			tny_transport_account_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,   /* instance_init */
		  NULL
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyTransportAccount", &info, 0);
	
	g_type_interface_add_prerequisite (type, TNY_TYPE_ACCOUNT);

	return GSIZE_TO_POINTER (type);
}

GType
tny_transport_account_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_transport_account_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
