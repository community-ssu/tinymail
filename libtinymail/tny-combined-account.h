#ifndef TNY_COMBINED_ACCOUNT_H
#define TNY_COMBINED_ACCOUNT_H

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

#include <tny-account.h>
#include <tny-store-account.h>
#include <tny-transport-account.h>
#include <tny-folder-store.h>

G_BEGIN_DECLS

#define TNY_TYPE_COMBINED_ACCOUNT             (tny_combined_account_get_type ())
#define TNY_COMBINED_ACCOUNT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_COMBINED_ACCOUNT, TnyCombinedAccount))
#define TNY_COMBINED_ACCOUNT_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_COMBINED_ACCOUNT, TnyCombinedAccountClass))
#define TNY_IS_COMBINED_ACCOUNT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_COMBINED_ACCOUNT))
#define TNY_IS_COMBINED_ACCOUNT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_COMBINED_ACCOUNT))
#define TNY_COMBINED_ACCOUNT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_COMBINED_ACCOUNT, TnyCombinedAccountClass))

#ifndef TNY_SHARED_H
typedef struct _TnyCombinedAccount TnyCombinedAccount;
typedef struct _TnyCombinedAccountClass TnyCombinedAccountClass;
#endif

struct _TnyCombinedAccount
{
	GObject parent;
};

struct _TnyCombinedAccountClass 
{
	GObjectClass parent;
};

GType tny_combined_account_get_type (void);


TnyAccount *tny_combined_account_new (TnyTransportAccount *ta, TnyStoreAccount *sa);
TnyTransportAccount* tny_combined_account_get_transport_account (TnyCombinedAccount *self);
TnyStoreAccount* tny_combined_account_get_store_account (TnyCombinedAccount *self);

G_END_DECLS

#endif

