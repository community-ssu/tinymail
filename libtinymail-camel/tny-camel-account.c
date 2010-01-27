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

#include <tny-camel-account.h>
#include <tny-session-camel.h>
#include <tny-account-store.h>
#include <tny-folder.h>
#include <tny-camel-folder.h>
#include <tny-error.h>
#include <tny-simple-list.h>
#include <tny-pair.h>

#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-store.h>
#include <camel/camel-offline-folder.h>
#include <camel/camel-offline-store.h>
#include <camel/camel-disco-folder.h>
#include <camel/camel-disco-store.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <tny-camel-shared.h>
#include <tny-status.h>

#include <tny-camel-default-connection-policy.h>

#include "tny-session-camel-priv.h"
#include "tny-camel-common-priv.h"

#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include "tny-camel-account-priv.h"

#include <tny-camel-store-account.h>
#include "tny-camel-store-account-priv.h"

static GObjectClass *parent_class = NULL;

guint tny_camel_account_signals [TNY_CAMEL_ACCOUNT_LAST_SIGNAL];

static gboolean
changed_idle (gpointer data)
{
	TnyAccount *self = (TnyAccount *) data;
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	tny_lockable_lock (apriv->session->priv->ui_lock);
	g_signal_emit (G_OBJECT (self), 
		tny_account_signals [TNY_ACCOUNT_CHANGED], 0);
	tny_lockable_unlock (apriv->session->priv->ui_lock);

	return FALSE;
}

static void
changed_idle_destroy (gpointer data)
{
	g_object_unref (data);
}



void
_tny_camel_account_emit_changed (TnyCamelAccount *self)
{
	g_idle_add_full (G_PRIORITY_HIGH, 
				changed_idle, 
				g_object_ref (self), 
				changed_idle_destroy);
}


typedef struct {
	TnyCamelQueueable parent;
	TnyCamelAccount *self;
} ReconInfo;

static gpointer
reconnect_thread (gpointer user_data)
{
	ReconInfo *info = (ReconInfo *) user_data;
	TnyCamelAccount *self = (TnyCamelAccount *) info->self;
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;

	apriv->service->reconnecting = TRUE;
	if (apriv->service->reconnecter)
		apriv->service->reconnecter (apriv->service, FALSE, apriv->service->data);
	camel_service_disconnect (apriv->service, FALSE, &ex);
	if (camel_exception_is_set (&ex))
		camel_exception_clear (&ex);
	camel_service_connect (apriv->service, &ex);
	if (apriv->service->reconnection)
	{
		if (!camel_exception_is_set (&ex))
			apriv->service->reconnection (apriv->service, TRUE, apriv->service->data);
		else
			apriv->service->reconnection (apriv->service, FALSE, apriv->service->data);
	}
	apriv->service->reconnecting = FALSE;

	return NULL;
}

static gboolean 
did_refresh (gpointer user_data)
{
	return FALSE;
}

static void 
did_refresh_destroy (gpointer user_data)
{
	ReconInfo *info = (ReconInfo *) user_data;
	g_object_unref (info->self);
	return;
}

static gboolean 
cancelled_refresh (gpointer user_data)
{
	return FALSE;
}

static void 
cancelled_refresh_destroy (gpointer user_data)
{
	ReconInfo *info = (ReconInfo *) user_data;
	g_object_unref (info->self);
	return;
}

static void
output_param (GQuark key_id, gpointer data, gpointer user_data)
{
	TnyCamelAccountPriv *apriv = user_data;

	if (*(char *)data) {
		apriv->options = g_list_prepend (apriv->options, 
			g_strdup_printf ("%s=%s", 
				g_quark_to_string (key_id), 
				(const gchar*) data));
	}
}

void
_tny_camel_account_refresh (TnyCamelAccount *self, gboolean recon_if)
{
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	g_static_rec_mutex_lock (apriv->service_lock);

	if (!apriv->custom_url_string)
	{
		CamelURL *url = NULL;
		GList *options = apriv->options;
		gchar *proto;
		gboolean urlneedfree = FALSE;

		if (apriv->proto == NULL)
			goto fail;

		proto = g_strdup_printf ("%s://", apriv->proto); 

		if (camel_exception_is_set (apriv->ex))
			camel_exception_clear (apriv->ex);

		if (!apriv->service) {
			url = camel_url_new (proto, apriv->ex);
			urlneedfree = TRUE;
		} else
			url = apriv->service->url;

		g_free (proto);

		if (!url)
			goto fail;

		camel_url_set_protocol (url, apriv->proto); 
		 if (apriv->user)
			camel_url_set_user (url, apriv->user);
		camel_url_set_host (url, apriv->host);
		if (apriv->port != -1)
			camel_url_set_port (url, (int)apriv->port);
		if (apriv->mech)
			camel_url_set_authmech (url, apriv->mech);

		while (options)
		{
			gchar *ptr, *dup = g_strdup (options->data);
			gchar *option, *value;
			ptr = strchr (dup, '=');
			if (ptr) {
				ptr++;
				value = g_strdup (ptr); ptr--;
				*ptr = '\0'; option = dup;
			} else {
				option = dup;
				value = g_strdup ("1");
			}
			camel_url_set_param (url, option, value);
			g_free (value);
			g_free (dup);
			options = g_list_next (options);
		}
		if (G_LIKELY (apriv->url_string))
			g_free (apriv->url_string);
		apriv->url_string = camel_url_to_string (url, 0);

		if (urlneedfree)
			camel_url_free (url);
	} else if (apriv->url_string) {
		CamelException uex = CAMEL_EXCEPTION_INITIALISER;
		CamelURL *url = camel_url_new (apriv->url_string, &uex);
		if (camel_exception_is_set (&uex))
			camel_exception_clear (&uex);

		if (url) {
			if (url->params)
				g_datalist_foreach (&url->params, output_param, apriv);

			if (apriv->proto)
				g_free (apriv->proto);
			if (url->protocol)
				apriv->proto = g_strdup (url->protocol);
			else
				apriv->proto = NULL;

			if (apriv->user)
				g_free (apriv->user);
			if (url->user)
				apriv->user = g_strdup (url->user);
			else
				apriv->user = NULL;

			if (url->port != -1)
				apriv->port = url->port;

			if (apriv->host)
				g_free (apriv->host);
			if (url->host)
				apriv->host = g_strdup (url->host);
			else
				apriv->host = NULL;

			if (apriv->mech)
				g_free (apriv->mech);
			if (url->authmech)
				apriv->mech = g_strdup (url->authmech);
			else
				apriv->mech = NULL;


			camel_url_free (url);
		}

	}

	if (recon_if && (apriv->status != TNY_CONNECTION_STATUS_DISCONNECTED))
	{
		if (!apriv->service)
			goto fail;

		if (TNY_IS_STORE_ACCOUNT (self))
		{
			TnyCamelStoreAccountPriv *aspriv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (self);
			ReconInfo *info = g_slice_new0 (ReconInfo);
			info->self = self;

			/* thread reference */
			g_object_ref (info->self);

			_tny_camel_queue_remove_items (aspriv->queue,
				TNY_CAMEL_QUEUE_RECONNECT_ITEM);

			_tny_camel_queue_launch_wflags (aspriv->queue, 
				reconnect_thread, 
				did_refresh, did_refresh_destroy,
				cancelled_refresh, 
				cancelled_refresh_destroy, 
				NULL, 
				info, sizeof (ReconInfo),
				TNY_CAMEL_QUEUE_RECONNECT_ITEM,
				__FUNCTION__);
		}
	}

fail:
	g_static_rec_mutex_unlock (apriv->service_lock);

	return;
}

static gboolean 
tny_camel_account_matches_url_string (TnyAccount *self, const gchar *url_string)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->matches_url_string(self, url_string);
}

