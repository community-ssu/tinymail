#ifndef TNY_FOLDER_STORE_OBSERVER_H
#define TNY_FOLDER_STORE_OBSERVER_H

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
#include <tny-folder-store-change.h>

G_BEGIN_DECLS

#define TNY_TYPE_FOLDER_STORE_OBSERVER             (tny_folder_store_observer_get_type ())
#define TNY_FOLDER_STORE_OBSERVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_FOLDER_STORE_OBSERVER, TnyFolderStoreObserver))
#define TNY_IS_FOLDER_STORE_OBSERVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_FOLDER_STORE_OBSERVER))
#define TNY_FOLDER_STORE_OBSERVER_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_FOLDER_STORE_OBSERVER, TnyFolderStoreObserverIface))

#ifndef TNY_SHARED_H
typedef struct _TnyFolderStoreObserver TnyFolderStoreObserver;
typedef struct _TnyFolderStoreObserverIface TnyFolderStoreObserverIface;
#endif

struct _TnyFolderStoreObserverIface
{
	GTypeInterface parent;

	void (*update) (TnyFolderStoreObserver *self, TnyFolderStoreChange *change);

};

GType tny_folder_store_observer_get_type (void);

void tny_folder_store_observer_update (TnyFolderStoreObserver *self, TnyFolderStoreChange *change);

G_END_DECLS

#endif
