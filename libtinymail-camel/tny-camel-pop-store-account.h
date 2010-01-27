#ifndef TNY_CAMEL_POP_STORE_ACCOUNT_H
#define TNY_CAMEL_POP_STORE_ACCOUNT_H

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

#include <tny-camel-store-account.h>
#include <tny-store-account.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_POP_STORE_ACCOUNT             (tny_camel_pop_store_account_get_type ())
#define TNY_CAMEL_POP_STORE_ACCOUNT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_POP_STORE_ACCOUNT, TnyCamelPOPStoreAccount))
#define TNY_CAMEL_POP_STORE_ACCOUNT_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_POP_STORE_ACCOUNT, TnyCamelPOPStoreAccountClass))
#define TNY_IS_CAMEL_POP_STORE_ACCOUNT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_POP_STORE_ACCOUNT))
#define TNY_IS_CAMEL_POP_STORE_ACCOUNT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_POP_STORE_ACCOUNT))
#define TNY_CAMEL_POP_STORE_ACCOUNT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_POP_STORE_ACCOUNT, TnyCamelPOPStoreAccountClass))

typedef struct _TnyCamelPOPStoreAccount TnyCamelPOPStoreAccount;
typedef struct _TnyCamelPOPStoreAccountClass TnyCamelPOPStoreAccountClass;

struct _TnyCamelPOPStoreAccount
{
	TnyCamelStoreAccount parent;
};

struct _TnyCamelPOPStoreAccountClass 
{
	TnyCamelStoreAccountClass parent;
};

GType tny_camel_pop_store_account_get_type (void);
TnyStoreAccount* tny_camel_pop_store_account_new (void);

void tny_camel_pop_store_account_set_leave_messages_on_server (TnyCamelPOPStoreAccount *self, gboolean enabled);

void tny_camel_pop_store_account_reconnect (TnyCamelPOPStoreAccount *self);

G_END_DECLS

#endif