static gboolean 
tny_camel_account_matches_url_string_default (TnyAccount *self, const gchar *url_string)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	CamelURL *in = NULL;
	CamelURL *org = NULL;
	gboolean retval = TRUE;

	if (url_string)
		in = camel_url_new (url_string, &ex);
	else
		return FALSE;

	if (camel_exception_is_set (&ex) || !in)
		return FALSE;

	if (priv->url_string)
		org = camel_url_new (priv->url_string, &ex);
	else {
		gchar *proto;
		if (priv->proto == NULL)
			return FALSE;
		proto = g_strdup_printf ("%s://", priv->proto); 
		org = camel_url_new (proto, &ex);
		g_free (proto);
		if (camel_exception_is_set (&ex) || !org)
			return FALSE;
		camel_url_set_protocol (org, priv->proto); 
		if (priv->user)
			camel_url_set_user (org, priv->user);
		camel_url_set_host (org, priv->host);
		if (priv->port != -1)
			camel_url_set_port (org, (int)priv->port);
	}

	if (camel_exception_is_set (&ex) || !org)
	{
		if (in)
			camel_url_free (in);
		if (org)
			camel_url_free (org);
		return FALSE;
	}

	if (in && org && in->protocol && (org->protocol && strcmp (org->protocol, in->protocol) != 0))
		retval = FALSE;

	if (in && org && in->user && (org->user && strcmp (org->user, in->user) != 0))
		retval = FALSE;

	if (in && org && in->host && (org->host && strcmp (org->host, in->host) != 0))
		retval = FALSE;

	if (in && org && in->port != 0 && (org->port != in->port))
		retval = FALSE;

	/* For local maildir accounts, compare their paths, before the folder part. */
	if (in && org && in->path && org->path && (in->protocol && strcmp (in->protocol, "maildir") == 0)) {
		gchar *in_path = NULL;
		gchar *in_pos_hash = NULL;
		gchar *org_path = NULL;
		gchar *org_pos_hash = NULL;

		/* The folders have a # before them: */
		/* Copy the paths and set null-termination at the #: */
		in_path = g_strdup (in->path);
		in_pos_hash = strchr (in_path, '#');
		if (in_pos_hash)
			*in_pos_hash = '\0';

		org_path = g_strdup (org->path);
		org_pos_hash = strchr (org_path, '#');
		if (org_pos_hash)
			*org_pos_hash = '\0';

		if (strcmp (in_path, org_path) != 0)
			retval = FALSE;

		g_free (in_path);
		g_free (org_path);
	}

	if (org)
		camel_url_free (org);

	if (in)
		camel_url_free (in);

	return retval;
}

static TnyAccountType
tny_camel_account_get_account_type (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_account_type(self);
}

static TnyAccountType
tny_camel_account_get_account_type_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	return priv->account_type;
}

/**
 * tny_camel_account_add_option:
 * @self: a #TnyCamelAccount object
 * @option: a #TnyPair Camel option
 *
 * Add a Camel option to this #TnyCamelAccount instance. 
 *
 * An often used option is the use_ssl one. For example "use_ssl=wrapped" or 
 * "use_ssl=tls" are the typical options added. Other possibilities for the 
 * "use_ssl" option are "never" and "when-possible":
 *
 * <informalexample><programlisting>
 * tny_camel_account_add_option (account, tny_pair_new ("use_ssl", "tls"))
 * </programlisting></informalexample>
 *
 * If the Camel option is a single word, just use "" for the value part of the 
 * pair:
 *
 * <informalexample><programlisting>
 * tny_camel_account_add_option (account, tny_pair_new ("use_lsub", ""))
 * </programlisting></informalexample>
 *
 * use_ssl=wrapped will wrap the connection on default port 993 with IMAP and
 * defualt port 995 with POP3 with SSL or also called "imaps" or "pops".
 *
 * use_ssl=tls will connect using default port 143 for IMAP and 110 for POP and 
 * requires support for STARTTLS, which is often a command for letting the 
 * connection be or become encrypted in IMAP and POP3 servers.
 *
 * use_ssl=when-possible will try to do a STARTTLS, but will fallback to a non
 * encrypted session if it fails (not recommended, as your users will want SSL
 * if they require this security for their accounts).
 *
 * use_ssl=never will forcefully make sure that no SSL is to be used.
 * 
 * One option for some accounts (like the IMAP accounts) is idle_delay. The
 * parameter is the amount of seconds that you want to wait for the IDLE state
 * to be stopped. Stopping the IDLE state will make the server flush all the 
 * pending events for the IDLE state. This improve responsibility of the Push 
 * E-mail and expunge events, although it will cause a little bit more continuous
 * bandwidth consumption (each delayth second). For example idle_delay=20. The
 * defualt value is 28 minutes.
 *
 * Another option is getsrv_delay, also for IMAP accounts, which allows you to 
 * specify the delay before the connection that gets created for receiving 
 * uncached messages gets shut down. If this service is not yet shut down, then
 * it'll be reused. Else a new one will be created that will be kept around for
 * delay seconds (in the hopes that new requests will happen). Keeping a socket
 * open for a long period of time might not be ideal for some situations, but
 * closing it very quickly will let almost each request create a new connection
 * with the IMAP server. Which is why you can play with the value yourself. For
 * example getsrv_delay=100. The default value is 100.
 **/
void 
tny_camel_account_add_option (TnyCamelAccount *self, TnyPair *option)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->add_option(self, option);
}

static void 
tny_camel_account_add_option_default (TnyCamelAccount *self, TnyPair *option)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	GList *copy = priv->options;
	gboolean found = FALSE;
	gchar *option_str, *val;

	if (!option)
		return;

	if (!TNY_IS_PAIR (option)) {
		g_critical ("The tny_camel_account_add_option API has changed. "
		"Instead of a string you must now pass a TnyPair instance");
	}

	g_assert (TNY_IS_PAIR (option));

	val = (gchar *) tny_pair_get_value (option);

	if (val && strlen (val) > 0)
		option_str = g_strdup_printf ("%s=%s",
			tny_pair_get_name (option), 
			tny_pair_get_value (option));
	else
		option_str = g_strdup_printf ("%s",
			tny_pair_get_name (option));

	while (copy)  {
		gchar *str = (gchar *) copy->data;

		if (str && !strcmp (option_str, str)) {
			found = TRUE;
			break;
		}
		copy = g_list_next (copy);
	}

	if (!found) {
		priv->options = g_list_prepend (priv->options, g_strdup (option_str));
		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(self, TRUE, FALSE);
		_tny_camel_account_emit_changed (self);
	}

	g_free (option_str);

	return;
}


/**
 * tny_camel_account_get_options:
 * @self: a #TnyCamelAccount object
 * @options: a #TnyList 
 *
 * Get options of this #TnyCamelAccount instance. @options will be filled with
 * #TnyPair instances.
 **/
void 
tny_camel_account_get_options (TnyCamelAccount *self, TnyList *options)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_options (self, options);
}



static void 
tny_camel_account_get_options_default (TnyCamelAccount *self, TnyList *options)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	GList *copy = priv->options;

	if (!options)
		return;

	while (copy)  {
		gchar *str = g_strdup (copy->data);
		gchar *key = str;
		gchar *value = strchr (str, '=');
		TnyPair *pair;

		if (value) {
			*value = '\0';
			value++;
		} else 
			value = "";

		pair = tny_pair_new (key, value);
		g_free (str);

		tny_list_prepend (options, (GObject *) pair);

		copy = g_list_next (copy);
	}

	return;
}


/**
 * tny_camel_account_clear_options:
 * @self: a #TnyCamelAccount object
 *
 * Clear options of this #TnyCamelAccount instance.
 **/
void 
tny_camel_account_clear_options (TnyCamelAccount *self)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->clear_options (self);
}


static void 
tny_camel_account_clear_options_default (TnyCamelAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	if (priv->options) {
		g_list_foreach (priv->options, (GFunc)g_free, NULL);
		g_list_free (priv->options);
		priv->options = NULL;

		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(self, TRUE, FALSE);
		_tny_camel_account_emit_changed (self);
	}

	return;
}

