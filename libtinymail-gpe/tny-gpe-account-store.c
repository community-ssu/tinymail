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

#include <sys/mman.h>

#include <config.h>
#include <glib/gi18n-lib.h>

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include <tny-platform-factory.h>
#include <tny-password-getter.h>
#include <tny-gpe-platform-factory.h>
#include <tny-account-store.h>
#include <tny-gpe-account-store.h>
#include <tny-gtk-password-dialog.h>

#include <tny-account.h>
#include <tny-store-account.h>
#include <tny-transport-account.h>
#include <tny-device.h>

#include <tny-camel-account.h>
#include <tny-camel-store-account.h>
#include <tny-camel-transport-account.h>
#include <tny-session-camel.h>
#include <tny-gpe-device.h>

#include <tny-gtk-lockable.h>

/* "GConf vs. Camel" account implementation */

static GObjectClass *parent_class = NULL;

typedef struct _TnyGpeAccountStorePriv TnyGpeAccountStorePriv;

struct _TnyGpeAccountStorePriv
{
	GConfClient *client;
	gchar *cache_dir;
	TnySessionCamel *session;
	TnyDevice *device;
	guint notify;
	GList *accounts;
};

#define TNY_GPE_ACCOUNT_STORE_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_GPE_ACCOUNT_STORE, TnyGpeAccountStorePriv))


static gchar* 
per_account_get_pass(TnyAccount *account, const gchar *prompt, gboolean *cancel)
{
	TnyPlatformFactory *platfact = tny_gpe_platform_factory_get_instance ();
	TnyPasswordGetter *pwdgetter;
	gchar *retval;

	pwdgetter = tny_platform_factory_new_password_getter (platfact);
	retval = (gchar*) tny_password_getter_get_password (pwdgetter, 
		tny_account_get_id (account), prompt, cancel);
	g_object_unref (G_OBJECT (pwdgetter));

	return retval;
}


static void
per_account_forget_pass(TnyAccount *account)
{
	TnyPlatformFactory *platfact = tny_gpe_platform_factory_get_instance ();
	TnyPasswordGetter *pwdgetter;

	pwdgetter = tny_platform_factory_new_password_getter (platfact);
	tny_password_getter_forget_password (pwdgetter, tny_account_get_id (account));
	g_object_unref (G_OBJECT (pwdgetter));

	return;
}

static gboolean
tny_gpe_account_store_alert (TnyAccountStore *self, TnyAccount *account, TnyAlertType type, gboolean question, const GError *error)
{
	GtkMessageType gtktype;
	gboolean retval = FALSE;
	GtkWidget *dialog;

	switch (type)
	{
		case TNY_ALERT_TYPE_INFO:
		gtktype = GTK_MESSAGE_INFO;
		break;
		case TNY_ALERT_TYPE_WARNING:
		gtktype = GTK_MESSAGE_WARNING;
		break;
		case TNY_ALERT_TYPE_ERROR:
		default:
		gtktype = GTK_MESSAGE_ERROR;
		break;
	}

	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
		gtktype, GTK_BUTTONS_YES_NO, error->message);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
		retval = TRUE;

	gtk_widget_destroy (dialog);

	return retval;
}


static void
kill_stored_accounts (TnyGpeAccountStorePriv *priv)
{
	if (priv->accounts)
	{
		g_list_foreach (priv->accounts, (GFunc) g_object_unref, NULL);
		g_list_free (priv->accounts);
		priv->accounts = NULL;
	}

	return;
}

static void
gconf_listener_account_changed (GConfClient *client, guint cnxn_id,
			GConfEntry *entry, gpointer user_data)
{
	TnyAccountStore *self = user_data;
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);

	gchar *key = g_strdup (entry->key);
	gchar *ptr = strrchr (key, '/'); ptr++;

	if (!strcmp (ptr, "count"))
		kill_stored_accounts (priv);

	g_free (key);

	return;
}

