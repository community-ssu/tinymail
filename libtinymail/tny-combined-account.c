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
 * TnyCombinedAccount:
 * 
 * A combined account
 *
 * free-function: g_object_unref
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-combined-account.h>


static GObjectClass *parent_class = NULL;

#define TNY_COMBINED_ACCOUNT_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_COMBINED_ACCOUNT, TnyCombinedAccountPriv))


typedef struct _TnyCombinedAccountPriv TnyCombinedAccountPriv;

struct _TnyCombinedAccountPriv
{
	TnyTransportAccount *transport_account;
	TnyStoreAccount *store_account;
	gchar *tid;
	guint subs_changed_signal;
};


static void
tny_combined_account_subscribe (TnyStoreAccount *self, TnyFolder *folder)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_store_account_subscribe (TNY_STORE_ACCOUNT (priv->store_account), folder);
}

static void
tny_combined_account_unsubscribe (TnyStoreAccount *self, TnyFolder *folder)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_store_account_unsubscribe (TNY_STORE_ACCOUNT (priv->store_account), folder);
}

static TnyFolder *
tny_combined_account_find_folder (TnyStoreAccount *self, const gchar *url_string, GError **err)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	return tny_store_account_find_folder (TNY_STORE_ACCOUNT (priv->store_account), url_string, err);
}

static void
tny_combined_account_send (TnyTransportAccount *self, TnyMsg *msg, GError **err)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_transport_account_send (TNY_TRANSPORT_ACCOUNT (priv->transport_account), msg, err);
}

static TnyConnectionStatus
tny_combined_account_get_connection_status (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	g_warning ("Don't use tny_account_is_connected on TnyCombinedAccount");

	return tny_account_get_connection_status (TNY_ACCOUNT (priv->store_account));
}

static void
tny_combined_account_set_id (TnyAccount *self, const gchar *id)
{
	g_warning ("Don't use tny_account_set_id on TnyCombinedAccount");
}

static void
tny_combined_account_set_name (TnyAccount *self, const gchar *name)
{
	g_warning ("Don't use tny_account_set_name on TnyCombinedAccount");
}

static void
tny_combined_account_set_secure_auth_mech (TnyAccount *self, const gchar *mech)
{
	g_warning ("Don't use tny_account_set_secure_auth_mech on TnyCombinedAccount");
}

static void
tny_combined_account_set_proto (TnyAccount *self, const gchar *proto)
{
	g_warning ("Don't use tny_account_set_proto on TnyCombinedAccount");
}

static void
tny_combined_account_set_user (TnyAccount *self, const gchar *user)
{
	g_warning ("Don't use tny_account_set_user on TnyCombinedAccount");
}

static void
tny_combined_account_set_hostname (TnyAccount *self, const gchar *host)
{
	g_warning ("Don't use tny_account_set_hostname on TnyCombinedAccount");
}

static void
tny_combined_account_set_port (TnyAccount *self, guint port)
{
	g_warning ("Don't use tny_account_set_port on TnyCombinedAccount");
}

static void
tny_combined_account_set_url_string (TnyAccount *self, const gchar *url_string)
{
	g_warning ("Don't use tny_account_set_url_string on TnyCombinedAccount");
}

static void
tny_combined_account_set_pass_func (TnyAccount *self, TnyGetPassFunc get_pass_func)
{
	g_warning ("Don't use tny_account_set_pass_func on TnyCombinedAccount");
}

static void
tny_combined_account_set_forget_pass_func (TnyAccount *self, TnyForgetPassFunc get_forget_pass_func)
{
	g_warning ("Don't use tny_account_set_forget_pass_func on TnyCombinedAccount");
}

static TnyGetPassFunc
tny_combined_account_get_pass_func (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	g_warning ("Don't use tny_account_get_pass_func on TnyCombinedAccount");

	return tny_account_get_pass_func (TNY_ACCOUNT (priv->store_account));
}

static TnyForgetPassFunc
tny_combined_account_get_forget_pass_func (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	g_warning ("Don't use tny_account_get_forget_pass_func on TnyCombinedAccount");

	return tny_account_get_forget_pass_func (TNY_ACCOUNT (priv->store_account));
}