void 
_tny_camel_account_try_connect (TnyCamelAccount *self, gboolean for_online, GError **err)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(TNY_CAMEL_ACCOUNT (self), for_online, TRUE);
	if (camel_exception_is_set (priv->ex)) {
		_tny_camel_exception_to_tny_error (priv->ex, err);
		camel_exception_clear (priv->ex);
	}
	return;
}




static void
tny_camel_account_set_url_string (TnyAccount *self, const gchar *url_string)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_url_string(self, url_string);
}

/* Simplify checks for NULLs: */
static gboolean
strings_are_equal (const gchar *a, const gchar *b)
{
	if (!a && !b)
		return TRUE;
	if (a && b)
	{
		return (strcmp (a, b) == 0);
	}
	else
		return FALSE;
}

static void
tny_camel_account_set_url_string_default (TnyAccount *self, const gchar *url_string)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	gboolean changed = FALSE;

	/* Ignore this if it is not a change: */
	if (strings_are_equal (priv->url_string, url_string))
		return;

	g_static_rec_mutex_lock (priv->service_lock);

	if (!url_string)
	{
		if (priv->url_string) {
			g_free (priv->url_string);
			changed = TRUE;
		}
		priv->custom_url_string = FALSE;
		priv->url_string = NULL;
	} else if ((!priv->url_string && url_string) || (!(strcmp (url_string, priv->url_string) == 0)))
	{
		if (priv->url_string)
			g_free (priv->url_string);
		priv->custom_url_string = TRUE;
		priv->url_string = g_strdup (url_string);
		changed = TRUE;
	}

	if (changed)
		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(TNY_CAMEL_ACCOUNT (self), TRUE, TRUE);

	g_static_rec_mutex_unlock (priv->service_lock);

	_tny_camel_account_emit_changed (TNY_CAMEL_ACCOUNT (self));

	return;
}

static gchar*
tny_camel_account_get_url_string (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_url_string(self);
}

static gchar*
tny_camel_account_get_url_string_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	return priv->url_string?g_strdup (priv->url_string):NULL;
}


static void
tny_camel_account_set_name (TnyAccount *self, const gchar *name)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_name(self, name);
}

static void
tny_camel_account_set_name_default (TnyAccount *self, const gchar *name)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	/* Ignore this if it is not a change: */
	if (strings_are_equal (priv->name, name))
		return;

	if (priv->name)
		g_free (priv->name);
	priv->name = g_strdup (name);

	_tny_camel_account_emit_changed (TNY_CAMEL_ACCOUNT (self));

	return;
}

static const gchar*
tny_camel_account_get_name (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_name(self);
}

static const gchar*
tny_camel_account_get_name_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	return (const gchar*)priv->name;
}

static void 
tny_camel_account_stop_camel_operation_priv (TnyCamelAccountPriv *priv)
{
	if (priv->cancel) {
		camel_operation_unregister (priv->cancel);
		camel_operation_end (priv->cancel);
		if (priv->cancel)
			camel_operation_unref (priv->cancel);
	}

	priv->cancel = NULL;
	priv->inuse_spin = FALSE;

	return;
}


/* TODO: Documentation.
 */
void 
_tny_camel_account_start_camel_operation_n (TnyCamelAccount *self, CamelOperationStatusFunc func, gpointer user_data, const gchar *what, gboolean cancel)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->cancel_lock);

	//while (priv->inuse_spin); 

	priv->inuse_spin = TRUE;

	if (priv->cancel) {
		_tny_camel_account_actual_uncancel (self);
		camel_operation_unregister (priv->cancel);
	}

	priv->cancel = camel_operation_new (func, user_data);

	camel_operation_register (priv->cancel);
	camel_operation_start (priv->cancel, (char*)what);

	g_static_rec_mutex_unlock (priv->cancel_lock);

	return;
}


void 
_tny_camel_account_start_camel_operation (TnyCamelAccount *self, CamelOperationStatusFunc func, gpointer user_data, const gchar *what)
{
	_tny_camel_account_start_camel_operation_n (self, func, user_data, what, TRUE);
}

/* TODO: Documentation.
 */
void 
_tny_camel_account_stop_camel_operation (TnyCamelAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	if (priv->cancel) {
		g_static_rec_mutex_lock (priv->cancel_lock);
		tny_camel_account_stop_camel_operation_priv (priv);
		g_static_rec_mutex_unlock (priv->cancel_lock);
	}

	return;
}

static void 
tny_camel_account_start_operation (TnyAccount *self, TnyStatusDomain domain, TnyStatusCode code, TnyStatusCallback status_callback, gpointer status_user_data)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->start_operation(self, domain, code, status_callback, status_user_data);
}

static void
tny_camel_account_stop_operation (TnyAccount *self, gboolean *canceled)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->stop_operation(self, canceled);
}




static void
refresh_status (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *user_data)
{
	RefreshStatusInfo *oinfo = user_data;
	TnyProgressInfo *info = NULL;
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (oinfo->self);

	info = tny_progress_info_new (G_OBJECT (oinfo->self), oinfo->status_callback, 
		oinfo->domain, oinfo->code, what, sofar, 
		oftotal, oinfo->stopper, apriv->session->priv->ui_lock, oinfo->user_data);

	if (oinfo->depth > 0)
	{
		g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			tny_progress_info_idle_func, info, 
			tny_progress_info_destroy);
	} else {
		tny_progress_info_idle_func (info);
		tny_progress_info_destroy (info);
	}

	return;
}

static void 
tny_camel_account_start_operation_default (TnyAccount *self, TnyStatusDomain domain, TnyStatusCode code, TnyStatusCallback status_callback, gpointer status_user_data)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	if (!priv->csyncop)
	{
		RefreshStatusInfo *info = g_slice_new (RefreshStatusInfo);

		info->self = TNY_ACCOUNT (g_object_ref (self));
		info->domain = domain;
		info->code = code;
		info->status_callback = status_callback;
		info->depth = g_main_depth ();
		info->user_data = status_user_data;
		info->stopper = tny_idle_stopper_new();

		priv->csyncop = info;

		_tny_camel_account_start_camel_operation_n (TNY_CAMEL_ACCOUNT (self), 
				refresh_status, info, "Starting operation", FALSE);

	} else
		g_critical ("Another synchronous operation is already in "
				"progress. This indicates an error in the "
				"software.");

	return;
}

static void
tny_camel_account_stop_operation_default (TnyAccount *self, gboolean *canceled)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	if (priv->csyncop)
	{
		RefreshStatusInfo *info = priv->csyncop;

		tny_idle_stopper_stop (info->stopper);
		tny_idle_stopper_destroy (info->stopper);
		info->stopper = NULL;
		g_object_unref (info->self);
		g_slice_free (RefreshStatusInfo, info);
		priv->csyncop = NULL;

		if (canceled)
			*canceled = camel_operation_cancel_check (priv->cancel);

		_tny_camel_account_stop_camel_operation (TNY_CAMEL_ACCOUNT (self));
	} else
		g_critical ("No synchronous operation was in "
				"progress while trying to stop one "
				"This indicates an error in the software.");

}


static TnyConnectionStatus 
tny_camel_account_get_connection_status (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_connection_status(self);
}

static TnyConnectionStatus 
tny_camel_account_get_connection_status_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	return priv->status;
}

/**
 * tny_camel_account_set_session:
 * @self: a #TnyCamelAccount object
 * @session: a #TnySessionCamel object
 *
 * Set the #TnySessionCamel session this account will use
 *
 **/
void 
tny_camel_account_set_session (TnyCamelAccount *self, TnySessionCamel *session)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->service_lock);

	if (!priv->session)
	{
		camel_object_ref (session);
		priv->session = session;
		_tny_session_camel_register_account (session, self);

		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(self, FALSE, FALSE);
	}

	g_static_rec_mutex_unlock (priv->service_lock);

	return;
}

static void
tny_camel_account_set_id (TnyAccount *self, const gchar *id)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_id(self, id);
}

