#ifndef TNY_GTK_FOLDER_STORE_TREE_MODEL_H
#define TNY_GTK_FOLDER_STORE_TREE_MODEL_H

/* libtinymailui-gtk - The Tiny Mail UI library for Gtk+
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
#include <gtk/gtk.h>
#include <tny-folder-store.h>
#include <tny-folder-store-query.h>
#include <tny-list.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_FOLDER_STORE_TREE_MODEL             (tny_gtk_folder_store_tree_model_get_type ())
#define TNY_GTK_FOLDER_STORE_TREE_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_FOLDER_STORE_TREE_MODEL, TnyGtkFolderStoreTreeModel))
#define TNY_GTK_FOLDER_STORE_TREE_MODEL_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_FOLDER_STORE_TREE_MODEL, TnyGtkFolderStoreTreeModelClass))
#define TNY_IS_GTK_FOLDER_STORE_TREE_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_FOLDER_STORE_TREE_MODEL))
#define TNY_IS_GTK_FOLDER_STORE_TREE_MODEL_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_FOLDER_STORE_TREE_MODEL))
#define TNY_GTK_FOLDER_STORE_TREE_MODEL_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_FOLDER_STORE_TREE_MODEL, TnyGtkFolderStoreTreeModelClass))

/* Implements GtkTreeModel and TnyList */

typedef struct _TnyGtkFolderStoreTreeModel TnyGtkFolderStoreTreeModel;
typedef struct _TnyGtkFolderStoreTreeModelClass TnyGtkFolderStoreTreeModelClass;

#define TNY_TYPE_GTK_FOLDER_STORE_TREE_MODEL_COLUMN (tny_gtk_folder_store_tree_model_column_get_type())

typedef enum
{
	TNY_GTK_FOLDER_STORE_TREE_MODEL_NAME_COLUMN,
	TNY_GTK_FOLDER_STORE_TREE_MODEL_UNREAD_COLUMN,
	TNY_GTK_FOLDER_STORE_TREE_MODEL_ALL_COLUMN,
	TNY_GTK_FOLDER_STORE_TREE_MODEL_TYPE_COLUMN,
	TNY_GTK_FOLDER_STORE_TREE_MODEL_INSTANCE_COLUMN,
	TNY_GTK_FOLDER_STORE_TREE_MODEL_N_COLUMNS
} TnyGtkFolderStoreTreeModelColumn;

typedef enum
{
	TNY_GTK_FOLDER_STORE_TREE_MODEL_FLAG_SHOW_PATH = 1<<0,
} TnyGtkFolderStoreTreeModelFlags;

struct _TnyGtkFolderStoreTreeModel
{
	GtkTreeStore parent;
	GList *first, *store_obs, *fol_obs;
	GMutex *iterator_lock;
	TnyFolderStoreQuery *query;
	gboolean first_needs_unref;
	GPtrArray *signals;

	TnyGtkFolderStoreTreeModelFlags flags;
};

struct _TnyGtkFolderStoreTreeModelClass
{
	GtkTreeStoreClass parent_class;
};

GType tny_gtk_folder_store_tree_model_get_type (void);
GType tny_gtk_folder_store_tree_model_column_get_type (void);
GtkTreeModel* tny_gtk_folder_store_tree_model_new (TnyFolderStoreQuery *query);
GtkTreeModel* tny_gtk_folder_store_tree_model_new_with_flags (TnyFolderStoreQuery *query, 
							      TnyGtkFolderStoreTreeModelFlags flags);
void tny_gtk_folder_store_tree_model_prepend (TnyGtkFolderStoreTreeModel *self, TnyFolderStore* item, const gchar *root_name);
void tny_gtk_folder_store_tree_model_append (TnyGtkFolderStoreTreeModel *self, TnyFolderStore* item, const gchar *root_name);

G_END_DECLS

#endif
