#ifndef TNY_GTK_FOLDER_LIST_STORE_ITERATOR_H
#define TNY_GTK_FOLDER_LIST_STORE_ITERATOR_H

/* libtinymail- The Tiny Mail base library
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
#include <glib-object.h>

#include <tny-shared.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-gtk-folder-list-store.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_FOLDER_LIST_STORE_ITERATOR             (_tny_gtk_folder_list_store_iterator_get_type ())
#define TNY_GTK_FOLDER_LIST_STORE_ITERATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_FOLDER_LIST_STORE_ITERATOR, TnyGtkFolderListStoreIterator))
#define TNY_GTK_FOLDER_LIST_STORE_ITERATOR_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_FOLDER_LIST_STORE_ITERATOR, TnyGtkFolderListStoreIteratorClass))
#define TNY_IS_GTK_FOLDER_LIST_STORE_ITERATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_FOLDER_LIST_STORE_ITERATOR))
#define TNY_IS_GTK_FOLDER_LIST_STORE_ITERATOR_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_FOLDER_LIST_STORE_ITERATOR))
#define TNY_GTK_FOLDER_LIST_STORE_ITERATOR_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_FOLDER_LIST_STORE_ITERATOR, TnyGtkFolderListStoreIteratorClass))

typedef struct _TnyGtkFolderListStoreIterator TnyGtkFolderListStoreIterator;
typedef struct _TnyGtkFolderListStoreIteratorClass TnyGtkFolderListStoreIteratorClass;

struct _TnyGtkFolderListStoreIterator
{
	GObject parent;
	TnyGtkFolderListStore *model;
	GList *current;
};

struct _TnyGtkFolderListStoreIteratorClass 
{
	GObjectClass parent;
};

void _tny_gtk_folder_list_store_iterator_set_model (TnyGtkFolderListStoreIterator *self, TnyGtkFolderListStore *model);
TnyGtkFolderListStoreIterator* _tny_gtk_folder_list_store_iterator_new (TnyGtkFolderListStore *model);


G_END_DECLS

#endif

