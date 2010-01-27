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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <tny-error.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <camel/camel.h>
#include <camel/camel-filter-driver.h>

#include <camel/camel-store.h>
#include <camel/camel.h>
#include <camel/camel-session.h>

#include <tny-session-camel.h>
#include <tny-account.h>

#include <tny-device.h>
#include <tny-account-store.h>
#include <tny-camel-store-account.h>
#include <tny-camel-transport-account.h>

#include <tny-noop-lockable.h>
#include <tny-status.h>
#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#include "tny-camel-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include "tny-session-camel-priv.h"
#include "tny-camel-store-account-priv.h"
#include "tny-camel-transport-account-priv.h"
#include "tny-camel-account-priv.h"

#include <tny-camel-shared.h>

gboolean _camel_type_init_done = FALSE;

static CamelSessionClass *ms_parent_class;

static void tny_session_camel_forget_password (CamelSession *, CamelService *, const char *, const char *, CamelException *);



/* Hey Rob, look here! This gets called when the password is needed. It can have
 * been implemented by the app developer without any Gtk+ code. Sometimes, 
 * however, it will launch Gtk+ code. For example the first time the password
 * is needed.
 *
 * This happens in a thread, so does the tny_session_camel_forget_password. The 
 * thread is always the queue thread of the account. 
 *
 * This is number #1 */


typedef struct
{
	TnySessionCamel *self;
	TnyAccount *account;
	gchar *prompt, *retval;
	gboolean cancel;
	TnyGetPassFunc func;
	guint32 flags;

	GCond* condition;
	gboolean had_callback;
	GMutex *mutex;

} HadToWaitForPasswordInfo;


static gboolean
get_password_idle_func (gpointer user_data)
{
	HadToWaitForPasswordInfo *info = (HadToWaitForPasswordInfo *) user_data;
	TnySessionCamel *self = info->self;
	TnyAccount *account = info->account;
	TnyGetPassFunc func = info->func;

	tny_lockable_lock (self->priv->ui_lock);

	info->retval = func (account, info->prompt, &info->cancel);
	tny_lockable_unlock (self->priv->ui_lock);

	return FALSE;
}

static void
get_password_destroy(gpointer user_data)
{
	HadToWaitForPasswordInfo *info = (HadToWaitForPasswordInfo *) user_data;

	g_object_unref (info->account);
	camel_object_unref (info->self);

	g_mutex_lock (info->mutex);
	g_cond_broadcast (info->condition);
	info->had_callback = TRUE;
	g_mutex_unlock (info->mutex);

	return;
}

static char *
tny_session_camel_get_password (CamelSession *session, CamelService *service, const char *domain,
		const char *prompt, const char *item, guint32 flags, CamelException *ex)
{
	TnySessionCamel *self = (TnySessionCamel *) session;
	TnyGetPassFunc func;
	TnyAccount *account;
	gboolean freeprmpt = FALSE, cancel = FALSE;
	gchar *retval = NULL, *prmpt = (gchar*) prompt;
	TnyCamelAccountPriv *apriv = NULL;

	account = service->data;
	if (account)
	{
		HadToWaitForPasswordInfo *info = NULL;

		func = tny_account_get_pass_func (account);

		if (!func)
			return g_strdup ("");

		if (prmpt == NULL)
		{
			freeprmpt = TRUE;
			prmpt = g_strdup_printf (_("Enter password for %s"), 
				tny_account_get_name (account));
		}

		apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (account);
		apriv->in_auth = TRUE;

		info = g_slice_new (HadToWaitForPasswordInfo);

		info->mutex = g_mutex_new ();
		info->condition = g_cond_new ();
		info->had_callback = FALSE;

		info->flags = flags;
		camel_object_ref (self);
		info->self = self;
		info->account = TNY_ACCOUNT (g_object_ref (account));
		info->prompt = prmpt;
		info->func = func;
		info->cancel = FALSE;
		info->retval = NULL;

		if (g_main_depth () == 0) {
			g_idle_add_full (G_PRIORITY_DEFAULT, 
				get_password_idle_func, info, 
				get_password_destroy);
		} else {
			get_password_idle_func (info);
			get_password_destroy(info);
		}

		/* Wait on the queue for the mainloop callback to be finished */
		g_mutex_lock (info->mutex);
		if (!info->had_callback)
			g_cond_wait (info->condition, info->mutex);
		g_mutex_unlock (info->mutex);

		g_mutex_free (info->mutex);
		g_cond_free (info->condition);

		cancel = info->cancel;
		retval = info->retval;

		g_slice_free (HadToWaitForPasswordInfo, info);

		apriv->in_auth = FALSE;

		if (freeprmpt)
			g_free (prmpt);
	}

	if (cancel || retval == NULL) 
	{
		if (account) {
			TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (account);

			if (CAMEL_IS_DISCO_STORE (apriv->service)) {
				CamelException tex = CAMEL_EXCEPTION_INITIALISER;
				camel_disco_store_set_status (CAMEL_DISCO_STORE (apriv->service),
						CAMEL_DISCO_STORE_OFFLINE, &tex);
			}
		}

		camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL,
			_("You cancelled when you had to enter a password"));

		if (retval)
			g_free (retval);
		retval = NULL;
	}

	return retval;
}


