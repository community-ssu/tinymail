#ifndef TNY_GTK_FOLDER_LIST_STORE_H
#define TNY_GTK_FOLDER_LIST_STORE_H

/* libtinymailui-gtk - The Tiny Mail UI library for Gtk+
 * Copyright (C) 2006-2008 Philip Van Hoof <pvanhoof@gnome.org>
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

#define TNY_TYPE_GTK_FOLDER_LIST_STORE             (tny_gtk_folder_list_store_get_type ())
#define TNY_GTK_FOLDER_LIST_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_FOLDER_LIST_STORE, TnyGtkFolderListStore))
#define TNY_GTK_FOLDER_LIST_STORE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_FOLDER_LIST_STORE, TnyGtkFolderListStoreClass))
#define TNY_IS_GTK_FOLDER_LIST_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_FOLDER_LIST_STORE))
#define TNY_IS_GTK_FOLDER_LIST_STORE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_FOLDER_LIST_STORE))
#define TNY_GTK_FOLDER_LIST_STORE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_FOLDER_LIST_STORE, TnyGtkFolderListStoreClass))

/* Implements GtkTreeModel and TnyList */

typedef struct _TnyGtkFolderListStore TnyGtkFolderListStore;
typedef struct _TnyGtkFolderListStoreClass TnyGtkFolderListStoreClass;

#define TNY_TYPE_GTK_FOLDER_LIST_STORE_COLUMN (tny_gtk_folder_list_store_column_get_type())

typedef enum
{
	TNY_GTK_FOLDER_LIST_STORE_NAME_COLUMN,
	TNY_GTK_FOLDER_LIST_STORE_UNREAD_COLUMN,
	TNY_GTK_FOLDER_LIST_STORE_ALL_COLUMN,
	TNY_GTK_FOLDER_LIST_STORE_TYPE_COLUMN,
	TNY_GTK_FOLDER_LIST_STORE_INSTANCE_COLUMN,
	TNY_GTK_FOLDER_LIST_STORE_N_COLUMNS
} TnyGtkFolderListStoreColumn;

typedef enum 
{
	TNY_GTK_FOLDER_LIST_STORE_FLAG_SHOW_PATH = 1<<0,
	TNY_GTK_FOLDER_LIST_STORE_FLAG_NO_REFRESH = 1<<2,
	TNY_GTK_FOLDER_LIST_STORE_FLAG_DELAYED_REFRESH = 1<<3,
	TNY_GTK_FOLDER_LIST_STORE_FLAG_DISPOSED = 1<<4,
} TnyGtkFolderListStoreFlags;

struct _TnyGtkFolderListStore
{
	GtkListStore parent;
	GList *first, *store_obs, *fol_obs;
	GMutex *iterator_lock;
	TnyFolderStoreQuery *query;
	gboolean first_needs_unref;
	GPtrArray *signals;

	TnyGtkFolderListStoreFlags flags;
	gchar *path_separator;
	gint progress_count;
	guint delayed_refresh_timeout_id;
};

struct _TnyGtkFolderListStoreClass
{
	GtkListStoreClass parent_class;

	/* Signals */
	void (*activity_changed) (TnyGtkFolderListStore *self, gboolean activity);
};

GType tny_gtk_folder_list_store_get_type (void);
GType tny_gtk_folder_list_store_column_get_type (void);
GtkTreeModel* tny_gtk_folder_list_store_new (TnyFolderStoreQuery *query);
GtkTreeModel* tny_gtk_folder_list_store_new_with_flags (TnyFolderStoreQuery *query, 
							      TnyGtkFolderListStoreFlags flags);
void tny_gtk_folder_list_store_set_path_separator (TnyGtkFolderListStore *self, const gchar *separator);
const gchar *tny_gtk_folder_list_store_get_path_separator (TnyGtkFolderListStore *self);
void tny_gtk_folder_list_store_prepend (TnyGtkFolderListStore *self, TnyFolderStore* item, const gchar *root_name);
void tny_gtk_folder_list_store_append (TnyGtkFolderListStore *self, TnyFolderStore* item, const gchar *root_name);
gboolean tny_gtk_folder_list_store_get_activity (TnyGtkFolderListStore *self);

G_END_DECLS

#endif
