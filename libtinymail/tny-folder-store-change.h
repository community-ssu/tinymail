#ifndef TNY_FOLDER_STORE_CHANGE_H
#define TNY_FOLDER_STORE_CHANGE_H

/* libtinymail- The Tiny Mail base library
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
#include <tny-folder-store.h>

G_BEGIN_DECLS

#define TNY_TYPE_FOLDER_STORE_CHANGE             (tny_folder_store_change_get_type ())
#define TNY_FOLDER_STORE_CHANGE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_FOLDER_STORE_CHANGE, TnyFolderStoreChange))
#define TNY_FOLDER_STORE_CHANGE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_FOLDER_STORE_CHANGE, TnyFolderStoreChangeClass))
#define TNY_IS_FOLDER_STORE_CHANGE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_FOLDER_STORE_CHANGE))
#define TNY_IS_FOLDER_STORE_CHANGE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_FOLDER_STORE_CHANGE))
#define TNY_FOLDER_STORE_CHANGE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_FOLDER_STORE_CHANGE, TnyFolderStoreChangeClass))

#ifndef TNY_SHARED_H
typedef struct _TnyFolderStoreChange TnyFolderStoreChange;
typedef struct _TnyFolderStoreChangeClass TnyFolderStoreChangeClass;
#endif

#define TNY_TYPE_FOLDER_STORE_CHANGE_CHANGED (tny_folder_store_change_changed_get_type())

typedef enum
{
	TNY_FOLDER_STORE_CHANGE_CHANGED_CREATED_FOLDERS = 1<<0,
	TNY_FOLDER_STORE_CHANGE_CHANGED_REMOVED_FOLDERS = 1<<1
} TnyFolderStoreChangeChanged;


struct _TnyFolderStoreChange
{
	GObject parent;
};

struct _TnyFolderStoreChangeClass 
{
	GObjectClass parent;
};

GType  tny_folder_store_change_get_type (void);
GType tny_folder_store_change_changed_get_type (void);

TnyFolderStoreChange* tny_folder_store_change_new (TnyFolderStore *folderstore);

void tny_folder_store_change_add_created_folder (TnyFolderStoreChange *self, TnyFolder *folder);
void tny_folder_store_change_add_removed_folder (TnyFolderStoreChange *self, TnyFolder *folder);

void tny_folder_store_change_get_created_folders (TnyFolderStoreChange *self, TnyList *folders);
void tny_folder_store_change_get_removed_folders (TnyFolderStoreChange *self, TnyList *folders);

void tny_folder_store_change_reset (TnyFolderStoreChange *self);
TnyFolderStore* tny_folder_store_change_get_folder_store (TnyFolderStoreChange *self);
TnyFolderStoreChangeChanged tny_folder_store_change_get_changed  (TnyFolderStoreChange *self);

G_END_DECLS

#endif

