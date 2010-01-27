/* libtinymail-camel - The Tiny Mail base library for Camel
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

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <string.h>

#include <tny-transport-account.h>
#include <tny-camel-transport-account.h>

#include <tny-folder.h>
#include <tny-camel-folder.h>
#include <tny-error.h>

#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-store.h>


static GObjectClass *parent_class = NULL;

#include <tny-camel-msg.h>
#include <tny-camel-transport-account.h>
#include <tny-status.h>
#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include "tny-camel-common-priv.h"
#include "tny-camel-msg-priv.h"
#include "tny-camel-header-priv.h"
#include "tny-camel-account-priv.h"
#include "tny-camel-transport-account-priv.h"

#include <tny-camel-shared.h>

#define TNY_CAMEL_TRANSPORT_ACCOUNT_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_CAMEL_TRANSPORT_ACCOUNT, TnyCamelTransportAccountPriv))

static void 
tny_camel_transport_account_prepare (TnyCamelAccount *self, gboolean recon_if, gboolean reservice)
{
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	_tny_camel_account_refresh (self, recon_if);

	g_static_rec_mutex_lock (apriv->service_lock);

	if (apriv->session && reservice && apriv->url_string)
	{
		if (!apriv->service && reservice && apriv->url_string)
		{
			if (apriv->service && CAMEL_IS_SERVICE (apriv->service))
			{
				camel_object_unref (CAMEL_OBJECT (apriv->service));
				apriv->service = NULL;
			} 

			if (camel_exception_is_set (apriv->ex))
				camel_exception_clear (apriv->ex);

			apriv->service = camel_session_get_service
				((CamelSession*) apriv->session, apriv->url_string, 
				apriv->type, apriv->ex);

			if (apriv->service && !camel_exception_is_set (apriv->ex)) 
			{
				apriv->service->data = self;
				apriv->service->connecting = (con_op) NULL;
				apriv->service->disconnecting = (con_op) NULL;
				apriv->service->reconnecter = (con_op) NULL;
				apriv->service->reconnection = (con_op) NULL;

			} else if (camel_exception_is_set (apriv->ex) && apriv->service)
			{
				g_warning ("Must cleanup service pointer\n");
				apriv->service = NULL;
			}
		}
	} else {
		if (reservice && apriv->url_string)
			camel_exception_set (apriv->ex, CAMEL_EXCEPTION_SYSTEM,
				_("Session not yet set, use tny_camel_account_set_session"));
	}

	g_static_rec_mutex_unlock (apriv->service_lock);

	return;
}

static void 
tny_camel_transport_account_try_connect (TnyAccount *self, GError **err)
{
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	if (!apriv->url_string || !apriv->service || !CAMEL_IS_SERVICE (apriv->service))
	{
		if (camel_exception_is_set (apriv->ex))
		{
			_tny_camel_exception_to_tny_error (apriv->ex, err);
			camel_exception_clear (apriv->ex);
		} else {
			g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_CONNECT,
				_("Account not yet fully configured. "
				"This problem indicates a bug in the software."));
		}

		return;
	}

	if (apriv->pass_func_set && apriv->forget_pass_func_set)
	{
		if (camel_exception_is_set (apriv->ex))
			camel_exception_clear (apriv->ex);

	} else {
			g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_CONNECT,
				_("Get and Forget password functions not yet set "
				"This problem indicates a bug in the software."));
	}

}


static void
tny_camel_transport_account_send (TnyTransportAccount *self, TnyMsg *msg, GError **err)
{
	TNY_CAMEL_TRANSPORT_ACCOUNT_GET_CLASS (self)->send(self, msg, err);
	return;
}


static char *normal_recs[] = {
	CAMEL_RECIPIENT_TYPE_TO,
	CAMEL_RECIPIENT_TYPE_CC,
	CAMEL_RECIPIENT_TYPE_BCC
};

static char *resent_recs[] = {
	CAMEL_RECIPIENT_TYPE_RESENT_TO,
	CAMEL_RECIPIENT_TYPE_RESENT_CC,
	CAMEL_RECIPIENT_TYPE_RESENT_BCC
};

static void
tny_camel_transport_account_send_default (TnyTransportAccount *self, TnyMsg *msg, GError **err)
{
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	CamelMimeMessage *message;
	CamelException ex =  CAMEL_EXCEPTION_INITIALISER;
	CamelTransport *transport;
	CamelAddress *from, *recipients;
	gboolean reperr = TRUE, suc = FALSE;
	const CamelInternetAddress *miaddr;
	const char *resentfrom; int i = 0;

	g_assert (CAMEL_IS_SESSION (apriv->session));
	g_assert (TNY_IS_CAMEL_MSG (msg));

	if (apriv->service == NULL || !CAMEL_IS_SERVICE (apriv->service)) {
		_tny_camel_exception_to_tny_error (apriv->ex, err);
		camel_exception_clear (apriv->ex);

		return;
	}

	transport = (CamelTransport *) apriv->service;
	g_assert (CAMEL_IS_TRANSPORT (transport));

	/* TODO: Why not simply use tny_account_try_connect here ? */

	g_static_rec_mutex_lock (apriv->service_lock);
	/* camel_service_connect can launch GUI things */

	apriv->service->data = self;

	if (!apriv->service || !camel_service_connect (apriv->service, &ex))
	{
		if (camel_exception_is_set (&ex)) {
			_tny_camel_exception_to_tny_error (&ex, err);
			camel_exception_clear (&ex);
		} else {
			g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_AUTHENTICATE,
				_("Authentication error"));
		}
		g_static_rec_mutex_unlock (apriv->service_lock);
		return;
	} 

	message = _tny_camel_msg_get_camel_mime_message (TNY_CAMEL_MSG (msg));

	from = (CamelAddress *) camel_internet_address_new ();
	resentfrom = camel_medium_get_header (CAMEL_MEDIUM (message), "Resent-From");

	if (resentfrom) {
		camel_address_decode (from, resentfrom);
	} else {
		miaddr = camel_mime_message_get_from (message);
		if (miaddr)
			camel_address_copy (from, CAMEL_ADDRESS (miaddr));
	}

	recipients = (CamelAddress *) camel_internet_address_new ();
	for (i = 0; i < 3; i++) {
		const char *mtype;
		mtype = resentfrom ? resent_recs[i] : normal_recs[i];
		miaddr = camel_mime_message_get_recipients (message, mtype);
		camel_address_cat (recipients, CAMEL_ADDRESS (miaddr));
	}

	if (camel_address_length(recipients) > 0) {
		suc = camel_transport_send_to (transport, message, from, 
			recipients, &ex);
	} else 
		suc = TRUE;

	if (camel_exception_is_set (&ex) || !suc)
	{
		if (camel_exception_is_set (&ex))
		{
			_tny_camel_exception_to_tny_error (&ex, err);
			camel_exception_clear (&ex);
		} else 
			g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_SEND,
				_("Unknown error while sending message"));

		reperr = FALSE;
	} else 
		camel_mime_message_set_date(message, CAMEL_MESSAGE_DATE_CURRENT, 0);

	camel_service_disconnect (apriv->service, TRUE, &ex);

	g_static_rec_mutex_unlock (apriv->service_lock);

	camel_object_unref (recipients);
	camel_object_unref (from);

	if (reperr && camel_exception_is_set (&ex)) {
		_tny_camel_exception_to_tny_error (&ex, err);
		camel_exception_clear (&ex);
	}

	return;
}

