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

#include <tny-platform-factory.h>
#include <tny-password-getter.h>
#include <tny-olpc-platform-factory.h>

#include <tny-account-store.h>
#include <tny-olpc-account-store.h>
#include <tny-account.h>
#include <tny-store-account.h>
#include <tny-transport-account.h>
#include <tny-device.h>

#include <tny-camel-account.h>
#include <tny-camel-store-account.h>
#include <tny-camel-transport-account.h>
#include <tny-session-camel.h>
#include <tny-olpc-device.h>

#include <tny-gtk-lockable.h>

/* GKeyFile vs. Camel implementation */

static GObjectClass *parent_class = NULL;

typedef struct _TnyOlpcAccountStorePriv TnyOlpcAccountStorePriv;

struct _TnyOlpcAccountStorePriv
{
	gchar *cache_dir;
	TnySessionCamel *session;
	TnyDevice *device;
	guint notify;
	GList *accounts;
};

#define TNY_OLPC_ACCOUNT_STORE_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_OLPC_ACCOUNT_STORE, TnyOlpcAccountStorePriv))



static gchar* 
per_account_get_pass(TnyAccount *account, const gchar *prompt, gboolean *cancel)
{
	TnyPlatformFactory *platfact = tny_olpc_platform_factory_get_instance ();
	TnyPasswordGetter *pwdgetter;
	gchar *retval;

	pwdgetter = tny_platform_factory_new_password_getter (platfact);
	retval = (gchar*) tny_password_getter_get_password (pwdgetter, 
		tny_account_get_id (account), prompt, cancel);
	g_object_unref (pwdgetter);

	return retval;
}


static void
per_account_forget_pass(TnyAccount *account)
{
	TnyPlatformFactory *platfact = tny_olpc_platform_factory_get_instance ();
	TnyPasswordGetter *pwdgetter;

	pwdgetter = tny_platform_factory_new_password_getter (platfact);
	tny_password_getter_forget_password (pwdgetter, tny_account_get_id (account));
	g_object_unref (pwdgetter);

	return;
}

