#ifndef TNY_ACCOUNT_STORE_VIEW_H
#define TNY_ACCOUNT_STORE_VIEW_H

/* libtinymailui - The Tiny Mail UI library
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
#include <tny-account-store.h>

G_BEGIN_DECLS

#define TNY_TYPE_ACCOUNT_STORE_VIEW             (tny_account_store_view_get_type ())
#define TNY_ACCOUNT_STORE_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_ACCOUNT_STORE_VIEW, TnyAccountStoreView))
#define TNY_IS_ACCOUNT_STORE_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_ACCOUNT_STORE_VIEW))
#define TNY_ACCOUNT_STORE_VIEW_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_ACCOUNT_STORE_VIEW, TnyAccountStoreViewIface))

typedef struct _TnyAccountStoreView TnyAccountStoreView;
typedef struct _TnyAccountStoreViewIface TnyAccountStoreViewIface;


struct _TnyAccountStoreViewIface
{
	GTypeInterface parent;

	void (*set_account_store) (TnyAccountStoreView *self, TnyAccountStore *account_store);
};

GType tny_account_store_view_get_type (void);

void tny_account_store_view_set_account_store (TnyAccountStoreView *self, TnyAccountStore *account_store);

G_END_DECLS

#endif