static void
tny_camel_account_set_id_default (TnyAccount *self, const gchar *id)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	/* Ignore this if it is not a change: */
	if (strings_are_equal (priv->id, id))
		return;

	if (priv->id)
		g_free (priv->id);
	priv->id = g_strdup (id);

	_tny_camel_account_emit_changed (TNY_CAMEL_ACCOUNT (self));

	return;
}

static void
tny_camel_account_set_secure_auth_mech (TnyAccount *self, const gchar *mech)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_secure_auth_mech(self, mech);
}

static void
tny_camel_account_set_secure_auth_mech_default (TnyAccount *self, const gchar *mech)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	gboolean changed = FALSE;

	/* Ignore this if it is not a change: */
	if (strings_are_equal (priv->mech, mech))
		return;

	g_static_rec_mutex_lock (priv->service_lock);

	priv->custom_url_string = FALSE;

	if (!mech)
	{
		if (priv->mech) {
			g_free (priv->mech);
			changed = TRUE;
		}
		priv->mech = NULL;
	} else if ((!priv->mech && mech) || (!(strcmp (mech, priv->mech) == 0)))
	{
		if (priv->mech)
			g_free (priv->mech);
		priv->mech = g_strdup (mech);
		changed = TRUE;
	}

	if (changed)
		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(TNY_CAMEL_ACCOUNT (self), TRUE, FALSE);

	g_static_rec_mutex_unlock (priv->service_lock);

	_tny_camel_account_emit_changed (TNY_CAMEL_ACCOUNT (self));

	return;
}

static void
tny_camel_account_set_proto (TnyAccount *self, const gchar *proto)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_proto(self, proto);
}

static void
tny_camel_account_set_proto_default (TnyAccount *self, const gchar *proto)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	gboolean changed = FALSE;

	/* Ignore this if it is not a change: */
	if (strings_are_equal (priv->proto, proto))
		return;

	g_static_rec_mutex_lock (priv->service_lock);

	priv->custom_url_string = FALSE;

	if (!proto)
	{
		if (priv->proto) {
			g_free (priv->proto);
			changed = TRUE;
		}
		priv->proto = NULL;
	} else if ((!priv->proto && proto) || (!(strcmp (proto, priv->proto) == 0)))
	{
		if (priv->proto)
			g_free (priv->proto);
		priv->proto = g_strdup (proto);
		changed = TRUE;
	}

	if (changed)
		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(TNY_CAMEL_ACCOUNT (self), TRUE, FALSE);

	g_static_rec_mutex_unlock (priv->service_lock);

	_tny_camel_account_emit_changed (TNY_CAMEL_ACCOUNT (self));

	return;
}

static void
tny_camel_account_set_user (TnyAccount *self, const gchar *user)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_user(self, user);
}

static void
tny_camel_account_set_user_default (TnyAccount *self, const gchar *user)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	gboolean changed = FALSE;

	/* Ignore this if it is not a change: */
	if (strings_are_equal (priv->user, user))
		return;

	g_static_rec_mutex_lock (priv->service_lock);

	priv->custom_url_string = FALSE;

	if (!user)
	{
		if (priv->user) {
			g_free (priv->user);
			changed = TRUE;
		}
		priv->user = NULL;
	} else if ((!priv->user && user) || (!(strcmp (user, priv->user) == 0)))
	{
		if (priv->user)
			g_free (priv->user);
		priv->user = g_strdup (user);
		changed = TRUE;
	}

	if (changed)
		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(TNY_CAMEL_ACCOUNT (self), 
			!priv->in_auth, FALSE);

	g_static_rec_mutex_unlock (priv->service_lock);

	_tny_camel_account_emit_changed (TNY_CAMEL_ACCOUNT (self));

	return;
}

static void
tny_camel_account_set_hostname (TnyAccount *self, const gchar *host)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_hostname(self, host);
}

static void
tny_camel_account_set_hostname_default (TnyAccount *self, const gchar *host)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	gboolean changed = FALSE;

	/* Ignore this if it is not a change: */
	if (strings_are_equal (priv->host, host))
		return;

	g_static_rec_mutex_lock (priv->service_lock);

	priv->custom_url_string = FALSE;

	if (!host)
	{
		if (priv->host) {
			g_free (priv->host);
			changed = TRUE;
		}
		priv->host = NULL;
	} else if ((!priv->host && host) || (!(strcmp (host, priv->host) == 0)))
	{
		if (priv->host)
			g_free (priv->host);
		priv->host = g_strdup (host);
		changed = TRUE;
	}

	if (changed)
		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(TNY_CAMEL_ACCOUNT (self), 
			TRUE, FALSE);

	g_static_rec_mutex_unlock (priv->service_lock);

	_tny_camel_account_emit_changed (TNY_CAMEL_ACCOUNT (self));

	return;
}



static void
tny_camel_account_set_port (TnyAccount *self, guint port)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_port(self, port);
}

static void
tny_camel_account_set_port_default (TnyAccount *self, guint port)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	gboolean changed = FALSE;

	/* Ignore this if it is not a change: */
	if (port == priv->port)
		return;

	g_static_rec_mutex_lock (priv->service_lock);

	priv->custom_url_string = FALSE;

	if (priv->port != (gint) port)
	{
		priv->port = (gint) port;
		changed = TRUE;
	}

	if (changed)
		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(TNY_CAMEL_ACCOUNT (self), 
			TRUE, FALSE);

	g_static_rec_mutex_unlock (priv->service_lock);

	_tny_camel_account_emit_changed (TNY_CAMEL_ACCOUNT (self));

	return;
}


static void
tny_camel_account_set_pass_func (TnyAccount *self, TnyGetPassFunc get_pass_func)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_pass_func (self, get_pass_func);
}

static void
tny_camel_account_set_pass_func_default (TnyAccount *self, TnyGetPassFunc get_pass_func)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	gboolean reconf_if = FALSE;
	gboolean changed = FALSE;

	g_static_rec_mutex_lock (priv->service_lock);

	if (priv->get_pass_func != get_pass_func)
		changed = TRUE;

	priv->get_pass_func = get_pass_func;
	priv->pass_func_set = TRUE;

	if (changed)
		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(TNY_CAMEL_ACCOUNT (self), 
			reconf_if, TRUE);

	if (priv->session)
		_tny_session_camel_activate_account (priv->session, TNY_CAMEL_ACCOUNT (self));

	g_static_rec_mutex_unlock (priv->service_lock);

	return;
}

static void
tny_camel_account_set_forget_pass_func(TnyAccount *self, TnyForgetPassFunc get_forget_pass_func)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_forget_pass_func (self, get_forget_pass_func);
}

static void
tny_camel_account_set_forget_pass_func_default (TnyAccount *self, TnyForgetPassFunc get_forget_pass_func)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	gboolean reconf_if = FALSE;
	gboolean changed = FALSE;

	/* Ignore this if it is not a change: */
	if (get_forget_pass_func == priv->forget_pass_func)
		return;

	g_static_rec_mutex_lock (priv->service_lock);

	if (priv->forget_pass_func != get_forget_pass_func)
		changed = TRUE;

	priv->forget_pass_func = get_forget_pass_func;
	priv->forget_pass_func_set = TRUE;

	if (changed)
		TNY_CAMEL_ACCOUNT_GET_CLASS (self)->prepare(TNY_CAMEL_ACCOUNT (self), 
			reconf_if, FALSE);

	g_static_rec_mutex_unlock (priv->service_lock);

	return;
}

static const gchar*
tny_camel_account_get_id (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_id(self);
}

static const gchar*
tny_camel_account_get_id_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	const gchar *retval;

	retval = (const gchar*)priv->id;

	return retval;
}

static const gchar*
tny_camel_account_get_secure_auth_mech (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_secure_auth_mech(self);
}

static const gchar*
tny_camel_account_get_secure_auth_mech_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	const gchar *retval;

	retval = (const gchar*)priv->mech;

	return retval;
}

