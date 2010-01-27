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
 * TnyAccountStore:
 * 
 * A account store, holding a list of accounts
 *
 * free-function: g_object_unref
 */

#include <config.h>

#ifdef DBC
#include <string.h>
#endif

#include <tny-account.h>
#include <tny-transport-account.h>
#include <tny-store-account.h>
#include <tny-device.h>
#include <tny-list.h>

#include <tny-account-store.h>
guint tny_account_store_signals [TNY_ACCOUNT_STORE_LAST_SIGNAL];


/**
 * tny_account_store_find_account:
 * @self: a #TnyAccountStore
 * @url_string: the url-string of the account to find
 *
 * Try to find the first account in account store @self that corresponds to 
 * @url_string. If found, the returned value must be unreferenced.
 *
 * returns: (null-ok) (caller-owns): the found account or NULL
 * since: 1.0
 * audience: application-developer
 * complexity: high
 **/
TnyAccount* 
tny_account_store_find_account (TnyAccountStore *self, const gchar *url_string)
{
	TnyAccount *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_ACCOUNT_STORE (self));
	g_assert (url_string);
	g_assert (strlen (url_string) > 0);
	g_assert (strstr (url_string, ":"));
	g_assert (TNY_ACCOUNT_STORE_GET_IFACE (self)->find_account!= NULL);
#endif

	retval = TNY_ACCOUNT_STORE_GET_IFACE (self)->find_account(self, url_string);

#ifdef DBC /* ensure */
	if (retval) {
		g_assert (TNY_IS_ACCOUNT (retval));
		g_assert (tny_account_matches_url_string (retval, url_string));
	}
#endif

	return retval;
}

/**
 * tny_account_store_alert:
 * @self: a #TnyAccountStore
 * @account: (null-ok): The account or NULL
 * @type: alert type
 * @question: whether or not this is a question
 * @error: (null-ok): A #GError with the alert
 *
 * This callback method must implements showing a message dialog appropriate 
 * for the @error and @type as message type. It will return TRUE if the reply 
 * was affirmative or FALSE if not.
 *
 * The two possible answers that must be supported are "Yes" and "No" which must
 * result in a TRUE or a FALSE return value, though your implementation should
 * attempt to use explicit button names such as "Accept Certificate" and 
 * "Reject Certificate". Likewise, the dialog should be arranged according to
 * the user interface guidelines of your target platform.
 *
 * Although there is a @question parameter, there is not always certainty about
 * whether or not the warning actually is a question. It's save to say, however
 * that in case @question is FALSE, that the return value of the function's 
 * implementation is not considered. In case of TRUE, it usually is.
 *
 * Example implementation for GTK+:
 * <informalexample><programlisting>
 * static gboolean
 * tny_gnome_account_store_alert (TnyAccountStore *self, TnyAccount *account, TnyAlertType type, const GError *error)
 * {
 *     GtkMessageType gtktype;
 *     GtkWidget *dialog;
 *     switch (type)
 *     {
 *         case TNY_ALERT_TYPE_INFO:
 *         gtktype = GTK_MESSAGE_INFO;
 *         break;
 *         case TNY_ALERT_TYPE_WARNING:
 *         gtktype = GTK_MESSAGE_WARNING;
 *         break;
 *         case TNY_ALERT_TYPE_ERROR:
 *         default:
 *         gtktype = GTK_MESSAGE_ERROR;
 *         break;
 *     }
 *     const gchar *prompt = NULL;
 *     switch (error->code)
 *     {
 *         case TNY_ACCOUNT_ERROR_TRY_CONNECT:
 *             prompt = _("Account not yet fully configured");
 *             break;
 *         default:
 *             g_warning ("%s: Unhandled GError code.", __FUNCTION__);
 *             prompt = NULL;
 *             break;
 *      }
 *      if (!prompt)
 *        return FALSE;
 *     gboolean retval = FALSE;
 *     dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
 *            gtktype, GTK_BUTTONS_YES_NO, prompt);
 *     if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
 *            retval = TRUE;
 *     gtk_widget_destroy (dialog);
 *     return retval;
 * }
 * </programlisting></informalexample>
 *
 * returns: Affirmative = TRUE,  Negative = FALSE
 * since: 1.0
 * audience: platform-developer, application-developer, type-implementer
 **/
gboolean 
tny_account_store_alert (TnyAccountStore *self, TnyAccount *account, TnyAlertType type, gboolean question, GError *error)
{
	gboolean retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_ACCOUNT_STORE (self));
	g_assert (TNY_ACCOUNT_STORE_GET_IFACE (self)->alert!= NULL);
#endif

	retval = TNY_ACCOUNT_STORE_GET_IFACE (self)->alert(self, account, type, question, error);

#ifdef DBC /* ensure */
#endif

	return retval;
}

/**
 * tny_account_store_get_device:
 * @self: a #TnyAccountStore
 *
 * This method returns the #TnyDevice instance used by #TnyAccountStore @self. You
 * must unreference the return value after use.
 *
 * returns: (caller-owns): the device used by @self
 * since: 1.0
 * audience: application-developer
 **/
