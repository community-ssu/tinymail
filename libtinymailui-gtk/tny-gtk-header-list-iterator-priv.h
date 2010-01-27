#ifndef TNY_GTK_HEADER_LIST_ITERATOR_H
#define TNY_GTK_HEADER_LIST_ITERATOR_H

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
#include <tny-list.h>
#include <tny-iterator.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_HEADER_LIST_ITERATOR             (_tny_gtk_header_list_iterator_get_type ())
#define TNY_GTK_HEADER_LIST_ITERATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_HEADER_LIST_ITERATOR, TnyGtkHeaderListIterator))
#define TNY_GTK_HEADER_LIST_ITERATOR_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_HEADER_LIST_ITERATOR, TnyGtkHeaderListIteratorClass))
#define TNY_IS_GTK_HEADER_LIST_ITERATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_HEADER_LIST_ITERATOR))
#define TNY_IS_GTK_HEADER_LIST_ITERATOR_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_HEADER_LIST_ITERATOR))
#define TNY_GTK_HEADER_LIST_ITERATOR_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_HEADER_LIST_ITERATOR, TnyGtkHeaderListIteratorClass))

typedef struct _TnyGtkHeaderListIterator TnyGtkHeaderListIterator;
typedef struct _TnyGtkHeaderListIteratorClass TnyGtkHeaderListIteratorClass;

struct _TnyGtkHeaderListIterator
{
	GObject parent;
	TnyGtkHeaderListModel *model;
	guint current;
};

struct _TnyGtkHeaderListIteratorClass 
{
	GObjectClass parent;
};

TnyIterator* _tny_gtk_header_list_iterator_new (TnyGtkHeaderListModel *model);

gboolean _tny_gtk_header_list_iterator_is_done_nl (TnyGtkHeaderListIterator *self);
gpointer _tny_gtk_header_list_iterator_get_current_nl (TnyGtkHeaderListIterator *me);
void _tny_gtk_header_list_iterator_nth_nl (TnyGtkHeaderListIterator *me, guint nth);
void _tny_gtk_header_list_iterator_first_nl (TnyGtkHeaderListIterator *me);
void _tny_gtk_header_list_iterator_prev_nl (TnyGtkHeaderListIterator *me);
void _tny_gtk_header_list_iterator_next_nl (TnyGtkHeaderListIterator *me);



#define TNY_GTK_HEADER_LIST_MODEL_GET_PRIVATE(o)	(TnyGtkHeaderListModelPriv*)(((TnyGtkHeaderListModel*)(o))->priv)

typedef struct _TnyGtkHeaderListModelPriv TnyGtkHeaderListModelPriv;

struct _TnyGtkHeaderListModelPriv
{
	GStaticRecMutex *iterator_lock;
	TnyFolder *folder;
	gint stamp, registered;
	gint updating_views;
	GMutex  *ra_lock, *to_lock;
	gint cur_len;
	guint add_timeout;

	guint timeout_span;
	GPtrArray *items;
	GArray *del_timeouts;
	TnyIterator *iterator;
	gboolean no_duplicates;
	gint show_latest;
	GPtrArray *not_latest_items;
	time_t oldest_received;
};

G_END_DECLS

#endif