/* Hey Rob, look here! This gets called when the password was wrong. It can
 * but usually wont use Gtk+ code by the implementer of the function pointer.
 * Usually the application developer will "forget" the password here. So that
 * The next request for a password gives the user a password dialog box. 
 *
 * That request will happen by the tny_session_camel_get_password function. This
 * happens in a thread, so does the tny_session_camel_get_password. The thread
 * is always the queue thread of the account. 
 *
 * This is number #2 */



typedef struct
{
	TnySessionCamel *self;
	TnyAccount *account;
	TnyForgetPassFunc func;

	GCond* condition;
	gboolean had_callback;
	GMutex *mutex;

} HadToWaitForForgetInfo;


static gboolean
forget_password_idle_func (gpointer user_data)
{
	HadToWaitForForgetInfo *info = (HadToWaitForForgetInfo *) user_data;
	TnySessionCamel *self = info->self;
	TnyAccount *account = info->account;
	TnyForgetPassFunc func = info->func;

	tny_lockable_lock (self->priv->ui_lock);
	func (account);
	tny_lockable_unlock (self->priv->ui_lock);

	return FALSE;
}

static void
forget_password_destroy(gpointer user_data)
{
	HadToWaitForForgetInfo *info = (HadToWaitForForgetInfo *) user_data;

	g_object_unref (info->account);
	camel_object_unref (info->self);

	g_mutex_lock (info->mutex);
	g_cond_broadcast (info->condition);
	info->had_callback = TRUE;
	g_mutex_unlock (info->mutex);

	return;
}

static void
tny_session_camel_forget_password (CamelSession *session, CamelService *service, const char *domain, const char *item, CamelException *ex)
{
	TnySessionCamel *self = (TnySessionCamel *)session;
	TnyForgetPassFunc func;
	TnyAccount *account;

	account = service->data;

	if (account)
	{
		HadToWaitForForgetInfo *info = NULL;

		func = tny_account_get_forget_pass_func (account);

		if (!func)
			return;

		info = g_slice_new (HadToWaitForForgetInfo);

		info->mutex = g_mutex_new ();
		info->condition = g_cond_new ();
		info->had_callback = FALSE;

		camel_object_ref (self);
		info->self = self;
		info->account = TNY_ACCOUNT (g_object_ref (account));
		info->func = func;

		if (g_main_depth () == 0) {
			g_idle_add_full (G_PRIORITY_DEFAULT, 
				forget_password_idle_func, info, 
				forget_password_destroy);
		} else {
			forget_password_idle_func (info); 
			forget_password_destroy(info);
		}

		/* Wait on the queue for the mainloop callback to be finished */
		g_mutex_lock (info->mutex);
		if (!info->had_callback)
			g_cond_wait (info->condition, info->mutex);
		g_mutex_unlock (info->mutex);

		g_mutex_free (info->mutex);
		g_cond_free (info->condition);

		g_slice_free (HadToWaitForForgetInfo, info);

	}

	return;
}

/* This is a wrapper for the alerting. Called by on_account_connect_done and
 * tny_session_camel_alert_user (see below for both) */

