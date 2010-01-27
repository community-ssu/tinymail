#ifndef TNY_ITERATOR_H
#define TNY_ITERATOR_H

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

G_BEGIN_DECLS

#define TNY_TYPE_ITERATOR             (tny_iterator_get_type ())
#define TNY_ITERATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_ITERATOR, TnyIterator))
#define TNY_IS_ITERATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_ITERATOR))
#define TNY_ITERATOR_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_ITERATOR, TnyIteratorIface))

#ifndef TNY_SHARED_H
typedef struct _TnyIterator TnyIterator;
typedef struct _TnyIteratorIface TnyIteratorIface;
#endif

struct _TnyIteratorIface
{
	GTypeInterface parent;

	void (*next) (TnyIterator *self);
	void (*prev) (TnyIterator *self);
	void (*first) (TnyIterator *self);
	void (*nth) (TnyIterator *self, guint nth);
	GObject* (*get_current) (TnyIterator *self);

	gboolean (*is_done) (TnyIterator *self);
	TnyList* (*get_list) (TnyIterator *self);
};

GType tny_iterator_get_type (void);

void tny_iterator_next (TnyIterator *self);
void tny_iterator_prev (TnyIterator *self);
void tny_iterator_first (TnyIterator *self);
void tny_iterator_nth (TnyIterator *self, guint nth);
GObject* tny_iterator_get_current (TnyIterator *self);
gboolean tny_iterator_is_done (TnyIterator *self);
TnyList* tny_iterator_get_list (TnyIterator *self);

G_END_DECLS

#endif
