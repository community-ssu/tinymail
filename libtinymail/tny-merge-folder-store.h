#ifndef TNY_MERGE_FOLDER_STORE_H
#define TNY_MERGE_FOLDER_STORE_H

/* libtinymail - The Tiny Mail base library
 * Copyright (C) 2006-2007 Philip Van Hoof <pvanhoof@gnome.org>
 * Copyright (C) Rob Taylor <rob.taylor@codethink.co.uk>
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

#define TNY_TYPE_MERGE_FOLDER_STORE             (tny_merge_folder_store_get_type ())
#define TNY_MERGE_FOLDER_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_MERGE_FOLDER_STORE, TnyMergeFolderStore))
#define TNY_MERGE_FOLDER_STORE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_MERGE_FOLDER_STORE, TnyMergeFolderStoreClass))
#define TNY_IS_MERGE_FOLDER_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_MERGE_FOLDER_STORE))
#define TNY_IS_MERGE_FOLDER_STORE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_MERGE_FOLDER_STORE))
#define TNY_MERGE_FOLDER_STORE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_MERGE_FOLDER_STORE, TnyMergeFolderStoreClass))

typedef struct _TnyMergeFolderStore TnyMergeFolderStore;
typedef struct _TnyMergeFolderStoreClass TnyMergeFolderStoreClass;

struct _TnyMergeFolderStore
{
	GObject parent;
};

struct _TnyMergeFolderStoreClass
{
	GObjectClass parent;

	void (*get_folders_async) (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, TnyGetFoldersCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	void (*get_folders) (TnyFolderStore *self, TnyList *list, TnyFolderStoreQuery *query, gboolean refresh, GError **err);
	void (*remove_folder) (TnyFolderStore *self, TnyFolder *folder, GError **err);
	TnyFolder* (*create_folder) (TnyFolderStore *self, const gchar *name, GError **err);
	void (*create_folder_async) (TnyFolderStore *self, const gchar *name, TnyCreateFolderCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	TnyFolderStore* (*get_folder_store) (TnyFolder *self);
	void (*add_store_observer) (TnyFolderStore *self, TnyFolderStoreObserver *observer);
	void (*remove_store_observer) (TnyFolderStore *self, TnyFolderStoreObserver *observer);
	void (*refresh_async) (TnyFolderStore *self, TnyFolderStoreCallback callback, TnyStatusCallback status_callback, gpointer user_data);

	void (*add_store) (TnyMergeFolderStore *self, TnyFolderStore *store);
	void (*remove_store) (TnyMergeFolderStore *self, TnyFolderStore *store);
};

GType tny_merge_folder_store_get_type (void);

TnyMergeFolderStore* tny_merge_folder_store_new (TnyLockable *ui_locker);

void tny_merge_folder_store_add_store (TnyMergeFolderStore *self, TnyFolderStore *store);
void tny_merge_folder_store_remove_store (TnyMergeFolderStore *self, TnyFolderStore *store);

G_END_DECLS

#endif
