#ifndef TNY_GTK_ATTACH_LIST_MODEL_H
#define TNY_GTK_ATTACH_LIST_MODEL_H

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
#include <tny-account.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_ATTACH_LIST_MODEL             (tny_gtk_attach_list_model_get_type ())
#define TNY_GTK_ATTACH_LIST_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_ATTACH_LIST_MODEL, TnyGtkAttachListModel))
#define TNY_GTK_ATTACH_LIST_MODEL_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_ATTACH_LIST_MODEL, TnyGtkAttachListModelClass))
#define TNY_IS_GTK_ATTACH_LIST_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_ATTACH_LIST_MODEL))
#define TNY_IS_GTK_ATTACH_LIST_MODEL_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_ATTACH_LIST_MODEL))
#define TNY_GTK_ATTACH_LIST_MODEL_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_ATTACH_LIST_MODEL, TnyGtkAttachListModelClass))

typedef struct _TnyGtkAttachListModel TnyGtkAttachListModel;
typedef struct _TnyGtkAttachListModelClass TnyGtkAttachListModelClass;


#define TNY_TYPE_GTK_ATTACH_LIST_MODEL_COLUMN (tny_gtk_attach_list_model_column_get_type())


typedef enum
{
	TNY_GTK_ATTACH_LIST_MODEL_PIXBUF_COLUMN,
	TNY_GTK_ATTACH_LIST_MODEL_FILENAME_COLUMN,
	TNY_GTK_ATTACH_LIST_MODEL_INSTANCE_COLUMN,
	TNY_GTK_ATTACH_LIST_MODEL_N_COLUMNS
} TnyGtkAttachListModelColumn;

struct _TnyGtkAttachListModel
{
	GtkListStore parent;
	GList *first;
	GMutex *iterator_lock;
	gboolean first_needs_unref;
};

struct _TnyGtkAttachListModelClass
{
	GtkListStoreClass parent_class;
};


GType tny_gtk_attach_list_model_get_type (void);
GType tny_gtk_attach_list_model_column_get_type (void);
GtkTreeModel* tny_gtk_attach_list_model_new (void);

G_END_DECLS

#endif
