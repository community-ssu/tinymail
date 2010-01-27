#ifndef TNY_STORE_ACCOUNT_H
#define TNY_STORE_ACCOUNT_H

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

#include <tny-account.h>
#include <tny-folder.h>
#include <tny-list.h>

G_BEGIN_DECLS

#define TNY_TYPE_STORE_ACCOUNT             (tny_store_account_get_type ())
#define TNY_STORE_ACCOUNT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_STORE_ACCOUNT, TnyStoreAccount))
#define TNY_IS_STORE_ACCOUNT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_STORE_ACCOUNT))
#define TNY_STORE_ACCOUNT_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_STORE_ACCOUNT, TnyStoreAccountIface))

#define TNY_TYPE_STORE_ACCOUNT_SIGNAL (tny_store_account_signal_get_type ())

enum _TnyStoreAccountSignal
{
	TNY_STORE_ACCOUNT_SUBSCRIPTION_CHANGED,
	TNY_STORE_ACCOUNT_LAST_SIGNAL
};

extern guint tny_store_account_signals [TNY_STORE_ACCOUNT_LAST_SIGNAL];

#ifndef TNY_SHARED_H
typedef struct _TnyStoreAccount TnyStoreAccount;
typedef struct _TnyStoreAccountIface TnyStoreAccountIface;
#endif

struct _TnyStoreAccountIface
{
	GTypeInterface parent;

	/* Signals */
	void (*subscription_changed) (TnyStoreAccount *self, TnyFolder *folder);

	/* Methods */
	void (*subscribe) (TnyStoreAccount *self, TnyFolder *folder);
	void (*unsubscribe) (TnyStoreAccount *self, TnyFolder *folder);
	TnyFolder * (*find_folder) (TnyStoreAccount *self, const gchar *url_string, GError **err);
	void (*delete_cache) (TnyStoreAccount *self);
};

GType tny_store_account_get_type (void);

void tny_store_account_subscribe (TnyStoreAccount *self, TnyFolder *folder);
void tny_store_account_unsubscribe (TnyStoreAccount *self, TnyFolder *folder);
TnyFolder* tny_store_account_find_folder (TnyStoreAccount *self, const gchar *url_string, GError **err);
void tny_store_account_delete_cache (TnyStoreAccount *self);

G_END_DECLS

#endif