typedef struct
{
	TnySessionCamel *self;
	TnyAccount *account;
	TnyAlertType tnytype;
	gboolean question, retval;
	GError *err;

	GCond* condition;
	gboolean had_callback;
	GMutex *mutex;

} HadToWaitForAlertInfo;


static gboolean
alert_idle_func (gpointer user_data)
{
	HadToWaitForAlertInfo *info = user_data;
	TnySessionCamel *self = info->self;
	TnyAccount *account = info->account;

	tny_lockable_lock (self->priv->ui_lock);

	if( self->priv->account_store ){
		info->retval = tny_account_store_alert (
			(TnyAccountStore*) self->priv->account_store, 
			account, info->tnytype, info->question, 
			info->err);
	} else {
		g_warning ("cannot alert: no account_store");
	}
	tny_lockable_unlock (self->priv->ui_lock);

	return FALSE;
}

static void
alert_destroy(gpointer user_data)
{
	HadToWaitForAlertInfo *info = (HadToWaitForAlertInfo *) user_data;

	if (info->account)
		g_object_unref (info->account);
	camel_object_unref (info->self);

	g_mutex_lock (info->mutex);
	g_cond_broadcast (info->condition);
	info->had_callback = TRUE;
	g_mutex_unlock (info->mutex);

	return;
}

static gboolean
tny_session_camel_do_an_error (TnySessionCamel *self, TnyAccount *account, TnyAlertType tnytype, gboolean question, GError *err)
{
	HadToWaitForAlertInfo *info = NULL;
	gboolean retval = FALSE;

	info = g_slice_new (HadToWaitForAlertInfo);

	info->mutex = g_mutex_new ();
	info->condition = g_cond_new ();
	info->had_callback = FALSE;

	camel_object_ref (self);
	info->self = self;
	info->account = account?TNY_ACCOUNT (g_object_ref (account)):NULL;
	info->tnytype = tnytype;
	info->question = question;
	info->err = err;
	info->retval = FALSE;

	if (g_main_depth () == 0) {
		g_idle_add_full (G_PRIORITY_DEFAULT, 
			alert_idle_func, info, 
			alert_destroy);
	} else {
		alert_idle_func (info); 
		alert_destroy(info);
	}

	/* Wait on the queue for the mainloop callback to be finished */
	g_mutex_lock (info->mutex);
	if (!info->had_callback)
		g_cond_wait (info->condition, info->mutex);
	g_mutex_unlock (info->mutex);

	g_mutex_free (info->mutex);
	g_cond_free (info->condition);

	retval = info->retval;

	g_slice_free (HadToWaitForAlertInfo, info);

	return retval;
}

/* tny_session_camel_alert_user will for example be called by camel when SSL is on and 
 * camel_session_get_service is issued (for example TnyCamelTransportAccount and
 * TnyCamelStore account issue this function). Its implementation is often done
 * with GUI components (it should therefore not be called from a thread). */

/* Hey Rob, look here! This is an alert that will be launched from a thread. 
 * usually the queue thread of the account (I think always, but I have to verify
 * this before making a statement out of it). The user sometimes has to answer 
 * this question. For example with a Yes or a No. The typical example is the 
 * SSL question and error reporting when the connection is lost) 
 *
 * This is number #3 */

static gboolean
tny_session_camel_alert_user (CamelSession *session, CamelSessionAlertType type, CamelException *ex, gboolean cancel, CamelService *service)
{
	TnySessionCamel *self = (TnySessionCamel *) session;
	TnySessionCamelPriv *priv = NULL;
	gboolean retval = FALSE;
	GError *err = NULL;
	TnyAccount *account = NULL;

	if (service && service->data)
		account = (TnyAccount *) service->data;

	if (!TNY_IS_SESSION_CAMEL (self)) {
		g_critical ("Internal problem in tny_session_camel_alert_user");
		return FALSE;
	}

	priv = self->priv;
	if (priv->account_store)
	{
		TnyAlertType tnytype;

		switch (type)
		{
			case CAMEL_SESSION_ALERT_INFO:
				tnytype = TNY_ALERT_TYPE_INFO;
			break;
			case CAMEL_SESSION_ALERT_WARNING:
				tnytype = TNY_ALERT_TYPE_WARNING;
			break;
			case CAMEL_SESSION_ALERT_ERROR:
			default:
				tnytype = TNY_ALERT_TYPE_ERROR;
			break;
		}

		_tny_camel_exception_to_tny_error (ex, &err);

		retval = tny_session_camel_do_an_error (self, account, tnytype, 
			TRUE, err);

		if (err)
			g_error_free (err);
	}

	return retval;
}