/**
 * tny_camel_transport_account_get_from:
 * @self: a #TnyCamelTransportAccount
 * 
 * Get the from of @self. Returned value must not be freed.
 * 
 * returns: (null-ok): a read-only string
 **/
const gchar *
tny_camel_transport_account_get_from (TnyCamelTransportAccount *self)
{
	TnyCamelTransportAccountPriv *priv = TNY_CAMEL_TRANSPORT_ACCOUNT_GET_PRIVATE (self);
	return priv->from;
}

/**
 * tny_camel_transport_account_set_from:
 * @self: a #TnyCamelTransportAccount
 * @from: (null-ok): a string or NULL
 * 
 * Set the from of @self.
 **/
void
tny_camel_transport_account_set_from (TnyCamelTransportAccount *self, const gchar *from)
{
	TnyCamelTransportAccountPriv *priv = TNY_CAMEL_TRANSPORT_ACCOUNT_GET_PRIVATE (self);
	if (priv->from)
		g_free (priv->from);
	priv->from = g_strdup (from);
}

/**
 * tny_camel_transport_account_new:
 * 
 * Create a new #TnyTransportAccount instance implemented for Camel
 * 
 * returns: (caller-owns): A new #TnyTransportAccount instance implemented for Camel
 **/
TnyTransportAccount*
tny_camel_transport_account_new (void)
{
	TnyCamelTransportAccount *self = g_object_new (TNY_TYPE_CAMEL_TRANSPORT_ACCOUNT, NULL);

	return TNY_TRANSPORT_ACCOUNT (self);
}

static void
tny_camel_transport_account_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelTransportAccount *self = (TnyCamelTransportAccount *)instance;
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	TnyCamelTransportAccountPriv *priv = TNY_CAMEL_TRANSPORT_ACCOUNT_GET_PRIVATE (self);

	priv->from = NULL;
	apriv->service = NULL;
	apriv->type = CAMEL_PROVIDER_TRANSPORT;
	apriv->account_type = TNY_ACCOUNT_TYPE_TRANSPORT;

	if (apriv->mech) /* Remove the PLAIN default */
		g_free (apriv->mech);
	apriv->mech = NULL;

	return;
}


static void
tny_camel_transport_account_finalize (GObject *object)
{
	TnyCamelTransportAccountPriv *priv = TNY_CAMEL_TRANSPORT_ACCOUNT_GET_PRIVATE (object);

	if (priv->from)
		g_free (priv->from);

	(*parent_class->finalize) (object);

	return;
}

static void
tny_transport_account_init (gpointer g, gpointer iface_data)
{
	TnyTransportAccountIface *klass = (TnyTransportAccountIface *)g;

	klass->send= tny_camel_transport_account_send;

	return;
}


static void 
tny_camel_transport_account_class_init (TnyCamelTransportAccountClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->send= tny_camel_transport_account_send_default;

	object_class->finalize = tny_camel_transport_account_finalize;

	TNY_CAMEL_ACCOUNT_CLASS (class)->prepare= tny_camel_transport_account_prepare;
	TNY_CAMEL_ACCOUNT_CLASS (class)->try_connect= tny_camel_transport_account_try_connect;

	g_type_class_add_private (object_class, sizeof (TnyCamelTransportAccountPriv));

	return;
}

static gpointer
tny_camel_transport_account_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelTransportAccountClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_transport_account_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelTransportAccount),
			0,      /* n_preallocs */
			tny_camel_transport_account_instance_init    /* instance_init */
		};
	
	static const GInterfaceInfo tny_transport_account_info = 
		{
			(GInterfaceInitFunc) tny_transport_account_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (TNY_TYPE_CAMEL_ACCOUNT,
				       "TnyCamelTransportAccount",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_TRANSPORT_ACCOUNT, 
				     &tny_transport_account_info);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_camel_transport_account_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_transport_account_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_transport_account_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
