#ifndef TNY_CAMEL_IMAP_STORE_ACCOUNT_H
#define TNY_CAMEL_IMAP_STORE_ACCOUNT_H

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

#include <tny-account.h>
#include <tny-store-account.h>
#include <tny-camel-account.h>
#include <tny-camel-store-account.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_IMAP_STORE_ACCOUNT             (tny_camel_imap_store_account_get_type ())
#define TNY_CAMEL_IMAP_STORE_ACCOUNT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_IMAP_STORE_ACCOUNT, TnyCamelIMAPStoreAccount))
#define TNY_CAMEL_IMAP_STORE_ACCOUNT_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_IMAP_STORE_ACCOUNT, TnyCamelIMAPStoreAccountClass))
#define TNY_IS_CAMEL_IMAP_STORE_ACCOUNT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_IMAP_STORE_ACCOUNT))
#define TNY_IS_CAMEL_IMAP_STORE_ACCOUNT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_IMAP_STORE_ACCOUNT))
#define TNY_CAMEL_IMAP_STORE_ACCOUNT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_IMAP_STORE_ACCOUNT, TnyCamelIMAPStoreAccountClass))

typedef struct _TnyCamelIMAPStoreAccount TnyCamelIMAPStoreAccount;
typedef struct _TnyCamelIMAPStoreAccountClass TnyCamelIMAPStoreAccountClass;

struct _TnyCamelIMAPStoreAccount
{
	TnyCamelStoreAccount parent;
};

struct _TnyCamelIMAPStoreAccountClass 
{
	TnyCamelStoreAccountClass parent;
};

GType tny_camel_imap_store_account_get_type (void);
TnyStoreAccount* tny_camel_imap_store_account_new (void);

G_END_DECLS

#endif