static void
load_accounts (TnyAccountStore *self)
{
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);

	gint i=0, count, port;

	count = gconf_client_get_int (priv->client, 
		"/apps/tinymail/accounts/count", NULL);

	for (i=0; i < count; i++)
	{
		gchar *proto, *type, *key, *name, *mech;
		TnyAccount *account = NULL;
		GSList *options;

		key = g_strdup_printf ("/apps/tinymail/accounts/%d", i);
		
		if (!gconf_client_dir_exists (priv->client, (const gchar*)key, NULL))
		{
			g_free (key);
			continue;
		}
		g_free (key);

		key = g_strdup_printf ("/apps/tinymail/accounts/%d/disabled", i);
		if (gconf_client_get_bool (priv->client, (const gchar*) key, NULL))
		{
			g_free (key);
			continue;
		}
		g_free (key);

		key = g_strdup_printf ("/apps/tinymail/accounts/%d/type", i);
		type = gconf_client_get_string (priv->client, 
			(const gchar*) key, NULL);
		g_free (key);

		key = g_strdup_printf ("/apps/tinymail/accounts/%d/proto", i);
		proto = gconf_client_get_string (priv->client, 
			(const gchar*) key, NULL);
		g_free (key);

		key = g_strdup_printf ("/apps/tinymail/accounts/%d/mech", i);
		mech = gconf_client_get_string (priv->client, 
			(const gchar*) key, NULL);
		g_free (key);

		if (!g_ascii_strncasecmp (proto, "smtp", 4))
			account = TNY_ACCOUNT (tny_camel_transport_account_new ());
		else if (!g_ascii_strncasecmp (proto, "imap", 4))
			account = TNY_ACCOUNT (tny_camel_imap_store_account_new ());
		else if (!g_ascii_strncasecmp (proto, "nntp", 4))
			account = TNY_ACCOUNT (tny_camel_nntp_store_account_new ());
		else if (!g_ascii_strncasecmp (proto, "pop", 3))
			account = TNY_ACCOUNT (tny_camel_pop_store_account_new ());
		else	/* Unknown, create a generic one? */
			account = TNY_ACCOUNT (tny_camel_store_account_new ());

		if (type)
			g_free (type);

		if (account)
		{
			tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (account), priv->session);
			tny_account_set_proto (TNY_ACCOUNT (account), proto);

			key = g_strdup_printf ("/apps/tinymail/accounts/%d/name", i);
			name = gconf_client_get_string (priv->client, 
				(const gchar*) key, NULL);
			g_free (key);
		
			if (name)
			{
				tny_account_set_name (TNY_ACCOUNT (account), name);
				g_free (name);
			}

			if (mech)
				tny_account_set_secure_auth_mech (TNY_ACCOUNT (account), mech);


			key = g_strdup_printf ("/apps/tinymail/accounts/%d/options", i);
			options = gconf_client_get_list (priv->client, 
				(const gchar*) key, GCONF_VALUE_STRING, NULL);
			g_free (key);

			if (options) {
				while (options) {
					gchar *key = options->data;
					gchar *value = strchr (options->data, '=');

					if (value) {
						*value = '\0';
						value++;
					} else
						value = "";

					tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (account), 
						tny_pair_new (key, value));
					g_free (options->data);
					options = g_slist_next (options);
				}
				g_slist_free (options);
			}

			if (!g_ascii_strncasecmp (proto, "pop", 3) ||
				!g_ascii_strncasecmp (proto, "imap", 4))
			{
				gchar *user, *hostname;

				key = g_strdup_printf ("/apps/tinymail/accounts/%d/user", i);
				user = gconf_client_get_string (priv->client, 
					(const gchar*) key, NULL);

				g_free (key);
				tny_account_set_user (TNY_ACCOUNT (account), user);

				key = g_strdup_printf ("/apps/tinymail/accounts/%d/hostname", i);
				hostname = gconf_client_get_string (priv->client, 
					(const gchar*) key, NULL);
				g_free (key); 
				tny_account_set_hostname (TNY_ACCOUNT (account), 
					hostname);

				key = g_strdup_printf ("/apps/tinymail/accounts/%d/port", i);
				port = gconf_client_get_int (priv->client, 
					(const gchar*) key, NULL);
				g_free (key); 
				if (port != 0) tny_account_set_port (TNY_ACCOUNT (account), 
					port);

				g_free (hostname); g_free (user);
			} else {
				gchar *url_string;

				/* Un officially supported provider */
				/* Assuming there's a url_string in this case */

				key = g_strdup_printf ("/apps/tinymail/accounts/%d/url_string", i);
				url_string = gconf_client_get_string (priv->client, 
					(const gchar*) key, NULL);

				g_free (key);
				tny_account_set_url_string (TNY_ACCOUNT (account), url_string);
				g_free (url_string);
			}

			key = g_strdup_printf ("/apps/tinymail/accounts/%d", i);
			tny_account_set_id (TNY_ACCOUNT (account), key);
			g_free (key);

			tny_account_set_forget_pass_func (TNY_ACCOUNT (account),
				per_account_forget_pass);

			tny_account_set_pass_func (TNY_ACCOUNT (account),
				per_account_get_pass);

			priv->accounts = g_list_prepend (priv->accounts, account);
		}

		if (proto)
			g_free (proto);

		if (mech)
			g_free (mech);
	}

	tny_session_camel_set_initialized (priv->session);

}