/**
 * tny_session_camel_set_ui_locker:
 * @self: a #TnySessionCamel instance
 * @ui_lock: a #TnyLockable instance 
 *
 * Set the user interface toolkit locker. The lock and unlock methods of this
 * locker should be implemented with the lock and unlock functionality of your
 * user interface toolkit.
 *
 * Good examples are gdk_threads_enter () and gdk_threads_leave () in gtk+.
 **/
void 
tny_session_camel_set_ui_locker (TnySessionCamel *self, TnyLockable *ui_lock)
{
	TnySessionCamelPriv *priv = self->priv;
	if (priv->ui_lock)
		g_object_unref (G_OBJECT (priv->ui_lock));
	priv->ui_lock = TNY_LOCKABLE (g_object_ref (ui_lock));
	return;
}


CamelFolder *
mail_tool_uri_to_folder (CamelSession *session, const char *uri, guint32 flags, CamelException *ex)
{
	CamelURL *url;
	CamelStore *store = NULL;
	CamelFolder *folder = NULL;
	/*int offset = 0;*/
	char *curi = NULL;

	g_return_val_if_fail (uri != NULL, NULL);
	
	url = camel_url_new (uri /*+ offset*/, ex);

	if (G_UNLIKELY (!url))
	{
		g_free(curi);
		return NULL;
	}

	store = (CamelStore *)camel_session_get_service(session, uri /* + offset */, CAMEL_PROVIDER_STORE, ex);
	if (G_LIKELY (store))
	{
		const char *name;

		if (url->fragment) 
		{
			name = url->fragment;
		} else {
			if (url->path && *url->path)
				name = url->path + 1;
			else
				name = "";
		}

		/*if (offset) 
		{
			if (offset == 7)
				folder = (CamelFolder*)camel_store_get_trash (store, ex);
			else if (offset == 6)
				folder = (CamelFolder*)camel_store_get_junk (store, ex);
			else
				g_assert (FALSE);
		} else*/
			folder = (CamelFolder*)camel_store_get_folder (store, name, flags, ex);
		camel_object_unref (store);
	}

	camel_url_free (url);
	g_free(curi);

	return folder;
}


static CamelFolder *
get_folder (CamelFilterDriver *d, const char *uri, void *data, CamelException *ex)
{
	CamelSession *session = data;
	return mail_tool_uri_to_folder(session, uri, 0, ex);
}

static CamelFilterDriver *
tny_session_camel_get_filter_driver (CamelSession *session, const char *type, CamelException *ex)
{
	CamelFilterDriver *driver = camel_filter_driver_new (session);
	camel_filter_driver_set_folder_func (driver, get_folder, session);
	return driver; 
}


static void 
my_receive_func(CamelSession *session, struct _CamelSessionThreadMsg *m)
{
	return;
}

static void
my_free_func (CamelSession *session, struct _CamelSessionThreadMsg *m)
{
	return;
}


static void 
my_cancel_func (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *data)
{
	return;
}

static void *
tny_session_camel_ms_thread_msg_new (CamelSession *session, CamelSessionThreadOps *ops, unsigned int size)
{
	CamelSessionThreadMsg *msg = ms_parent_class->thread_msg_new(session, ops, size);

	msg->ops = g_new0 (CamelSessionThreadOps,1);
	msg->ops->free = my_free_func;
	msg->ops->receive = my_receive_func;
	msg->data = NULL;
	msg->op = camel_operation_new (my_cancel_func, NULL);

	return msg;

}

static void
tny_session_camel_ms_thread_msg_free (CamelSession *session, CamelSessionThreadMsg *m)
{
	ms_parent_class->thread_msg_free(session, m);
	return;
}

static void
tny_session_camel_ms_thread_status (CamelSession *session, CamelSessionThreadMsg *msg, const char *text, int pc)
{
	return;
}