static const gchar*
tny_camel_account_get_proto (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_proto(self);
}

static const gchar*
tny_camel_account_get_proto_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	const gchar *retval;

	retval = (const gchar*)priv->proto;

	return retval;
}

static const gchar*
tny_camel_account_get_user (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_user(self);
}

static const gchar*
tny_camel_account_get_user_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	const gchar *retval;

	retval = (const gchar*)priv->user;

	return retval;
}

static const gchar*
tny_camel_account_get_hostname (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_hostname(self);
}

static const gchar*
tny_camel_account_get_hostname_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	const gchar *retval;

	retval = (const gchar*)priv->host;

	return retval;
}


static void 
tny_camel_account_cancel (TnyAccount *self)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->cancel(self);
}

#if 0 /* Not used. */
static gpointer
camel_cancel_hack_thread (gpointer data)
{
	camel_operation_cancel (NULL);

	g_thread_exit (NULL);
	return NULL;
}
#endif

void 
_tny_camel_account_actual_cancel (TnyCamelAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->cancel_lock);
	if (priv->cancel)
		camel_operation_cancel (priv->cancel);
	if (priv->getmsg_cancel)
		camel_operation_cancel (priv->getmsg_cancel);
	g_static_rec_mutex_unlock (priv->cancel_lock);

	return;
}


void 
_tny_camel_account_actual_uncancel (TnyCamelAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	g_static_rec_mutex_lock (priv->cancel_lock);
	if (priv->cancel)
		camel_operation_uncancel (priv->cancel); 
	g_static_rec_mutex_unlock (priv->cancel_lock);

	return;
}

static void 
tny_camel_account_cancel_default (TnyAccount *self)
{
	if (TNY_IS_CAMEL_STORE_ACCOUNT (self))
	{
		TnyCamelStoreAccountPriv *priv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (self);
		TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

		_tny_camel_queue_cancel_remove_items (priv->queue, 
			TNY_CAMEL_QUEUE_GET_HEADERS_ITEM|TNY_CAMEL_QUEUE_SYNC_ITEM|
			TNY_CAMEL_QUEUE_REFRESH_ITEM|TNY_CAMEL_QUEUE_CANCELLABLE_ITEM);

		g_static_rec_mutex_lock (apriv->cancel_lock);
		if (apriv->getmsg_cancel)
			camel_operation_cancel (apriv->getmsg_cancel);
		g_static_rec_mutex_unlock (apriv->cancel_lock);

	} else {
		_tny_camel_account_actual_cancel (TNY_CAMEL_ACCOUNT (self));
	}

	return;
}


static guint 
tny_camel_account_get_port (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_port(self);
}

static guint 
tny_camel_account_get_port_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	return (guint)priv->port;
}

static TnyGetPassFunc
tny_camel_account_get_pass_func (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_pass_func (self);
}

static TnyGetPassFunc
tny_camel_account_get_pass_func_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	TnyGetPassFunc retval;

	retval = priv->get_pass_func;

	return retval;
}

static TnyForgetPassFunc
tny_camel_account_get_forget_pass_func (TnyAccount *self)
{
	return TNY_CAMEL_ACCOUNT_GET_CLASS (self)->get_forget_pass_func (self);
}

static TnyForgetPassFunc
tny_camel_account_get_forget_pass_func_default (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	TnyForgetPassFunc retval;

	retval = priv->forget_pass_func;

	return retval;
}

const CamelService*
_tny_camel_account_get_service (TnyCamelAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	const CamelService *retval;

	retval = (const CamelService *)priv->service;

	return retval;
}

const gchar* 
_tny_camel_account_get_url_string (TnyCamelAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	const gchar *retval;

	retval = (const gchar*)priv->url_string;

	return retval;
}


static void
tny_camel_account_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelAccount *self = (TnyCamelAccount *)instance;
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);

	priv->retry_connect = FALSE;
	priv->con_strat = tny_camel_default_connection_policy_new ();
	priv->queue = _tny_camel_queue_new (self);
	priv->delete_this = NULL;
	priv->is_ready = FALSE;
	priv->is_connecting = FALSE;
	priv->in_auth = FALSE;
	priv->csyncop = NULL;
	priv->get_pass_func = NULL;
	priv->forget_pass_func = NULL;
	priv->port = -1;
	priv->cache_location = NULL;
	priv->service = NULL;
	priv->session = NULL;
	priv->url_string = NULL;
	priv->status = TNY_CONNECTION_STATUS_INIT;

	priv->ex = camel_exception_new ();
	camel_exception_init (priv->ex);

	priv->custom_url_string = FALSE;
	priv->inuse_spin = FALSE;

	priv->name = NULL;
	priv->options = NULL;
	priv->id = NULL;
	priv->user = NULL;
	priv->host = NULL;
	priv->proto = NULL;
	priv->mech = NULL;
	priv->forget_pass_func_set = FALSE;
	priv->pass_func_set = FALSE;
	priv->cancel = NULL;
	priv->getmsg_cancel = NULL;

	priv->service_lock = g_new (GStaticRecMutex, 1);
	g_static_rec_mutex_init (priv->service_lock);
	priv->account_lock = g_new (GStaticRecMutex, 1);
	g_static_rec_mutex_init (priv->account_lock);
	priv->cancel_lock = g_new (GStaticRecMutex, 1);
	g_static_rec_mutex_init (priv->cancel_lock);

	return;
}

/* The is the protected version that will actually set it online. This should
 * always happen in a thread. In fact, it will always happen in the operations
 * queue of @self (else it's a bug). */

typedef struct {
	TnySessionCamel *session;
	TnyAccount *account;
	gboolean online;
} SetOnlInfo;


static gboolean
set_online_happened_idle (gpointer user_data)
{
	SetOnlInfo *info = (SetOnlInfo *) user_data;
	tny_lockable_lock (info->session->priv->ui_lock);
	g_signal_emit (info->account, 
		tny_camel_account_signals [TNY_CAMEL_ACCOUNT_SET_ONLINE_HAPPENED], 
		0, info->online);
	tny_lockable_unlock (info->session->priv->ui_lock);
	return FALSE;
}

static void
set_online_happened_destroy (gpointer user_data)
{
	SetOnlInfo *info = (SetOnlInfo *) user_data;
	camel_object_unref (info->session);
	g_object_unref (info->account);
	g_slice_free (SetOnlInfo, info);
	return;
}

static void
set_online_happened (TnySessionCamel *session, TnyCamelAccount *account, gboolean online)
{
	SetOnlInfo *info = g_slice_new (SetOnlInfo);

	info->session = session;
	camel_object_ref (info->session);
	info->account = TNY_ACCOUNT (g_object_ref (account));
	info->online = online;

	g_idle_add_full (G_PRIORITY_DEFAULT, set_online_happened_idle, 
		info, set_online_happened_destroy);

	return;
}


