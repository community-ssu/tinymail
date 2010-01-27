#ifndef TNY_GTK_HEADER_LIST_MODEL_H
#define TNY_GTK_HEADER_LIST_MODEL_H

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

#include <gtk/gtktreemodel.h>
#include <tny-header.h>
#include <tny-folder.h>
#include <tny-list.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_HEADER_LIST_MODEL             (tny_gtk_header_list_model_get_type ())
#define TNY_GTK_HEADER_LIST_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_HEADER_LIST_MODEL, TnyGtkHeaderListModel))
#define TNY_GTK_HEADER_LIST_MODEL_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_HEADER_LIST_MODEL, TnyGtkHeaderListModelClass))
#define TNY_IS_GTK_HEADER_LIST_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_HEADER_LIST_MODEL))
#define TNY_IS_GTK_HEADER_LIST_MODEL_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_HEADER_LIST_MODEL))
#define TNY_GTK_HEADER_LIST_MODEL_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_HEADER_LIST_MODEL, TnyGtkHeaderListModelClass))

/* Implements GtkTreeModel and TnyList */

typedef struct _TnyGtkHeaderListModel TnyGtkHeaderListModel;
typedef struct _TnyGtkHeaderListModelClass TnyGtkHeaderListModelClass;

#define TNY_TYPE_GTK_HEADER_LIST_MODEL_COLUMN (tny_gtk_header_list_model_column_get_type())

typedef enum
{
	TNY_GTK_HEADER_LIST_MODEL_FROM_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_TO_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_SUBJECT_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_CC_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_TIME_T_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_DATE_SENT_TIME_T_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_DATE_RECEIVED_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_MESSAGE_SIZE_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_INSTANCE_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_FLAGS_COLUMN,
	TNY_GTK_HEADER_LIST_MODEL_N_COLUMNS
} TnyGtkHeaderListModelColumn;

struct _TnyGtkHeaderListModel 
{
	GObject parent;
	gpointer priv;
};

struct _TnyGtkHeaderListModelClass 
{
	GObjectClass parent;
};

GType tny_gtk_header_list_model_get_type (void);
GType tny_gtk_header_list_model_column_get_type (void);

GtkTreeModel* tny_gtk_header_list_model_new (void);
void tny_gtk_header_list_model_set_folder (TnyGtkHeaderListModel *self, TnyFolder *folder, gboolean refresh, TnyGetHeadersCallback callback, TnyStatusCallback status_callback, gpointer user_data);
gint tny_gtk_header_list_model_received_date_sort_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
gint tny_gtk_header_list_model_sent_date_sort_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
void tny_gtk_header_list_model_set_no_duplicates (TnyGtkHeaderListModel *self, gboolean setting);
gboolean tny_gtk_header_list_model_get_no_duplicates (TnyGtkHeaderListModel *self);
void tny_gtk_header_list_model_set_show_latest (TnyGtkHeaderListModel *self, gint show_latest_n);
gint tny_gtk_header_list_model_get_show_latest (TnyGtkHeaderListModel *self);

G_END_DECLS

#endif