static void
tny_session_camel_init (TnySessionCamel *instance)
{
	TnySessionCamelPriv *priv;
	instance->priv = g_slice_new (TnySessionCamelPriv);
	priv = instance->priv;

	priv->initialized = FALSE;
	priv->stop_now = FALSE;
	priv->regged_queues = NULL;
	priv->is_inuse = FALSE;
	priv->queue_lock = g_mutex_new ();

	/* I know it would be better to have a hashtable that does reference
	 * counting. But then you'll get embraced references, which gobject
	 * can't really handle (unless doing it with a weak reference) */

	priv->current_accounts = g_hash_table_new_full (g_str_hash, g_direct_equal, 
					g_free, NULL);

	priv->current_accounts_lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (priv->current_accounts_lock);
	priv->prev_constat = FALSE;
	priv->device = NULL;
	priv->camel_dir = NULL;
	priv->ui_lock = tny_noop_lockable_new ();
	priv->camel_dir = NULL;
	priv->in_auth_function = FALSE;
	priv->is_connecting = FALSE;

	return;
}


/* Happens at tny_camel_account_set_session. This does very few things, actually.
 * Maybe we can factor this away and make the tny_camel_account_set_session not
 * required anymore? Though, the account will always need to know what the session
 * is. So we'd have to find another way to let the account know about this ... */

void 
_tny_session_camel_register_account (TnySessionCamel *self, TnyCamelAccount *account)
{
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (account);
	TnySessionCamelPriv *priv = self->priv;

	if (apriv->cache_location)
		g_free (apriv->cache_location);
	apriv->cache_location = g_strdup (priv->camel_dir);

	return;
}

static void
on_account_connect_done (TnyCamelAccount *account, gboolean canceled, GError *err, gpointer user_data)
{
	TnySessionCamel *self = (TnySessionCamel *) user_data;
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (account);

	/* This one happens when a account is finished with connecting. On 
	 * failure, err is not NULL on success it is NULL. This happens in a
	 * thread: the queue thread of @account. */

	/* Hey Rob, look here! This will happen when you have an error. This
	 * "do_an_error" thing will invoke the alert_func from a thread. */


	/* We set the account to ready, so that tny_account_is_ready returns TRUE
	 * from now on. _tny_session_camel_activate_account caused this callback
	 * to happen (either successful or non-successful, this happens for each
	 * account that is internally ready) */

	apriv->is_ready = TRUE;

	if (err) {

		/* It seems err is forgotten here ... if the disk is full ! */
		if( self->priv->account_store ){
			tny_account_store_alert (
				(TnyAccountStore*) self->priv->account_store, 
				TNY_ACCOUNT (account), TNY_ALERT_TYPE_ERROR, FALSE, 
				err);
		} else {
			g_warning("disk full: no account_store");
		}
	}

	camel_object_unref (self);

	return;
}

typedef struct {
	TnySessionCamel *self;
	TnyAccount *account;
	gboolean online;
} SetOnlHapInfo;

static gboolean
set_online_happened_idle (gpointer user_data)
{
	SetOnlHapInfo *info = (SetOnlHapInfo *) user_data;
	tny_lockable_lock (info->self->priv->ui_lock);
	g_signal_emit (info->account, 
		tny_camel_account_signals [TNY_CAMEL_ACCOUNT_SET_ONLINE_HAPPENED], 
		0, info->online);
	tny_lockable_unlock (info->self->priv->ui_lock);
	return FALSE;
}

static void
set_online_happened_destroy (gpointer user_data)
{
	SetOnlHapInfo *info = (SetOnlHapInfo *) user_data;
	camel_object_unref (info->self);
	g_object_unref (info->account);
	g_slice_free (SetOnlHapInfo, info);
}

