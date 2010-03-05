/* tinymail - Tiny Mail
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
#include <string.h>
#include <glib.h>

#include <tny-password-getter.h>
#include <tny-account-store.h>
#include <tny-acap-account-store.h>

#include <tny-account.h>
#include <tny-store-account.h>
#include <tny-transport-account.h>


static GObjectClass *parent_class = NULL;

typedef struct _TnyAcapAccountStorePriv TnyAcapAccountStorePriv;

struct _TnyAcapAccountStorePriv
{
	TnyAccountStore *real;
	TnyPasswordGetter *pwdgetter;
	guint connchanged_signal;
};

#define TNY_ACAP_ACCOUNT_STORE_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_ACAP_ACCOUNT_STORE, TnyAcapAccountStorePriv))


static gboolean
tny_acap_account_store_alert (TnyAccountStore *self, TnyAccount *account, TnyAlertType type, const GError *error)
{
	TnyAcapAccountStorePriv *priv = TNY_ACAP_ACCOUNT_STORE_GET_PRIVATE (self);
	return tny_account_store_alert (priv->real, account, type, error);
}


static TnyAccount* 
tny_acap_account_store_find_account (TnyAccountStore *self, const gchar *url_string)
{
	TnyAcapAccountStorePriv *priv = TNY_ACAP_ACCOUNT_STORE_GET_PRIVATE (self);

	return tny_account_store_find_account (priv->real, url_string);
}

static const gchar*
tny_acap_account_store_get_cache_dir (TnyAccountStore *self)
{
	TnyAcapAccountStorePriv *priv = TNY_ACAP_ACCOUNT_STORE_GET_PRIVATE (self);

	return tny_account_store_get_cache_dir (priv->real);
}



static void
connection_changed (TnyDevice *device, gboolean online, gpointer user_data)
{
	TnyAcapAccountStore *self;

	/* TODO: sync journal! */
}


static void
tny_acap_account_store_get_accounts (TnyAccountStore *self, TnyList *list, TnyGetAccountsRequestType types)
{
	TnyAcapAccountStorePriv *priv = TNY_ACAP_ACCOUNT_STORE_GET_PRIVATE (self);
	TnyDevice *device = tny_account_store_get_device (priv->real);

	if (tny_device_is_online (device))
	{
		gboolean cancel = FALSE;
		const gchar *passw = tny_password_getter_get_password (priv->pwdgetter, 
			"ACAP", _("Give the password for your ACAP server"), &cancel);

		if (!cancel)
			g_print ("Using %s as password for ACAP\n", passw);

		/* TODO: if online, sync local with remote. 
		   Don't forget the journal! */
	}

	g_object_unref (G_OBJECT (device));

	tny_account_store_get_accounts (self, list, types);

	return;
}


static TnyDevice*
tny_acap_account_store_get_device (TnyAccountStore *self)
{
	TnyAcapAccountStorePriv *priv = TNY_ACAP_ACCOUNT_STORE_GET_PRIVATE (self);

	return tny_account_store_get_device (priv->real);
}

/**
 * tny_acap_account_store_new:
 *
 *
 * Return value: A new #TnyAccountStore implemented for ACAP
 **/
TnyAccountStore*
tny_acap_account_store_new (TnyAccountStore *real, TnyPasswordGetter *pwdgetter)
{
	TnyAcapAccountStore *self = g_object_new (TNY_TYPE_ACAP_ACCOUNT_STORE, NULL);
	TnyAcapAccountStorePriv *priv = TNY_ACAP_ACCOUNT_STORE_GET_PRIVATE (self);
	TnyDevice *device;

	priv->real = TNY_ACCOUNT_STORE (g_object_ref (G_OBJECT (real))); 
	priv->pwdgetter = TNY_PASSWORD_GETTER (g_object_ref (G_OBJECT (pwdgetter))); 

	device = tny_account_store_get_device (priv->real);
	priv->connchanged_signal = g_signal_connect (
			G_OBJECT (device), "connection_changed",
			G_CALLBACK (connection_changed), self);
	g_object_unref (G_OBJECT (device));

	return TNY_ACCOUNT_STORE (self);
}


static void
tny_acap_account_store_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyAcapAccountStorePriv *priv = TNY_ACAP_ACCOUNT_STORE_GET_PRIVATE (instance);

	priv->real = NULL;
	priv->pwdgetter;

	return;
}



static void
tny_acap_account_store_finalize (GObject *object)
{	
	TnyAcapAccountStorePriv *priv = TNY_ACAP_ACCOUNT_STORE_GET_PRIVATE (object);
	TnyDevice *device;

	device = tny_account_store_get_device (priv->real);
	g_signal_handler_disconnect (G_OBJECT (device), 
			priv->connchanged_signal);
	g_object_unref (G_OBJECT (device));

	if (priv->real)
		g_object_unref (G_OBJECT (priv->real));

	if (priv->pwdgetter)
		g_object_unref (G_OBJECT (priv->pwdgetter));

	(*parent_class->finalize) (object);

	return;
}


/**
 * tny_acap_account_store_get_real:
 * @self: The #TnyAcapAccountStore instance
 *
 * Get the real from proxy @self. You must unreference the return value after 
 * use.
 *
 * Return value: A #TnyAccountStore instance
 **/
TnyAccountStore*
tny_acap_account_store_get_real (TnyAcapAccountStore *self)
{
	TnyAcapAccountStorePriv *priv = TNY_ACAP_ACCOUNT_STORE_GET_PRIVATE (self);

	return TNY_ACCOUNT_STORE (g_object_ref (G_OBJECT (priv->real)));
}

static void 
tny_acap_account_store_class_init (TnyAcapAccountStoreClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_acap_account_store_finalize;

	g_type_class_add_private (object_class, sizeof (TnyAcapAccountStorePriv));

	return;
}

static void
tny_account_store_init (gpointer g, gpointer iface_data)
{
	TnyAccountStoreIface *klass = (TnyAccountStoreIface *)g;

	klass->get_accounts= tny_acap_account_store_get_accounts;
	klass->get_cache_dir= tny_acap_account_store_get_cache_dir;
	klass->get_device= tny_acap_account_store_get_device;
	klass->alert= tny_acap_account_store_alert;
	klass->find_account= tny_acap_account_store_find_account;

	return;
}


static gpointer
tny_acap_account_store_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyAcapAccountStoreClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_acap_account_store_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyAcapAccountStore),
			0,      /* n_preallocs */
			tny_acap_account_store_instance_init    /* instance_init */
		};
	
	static const GInterfaceInfo tny_account_store_info = 
		{
			(GInterfaceInitFunc) tny_account_store_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyAcapAccountStore",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_ACCOUNT_STORE, 
				     &tny_account_store_info);

	return GSIZE_TO_POINTER (type);
}

GType 
tny_acap_account_store_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_acap_account_store_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