TnyDevice* 
tny_account_store_get_device (TnyAccountStore *self)
{
	TnyDevice *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_ACCOUNT_STORE (self));
	g_assert (TNY_ACCOUNT_STORE_GET_IFACE (self)->get_device!= NULL);
#endif

	retval = TNY_ACCOUNT_STORE_GET_IFACE (self)->get_device(self);

#ifdef DBC /* ensure */
	g_assert (retval);
	g_assert (TNY_IS_DEVICE (retval));
#endif

	return retval;
}

/**
 * tny_account_store_get_cache_dir:
 * @self: a #TnyAccountStore
 * 
 * Returns the path that will be used for storing cache. Note that the callers
 * of this method will not free up the result. The implementation of 
 * #TnyAccountStore @self is responsible for freeing up. 
 *
 * returns: cache's path
 * since: 1.0
 * audience: platform-developer, type-implementer
 **/
const gchar*
tny_account_store_get_cache_dir (TnyAccountStore *self)
{
	const gchar *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_ACCOUNT_STORE (self));
	g_assert (TNY_ACCOUNT_STORE_GET_IFACE (self)->get_cache_dir!= NULL);
#endif

	retval = TNY_ACCOUNT_STORE_GET_IFACE (self)->get_cache_dir(self);

#ifdef DBC /* ensure */
	g_assert (retval);
	g_assert (strlen (retval) > 0);
#endif

	return retval;
}


/**
 * tny_account_store_get_accounts:
 * @self: a #TnyAccountStore
 * @list: a #TnyList that will be filled with #TnyAccount instances
 * @types: a #TnyGetAccountsRequestType that indicates which account types are wanted
 * 
 * Get a list of accounts in the account store @self.
 *
 * Example:
 * <informalexample><programlisting>
 * TnyList *list = tny_simple_list_new ();
 * tny_account_store_get_accounts (astore, list, TNY_ACCOUNT_STORE_STORE_ACCOUNTS);
 * TnyIterator *iter = tny_list_create_iterator (list);
 * while (!tny_iterator_is_done (iter))
 * {
 *    TnyStoreAccount *cur = TNY_STORE_ACCOUNT (tny_iterator_get_current (iter));
 *    printf ("%s\n", tny_store_account_get_name (cur));
 *    g_object_unref (G_OBJECT (cur));
 *    tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * g_object_unref (list);
 * </programlisting></informalexample>
 *
 * When implementing this API it is allowed but not required to cache the list.
 * If you are implementing an account store for #TnyCamelAccount account 
 * instances, register the created accounts with a #TnySessionCamel instance 
 * using the API tny_session_camel_set_current_accounts right after creating
 * the accounts for the first time.
 *
 * If you use the #TnySessionCamel to register accounts, register each account
 * using tny_camel_account_set_session and after registering your last account
 * use the API tny_session_camel_set_initialized
 *
 * The method must fill up @list with available accounts. Note that if
 * you cache the list, you must add a reference to each account added to the
 * list.
 *
 * There are multiple samples of #TnyAccountStore implementations in
 * libtinymail-gnome-desktop, libtinymail-olpc, libtinymail-gpe, 
 * libtinymail-maemo and tests/shared/account-store.c which is being used by
 * the unit tests and the normal tests.
 *
 * The get_pass and forget_pass functionality of the example below is usually
 * implemented by utilizing a #TnyPasswordGetter that is returned by the 
 * #TnyPlatformFactory.
 *
 * Example (that uses a cache):
 * <informalexample><programlisting>
 * static TnyCamelSession *session = NULL;
 * static TnyList *accounts = NULL;
 * static gchar* 
 * account_get_pass(TnyAccount *account, const gchar *prompt, gboolean *cancel)
 * {
 *      return g_strdup ("the password");
 * }
 * static void
 * account_forget_pass(TnyAccount *account)
 * {
 *      g_print ("Password was incorrect\n");
 * }
 * static void
 * tny_my_account_store_get_accounts (TnyAccountStore *self, TnyList *list, TnyGetAccountsRequestType types)
 * {
 *    TnyIterator *iter;
 *    if (session == NULL)
 *        session = tny_session_camel_new (TNY_ACCOUNT_STORE (self));
 *    if (accounts == NULL)
 *    {
 *        accounts = tny_simple_list_new ();
 *        for (... each account ... )
 *        {
 *           TnyAccount *account = TNY_ACCOUNT (tny_camel_store_account_new ());
 *           tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (account), session);
 *           tny_account_set_proto (account, "imap");
 *           tny_account_set_name (account, "account i");
 *           tny_account_set_user (account, "user of account i");
 *           tny_account_set_hostname (account, "server.domain of account i");
 *           tny_account_set_id (account, "i");
 *           tny_account_set_forget_pass_func (account, account_forget_pass);
 *           tny_account_set_pass_func (account, account_get_pass);
 *           tny_list_prepend (accounts, account);
 *           g_object_unref (G_OBJECT (account));
 *        }
 *    }
 *    iter = tny_list_create_iterator (accounts);
 *    while (tny_iterator_is_done (iter))
 *    {
 *        GObject *cur = tny_iterator_get_current (iter);
 *        tny_list_prepend (list, cur);
 *        g_object_unref (cur);
 *        tny_iterator_next (iter);
 *    }
 *    g_object_unref (iter);
 *    tny_session_camel_set_initialized (session);
 * }
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: platform-developer, application-developer, type-implementer
 **/