static void 
tny_session_queue_going_online_for_account (TnySessionCamel *self, TnyCamelAccount *account, gboolean online)
{
	/* So this is that wrapper that I talked about (see below). In case we 
	 * are a store account, this means that we need to throw the request to
	 * go online to the account's queue. This is implemented in a protected
	 * method in TnyCamelStoreAccount. Go take a look! */

	if (TNY_IS_CAMEL_STORE_ACCOUNT (account)) {
		camel_object_ref (self);
		if (online)
			camel_session_set_online ((CamelSession *) self, TRUE);
		_tny_camel_store_account_queue_going_online (
			TNY_CAMEL_STORE_ACCOUNT (account), self, online,
			on_account_connect_done, self);
	}

	/* Else, if it's a transport account, we don't have any transport
	 * account implementations that actually need to go online at this
	 * moment yet. At the moment of transferring the first message, the
	 * current implementations will automatically connect themselves. */

	if (TNY_IS_CAMEL_TRANSPORT_ACCOUNT (account))
	{
		SetOnlHapInfo *info = g_slice_new (SetOnlHapInfo);

		info->self = self;
		camel_object_ref (info->self);
		info->account = TNY_ACCOUNT (g_object_ref (account));
		info->online = online;

		g_idle_add_full (G_PRIORITY_DEFAULT, set_online_happened_idle, 
			info, set_online_happened_destroy);

		return;
	}
}

/* This happens when the password function of the account is set. It will after 
 * registering the account for future connection-changes, already set the online
 * state of it to the current connectivity setting of the device */

void
_tny_session_camel_activate_account (TnySessionCamel *self, TnyCamelAccount *account)
{
	TnySessionCamelPriv *priv = self->priv;

	g_static_rec_mutex_lock (priv->current_accounts_lock);

	/* It would indeed be better to add a reference here. But then you'll
	 * get embraced references at some point. We'll get rid of the instance
	 * in the hashtable before the finalization of it, take a look at the
	 * unregister stuff below. */

	g_hash_table_insert (priv->current_accounts, 
			g_strdup (tny_account_get_id (TNY_ACCOUNT (account))),
			account);
	g_static_rec_mutex_unlock (priv->current_accounts_lock);

	tny_session_queue_going_online_for_account (self, account, 
		tny_device_is_online (priv->device));

	return;
}


/* This happens when an account gets finalized. We more or less simply get rid
 * of the instance in the hashtable. */

void 
_tny_session_camel_unregister_account (TnySessionCamel *self, TnyCamelAccount *account)
{
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (account);
	TnySessionCamelPriv *priv = self->priv;

	if (apriv->cache_location)
		g_free (apriv->cache_location);
	apriv->cache_location = NULL;

	g_static_rec_mutex_lock (priv->current_accounts_lock);
	g_hash_table_remove (priv->current_accounts, 
		tny_account_get_id (TNY_ACCOUNT (account)));
	g_static_rec_mutex_unlock (priv->current_accounts_lock);

	return;
}

typedef struct {
	TnySessionCamel *self;
	gboolean online;
} ConChangedForEachInfo;

static void 
tny_session_camel_connection_changed_for_each (gpointer key, gpointer value, gpointer user_data)
{
	ConChangedForEachInfo *info = (ConChangedForEachInfo *) user_data;

	/* For each registered account, we'll set its online state. Throwing it
	 * in its queue is implemented in the TnyCamelStoreAccount btw. For now,
	 * because it's shared with the account registration, this is a wrapper */

	tny_session_queue_going_online_for_account (info->self, 
		TNY_CAMEL_ACCOUNT (value), info->online);

	return;
}

/* In case our TnyDevice's connection_changed event happened. We'll set all
 * registered accounts to the proper online state here. Because these accounts
 * all register their connecting on their own queue, it's not detectable when
 * connecting got finished. They also all start doing their connecting stuff
 * in parallel. Quite difficult to know when all are finished with that. 
 *
 * In stead we provide components, like the TnyGtkFolderStoreTreeModel, that can
 * cope with connection changes. For example: they'll load-in new folders as 
 * soon as it's known that the account went online. No more accounts-reloaded
 * things as it's far more fine grained this way ... */

