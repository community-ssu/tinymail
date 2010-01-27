/* Your copyright here */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-folder.h>

#include <tny-camel-recover-connection-policy.h>
#include <tny-camel-account.h>

static GObjectClass *parent_class = NULL;


typedef struct _TnyCamelRecoverConnectionPolicyPriv TnyCamelRecoverConnectionPolicyPriv;

struct _TnyCamelRecoverConnectionPolicyPriv
{
	TnyFolder *folder;
	gboolean recover, recov_folder;
	gint recon_delay;
};

#define TNY_CAMEL_RECOVER_CONNECTION_POLICY_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_CAMEL_RECOVER_CONNECTION_POLICY, TnyCamelRecoverConnectionPolicyPriv))


static void
tny_camel_recover_connection_policy_on_connect (TnyConnectionPolicy *self, TnyAccount *account)
{
	TnyCamelRecoverConnectionPolicyPriv *priv = TNY_CAMEL_RECOVER_CONNECTION_POLICY_GET_PRIVATE (self);

	if (priv->folder && priv->recover && priv->recov_folder)
		tny_folder_refresh_async (priv->folder, NULL, NULL, NULL);

	priv->recover = FALSE;

	return;
}

static gboolean 
reconnect_it (gpointer user_data)
{
	TnyCamelAccount *account = (TnyCamelAccount *) user_data;
	tny_camel_account_set_online (account, TRUE, NULL, NULL);

	return FALSE;
}

static void 
reconnect_destroy (gpointer user_data)
{
	g_object_unref (user_data);
}

/**
 * tny_camel_recover_connection_policy_set_recover_active_folder:
 * @self: a #TnyCamelRecoverConnectionPolicy instance
 * @setting: whether to recover the active folder
 * 
 * Sets whether to recover the active folder. Recovering the active folder means
 * that the folder will be refreshed once the account is back online, and if
 * the folder supports IDLE (in case of IMAP), its IDLE state is turned on.
 * Default value is TRUE.
 **/
void 
tny_camel_recover_connection_policy_set_recover_active_folder (TnyCamelRecoverConnectionPolicy *self, gboolean setting)
{
	TnyCamelRecoverConnectionPolicyPriv *priv = TNY_CAMEL_RECOVER_CONNECTION_POLICY_GET_PRIVATE (self);
	priv->recov_folder = setting;
}


/**
 * tny_camel_recover_connection_policy_set_reconnect_delay:
 * @self: a #TnyCamelRecoverConnectionPolicy instance
 * @milliseconds: delay before a reconnect attempt happens, use -1 to disable
 * 
 * Sets the amount of milliseconds before a reconnect attempt takes place. Use
 * -1 to disable reconnecting. Default value is -1 or disabled.
 **/
void 
tny_camel_recover_connection_policy_set_reconnect_delay (TnyCamelRecoverConnectionPolicy *self, gint milliseconds)
{
	TnyCamelRecoverConnectionPolicyPriv *priv = TNY_CAMEL_RECOVER_CONNECTION_POLICY_GET_PRIVATE (self);
	priv->recon_delay = milliseconds;
}

static void
tny_camel_recover_connection_policy_on_connection_broken (TnyConnectionPolicy *self, TnyAccount *account)
{
	TnyCamelRecoverConnectionPolicyPriv *priv = TNY_CAMEL_RECOVER_CONNECTION_POLICY_GET_PRIVATE (self);

	priv->recover = TRUE;

	if (priv->recon_delay > 0) {
		g_timeout_add_full (G_PRIORITY_HIGH, priv->recon_delay, reconnect_it, 
			g_object_ref (account), reconnect_destroy);
	}

}

static void
tny_camel_recover_connection_policy_on_disconnect (TnyConnectionPolicy *self, TnyAccount *account)
{
	return;
}

static void 
notify_folder_del (gpointer user_data, GObject *account)
{
	TnyCamelRecoverConnectionPolicyPriv *priv = TNY_CAMEL_RECOVER_CONNECTION_POLICY_GET_PRIVATE (user_data);
	priv->folder = NULL;
}

static void
tny_camel_recover_connection_policy_set_current (TnyConnectionPolicy *self, TnyAccount *account, TnyFolder *folder)
{
	TnyCamelRecoverConnectionPolicyPriv *priv = TNY_CAMEL_RECOVER_CONNECTION_POLICY_GET_PRIVATE (self);
	if (priv->folder)
		g_object_weak_unref (G_OBJECT (priv->folder), notify_folder_del, self);
	priv->folder = folder;
	g_object_weak_ref (G_OBJECT (priv->folder), notify_folder_del, self);
}

static void
tny_camel_recover_connection_policy_finalize (GObject *object)
{
	TnyCamelRecoverConnectionPolicyPriv *priv = TNY_CAMEL_RECOVER_CONNECTION_POLICY_GET_PRIVATE (object);

	if (priv->folder)
		g_object_weak_unref (G_OBJECT (priv->folder), notify_folder_del, object);

	parent_class->finalize (object);
}

static void
tny_camel_recover_connection_policy_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelRecoverConnectionPolicyPriv *priv = TNY_CAMEL_RECOVER_CONNECTION_POLICY_GET_PRIVATE (instance);
	priv->folder = NULL;
	priv->recover = FALSE;
	priv->recon_delay = -1;
	priv->recov_folder = TRUE;
}

static void
tny_connection_policy_init (TnyConnectionPolicyIface *klass)
{
	klass->on_connect= tny_camel_recover_connection_policy_on_connect;
	klass->on_connection_broken= tny_camel_recover_connection_policy_on_connection_broken;
	klass->on_disconnect= tny_camel_recover_connection_policy_on_disconnect;
	klass->set_current= tny_camel_recover_connection_policy_set_current;
}

static void
tny_camel_recover_connection_policy_class_init (TnyCamelRecoverConnectionPolicyClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = tny_camel_recover_connection_policy_finalize;

	g_type_class_add_private (object_class, sizeof (TnyCamelRecoverConnectionPolicyPriv));

}

/**
 * tny_camel_recover_connection_policy_new:
 * 
 * A connection policy that tries to camel_recover the connection and the currently
 * selected folder.
 *
 * Return value: A new #TnyConnectionPolicy instance 
 **/
TnyConnectionPolicy*
tny_camel_recover_connection_policy_new (void)
{
	return TNY_CONNECTION_POLICY (g_object_new (TNY_TYPE_CAMEL_RECOVER_CONNECTION_POLICY, NULL));
}


static gpointer
tny_camel_recover_connection_policy_register_type (gpointer notused)
{
	GType type = 0;
	static const GTypeInfo info = 
		{
			sizeof (TnyCamelRecoverConnectionPolicyClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_recover_connection_policy_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelRecoverConnectionPolicy),
			0,      /* n_preallocs */
			tny_camel_recover_connection_policy_instance_init,    /* instance_init */
			NULL
		};


	static const GInterfaceInfo tny_connection_policy_info = 
		{
			(GInterfaceInitFunc) tny_connection_policy_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelRecoverConnectionPolicy",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_CONNECTION_POLICY,
				     &tny_connection_policy_info);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_camel_recover_connection_policy_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_camel_recover_connection_policy_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