static const gchar*
tny_combined_account_get_id (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	if (priv->tid == NULL)
		priv->tid = g_strdup_printf ("%s%s", 
			tny_account_get_id (TNY_ACCOUNT (priv->store_account)),
			tny_account_get_id (TNY_ACCOUNT (priv->transport_account)));

	return priv->tid;
}

static const gchar*
tny_combined_account_get_name (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	g_warning ("Don't use tny_account_get_name on TnyCombinedAccount");

	return tny_account_get_name (TNY_ACCOUNT (priv->store_account));
}

static const gchar*
tny_combined_account_get_secure_auth_mech (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	g_warning ("Don't use tny_account_get_secure_auth_mech on TnyCombinedAccount");

	return tny_account_get_secure_auth_mech (TNY_ACCOUNT (priv->store_account));
}

static const gchar*
tny_combined_account_get_proto (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	g_warning ("Don't use tny_account_get_proto on TnyCombinedAccount");

	return tny_account_get_proto (TNY_ACCOUNT (priv->store_account));
}

static const gchar*
tny_combined_account_get_user (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	g_warning ("Don't use tny_account_get_user on TnyCombinedAccount");

	return tny_account_get_user (TNY_ACCOUNT (priv->store_account));
}

static const gchar*
tny_combined_account_get_hostname (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	g_warning ("Don't use tny_account_get_hostname on TnyCombinedAccount");

	return tny_account_get_hostname (TNY_ACCOUNT (priv->store_account));
}

static guint
tny_combined_account_get_port (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	g_warning ("Don't use tny_account_get_port on TnyCombinedAccount");

	return tny_account_get_port (TNY_ACCOUNT (priv->store_account));
}

static gchar*
tny_combined_account_get_url_string (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	g_warning ("Don't use tny_account_get_url_string on TnyCombinedAccount");

	return tny_account_get_url_string (TNY_ACCOUNT (priv->store_account));
}

static TnyAccountType
tny_combined_account_get_account_type (TnyAccount *self)
{
	return TNY_ACCOUNT_TYPE_COMBINED;
}

static void
tny_combined_account_cancel (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_account_cancel (TNY_ACCOUNT (priv->store_account));
	tny_account_cancel (TNY_ACCOUNT (priv->transport_account));
}

static gboolean
tny_combined_account_matches_url_string (TnyAccount *self, const gchar *url_string)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);
	gboolean retval = FALSE;

	retval = tny_account_matches_url_string (TNY_ACCOUNT (priv->store_account), url_string);
	if (!retval)
		retval = tny_account_matches_url_string (TNY_ACCOUNT (priv->transport_account), url_string);

	return retval;
}

static void
tny_combined_account_remove_folder (TnyFolderStore *self, TnyFolder *folder, GError **err)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_folder_store_remove_folder (TNY_FOLDER_STORE (priv->store_account), folder, err);
}

static TnyFolder*
tny_combined_account_create_folder (TnyFolderStore *self, const gchar *name, GError **err)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	return tny_folder_store_create_folder (TNY_FOLDER_STORE (priv->store_account), name, err);
}

static void
tny_combined_account_get_folders (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, GError **err)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_folder_store_get_folders (TNY_FOLDER_STORE (priv->store_account), list, query, refresh, err);
}

static void
tny_combined_account_get_folders_async (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, TnyGetFoldersCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_folder_store_get_folders_async (TNY_FOLDER_STORE (priv->store_account), list, query, refresh, callback, status_callback, user_data);
}

static void
tny_combined_account_refresh_async (TnyFolderStore *self, TnyFolderStoreCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_folder_store_refresh_async (TNY_FOLDER_STORE (priv->store_account), callback, status_callback, user_data);
}

static void
tny_combined_account_add_observer (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_folder_store_add_observer (TNY_FOLDER_STORE (priv->store_account), observer);
}

static void
tny_combined_account_remove_observer (TnyFolderStore *self, TnyFolderStoreObserver *observer)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_folder_store_remove_observer (TNY_FOLDER_STORE (priv->store_account), observer);
}

static void 
tny_combined_account_start_operation (TnyAccount *self, TnyStatusDomain domain, TnyStatusCode code, TnyStatusCallback status_callback, gpointer status_user_data)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_account_start_operation (TNY_ACCOUNT (priv->store_account), domain, code, status_callback, status_user_data);

	return;
}