static void
tny_session_camel_connection_changed (TnyDevice *device, gboolean online, gpointer user_data)
{
	TnySessionCamel *self = user_data;
	TnySessionCamelPriv *priv = self->priv;
	ConChangedForEachInfo *info = NULL;


	/* Maybe we can remove this check after Rob is finished with throwing  
	 * the questions in the GMainLoop. I'm leaving it here for now ... */

	info = g_slice_new (ConChangedForEachInfo);

	info->online = online;
	info->self = self;


	/* This Camel API exists, I haven't yet figured out what exactly it does
	 * or change in terms of behaviour. */

	camel_session_set_online ((CamelSession *) self, online); 


	/* As said, we can issue this signal. We can't issue when connecting 
	 * actually finished: that's because all accounts register getting 
	 * connected on their own queues. So it'll all happen in parallel and
	 * a little bit less in control as when we would dedicate a thread to it,
	 * and in a serialized way get all accounts connected (the old way) */

	if (priv->account_store)
		g_signal_emit (priv->account_store,
			tny_account_store_signals [TNY_ACCOUNT_STORE_CONNECTING_STARTED], 0);

	g_static_rec_mutex_lock (priv->current_accounts_lock);
	/* So let's look above, at that for_each function ... */
	g_hash_table_foreach (priv->current_accounts, 
		tny_session_camel_connection_changed_for_each, info);
	g_static_rec_mutex_unlock (priv->current_accounts_lock);

	g_slice_free (ConChangedForEachInfo, info);

	return;
}


/**
 * tny_session_camel_set_initialized:
 * @self: the #TnySessionCamel instance
 *
 * This method must be called once the initial accounts are created in your
 * #TnyAccountStore implementation and should indicate that the initialization
 * of your initial accounts is completed.
 **/
void 
tny_session_camel_set_initialized (TnySessionCamel *self)
{
	TnySessionCamelPriv *priv = self->priv;
	TnyDevice *device = NULL;

	if (priv->initialized)
		return;

	device = priv->device;

	if (!device || !TNY_IS_DEVICE (device))
	{
		g_critical ("Please use tny_session_camel_set_device "
			"before tny_session_camel_set_initialized");
		return;
	}

	priv->initialized = TRUE;

	priv->connchanged_signal = g_signal_connect (
		G_OBJECT (device), "connection_changed",
		G_CALLBACK (tny_session_camel_connection_changed), self);

	/* tny_session_camel_connection_changed (device, 
		tny_device_is_online (device), self); */
}

/**
 * tny_session_camel_set_device:
 * @self: a #TnySessionCamel object
 * @device: a #TnyDevice instance
 *
 * Set the device of @self.
 *
 **/
void 
tny_session_camel_set_device (TnySessionCamel *self, TnyDevice *device)
{
	TnySessionCamelPriv *priv = self->priv;

	if (priv->device && g_signal_handler_is_connected (G_OBJECT (priv->device), 
		priv->connchanged_signal))
	{
		g_signal_handler_disconnect (G_OBJECT (device), 
			priv->connchanged_signal);
	}

	if (priv->device)
		g_object_unref (device);

	priv->device = g_object_ref (device);

	return;
}

/**
 * tny_session_camel_set_account_store:
 * @self: a #TnySessionCamel object
 * @account_store: A #TnyAccountStore account store instance
 *
 * Set the account store of @self.
 **/
void 
tny_session_camel_set_account_store (TnySessionCamel *self, TnyAccountStore *account_store)
{
	CamelSession *session = (CamelSession*) self;
	TnyDevice *device = (TnyDevice*)tny_account_store_get_device (account_store);
	gchar *base_directory = NULL;
	gchar *camel_dir = NULL;
	gboolean online;
	TnySessionCamelPriv *priv = self->priv;

	priv->account_store = (gpointer)account_store;    
	g_object_add_weak_pointer (priv->account_store, &priv->account_store);

	base_directory = g_strdup (tny_account_store_get_cache_dir (account_store));

	if (camel_init (base_directory, TRUE) != 0)
	{
		g_error (_("Critical ERROR: Cannot init %s as camel directory\n"), base_directory);
		g_object_unref (G_OBJECT (device));
		exit (1);
	}

	camel_dir = g_build_filename (base_directory, "mail", NULL);
	camel_provider_init();
	camel_session_construct (session, camel_dir);

	online = tny_device_is_online (device);
	camel_session_set_online ((CamelSession *) session, online); 
	priv->camel_dir = camel_dir;
	g_free (base_directory);
	tny_session_camel_set_device (self, device);
	g_object_unref (G_OBJECT (device));

	return;
}


