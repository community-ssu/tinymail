#ifndef TNY_ACAP_ACCOUNT_STORE_H
#define TNY_ACAP_ACCOUNT_STORE_H

/* libtinymail-acap - The Tiny Mail base library for ACAP
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

#define TNY_TYPE_ACAP_ACCOUNT_STORE             (tny_acap_account_store_get_type ())
#define TNY_ACAP_ACCOUNT_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_ACAP_ACCOUNT_STORE, TnyAcapAccountStore))
#define TNY_ACAP_ACCOUNT_STORE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_ACAP_ACCOUNT_STORE, TnyAcapAccountStoreClass))
#define TNY_IS_ACAP_ACCOUNT_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_ACAP_ACCOUNT_STORE))
#define TNY_IS_ACAP_ACCOUNT_STORE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_ACAP_ACCOUNT_STORE))
#define TNY_ACAP_ACCOUNT_STORE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_ACAP_ACCOUNT_STORE, TnyAcapAccountStoreClass))

typedef struct _TnyAcapAccountStore TnyAcapAccountStore;
typedef struct _TnyAcapAccountStoreClass TnyAcapAccountStoreClass;

struct _TnyAcapAccountStore
{
	GObject parent;
};

struct _TnyAcapAccountStoreClass
{
	GObjectClass parent;
};

GType tny_acap_account_store_get_type (void);
TnyAccountStore* tny_acap_account_store_new (TnyAccountStore *real, TnyPasswordGetter *pwdgetter);
TnyAccountStore* tny_acap_account_store_get_real (TnyAcapAccountStore *self);

G_END_DECLS

#endif
