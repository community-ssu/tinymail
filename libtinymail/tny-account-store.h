#ifndef TNY_ACCOUNT_STORE_H
#define TNY_ACCOUNT_STORE_H

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

#include <glib.h>
#include <glib-object.h>
#include <tny-shared.h>

G_BEGIN_DECLS

#define TNY_TYPE_ACCOUNT_STORE             (tny_account_store_get_type ())
#define TNY_ACCOUNT_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_ACCOUNT_STORE, TnyAccountStore))
#define TNY_IS_ACCOUNT_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_ACCOUNT_STORE))
#define TNY_ACCOUNT_STORE_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_ACCOUNT_STORE, TnyAccountStoreIface))

#define TNY_TYPE_ACCOUNT_STORE_SIGNAL (tny_account_store_signal_get_type())

typedef enum
{
	TNY_ACCOUNT_STORE_CONNECTING_STARTED,
	TNY_ACCOUNT_STORE_LAST_SIGNAL
} TnyAccountStoreSignal;

#define TNY_TYPE_ALERT_TYPE (tny_alert_type_get_type())

typedef enum
{
	TNY_ALERT_TYPE_INFO,
	TNY_ALERT_TYPE_WARNING,
	TNY_ALERT_TYPE_ERROR
} TnyAlertType;


#define TNY_TYPE_GET_ACCOUNTS_REQUEST_TYPE (tny_get_accounts_request_type_get_type())

typedef enum
{
	TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS,
	TNY_ACCOUNT_STORE_STORE_ACCOUNTS,
	TNY_ACCOUNT_STORE_BOTH
} TnyGetAccountsRequestType;


extern guint tny_account_store_signals [TNY_ACCOUNT_STORE_LAST_SIGNAL];

#ifndef TNY_SHARED_H
typedef struct _TnyAccountStore TnyAccountStore;
typedef struct _TnyAccountStoreIface TnyAccountStoreIface;
#endif

struct _TnyAccountStoreIface {
	GTypeInterface parent;

	/* Methods */
	void (*get_accounts) (TnyAccountStore *self, TnyList *list, TnyGetAccountsRequestType types);
	const gchar* (*get_cache_dir) (TnyAccountStore *self);
	TnyDevice* (*get_device) (TnyAccountStore *self);
	gboolean (*alert) (TnyAccountStore *self, TnyAccount *account, TnyAlertType type, gboolean question, GError *error);
	TnyAccount* (*find_account) (TnyAccountStore *self, const gchar *url_string);

	/* Signals */
	void (*connecting_started) (TnyAccountStore *self);
};

GType tny_account_store_get_type (void);
GType tny_get_accounts_request_type_get_type (void);
GType tny_alert_type_get_type (void);

void tny_account_store_get_accounts (TnyAccountStore *self, TnyList *list, TnyGetAccountsRequestType types);
const gchar*  tny_account_store_get_cache_dir (TnyAccountStore *self);
TnyDevice* tny_account_store_get_device (TnyAccountStore *self);
gboolean tny_account_store_alert (TnyAccountStore *self, TnyAccount *account, TnyAlertType type, gboolean question, GError *error);
TnyAccount* tny_account_store_find_account (TnyAccountStore *self, const gchar *url_string);

G_END_DECLS

#endif
