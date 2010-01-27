#ifndef TNY_CAMEL_ACCOUNT_PRIV_H
#define TNY_CAMEL_ACCOUNT_PRIV_H

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

#include <tny-session-camel.h>

#include <tny-status.h>

#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include <camel/camel-operation.h>

#include "tny-camel-queue-priv.h"

typedef struct _TnyCamelAccountPriv TnyCamelAccountPriv;
typedef struct _RefreshStatusInfo RefreshStatusInfo;

struct _RefreshStatusInfo
{
	TnyAccount *self;
	TnyStatusCallback status_callback;
	TnyStatusDomain domain;
	TnyStatusCode code;
	guint depth;
	gpointer user_data;
	TnyIdleStopper *stopper;
};


struct _TnyCamelAccountPriv
{
	TnySessionCamel *session;
	
	GStaticRecMutex *service_lock, *account_lock;
	
	/* Set in tny_camel_store_account_prepare(). */
	CamelService *service;
	
	CamelException *ex;
	gchar *url_string, *id, *user, *host, *proto, *mech;
	TnyGetPassFunc get_pass_func;
	TnyForgetPassFunc forget_pass_func;
	gboolean pass_func_set, forget_pass_func_set;
	CamelProviderType type;
	CamelOperation *cancel, *getmsg_cancel;
	GStaticRecMutex *cancel_lock;
	gboolean inuse_spin, in_auth;
	gchar *name; GList *options;
	gchar *cache_location; gint port;
	TnyAccountType account_type;
	gboolean custom_url_string;
	RefreshStatusInfo *csyncop;
	GList *chooks;
	TnyConnectionStatus status;
	gboolean is_connecting, is_ready, retry_connect;
	gchar *delete_this;
	TnyCamelQueue *queue;
	TnyConnectionPolicy *con_strat;
};

const CamelService* _tny_camel_account_get_service (TnyCamelAccount *self);
const gchar* _tny_camel_account_get_url_string (TnyCamelAccount *self);
void _tny_camel_account_start_camel_operation (TnyCamelAccount *self, CamelOperationStatusFunc func, gpointer user_data, const gchar *what);
void _tny_camel_account_start_camel_operation_n (TnyCamelAccount *self, CamelOperationStatusFunc func, gpointer user_data, const gchar *what, gboolean cancel);
void _tny_camel_account_stop_camel_operation (TnyCamelAccount *self);
void _tny_camel_account_try_connect (TnyCamelAccount *self, gboolean for_online, GError **err);
void _tny_camel_account_clear_hooks (TnyCamelAccount *self);
void _tny_camel_account_refresh (TnyCamelAccount *self, gboolean recon_if);
void _tny_camel_account_set_online (TnyCamelAccount *self, gboolean online, GError **err);
TnyError _tny_camel_account_get_tny_error_code_for_camel_exception_id (CamelException* ex);
void _tny_camel_account_emit_changed (TnyCamelAccount *self);
void _tny_camel_account_actual_cancel (TnyCamelAccount *self);
void _tny_camel_account_actual_uncancel (TnyCamelAccount *self);

#define TNY_CAMEL_ACCOUNT_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_CAMEL_ACCOUNT, TnyCamelAccountPriv))

#endif