static gboolean
tny_olpc_account_store_alert (TnyAccountStore *self, TnyAccount *account, TnyAlertType type, gboolean question, GError *error)
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
kill_stored_accounts (TnyOlpcAccountStorePriv *priv)
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
load_accounts (TnyAccountStore *self)
{
	TnyOlpcAccountStorePriv *priv = TNY_OLPC_ACCOUNT_STORE_GET_PRIVATE (self);

	const gchar *filen;
	gchar *configd;
	GDir *dir;

	configd = g_build_path (G_DIR_SEPARATOR_S, g_get_home_dir(), 
		".tinymail", "accounts", NULL);
	dir = g_dir_open (configd, 0, NULL);
	g_free (configd);

	if (!dir)
		return;

	for (filen = g_dir_read_name (dir); filen; filen = g_dir_read_name (dir))
	{
		GKeyFile *keyfile;
		gchar *proto, *type, *key, *name, *mech;
		TnyAccount *account = NULL; gint port = 0;
		gchar *fullfilen = g_build_filename (g_get_home_dir(), 
			".tinymail", "accounts", filen, NULL);
		keyfile = g_key_file_new ();

		if (!g_key_file_load_from_file (keyfile, fullfilen, G_KEY_FILE_NONE, NULL))
		{
			g_free (fullfilen);
			continue;
		}

		if (g_key_file_get_boolean (keyfile, "tinymail", "disabled", NULL))
		{
			g_free (fullfilen);
			continue;
		}

		type = g_key_file_get_value (keyfile, "tinymail", "type", NULL);
		proto = g_key_file_get_value (keyfile, "tinymail", "proto", NULL);
		mech = g_key_file_get_value (keyfile, "tinymail", "mech", NULL);

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
			gsize options_len; gint i;
			gchar **options;

			tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (account), priv->session);
			tny_account_set_proto (TNY_ACCOUNT (account), proto);

			name = g_key_file_get_value (keyfile, "tinymail", "name", NULL);

			if (name) 
			{
				tny_account_set_name (TNY_ACCOUNT (account), name);
				g_free (name);
			}

			if (mech)
				tny_account_set_secure_auth_mech (TNY_ACCOUNT (account), mech);

			options = g_key_file_get_string_list (keyfile, "tinymail", "options", &options_len, NULL);
			if (options) {
				for (i=0; i < options_len; i++) {
					gchar *key = options[i];
					gchar *value = strchr (options[i], '=');

					if (value) {
						*value = '\0';
						value++;
					} else
						value = "";

					tny_camel_account_add_option (TNY_CAMEL_ACCOUNT (account), 
						tny_pair_new (key, value));
				}
				g_strfreev (options);
			}

			if (!g_ascii_strncasecmp (proto, "pop", 3) ||
				!g_ascii_strncasecmp (proto, "imap", 4))
			{
				gchar *user, *hostname;
				GError *err = NULL;

				user = g_key_file_get_value (keyfile, "tinymail", "user", NULL);
				tny_account_set_user (TNY_ACCOUNT (account), user);

				hostname = g_key_file_get_value (keyfile, "tinymail", "hostname", NULL);
				tny_account_set_hostname (TNY_ACCOUNT (account), hostname);

				port = g_key_file_get_integer (keyfile, "tinymail", "port", &err);
				if (err == NULL)
					tny_account_set_port (TNY_ACCOUNT (account), port);

				if (err != NULL)
					g_error_free (err);

				g_free (hostname); g_free (user);
			} else {
				gchar *url_string;

				/* Un officially supported provider */
				/* Assuming there's a url_string in this case */

				url_string = g_key_file_get_value (keyfile, "tinymail", "url_string", NULL);
				tny_account_set_url_string (TNY_ACCOUNT (account), url_string);
				g_free (url_string);
			}

			tny_account_set_id (TNY_ACCOUNT (account), fullfilen);

			g_free (fullfilen);

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

		g_key_file_free (keyfile);
	}
	g_dir_close (dir);

	tny_session_camel_set_initialized (priv->session);
}

static TnyAccount* 
tny_olpc_account_store_find_account (TnyAccountStore *self, const gchar *url_string)
{
	TnyOlpcAccountStorePriv *priv = TNY_OLPC_ACCOUNT_STORE_GET_PRIVATE (self);
	TnyAccount *found = NULL;

	if (!priv->accounts)
		load_accounts (self);

	if (priv->accounts)
	{
		GList *copy = priv->accounts;
		while (copy)
		{
			TnyAccount *account = copy->data;

			if (tny_account_matches_url_string (account, url_string))
			{
				found = TNY_ACCOUNT (g_object_ref (G_OBJECT (account)));
				break;
			}

			copy = g_list_next (copy);
		}
	}

	return found;
}

static const gchar*
tny_olpc_account_store_get_cache_dir (TnyAccountStore *self)
{
	TnyOlpcAccountStorePriv *priv = TNY_OLPC_ACCOUNT_STORE_GET_PRIVATE (self);

	if (!priv->cache_dir)
		priv->cache_dir = g_build_path (G_DIR_SEPARATOR_S, g_get_home_dir(), ".tinymail", NULL);

	return priv->cache_dir;
}


static void
tny_olpc_account_store_get_accounts (TnyAccountStore *self, TnyList *list, TnyGetAccountsRequestType types)
{
	TnyOlpcAccountStorePriv *priv = TNY_OLPC_ACCOUNT_STORE_GET_PRIVATE (self);

	g_assert (TNY_IS_LIST (list));

	if (!priv->accounts)
		load_accounts (self);

	if (priv->accounts)
	{
		GList *copy = priv->accounts;
		while (copy)
		{
			TnyAccount *account = copy->data;

			if (types == TNY_ACCOUNT_STORE_BOTH || types == TNY_ACCOUNT_STORE_STORE_ACCOUNTS)
			{
				if (TNY_IS_STORE_ACCOUNT (account))
					tny_list_prepend (list, (GObject*)account);
			} 

			if (types == TNY_ACCOUNT_STORE_BOTH || types == TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS)
			{
				if (TNY_IS_TRANSPORT_ACCOUNT (account))
					tny_list_prepend (list, (GObject*)account);
			}

			copy = g_list_next (copy);
		}
	}

	return;
}