static void 
tny_combined_account_stop_operation (TnyAccount *self, gboolean *canceled)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	tny_account_stop_operation (TNY_ACCOUNT (priv->store_account), canceled);

	return;
}

static gboolean 
tny_combined_account_is_ready (TnyAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);
	gboolean retval = FALSE;

	if (tny_account_is_ready (TNY_ACCOUNT (priv->store_account)) &&
	    tny_account_is_ready (TNY_ACCOUNT (priv->transport_account))) {
		retval = TRUE;
	}
	return retval;
}


static void
on_subscription_changed_signal (TnyStoreAccount *sa, TnyFolder *folder, gpointer user_data)
{
	GObject *self = user_data;

	/* TNY TODO: tny_lockable_lock (ui_lock); */
	g_signal_emit (self,  tny_store_account_signals [TNY_STORE_ACCOUNT_SUBSCRIPTION_CHANGED], 
		0, folder);
	/* TNY TODO: tny_lockable_unlock (ui_lock); */
}

/**
 * tny_combined_account_new:
 * @ta: A #TnyTransportAccount
 * @sa: a #TnyStoreAccount
 * 
 * Create a decorator for @ta, in case the tny_transport_account_send is used,
 * and for @sa in case a method of either #TnyFolderStore, #TnyAccount or 
 * #TnyStoreAccount is used.
 *
 * Note though that you must not use instances created by this constructor for
 * either setting or getting members of the #TnyAccount type. You must get the
 * actual instances to read these properties.
 *
 * returns: (caller-owns): A new account instance that decorates both @ta and @sa
 *
 * since: 1.0
 * complexity: high
 * audience: application-developer
 **/
TnyAccount *
tny_combined_account_new (TnyTransportAccount *ta, TnyStoreAccount *sa)
{
	TnyCombinedAccount *self = g_object_new (TNY_TYPE_COMBINED_ACCOUNT, NULL);
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	priv->transport_account = TNY_TRANSPORT_ACCOUNT (g_object_ref (ta));
	priv->store_account = TNY_STORE_ACCOUNT (g_object_ref (sa));

	priv->subs_changed_signal = g_signal_connect (G_OBJECT (sa), 
		"subscription_changed", 
		G_CALLBACK (on_subscription_changed_signal), self);

	return TNY_ACCOUNT (self);
}

/**
 * tny_combined_account_get_transport_account:
 * @self: a #TnyCombinedAccount
 * 
 * Get the transport account that is being decorated by @self. You must unreference
 * the returned value if not needed anymore.
 *
 * returns: (caller-owns): the transport account in @self
 * since: 1.0
 * complexity: low
 * audience: application-developer
 **/
TnyTransportAccount* 
tny_combined_account_get_transport_account (TnyCombinedAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	return TNY_TRANSPORT_ACCOUNT (g_object_ref (priv->transport_account));
}

/**
 * tny_combined_account_get_store_account:
 * @self: a #TnyCombinedAccount
 * 
 * Get the store account that is being decorated by @self. You must unreference
 * the returned value if not needed anymore.
 *
 * returns: (caller-owns): the store account in @self
 * since: 1.0
 * complexity: low
 * audience: application-developer
 **/
TnyStoreAccount* 
tny_combined_account_get_store_account (TnyCombinedAccount *self)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	return TNY_STORE_ACCOUNT (g_object_ref (priv->store_account));
}

static void
tny_combined_account_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCombinedAccount *self = (TnyCombinedAccount *)instance;
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (self);

	priv->transport_account = NULL;
	priv->store_account = NULL;
	priv->tid = NULL;

	return;
}

static void
tny_combined_account_finalize (GObject *object)
{
	TnyCombinedAccountPriv *priv = TNY_COMBINED_ACCOUNT_GET_PRIVATE (object);

	g_signal_handler_disconnect (priv->store_account,
			priv->subs_changed_signal);

	g_object_unref (priv->store_account);
	g_object_unref (priv->transport_account);

	if (priv->tid)
		g_free (priv->tid);
	priv->tid = NULL;


	parent_class->finalize (object);
}

