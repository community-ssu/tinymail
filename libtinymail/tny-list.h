#ifndef TNY_LIST_H
#define TNY_LIST_H

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
#include <tny-iterator.h>

G_BEGIN_DECLS

#define TNY_TYPE_LIST             (tny_list_get_type ())
#define TNY_LIST(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_LIST, TnyList))
#define TNY_IS_LIST(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_LIST))
#define TNY_LIST_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_LIST, TnyListIface))

#ifndef TNY_SHARED_H
typedef struct _TnyList TnyList;
typedef struct _TnyListIface TnyListIface;
#endif

typedef gboolean (*TnyListMatcher) (TnyList *list, GObject *item, gpointer match_data);

struct _TnyListIface
{
	GTypeInterface parent;

	guint (*get_length) (TnyList *self);
	void (*prepend) (TnyList *self, GObject* item);
	void (*append) (TnyList *self, GObject* item);
	void (*remove) (TnyList *self, GObject* item);
	void (*foreach) (TnyList *self, GFunc func, gpointer user_data);
	TnyList* (*copy) (TnyList *self);
	TnyIterator* (*create_iterator) (TnyList *self);
	void (*remove_matches) (TnyList *self, TnyListMatcher matcher, gpointer match_data);
};


GType tny_list_get_type (void);

guint tny_list_get_length (TnyList *self);
void tny_list_prepend (TnyList *self, GObject* item);
void tny_list_append (TnyList *self, GObject* item);
void tny_list_remove (TnyList *self, GObject* item);
void tny_list_foreach (TnyList *self, GFunc func, gpointer user_data);
TnyIterator* tny_list_create_iterator (TnyList *self);
TnyList *tny_list_copy (TnyList *self);
void tny_list_remove_matches (TnyList *self, TnyListMatcher matcher, gpointer match_data);

G_END_DECLS

#endif