void 
_tny_camel_account_set_online (TnyCamelAccount *self, gboolean online, GError **err)
{
	/* Note that the human-readable GError:message strings here are only for debugging.
	 * They should never be shown to the user. The application should make its own 
	 * decisions about how to show these errors in the UI, and should make sure that 
	 * they are translated in the user's locale.
	 */

	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;

	if (!priv->service || !CAMEL_IS_SERVICE (priv->service))
	{
		if (camel_exception_is_set (priv->ex))
		{
			_tny_camel_exception_to_tny_error (priv->ex, err);
			camel_exception_clear (priv->ex);
		} else {
			g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_SERVICE_ERROR_CONNECT,
				"Account not yet fully configured. "
				"This problem indicates a bug in the software.");
		}


		return;
	}

	camel_object_ref (priv->service);

	if (CAMEL_IS_DISCO_STORE (priv->service)) {
		if (online) {
			camel_disco_store_set_status (CAMEL_DISCO_STORE (priv->service),
					CAMEL_DISCO_STORE_ONLINE, &ex);

			if (!camel_exception_is_set (&ex))
				camel_service_connect (CAMEL_SERVICE (priv->service), &ex);

			if (TNY_IS_CAMEL_STORE_ACCOUNT (self)) 
			{
				if (!camel_exception_is_set (&ex))
					priv->status = TNY_CONNECTION_STATUS_CONNECTED;
				else
					priv->status = TNY_CONNECTION_STATUS_CONNECTED_BROKEN;

				_tny_camel_store_account_emit_conchg_signal (TNY_CAMEL_STORE_ACCOUNT (self));
			}

			if (!camel_exception_is_set (&ex)) {
				if (TNY_IS_CAMEL_STORE_ACCOUNT (self))
					camel_store_restore (CAMEL_STORE (priv->service)); 
				set_online_happened (priv->session, self, TRUE);
			} 

			goto done;

		} else if (camel_disco_store_can_work_offline (CAMEL_DISCO_STORE (priv->service))) {
			
			camel_disco_store_set_status (CAMEL_DISCO_STORE (priv->service),
					CAMEL_DISCO_STORE_OFFLINE, &ex);

			if (TNY_IS_CAMEL_STORE_ACCOUNT (self)) 
			{
				if (!camel_exception_is_set (&ex))
					priv->status = TNY_CONNECTION_STATUS_DISCONNECTED;
				else
					priv->status = TNY_CONNECTION_STATUS_DISCONNECTED_BROKEN;

				_tny_camel_store_account_emit_conchg_signal (TNY_CAMEL_STORE_ACCOUNT (self));
			}

			goto done;
		}
	} else if (CAMEL_IS_OFFLINE_STORE (priv->service)) {
		
		if (online) {
			
			camel_offline_store_set_network_state (CAMEL_OFFLINE_STORE (priv->service),
					CAMEL_OFFLINE_STORE_NETWORK_AVAIL, &ex);

			if (TNY_IS_CAMEL_STORE_ACCOUNT (self)) 
			{
				if (!camel_exception_is_set (&ex))
					priv->status = TNY_CONNECTION_STATUS_CONNECTED;
				else
					priv->status = TNY_CONNECTION_STATUS_CONNECTED_BROKEN;

				_tny_camel_store_account_emit_conchg_signal (TNY_CAMEL_STORE_ACCOUNT (self));
			}

			if (!camel_exception_is_set (&ex))
				set_online_happened (priv->session, self, TRUE);

			goto done;
		} else {
			camel_offline_store_set_network_state (CAMEL_OFFLINE_STORE (priv->service),
					CAMEL_OFFLINE_STORE_NETWORK_UNAVAIL, &ex);

			if (TNY_IS_CAMEL_STORE_ACCOUNT (self)) 
			{
				if (!camel_exception_is_set (&ex))
					priv->status = TNY_CONNECTION_STATUS_DISCONNECTED;
				else
					priv->status = TNY_CONNECTION_STATUS_DISCONNECTED_BROKEN;

				_tny_camel_store_account_emit_conchg_signal (TNY_CAMEL_STORE_ACCOUNT (self));
			}

			goto done;
		}
	}

	if (!online) {
		camel_service_disconnect (CAMEL_SERVICE (priv->service),
			  TRUE, &ex);

		if (TNY_IS_CAMEL_STORE_ACCOUNT (self)) 
		{
			if (!camel_exception_is_set (&ex))
				priv->status = TNY_CONNECTION_STATUS_DISCONNECTED;
			else
				priv->status = TNY_CONNECTION_STATUS_DISCONNECTED_BROKEN;

			_tny_camel_store_account_emit_conchg_signal (TNY_CAMEL_STORE_ACCOUNT (self));
		}

		if (!camel_exception_is_set (&ex))
			set_online_happened (priv->session, self, FALSE);
	}

done:

	if (camel_exception_is_set (&ex))
	{
		_tny_camel_exception_to_tny_error (&ex, err);
		camel_exception_clear (&ex);
	}

	camel_object_unref (priv->service);

	return;
}





/**
 * tny_camel_account_set_online:
 * @self: a #TnyCamelAccount object
 * @online: whether or not the account is online
 * @callback: a callback when the account went online
 * @user_data: user data for the callback
 *
 * Set the connectivity status of an account. Setting this to FALSE means that 
 * the account will not attempt to use the network, and will use only the cache.
 * Setting this to TRUE means that the account may use the network to 
 * provide up-to-date information.
 *
 * The @callback will be invoke as soon as the account is actually online. It's
 * guaranteed that the @callback will happen in the mainloop, if available.
 *
 * This is a cancelable operation which means that if another cancelable 
 * operation executes, this operation will be aborted. Being aborted means that
 * the callback will still be called, but with cancelled=TRUE.
 *
 * Only one instance of @tny_camel_account_set_online for folder @self can run
 * at the same time. If you call for another, the first will be aborted. This
 * means that it's callback will be called with cancelled=TRUE. If the 
 * TnyDevice's online state changes while operation is taking place, the 
 * behaviour is undefined. Although the @callback will always happen and in that
 * case with cancelled=TRUE.
 **/
void 
tny_camel_account_set_online (TnyCamelAccount *self, gboolean online, TnyCamelSetOnlineCallback callback, gpointer user_data)
{
	TNY_CAMEL_ACCOUNT_GET_CLASS (self)->set_online(self, online, callback, user_data);
}

typedef struct
{
	TnyCamelAccount *account;
	GError *err;
	TnyCamelSetOnlineCallback callback;
	gpointer user_data;
	gboolean cancel;

	GCond* condition;
	gboolean had_callback;
	GMutex *mutex;

} OnSetOnlineInfo;

static gboolean 
on_set_online_done_idle_func (gpointer data)
{
	OnSetOnlineInfo *info = (OnSetOnlineInfo *) data;
	TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (info->account);
	TnySessionCamel *session = apriv->session;

	if (info->callback) {
		tny_lockable_lock (session->priv->ui_lock);
		info->callback (info->account, info->cancel, info->err, info->user_data);
		tny_lockable_unlock (session->priv->ui_lock);
	}
	return FALSE;
}

static void 
on_set_online_done_destroy(gpointer data)
{
	OnSetOnlineInfo *info = (OnSetOnlineInfo *) data;

	/* We copied it, so we must also free it */
	if (info->err)
		g_error_free (info->err);

	/* Thread reference */
	g_object_unref (info->account);

	if (info->condition) {
		g_mutex_lock (info->mutex);
		g_cond_broadcast (info->condition);
		info->had_callback = TRUE;
		g_mutex_unlock (info->mutex);
	} else /* it's a transport account */
		g_slice_free (OnSetOnlineInfo, data);

	return;
}


void 
tny_camel_account_set_online_default (TnyCamelAccount *self, gboolean online, TnyCamelSetOnlineCallback callback, gpointer user_data)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	TnySessionCamel *session = priv->session;

	/* In case we  are a store account, this means that we need to throw the 
	 * request to go online to the account's queue. */

	if (session == NULL)
		return;

	if (TNY_IS_CAMEL_STORE_ACCOUNT (self)) {
		priv->retry_connect = TRUE;
		if (online)
			camel_session_set_online ((CamelSession *) session, TRUE); 
		_tny_camel_store_account_queue_going_online (
			TNY_CAMEL_STORE_ACCOUNT (self), session, online, 
			callback, user_data);
	}


	/* Else, if it's a transport account, we don't have any transport 
	 * account implementations that actually need to go online at this 
	 * moment yet. At the moment of transferring the first message, the
	 * current implementations will automatically connect themselves. */

	if (TNY_IS_CAMEL_TRANSPORT_ACCOUNT (self)) 
	{
		OnSetOnlineInfo *info = g_slice_new0 (OnSetOnlineInfo);

		set_online_happened (session, self, online);

		info->err = NULL;
		info->callback = callback;
		info->user_data = user_data;
		info->mutex = NULL;
		info->condition = NULL;
		info->had_callback = FALSE;
		info->account = TNY_CAMEL_ACCOUNT (g_object_ref (self));

		g_idle_add_full (G_PRIORITY_HIGH, 
			on_set_online_done_idle_func, 
			info, on_set_online_done_destroy);

	}
}