void
tny_account_store_get_accounts (TnyAccountStore *self, TnyList *list, TnyGetAccountsRequestType types)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_ACCOUNT_STORE (self));
	g_assert (list);
	g_assert (TNY_IS_LIST (list));
	g_assert (TNY_ACCOUNT_STORE_GET_IFACE (self)->get_accounts!= NULL);
#endif

	TNY_ACCOUNT_STORE_GET_IFACE (self)->get_accounts(self, list, types);

#ifdef DBC /* ensure */
#endif

	return;
}


static void
tny_account_store_base_init (gpointer g_class)
{
	static gboolean tny_account_store_initialized = FALSE;

	if (!tny_account_store_initialized) 
	{
/**
 * TnyAccountStore::connecting-started
 * @self: the object on which the signal is emitted
 * @user_data: (null-ok): user data set when the signal handler was connected.
 *
 * Emitted when the store starts trying to connect the accounts. Usage of this
 * signal is not recommended as it's a candidate for API changes.
 *
 * since: 1.0
 */
	tny_account_store_signals[TNY_ACCOUNT_STORE_CONNECTING_STARTED] =
	   g_signal_new ("connecting_started",
		TNY_TYPE_ACCOUNT_STORE,
		G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (TnyAccountStoreIface, connecting_started),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID, 
		G_TYPE_NONE, 0);

		tny_account_store_initialized = TRUE;
	}
	return;
}

gpointer
tny_account_store_register_type (gpointer notused)
{	
	GType type = 0;
	static const GTypeInfo info = 
		{
			sizeof (TnyAccountStoreIface),
			tny_account_store_base_init,   /* base_init */
			NULL,   /*    base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,    /* instance_init */
			NULL
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyAccountStore", &info, 0);

	g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_account_store_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 * since: 1.0
 **/
GType
tny_account_store_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	g_once (&once, tny_account_store_register_type, NULL);

	return GPOINTER_TO_UINT (once.retval);
}

static gpointer
tny_alert_type_register_type (gpointer notused)
{
  GType etype = 0;
  static const GEnumValue values[] = {
	  { TNY_ALERT_TYPE_INFO, "TNY_ALERT_TYPE_INFO", "info" },
	  { TNY_ALERT_TYPE_WARNING, "TNY_ALERT_TYPE_WARNING", "warning" },
	  { TNY_ALERT_TYPE_ERROR, "TNY_ALERT_TYPE_ERROR", "error" },
	  { 0, NULL, NULL }
  };

  etype = g_enum_register_static ("TnyAlertType", values);

  return GUINT_TO_POINTER (etype);
}

/**
 * tny_alert_type_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 * since: 1.0
 **/
GType
tny_alert_type_get_type (void)
{
  static GOnce once = G_ONCE_INIT;
  
  g_once (&once, tny_alert_type_register_type, NULL);

  return GPOINTER_TO_UINT (once.retval);;
}

static gpointer
tny_get_accounts_request_type_register_type (gpointer notused)
{
  GType etype = 0;
  static const GEnumValue values[] = {
    { TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS, "TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS", "transport" },
    { TNY_ACCOUNT_STORE_STORE_ACCOUNTS, "TNY_ACCOUNT_STORE_STORE_ACCOUNTS", "store" },
    { TNY_ACCOUNT_STORE_BOTH, "TNY_ACCOUNT_STORE_BOTH", "both" },
    { 0, NULL, NULL }
  };

  etype = g_enum_register_static ("TnyGetAccountsRequestType", values);
  return GUINT_TO_POINTER (etype);
}

/**
 * tny_get_accounts_request_type_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 * since: 1.0
 **/
GType
tny_get_accounts_request_type_get_type (void)
{
  static GOnce once = G_ONCE_INIT;
  g_once (&once, tny_get_accounts_request_type_register_type, NULL);
  return GPOINTER_TO_UINT (once.retval);
}

static gpointer
tny_account_store_signal_register_type (gpointer notused)
{
  GType etype = 0;
  static const GEnumValue values[] = {
    { TNY_ACCOUNT_STORE_CONNECTING_STARTED, "TNY_ACCOUNT_STORE_CONNECTING_STARTED", "started" },
    { TNY_ACCOUNT_STORE_LAST_SIGNAL,  "TNY_ACCOUNT_STORE_LAST_SIGNAL", "last-signal" },
    { 0, NULL, NULL }
  };
  etype = g_enum_register_static ("TnyAccountStoreSignal", values);
  return GUINT_TO_POINTER (etype);
}

/**
 * tny_account_store_signal_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 * since: 1.0
 **/
GType
tny_account_store_signal_get_type (void)
{
  static GOnce once = G_ONCE_INIT;

  g_once (&once, tny_account_store_signal_register_type, NULL);

  return GPOINTER_TO_UINT (once.retval);
}
