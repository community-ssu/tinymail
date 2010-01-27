#ifndef TNY_PAIR_H
#define TNY_PAIR_H

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

G_BEGIN_DECLS

#define TNY_TYPE_PAIR             (tny_pair_get_type ())
#define TNY_PAIR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_PAIR, TnyPair))
#define TNY_PAIR_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_PAIR, TnyPairClass))
#define TNY_IS_PAIR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_PAIR))
#define TNY_IS_PAIR_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_PAIR))
#define TNY_PAIR_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_PAIR, TnyPairClass))

#ifndef TNY_SHARED_H
typedef struct _TnyPair TnyPair;
typedef struct _TnyPairClass TnyPairClass;
#endif

struct _TnyPair
{
	GObject parent;
};

struct _TnyPairClass 
{
	GObjectClass parent;
};

GType  tny_pair_get_type (void);
TnyPair* tny_pair_new (const gchar *name, const gchar *value);

void tny_pair_set_name (TnyPair *self, const gchar *name);
const gchar* tny_pair_get_name (TnyPair *self);
void tny_pair_set_value (TnyPair *self, const gchar *value);
const gchar* tny_pair_get_value (TnyPair *self);

G_END_DECLS

#endif

