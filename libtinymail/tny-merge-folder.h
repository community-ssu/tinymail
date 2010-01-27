#ifndef TNY_MERGE_FOLDER_H
#define TNY_MERGE_FOLDER_H

/* libtinymail - The Tiny Mail base library for Merge
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

#include <tny-folder.h>
#include <tny-shared.h>
#include <tny-msg.h>
#include <tny-header.h>
#include <tny-msg-remove-strategy.h>
#include <tny-msg-receive-strategy.h>
#include <tny-folder-store.h>
#include <tny-folder-stats.h>
#include <tny-lockable.h>

G_BEGIN_DECLS

#define TNY_TYPE_MERGE_FOLDER             (tny_merge_folder_get_type ())
#define TNY_MERGE_FOLDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_MERGE_FOLDER, TnyMergeFolder))
#define TNY_MERGE_FOLDER_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_MERGE_FOLDER, TnyMergeFolderClass))
#define TNY_IS_MERGE_FOLDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_MERGE_FOLDER))
#define TNY_IS_MERGE_FOLDER_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_MERGE_FOLDER))
#define TNY_MERGE_FOLDER_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_MERGE_FOLDER, TnyMergeFolderClass))

typedef struct _TnyMergeFolder TnyMergeFolder;
typedef struct _TnyMergeFolderClass TnyMergeFolderClass;

struct _TnyMergeFolder
{
	GObject parent;
};

struct _TnyMergeFolderClass 
{
	GObjectClass parent;
};

GType tny_merge_folder_get_type (void);

TnyFolder* tny_merge_folder_new (const gchar *folder_name);
TnyFolder* tny_merge_folder_new_with_ui_locker (const gchar *folder_name, TnyLockable *ui_locker);
void tny_merge_folder_set_ui_locker (TnyMergeFolder *self, TnyLockable *ui_locker);
void tny_merge_folder_add_folder (TnyMergeFolder *self, TnyFolder *folder);
void tny_merge_folder_remove_folder (TnyMergeFolder *self, TnyFolder *folder);
void tny_merge_folder_set_folder_type (TnyMergeFolder *self, TnyFolderType folder_type);
void tny_merge_folder_get_folders (TnyMergeFolder *self, TnyList *list);

G_END_DECLS

#endif