static TnyDevice*
tny_olpc_account_store_get_device (TnyAccountStore *self)
{
	TnyOlpcAccountStorePriv *priv = TNY_OLPC_ACCOUNT_STORE_GET_PRIVATE (self);

	return g_object_ref (priv->device);
}

/**
 * tny_olpc_account_store_new:
 *
 *
 * Return value: A new #TnyAccountStore implemented for OLPC
 **/
TnyAccountStore*
tny_olpc_account_store_new (void)
{
	TnyOlpcAccountStore *self = g_object_new (TNY_TYPE_OLPC_ACCOUNT_STORE, NULL);
	TnyOlpcAccountStorePriv *priv = TNY_OLPC_ACCOUNT_STORE_GET_PRIVATE (self);
	priv->session = tny_session_camel_new (TNY_ACCOUNT_STORE (self));

	tny_session_camel_set_ui_locker (priv->session, tny_gtk_lockable_new ());

	return TNY_ACCOUNT_STORE (self);
}


static void
tny_olpc_account_store_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyOlpcAccountStorePriv *priv = TNY_OLPC_ACCOUNT_STORE_GET_PRIVATE (instance);
	TnyPlatformFactory *platfact = TNY_PLATFORM_FACTORY (
		tny_olpc_platform_factory_get_instance ());

	priv->cache_dir = NULL;
	priv->accounts = NULL;
	priv->device = tny_platform_factory_new_device (platfact);

	return;
}



static void
tny_olpc_account_store_finalize (GObject *object)
{	
	TnyOlpcAccountStorePriv *priv = TNY_OLPC_ACCOUNT_STORE_GET_PRIVATE (object);

	kill_stored_accounts (priv);

	if (priv->cache_dir)
		g_free (priv->cache_dir);

	g_object_unref (priv->device);

	(*parent_class->finalize) (object);

	return;
}


/**
 * tny_olpc_account_store_get_session:
 * @self: The #TnyOlpcAccountStore instance
 *
 * Return value: A #TnySessionCamel instance
 **/
TnySessionCamel*
tny_olpc_account_store_get_session (TnyOlpcAccountStore *self)
{
	TnyOlpcAccountStorePriv *priv = TNY_OLPC_ACCOUNT_STORE_GET_PRIVATE (self);

	return priv->session;
}

static void 
tny_olpc_account_store_class_init (TnyOlpcAccountStoreClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_olpc_account_store_finalize;

	g_type_class_add_private (object_class, sizeof (TnyOlpcAccountStorePriv));

	return;
}

static void
tny_account_store_init (gpointer g, gpointer iface_data)
{
	TnyAccountStoreIface *klass = (TnyAccountStoreIface *)g;

	klass->get_accounts= tny_olpc_account_store_get_accounts;
	klass->get_cache_dir= tny_olpc_account_store_get_cache_dir;
	klass->get_device= tny_olpc_account_store_get_device;
	klass->alert= tny_olpc_account_store_alert;
	klass->find_account= tny_olpc_account_store_find_account;

	return;
}


GType 
tny_olpc_account_store_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (TnyOlpcAccountStoreClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_olpc_account_store_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyOlpcAccountStore),
		  0,      /* n_preallocs */
		  tny_olpc_account_store_instance_init    /* instance_init */
		};

		static const GInterfaceInfo tny_account_store_info = 
		{
		  (GInterfaceInitFunc) tny_account_store_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		type = g_type_register_static (G_TYPE_OBJECT,
			"TnyOlpcAccountStore",
			&info, 0);

		g_type_add_interface_static (type, TNY_TYPE_ACCOUNT_STORE, 
			&tny_account_store_info);
	}

	return type;
}