void 
_tny_session_camel_unreg_queue (TnySessionCamel *self, TnyCamelSendQueue *queue)
{
	TnySessionCamelPriv *priv = self->priv;

	g_mutex_lock (priv->queue_lock);
	priv->regged_queues = g_list_remove (priv->regged_queues, queue);
	g_mutex_unlock (priv->queue_lock);
}

void 
_tny_session_camel_reg_queue (TnySessionCamel *self, TnyCamelSendQueue *queue)
{
	TnySessionCamelPriv *priv = self->priv;

	g_mutex_lock (priv->queue_lock);
	priv->regged_queues = g_list_prepend (priv->regged_queues, queue);
	g_mutex_unlock (priv->queue_lock);
}


/**
 * tny_session_camel_new:
 * @account_store: A TnyAccountStore instance
 *
 * A #CamelSession for tinymail
 *
 * Return value: The #TnySessionCamel singleton instance
 **/
TnySessionCamel*
tny_session_camel_new (TnyAccountStore *account_store)
{
	TnySessionCamel *retval = TNY_SESSION_CAMEL 
			(camel_object_new (TNY_TYPE_SESSION_CAMEL));

	tny_session_camel_set_account_store (retval, account_store);

	return retval;
}


static void
tny_session_camel_finalise (CamelObject *object)
{
	TnySessionCamel *self = (TnySessionCamel*)object;
	TnySessionCamelPriv *priv = self->priv;

	g_static_rec_mutex_lock (priv->current_accounts_lock);
	g_hash_table_destroy (priv->current_accounts);
	g_static_rec_mutex_unlock (priv->current_accounts_lock);

	g_mutex_lock (priv->queue_lock);
	g_list_free (priv->regged_queues);
	priv->regged_queues = NULL;
	g_mutex_unlock (priv->queue_lock);

	if (priv->device && g_signal_handler_is_connected (G_OBJECT (priv->device), priv->connchanged_signal))
	{
		g_signal_handler_disconnect (G_OBJECT (priv->device), 
			priv->connchanged_signal);
	}

	if( priv->account_store ){
		g_object_remove_weak_pointer (priv->account_store, &priv->account_store);
	}

	if (priv->device) {
		g_object_unref (priv->device);
	}

	if (priv->ui_lock)
		g_object_unref (G_OBJECT (priv->ui_lock));

	if (priv->camel_dir)
		g_free (priv->camel_dir);

	g_mutex_free (priv->queue_lock);

	/* g_static_rec_mutex_free (priv->current_accounts_lock); */
	g_free (priv->current_accounts_lock);
	priv->current_accounts_lock = NULL;

	g_slice_free (TnySessionCamelPriv, self->priv);

	/* CamelObject types don't need parent finalization (build-in camel) */

	return;
}

static void
tny_session_camel_class_init (TnySessionCamelClass *tny_session_camel_class)
{
	CamelSessionClass *camel_session_class = CAMEL_SESSION_CLASS (tny_session_camel_class);

	camel_session_class->get_password = tny_session_camel_get_password;
	camel_session_class->forget_password = tny_session_camel_forget_password;
	camel_session_class->alert_user = tny_session_camel_alert_user;
	camel_session_class->get_filter_driver = tny_session_camel_get_filter_driver;
	camel_session_class->thread_msg_new = tny_session_camel_ms_thread_msg_new;
	camel_session_class->thread_msg_free = tny_session_camel_ms_thread_msg_free;
	camel_session_class->thread_status = tny_session_camel_ms_thread_status;

	return;
}

/**
 * tny_session_camel_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
CamelType
tny_session_camel_get_type (void)
{
	static CamelType tny_session_camel_type = CAMEL_INVALID_TYPE;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);
		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	if (G_UNLIKELY (tny_session_camel_type == CAMEL_INVALID_TYPE)) 
	{
		ms_parent_class = (CamelSessionClass *)camel_session_get_type();
		tny_session_camel_type = camel_type_register (
			camel_session_get_type (),
			"TnySessionCamel",
			sizeof (TnySessionCamel),
			sizeof (TnySessionCamelClass),
			(CamelObjectClassInitFunc) tny_session_camel_class_init,
			NULL,
			(CamelObjectInitFunc) tny_session_camel_init,
			(CamelObjectFinalizeFunc) tny_session_camel_finalise);
	}

	return tny_session_camel_type;
}
