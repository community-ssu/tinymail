#ifndef TNY_GNOME_ACCOUNT_STORE_H
#define TNY_GNOME_ACCOUNT_STORE_H

/* libtinymail-gnome-desktop - The Tiny Mail base library for Gnome
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

#define TNY_TYPE_GNOME_ACCOUNT_STORE             (tny_gnome_account_store_get_type ())
#define TNY_GNOME_ACCOUNT_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GNOME_ACCOUNT_STORE, TnyGnomeAccountStore))
#define TNY_GNOME_ACCOUNT_STORE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GNOME_ACCOUNT_STORE, TnyGnomeAccountStoreClass))
#define TNY_IS_GNOME_ACCOUNT_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GNOME_ACCOUNT_STORE))
#define TNY_IS_GNOME_ACCOUNT_STORE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GNOME_ACCOUNT_STORE))
#define TNY_GNOME_ACCOUNT_STORE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GNOME_ACCOUNT_STORE, TnyGnomeAccountStoreClass))

typedef struct _TnyGnomeAccountStore TnyGnomeAccountStore;
typedef struct _TnyGnomeAccountStoreClass TnyGnomeAccountStoreClass;

struct _TnyGnomeAccountStore
{
	GObject parent;
};

struct _TnyGnomeAccountStoreClass
{
	GObjectClass parent;
};

GType tny_gnome_account_store_get_type (void);
TnyAccountStore* tny_gnome_account_store_new (void);
TnySessionCamel* tny_gnome_account_store_get_session (TnyGnomeAccountStore *self);


G_END_DECLS

#endif
