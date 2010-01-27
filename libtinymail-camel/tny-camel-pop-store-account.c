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
#include <glib/gi18n-lib.h>
#include <glib.h>
#include <string.h>

#include <tny-list.h>
#include <tny-account.h>
#include <tny-store-account.h>
#include <tny-camel-store-account.h>
#include <tny-camel-pop-store-account.h>

#include <tny-folder.h>
#include <tny-folder-store.h>
#include <tny-camel-folder.h>

#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-store.h>
#include <camel/camel-service.h>

#include <camel/providers/pop3/camel-pop3-store.h>

#ifndef CAMEL_FOLDER_TYPE_SENT
#define CAMEL_FOLDER_TYPE_SENT (5 << 10)
#endif

#include <tny-folder.h>
#include <tny-status.h>
#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include "tny-camel-account-priv.h"
#include "tny-camel-store-account-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-common-priv.h"
#include "tny-camel-pop-store-account-priv.h"
#include "tny-camel-pop-folder-priv.h"

#include <tny-camel-shared.h>
#include <tny-account-store.h>
#include <tny-error.h>

static GObjectClass *parent_class = NULL;



static void 
tny_camel_pop_store_account_remove_folder (TnyFolderStore *self, TnyFolder *folder, GError **err)
{
	g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNSUPPORTED,
			"You can't use the tny_folder_store_remove_folder API "
			"on POP accounts. This problem indicates a bug in the "
			"software.");

	return;
}

static TnyFolder* 
tny_camel_pop_store_account_create_folder (TnyFolderStore *self, const gchar *name, GError **err)
{
	g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNSUPPORTED,
			"You can't use the tny_folder_store_create_folder "
			"API on POP accounts. This problem indicates a "
			"bug in the software.");

	return NULL;
}


static void 
notify_factory_del (TnyCamelStoreAccount *self, GObject *folder)
{
	if (self && TNY_IS_CAMEL_STORE_ACCOUNT (self)) {
		TnyCamelPopStoreAccountPriv *priv = TNY_CAMEL_POP_STORE_ACCOUNT_GET_PRIVATE (self);

		if (((GObject *) priv->inbox) == folder) {
			priv->inbox = NULL;
		}
	}
}

static TnyFolder * 
tny_camel_pop_store_account_factor_folder (TnyCamelStoreAccount *self, const gchar *full_name, gboolean *was_new)
{
	TnyCamelStoreAccountPriv *priv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (self);
	TnyCamelPopStoreAccountPriv *ppriv = TNY_CAMEL_POP_STORE_ACCOUNT_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->factory_lock);

	if (!ppriv->inbox) 
	{
		*was_new = TRUE;
		ppriv->inbox = TNY_FOLDER (_tny_camel_pop_folder_new ());
		_tny_camel_store_account_add_to_managed_folders (self, ppriv->inbox);
		g_object_weak_ref (G_OBJECT (ppriv->inbox), (GWeakNotify) notify_factory_del, self);
	} else {
		g_object_ref (ppriv->inbox);
	}

	g_static_rec_mutex_unlock (priv->factory_lock);

	return (TnyFolder *) ppriv->inbox;
}

/**
 * tny_camel_pop_store_account_new:
 * 
 * Create a new POP #TnyStoreAccount instance implemented for Camel
 *
 * Return value: A new POP #TnyStoreAccount instance implemented for Camel
 **/
TnyStoreAccount*
tny_camel_pop_store_account_new (void)
{
	TnyCamelPOPStoreAccount *self = g_object_new (TNY_TYPE_CAMEL_POP_STORE_ACCOUNT, NULL);

	return TNY_STORE_ACCOUNT (self);
}

static void
tny_camel_pop_store_account_finalize (GObject *object)
{
	TnyCamelPopStoreAccountPriv *priv = TNY_CAMEL_POP_STORE_ACCOUNT_GET_PRIVATE (object);

	g_mutex_free (priv->lock);

	(*parent_class->finalize) (object);

	return;
}

/**
 * tny_camel_pop_store_account_reconnect:
 * @self: a #TnyCamelPOPStoreAccount instance
 * 
 * Reconnect to the POP3 service. The reason why this API exists is because
 * certain services (like GMail in 2007) suddenly give you more messages in the
 * LIST result of POP after you disconnected and reconnect.
 **/
void 
tny_camel_pop_store_account_reconnect (TnyCamelPOPStoreAccount *self)
{
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	TnyCamelPopStoreAccountPriv *priv = TNY_CAMEL_POP_STORE_ACCOUNT_GET_PRIVATE (self);
	CamelService *service = (CamelService *) _tny_camel_account_get_service (TNY_CAMEL_ACCOUNT (self));

	g_mutex_lock (priv->lock);

	service->reconnecting = TRUE;

	if (service->reconnecter)
		service->reconnecter (service, FALSE, service->data);

	camel_service_disconnect ((CamelService *) service, TRUE, &ex);
	if (camel_exception_is_set (&ex))
		camel_exception_clear (&ex);
	camel_service_connect ((CamelService *) service, &ex);

	if (camel_exception_is_set (&ex))
	{
		camel_exception_clear (&ex);
		sleep (1);
		camel_service_connect (service, &ex);
	}

	if (service->reconnection)
	{
		if (!camel_exception_is_set (&ex))
			service->reconnection (service, TRUE, service->data);
		else
			service->reconnection (service, FALSE, service->data);
	}

	service->reconnecting = FALSE;

	g_mutex_unlock (priv->lock);

	return;
}

static void 
tny_camel_pop_store_account_class_init (TnyCamelPOPStoreAccountClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	TNY_CAMEL_STORE_ACCOUNT_CLASS (class)->remove_folder= tny_camel_pop_store_account_remove_folder;
	TNY_CAMEL_STORE_ACCOUNT_CLASS (class)->create_folder= tny_camel_pop_store_account_create_folder;

	/* Protected override */
	TNY_CAMEL_STORE_ACCOUNT_CLASS (class)->factor_folder= tny_camel_pop_store_account_factor_folder;

	object_class->finalize = tny_camel_pop_store_account_finalize;

	g_type_class_add_private (object_class, sizeof (TnyCamelPopStoreAccountPriv));

	return;
}


/**
 * tny_camel_pop_store_account_set_leave_messages_on_server:
 * @self: a TnyCamelPOPStoreAccount
 * @enabled: whether to leave messages on the server
 *
 * Set whether messages should be left on the server. The initialization value
 * of @enabled is TRUE (so by default, messages are left on the server).
 **/
void 
tny_camel_pop_store_account_set_leave_messages_on_server (TnyCamelPOPStoreAccount *self, gboolean enabled)
{
	const CamelService *service = _tny_camel_account_get_service (TNY_CAMEL_ACCOUNT (self));
	CamelPOP3Store *pop3_store = (CamelPOP3Store *) service;

	pop3_store->immediate_delete_after = !enabled;
}


static void
tny_camel_pop_store_account_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelPopStoreAccountPriv *priv = TNY_CAMEL_POP_STORE_ACCOUNT_GET_PRIVATE (instance);

	priv->inbox = NULL;
	priv->lock = g_mutex_new ();

	return;
}


static gpointer 
tny_camel_pop_store_account_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelPOPStoreAccountClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_pop_store_account_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelPOPStoreAccount),
			0,      /* n_preallocs */
			tny_camel_pop_store_account_instance_init    /* instance_init */
		};
	
	type = g_type_register_static (TNY_TYPE_CAMEL_STORE_ACCOUNT,
				       "TnyCamelPOPStoreAccount",
				       &info, 0);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_camel_pop_store_account_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_pop_store_account_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_pop_store_account_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