static TnyAccount* 
tny_gpe_account_store_find_account (TnyAccountStore *self, const gchar *url_string)
{
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);
	TnyAccount *found = NULL;

	if (!priv->accounts)
		load_accounts (self);

	if (priv->accounts) {
		GList *copy = priv->accounts;
		while (copy) {
			TnyAccount *account = copy->data;
			if (tny_account_matches_url_string (account, url_string)) {
				found = TNY_ACCOUNT (g_object_ref (G_OBJECT (account)));
				break;
			}
			copy = g_list_next (copy);
		}
	}

	return found;
}


static const gchar*
tny_gpe_account_store_get_cache_dir (TnyAccountStore *self)
{
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);

	if (!priv->cache_dir) {
		gchar *cache_dir = gconf_client_get_string (priv->client, 
			"/apps/tinymail/cache_dir", NULL);
		priv->cache_dir = g_build_filename (g_get_home_dir (), 
			cache_dir, NULL);
		g_free (cache_dir);
	}

	return priv->cache_dir;
}


static void
tny_gpe_account_store_get_accounts (TnyAccountStore *self, TnyList *list, TnyGetAccountsRequestType types)
{
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);

	g_assert (TNY_IS_LIST (list));

	if (!priv->accounts)
		load_accounts (self);

	if (priv->accounts)
	{
		GList *copy = priv->accounts;
		while (copy) {
			TnyAccount *account = copy->data;
			if (types == TNY_ACCOUNT_STORE_BOTH || types == TNY_ACCOUNT_STORE_STORE_ACCOUNTS) {
				if (TNY_IS_STORE_ACCOUNT (account))
					tny_list_prepend (list, (GObject*)account);
			} 
			if (types == TNY_ACCOUNT_STORE_BOTH || types == TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS) {
				if (TNY_IS_TRANSPORT_ACCOUNT (account))
					tny_list_prepend (list, (GObject*)account);
			}
			copy = g_list_next (copy);
		}
	}

	return;
}


static void
tny_gpe_account_store_notify_add (TnyAccountStore *self)
{
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);
	priv->notify = gconf_client_notify_add (priv->client, 
		"/apps/tinymail/accounts", gconf_listener_account_changed,
		self, NULL, NULL);
	return;
}

static void
tny_gpe_account_store_notify_remove (TnyAccountStore *self)
{
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);
	gconf_client_notify_remove (priv->client, priv->notify);
	return;
}

