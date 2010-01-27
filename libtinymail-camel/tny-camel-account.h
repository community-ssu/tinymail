#ifndef TNY_CAMEL_ACCOUNT_H
#define TNY_CAMEL_ACCOUNT_H

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

#include <glib.h>
#include <glib-object.h>
#include <tny-camel-shared.h>
#include <tny-account.h>
#include <tny-pair.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_ACCOUNT             (tny_camel_account_get_type ())
#define TNY_CAMEL_ACCOUNT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_ACCOUNT, TnyCamelAccount))
#define TNY_CAMEL_ACCOUNT_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_ACCOUNT, TnyCamelAccountClass))
#define TNY_IS_CAMEL_ACCOUNT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_ACCOUNT))
#define TNY_IS_ACAMEL_CCOUNT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_ACCOUNT))
#define TNY_CAMEL_ACCOUNT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_ACCOUNT, TnyCamelAccountClass))

/* This is an abstract type, you cannot (should not) instantiate it */

typedef struct _TnyCamelAccount TnyCamelAccount;
typedef struct _TnyCamelAccountClass TnyCamelAccountClass;

enum _TnyCamelAccountSignal
{
	TNY_CAMEL_ACCOUNT_SET_ONLINE_HAPPENED,
	TNY_CAMEL_ACCOUNT_LAST_SIGNAL
};

extern guint tny_camel_account_signals [TNY_CAMEL_ACCOUNT_LAST_SIGNAL];

typedef void (*TnyCamelSetOnlineCallback) (TnyCamelAccount *account, gboolean canceled, GError *err, gpointer user_data);


struct _TnyCamelAccount
{
	GObject parent;
};

struct _TnyCamelAccountClass 
{
	GObjectClass parent;

	/* Virtual methods */
	TnyConnectionStatus (*get_connection_status)(TnyAccount *self);
	void (*set_id) (TnyAccount *self, const gchar *id);
	void (*set_name) (TnyAccount *self, const gchar *name);
	void (*set_secure_auth_mech) (TnyAccount *self, const gchar *name);
	void (*set_proto) (TnyAccount *self, const gchar *proto);
	void (*set_user) (TnyAccount *self, const gchar *user);
	void (*set_hostname) (TnyAccount *self, const gchar *host);
	void (*set_port) (TnyAccount *self, guint port);
	void (*set_url_string) (TnyAccount *self, const gchar *url_string);
	void (*set_pass_func) (TnyAccount *self, TnyGetPassFunc get_pass_func);
	void (*set_forget_pass_func) (TnyAccount *self, TnyForgetPassFunc get_forget_pass_func);
	TnyGetPassFunc (*get_pass_func) (TnyAccount *self);
	TnyForgetPassFunc (*get_forget_pass_func) (TnyAccount *self);
	const gchar* (*get_id) (TnyAccount *self);
	const gchar* (*get_name) (TnyAccount *self);
	const gchar* (*get_secure_auth_mech) (TnyAccount *self);
	const gchar* (*get_proto) (TnyAccount *self);
	const gchar* (*get_user) (TnyAccount *self);
	const gchar* (*get_hostname) (TnyAccount *self);
	guint (*get_port) (TnyAccount *self);
	gchar* (*get_url_string) (TnyAccount *self);
	TnyAccountType (*get_account_type) (TnyAccount *self);
	void (*try_connect) (TnyAccount *self, GError **err);
	void (*cancel) (TnyAccount *self);
	gboolean (*matches_url_string) (TnyAccount *self, const gchar *url_string);
	void (*start_operation) (TnyAccount *self, TnyStatusDomain domain, TnyStatusCode code, TnyStatusCallback status_callback, gpointer status_user_data);
	void (*stop_operation) (TnyAccount *self, gboolean *canceled);

	void (*add_option) (TnyCamelAccount *self, TnyPair *option);
	void (*clear_options) (TnyCamelAccount *self);
	void (*get_options) (TnyCamelAccount *self, TnyList *options);

	void (*set_online) (TnyCamelAccount *self, gboolean online, TnyCamelSetOnlineCallback callback, gpointer user_data);

	/* Abstract methods */
	void (*prepare) (TnyCamelAccount *self, gboolean recon_if, gboolean reservice);

	/* Signals */
	void (*set_online_happened) (TnyCamelAccount *self, gboolean online);

};

GType tny_camel_account_get_type (void);

void tny_camel_account_get_options (TnyCamelAccount *self, TnyList *options);
void tny_camel_account_clear_options (TnyCamelAccount *self);
void tny_camel_account_add_option (TnyCamelAccount *self, TnyPair *option);
void tny_camel_account_set_session (TnyCamelAccount *self, TnySessionCamel *session);
void tny_camel_account_set_online (TnyCamelAccount *self, gboolean online, TnyCamelSetOnlineCallback callback, gpointer user_data);


typedef void (*TnyCamelGetSupportedSecureAuthCallback) (TnyCamelAccount *self, gboolean cancelled, TnyList *auth_types, GError *err, gpointer user_data);
void tny_camel_account_get_supported_secure_authentication(TnyCamelAccount *self, TnyCamelGetSupportedSecureAuthCallback callback, TnyStatusCallback status_callback, gpointer user_data);

G_END_DECLS

#endif