static gboolean
tny_camel_account_is_ready (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	return priv->is_ready;
}


static TnyConnectionPolicy* 
tny_camel_account_get_connection_policy (TnyAccount *self)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	return TNY_CONNECTION_POLICY (g_object_ref (priv->con_strat));
}

static void 
tny_camel_account_set_connection_policy (TnyAccount *self, TnyConnectionPolicy *policy)
{
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	g_object_unref (priv->con_strat);
	priv->con_strat = TNY_CONNECTION_POLICY (g_object_ref (policy));
	return;
}





typedef struct 
{
	TnyCamelQueueable parent;

	TnyCamelAccount *self;
	TnyCamelGetSupportedSecureAuthCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	gboolean cancelled;
	TnyList *result;
	TnyIdleStopper* stopper;
	GError *err;
	TnySessionCamel *session;

} GetSupportedAuthInfo;


static gboolean 
on_supauth_idle_func (gpointer user_data)
{
	GetSupportedAuthInfo *info = (GetSupportedAuthInfo *) user_data;

	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, info->cancelled, info->result, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}

	tny_idle_stopper_stop (info->stopper);
	
	return FALSE; /* Don't call this again. */
}


static gboolean 
on_supauth_cancelled_idle_func (gpointer user_data)
{
	GetSupportedAuthInfo *info = (GetSupportedAuthInfo *) user_data;

	if (info->callback) {
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (info->self, TRUE, info->result, info->err, info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}

	tny_idle_stopper_stop (info->stopper);

	return FALSE; /* Don't call this again. */
}

static void 
on_supauth_destroy_func (gpointer user_data)
{
	GetSupportedAuthInfo *info = (GetSupportedAuthInfo *) user_data;

	/* Thread reference */
	g_object_unref (info->self);
	camel_object_ref (info->session);

	/* Result reference */
	if (info->result)
		g_object_unref (info->result);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	if (info->err) {
		g_error_free (info->err);
		info->err = NULL;
	}

	return;
}

static void 
on_supauth_destroy_and_kill_func (gpointer user_data)
{
	on_supauth_destroy_func (user_data);
}

/* Starts the operation in the thread: */
static gpointer 
tny_camel_account_get_supported_secure_authentication_async_thread (
	gpointer thr_user_data)
{
	GetSupportedAuthInfo *info = thr_user_data;
	TnyCamelAccount *self = info->self;
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	TnyStatus* status;
	GList *authtypes = NULL;
	TnyList *result = NULL;
	GList *iter = NULL;

	g_static_rec_mutex_lock (priv->service_lock);

	status =  tny_status_new_literal(TNY_GET_SUPPORTED_SECURE_AUTH_STATUS, 
		TNY_GET_SUPPORTED_SECURE_AUTH_STATUS_GET_SECURE_AUTH, 0, 1,
		"Get secure authentication methods");

	/* Prepare, so service is set for query_auth_types */
	TNY_CAMEL_ACCOUNT_GET_CLASS(self)->prepare (self, FALSE, TRUE);

	authtypes = camel_service_query_auth_types (priv->service, &ex);
	
	/* Result reference */
	result = tny_simple_list_new ();
	iter = authtypes;

	while (iter) 
	{
		CamelServiceAuthType *item = (CamelServiceAuthType *)iter->data;
		if (item) {
			TnyPair *pair = tny_pair_new (item->name, NULL);
			tny_list_append (result, G_OBJECT (pair));
			g_object_unref (pair);
		}
		iter = g_list_next (iter);
	}

	g_list_free (authtypes);
	authtypes = NULL;
	info->result = result;

	info->err = NULL;
	if (camel_exception_is_set (&ex)) {
		_tny_camel_exception_to_tny_error (&ex, &info->err);
		camel_exception_clear (&ex);
	}

	g_static_rec_mutex_unlock (priv->service_lock);

	tny_status_free(status);

	return NULL;
}


/** 
 * TnyCamelGetSupportedSecureAuthCallback:
 * @self: The TnyCamelAccount on which tny_camel_account_get_supported_secure_authentication() was called.
 * @cancelled: Whether the operation was cancelled.
 * @auth_types: A TnyList of TnyPair objects. Each TnyPair in the list has a supported secure authentication method name as its name. This list must be freed with g_object_unref().
 * @err: A GError if an error occurred, or NULL.
 * @user_data: The user data that was provided to tny_camel_account_get_supported_secure_authentication().
 * 
 * The callback for tny_camel_account_get_supported_secure_authentication().
 **/

/**
 * tny_camel_account_get_supported_secure_authentication:
 * @self: a #TnyCamelAccount object.
 * @callback: A function to be called when the operation is complete.
 * @status_callback: A function to be called one or more times while the operation is in progress.
 * @user_data: Data to be passed to the callback and status callback.
 * 
 * Query the server for the list of supported secure authentication mechanisms.
 * The #TnyCamelAccount must have a valid hostname and the port number 
 * must be set if appropriate.
 * The returned strings may be used as parameters to 
 * tny_account_set_secure_auth_mech().
 **/
void 
tny_camel_account_get_supported_secure_authentication (TnyCamelAccount *self, TnyCamelGetSupportedSecureAuthCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{ 
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	GetSupportedAuthInfo *info = NULL;
	GError *err = NULL;

	if (!_tny_session_check_operation (priv->session, TNY_ACCOUNT (self), &err, 
			TNY_ERROR_DOMAIN, TNY_SERVICE_ERROR_UNKNOWN))
	{
		if (callback) {
			callback (self, TRUE /* cancelled */, NULL, err, user_data);
		}

		if (err)
			g_error_free (err);
		
		return;
	}

	if (err) {
		g_error_free (err);
		err = NULL;
	}

	info = g_slice_new (GetSupportedAuthInfo);
	info->session = priv->session;
	info->err = NULL;
	info->self = self;
	info->callback = callback;
	info->result = NULL;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->cancelled = FALSE;
	info->stopper = tny_idle_stopper_new();

	/* thread reference */
	g_object_ref (self);
	camel_object_ref (info->session);


	if (TNY_IS_CAMEL_STORE_ACCOUNT (self)) 
	{
		TnyCamelStoreAccountPriv *aspriv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (self);
		_tny_camel_queue_launch (aspriv->queue, 
			tny_camel_account_get_supported_secure_authentication_async_thread, 
			on_supauth_idle_func, on_supauth_destroy_func, 
			on_supauth_cancelled_idle_func, on_supauth_destroy_and_kill_func, 
			&info->cancelled, info, sizeof (GetSupportedAuthInfo),
			__FUNCTION__);
	} else {
		/* TnyCamelQueue *temp_queue = _tny_camel_queue_new (self); */
		g_thread_create (tny_camel_account_get_supported_secure_authentication_async_thread,
			info, FALSE, NULL);
	}
}


static void
tny_camel_account_finalize (GObject *object)
{
	TnyCamelAccount *self = (TnyCamelAccount *)object;
	TnyCamelAccountPriv *priv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (self);
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;

	if (priv->service && CAMEL_IS_SERVICE (priv->service))
	{
		priv->service->connecting = NULL;
		priv->service->disconnecting = NULL;
		priv->service->reconnecter = NULL;
		priv->service->reconnection = NULL;
		camel_service_disconnect (CAMEL_SERVICE (priv->service), FALSE, &ex);
	}

	if (priv->session) {
		_tny_session_camel_unregister_account (priv->session, (TnyCamelAccount*) object);    
		camel_object_unref (priv->session);
	}
	_tny_camel_account_start_camel_operation (self, NULL, NULL, NULL);
	_tny_camel_account_stop_camel_operation (self);

	g_static_rec_mutex_lock (priv->cancel_lock);
	if (G_UNLIKELY (priv->cancel))
		camel_operation_unref (priv->cancel);
	if (G_UNLIKELY (priv->getmsg_cancel))
		camel_operation_unref (priv->getmsg_cancel);
	g_static_rec_mutex_unlock (priv->cancel_lock);
	
	if (priv->csyncop) {
  		RefreshStatusInfo *info = priv->csyncop;
		tny_idle_stopper_stop (info->stopper);
		tny_idle_stopper_destroy (info->stopper);
		info->stopper = NULL;
		g_object_unref (info->self);
		g_slice_free (RefreshStatusInfo, info);
		priv->csyncop = NULL; 
	}

	/* g_static_rec_mutex_free (priv->cancel_lock); */
	g_free (priv->cancel_lock);
	priv->cancel_lock = NULL;
	priv->inuse_spin = FALSE;

	if (priv->options) {
		g_list_foreach (priv->options, (GFunc)g_free, NULL);
		g_list_free (priv->options);
	}

	g_static_rec_mutex_lock (priv->service_lock);

	if (priv->service && CAMEL_IS_OBJECT (priv->service)) {
		if (priv->service->url) {
			/* known leak to enforce creating a new service */
			priv->service->url->user = g_strdup ("non existing dummy user");
		}
		camel_object_unref (CAMEL_OBJECT (priv->service));
		priv->service = NULL;
	}

	if (G_LIKELY (priv->cache_location))
		g_free (priv->cache_location);

	if (G_LIKELY (priv->id))
		g_free (priv->id);

	if (G_LIKELY (priv->name))
		g_free (priv->name);

	if (G_LIKELY (priv->user))
		g_free (priv->user);

	if (G_LIKELY (priv->host))
		g_free (priv->host);

	if (G_LIKELY (priv->mech))
		g_free (priv->mech);

	if (G_LIKELY (priv->proto))
		g_free (priv->proto);

	if (G_LIKELY (priv->url_string))
		g_free (priv->url_string);

	if (priv->delete_this && strlen (priv->delete_this) > 0)
		camel_rm (priv->delete_this);
	if (priv->delete_this)
		g_free (priv->delete_this);
	priv->delete_this = NULL;

	g_static_rec_mutex_unlock (priv->service_lock);

	g_object_unref (priv->queue);
	g_object_unref (priv->con_strat);

	camel_exception_free (priv->ex);

	/* g_static_rec_mutex_free (priv->service_lock); */
	g_free (priv->service_lock);
	priv->service_lock = NULL;

	/* g_static_rec_mutex_free (priv->account_lock); */
	g_free (priv->account_lock);
	priv->account_lock = NULL;

	(*parent_class->finalize) (object);

	return;
}


static void
tny_account_init (gpointer g, gpointer iface_data)
{
	TnyAccountIface *klass = (TnyAccountIface *)g;

	klass->get_port= tny_camel_account_get_port;
	klass->set_port= tny_camel_account_set_port;
	klass->get_hostname= tny_camel_account_get_hostname;
	klass->set_hostname= tny_camel_account_set_hostname;
	klass->get_proto= tny_camel_account_get_proto;
	klass->set_proto= tny_camel_account_set_proto;
	klass->get_secure_auth_mech= tny_camel_account_get_secure_auth_mech;
	klass->set_secure_auth_mech= tny_camel_account_set_secure_auth_mech;
	klass->get_user= tny_camel_account_get_user;
	klass->set_user= tny_camel_account_set_user;
	klass->get_pass_func = tny_camel_account_get_pass_func;
	klass->set_pass_func = tny_camel_account_set_pass_func;
	klass->get_forget_pass_func = tny_camel_account_get_forget_pass_func;
	klass->set_forget_pass_func = tny_camel_account_set_forget_pass_func;
	klass->set_id= tny_camel_account_set_id;
	klass->get_id= tny_camel_account_get_id;
	klass->get_connection_status= tny_camel_account_get_connection_status;
	klass->set_url_string= tny_camel_account_set_url_string;
	klass->get_url_string= tny_camel_account_get_url_string;
	klass->get_name= tny_camel_account_get_name;
	klass->set_name= tny_camel_account_set_name;
	klass->get_account_type= tny_camel_account_get_account_type;
	klass->cancel= tny_camel_account_cancel;
	klass->matches_url_string= tny_camel_account_matches_url_string;
	klass->start_operation= tny_camel_account_start_operation;
	klass->stop_operation=  tny_camel_account_stop_operation;
	klass->is_ready= tny_camel_account_is_ready;
	klass->set_connection_policy= tny_camel_account_set_connection_policy;
	klass->get_connection_policy= tny_camel_account_get_connection_policy;

	return;
}

static void 
tny_camel_account_class_init (TnyCamelAccountClass *class)
{
	GObjectClass *object_class;
	static gboolean initialized = FALSE;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;


	if (!initialized) {
		/* create interface signals here. */

/**
 * TnyCamelAccount::set-online-happened
 * @self: the object on which the signal is emitted
 * @online: whether it was online
 * @user_data: user data set when the signal handler was connected.
 *
 * Emitted when tny_camel_account_set_online happened
 **/
		tny_camel_account_signals[TNY_CAMEL_ACCOUNT_SET_ONLINE_HAPPENED] =
		   g_signal_new ("set_online_happened",
			TNY_TYPE_CAMEL_ACCOUNT,
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (TnyCamelAccountClass, set_online_happened),
			NULL, NULL,
			g_cclosure_marshal_VOID__BOOLEAN, 
			G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	}

	class->get_port= tny_camel_account_get_port_default;
	class->set_port= tny_camel_account_set_port_default;
	class->get_hostname= tny_camel_account_get_hostname_default;
	class->set_hostname= tny_camel_account_set_hostname_default;
	class->get_proto= tny_camel_account_get_proto_default;
	class->set_proto= tny_camel_account_set_proto_default;
	class->get_secure_auth_mech= tny_camel_account_get_secure_auth_mech_default;
	class->set_secure_auth_mech= tny_camel_account_set_secure_auth_mech_default;
	class->get_user= tny_camel_account_get_user_default;
	class->set_user= tny_camel_account_set_user_default;
	class->get_pass_func = tny_camel_account_get_pass_func_default;
	class->set_pass_func = tny_camel_account_set_pass_func_default;
	class->get_forget_pass_func = tny_camel_account_get_forget_pass_func_default;
	class->set_forget_pass_func = tny_camel_account_set_forget_pass_func_default;
	class->set_id= tny_camel_account_set_id_default;
	class->get_id= tny_camel_account_get_id_default;
	class->get_connection_status= tny_camel_account_get_connection_status_default;
	class->set_url_string= tny_camel_account_set_url_string_default;
	class->get_url_string= tny_camel_account_get_url_string_default;
	class->get_name= tny_camel_account_get_name_default;
	class->set_name= tny_camel_account_set_name_default;
	class->get_account_type= tny_camel_account_get_account_type_default;
	class->cancel= tny_camel_account_cancel_default;
	class->matches_url_string= tny_camel_account_matches_url_string_default;
	class->start_operation= tny_camel_account_start_operation_default;
	class->stop_operation=  tny_camel_account_stop_operation_default;

	class->add_option= tny_camel_account_add_option_default;
	class->clear_options= tny_camel_account_clear_options_default;
	class->get_options= tny_camel_account_get_options_default;

	class->set_online= tny_camel_account_set_online_default;

	object_class->finalize = tny_camel_account_finalize;

	g_type_class_add_private (object_class, sizeof (TnyCamelAccountPriv));

	return;
}

static gpointer 
tny_camel_account_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelAccountClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_account_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelAccount),
			0,      /* n_preallocs */
			tny_camel_account_instance_init    /* instance_init */
		};
	
	static const GInterfaceInfo tny_account_info = 
		{
			(GInterfaceInitFunc) tny_account_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelAccount",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_ACCOUNT, 
				     &tny_account_info);
	

	return GUINT_TO_POINTER (type);
}

/**
 * tny_camel_account_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_account_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);
		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_account_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