/*
	gconftool-2 -s /apps/tinymail/cache_dir -t string .tinymail

	gconftool-2 -s /apps/tinymail/accounts/count -t int COUNT
	gconftool-2 -s /apps/tinymail/accounts/0/proto -t string [smtp|imap|pop]
	gconftool-2 -s /apps/tinymail/accounts/0/type -t string [transport|store]

	gconftool-2 -s /apps/tinymail/accounts/0/user -t string username
	gconftool-2 -s /apps/tinymail/accounts/0/hostname -t string mailserver
or
	gconftool-2 -s /apps/tinymail/accounts/0/url_string -t string url_string

*/


static TnyDevice*
tny_gpe_account_store_get_device (TnyAccountStore *self)
{
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);

	return g_object_ref (G_OBJECT (priv->device));
}

/**
 * tny_gpe_account_store_new:
 *
 *
 * Return value: A new #TnyAccountStore instance implemented for GPE
 **/
TnyAccountStore*
tny_gpe_account_store_new (void)
{
	TnyGpeAccountStore *self = g_object_new (TNY_TYPE_GPE_ACCOUNT_STORE, NULL);
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);
	priv->session = tny_session_camel_new (TNY_ACCOUNT_STORE (self));

	tny_session_camel_set_ui_locker (priv->session, tny_gtk_lockable_new ());

	return TNY_ACCOUNT_STORE (self);
}


static void
tny_gpe_account_store_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGpeAccountStore *self = (TnyGpeAccountStore *)instance;
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);
	TnyPlatformFactory *platfact;

	priv->cache_dir = NULL;
	priv->accounts = NULL;
	priv->client = gconf_client_get_default ();

	gconf_client_add_dir (priv->client, "/apps/tinymail", 
		GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

	tny_gpe_account_store_notify_add (TNY_ACCOUNT_STORE (self));

	platfact = tny_gpe_platform_factory_get_instance ();

	priv->device = tny_platform_factory_new_device (platfact);

	return;
}


static void
tny_gpe_account_store_finalize (GObject *object)
{
	TnyGpeAccountStore *self = (TnyGpeAccountStore *) object;
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);

	tny_gpe_account_store_notify_remove (TNY_ACCOUNT_STORE (self));
	g_object_unref (G_OBJECT (priv->client));

	kill_stored_accounts (priv);

	if (priv->cache_dir)
		g_free (priv->cache_dir);

	(*parent_class->finalize) (object);

	return;
}


/**
 * tny_gpe_account_store_get_session:
 * @self: The #TnyGpeAccountStore instance
 *
 * Return value: A #TnySessionCamel instance
 **/
TnySessionCamel*
tny_gpe_account_store_get_session (TnyGpeAccountStore *self)
{
	TnyGpeAccountStorePriv *priv = TNY_GPE_ACCOUNT_STORE_GET_PRIVATE (self);

	return priv->session;
}

static void 
tny_gpe_account_store_class_init (TnyGpeAccountStoreClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_gpe_account_store_finalize;

	g_type_class_add_private (object_class, sizeof (TnyGpeAccountStorePriv));

	return;
}

static void
tny_account_store_init (gpointer g, gpointer iface_data)
{
	TnyAccountStoreIface *klass = (TnyAccountStoreIface *)g;

	klass->get_accounts= tny_gpe_account_store_get_accounts;
	klass->get_cache_dir= tny_gpe_account_store_get_cache_dir;
	klass->get_device= tny_gpe_account_store_get_device;
	klass->alert= tny_gpe_account_store_alert;
	klass->find_account= tny_gpe_account_store_find_account;

	return;
}


static gpointer 
tny_gpe_account_store_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyGpeAccountStoreClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_gpe_account_store_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyGpeAccountStore),
			0,      /* n_preallocs */
			tny_gpe_account_store_instance_init    /* instance_init */
		};
	
	static const GInterfaceInfo tny_account_store_info = 
		{
			(GInterfaceInitFunc) tny_account_store_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyGpeAccountStore",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_ACCOUNT_STORE, 
				     &tny_account_store_info);

	return GUINT_TO_POINTER (type);
}

GType 
tny_gpe_account_store_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gpe_account_store_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