static void
tny_store_account_init (TnyStoreAccountIface *klass)
{
	klass->subscribe= tny_combined_account_subscribe;
	klass->unsubscribe= tny_combined_account_unsubscribe;
	klass->find_folder= tny_combined_account_find_folder;
}


static void
tny_transport_account_init (TnyTransportAccountIface *klass)
{
	klass->send= tny_combined_account_send;
}


static void
tny_account_init (TnyAccountIface *klass)
{
	klass->get_connection_status= tny_combined_account_get_connection_status;
	klass->set_id= tny_combined_account_set_id;
	klass->set_name= tny_combined_account_set_name;
	klass->set_secure_auth_mech= tny_combined_account_set_secure_auth_mech;
	klass->set_proto= tny_combined_account_set_proto;
	klass->set_user= tny_combined_account_set_user;
	klass->set_hostname= tny_combined_account_set_hostname;
	klass->set_port= tny_combined_account_set_port;
	klass->set_url_string= tny_combined_account_set_url_string;
	klass->set_pass_func= tny_combined_account_set_pass_func;
	klass->set_forget_pass_func= tny_combined_account_set_forget_pass_func;
	klass->get_pass_func= tny_combined_account_get_pass_func;
	klass->get_forget_pass_func= tny_combined_account_get_forget_pass_func;
	klass->get_id= tny_combined_account_get_id;
	klass->get_name= tny_combined_account_get_name;
	klass->get_secure_auth_mech= tny_combined_account_get_secure_auth_mech;
	klass->get_proto= tny_combined_account_get_proto;
	klass->get_user= tny_combined_account_get_user;
	klass->get_hostname= tny_combined_account_get_hostname;
	klass->get_port= tny_combined_account_get_port;
	klass->get_url_string= tny_combined_account_get_url_string;
	klass->get_account_type= tny_combined_account_get_account_type;
	klass->cancel= tny_combined_account_cancel;
	klass->matches_url_string= tny_combined_account_matches_url_string;
	klass->start_operation= tny_combined_account_start_operation;
	klass->stop_operation= tny_combined_account_stop_operation;
	klass->is_ready= tny_combined_account_is_ready;
}


static void
tny_folder_store_init (TnyFolderStoreIface *klass)
{
	klass->remove_folder= tny_combined_account_remove_folder;
	klass->create_folder= tny_combined_account_create_folder;
	klass->get_folders= tny_combined_account_get_folders;
	klass->get_folders_async= tny_combined_account_get_folders_async;
	klass->add_observer= tny_combined_account_add_observer;
	klass->remove_observer= tny_combined_account_remove_observer;
	klass->refresh_async = tny_combined_account_refresh_async;
}

static void
tny_combined_account_class_init (TnyCombinedAccountClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = tny_combined_account_finalize;
	g_type_class_add_private (object_class, sizeof (TnyCombinedAccountPriv));
}

static gpointer
tny_combined_account_register_type (gpointer notused)
{
	GType type = 0;
	static const GTypeInfo info = 
		{
			sizeof (TnyCombinedAccountClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_combined_account_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCombinedAccount),
			0,      /* n_preallocs */
			tny_combined_account_instance_init,    /* instance_init */
			NULL
		};
	
	
	static const GInterfaceInfo tny_store_account_info = 
		{
			(GInterfaceInitFunc) tny_store_account_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	static const GInterfaceInfo tny_transport_account_info = 
		{
			(GInterfaceInitFunc) tny_transport_account_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	static const GInterfaceInfo tny_account_info = 
		{
			(GInterfaceInitFunc) tny_account_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	static const GInterfaceInfo tny_folder_store_info = 
		{
			(GInterfaceInitFunc) tny_folder_store_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCombinedAccount",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_ACCOUNT,
				     &tny_account_info);
	
	g_type_add_interface_static (type, TNY_TYPE_FOLDER_STORE,
				     &tny_folder_store_info);
	
	g_type_add_interface_static (type, TNY_TYPE_STORE_ACCOUNT,
				     &tny_store_account_info);
	
	g_type_add_interface_static (type, TNY_TYPE_TRANSPORT_ACCOUNT,
				     &tny_transport_account_info);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_combined_account_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	g_once (&once, tny_combined_account_register_type, NULL);

	return GPOINTER_TO_UINT (once.retval);
}
