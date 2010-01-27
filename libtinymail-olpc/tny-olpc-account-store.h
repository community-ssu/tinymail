#ifndef TNY_OLPC_ACCOUNT_STORE_H
#define TNY_OLPC_ACCOUNT_STORE_H

/* libtinymail-olpc - The Tiny Mail base library for OLPC
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

#include <tny-camel-shared.h>

G_BEGIN_DECLS

#define TNY_TYPE_OLPC_ACCOUNT_STORE             (tny_olpc_account_store_get_type ())
#define TNY_OLPC_ACCOUNT_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_OLPC_ACCOUNT_STORE, TnyOlpcAccountStore))
#define TNY_OLPC_ACCOUNT_STORE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_OLPC_ACCOUNT_STORE, TnyOlpcAccountStoreClass))
#define TNY_IS_OLPC_ACCOUNT_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_OLPC_ACCOUNT_STORE))
#define TNY_IS_OLPC_ACCOUNT_STORE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_OLPC_ACCOUNT_STORE))
#define TNY_OLPC_ACCOUNT_STORE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_OLPC_ACCOUNT_STORE, TnyOlpcAccountStoreClass))

typedef struct _TnyOlpcAccountStore TnyOlpcAccountStore;
typedef struct _TnyOlpcAccountStoreClass TnyOlpcAccountStoreClass;

struct _TnyOlpcAccountStore
{
	GObject parent;
};

struct _TnyOlpcAccountStoreClass
{
	GObjectClass parent;
};

GType tny_olpc_account_store_get_type (void);
TnyAccountStore* tny_olpc_account_store_new (void);
TnySessionCamel* tny_olpc_account_store_get_session (TnyOlpcAccountStore *self);

G_END_DECLS

#endif
