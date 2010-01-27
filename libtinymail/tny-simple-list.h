#ifndef TNY_SIMPLE_LIST_H
#define TNY_SIMPLE_LIST_H

/* libtinymailui-gtk - The Tiny Mail library
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
#include <tny-shared.h>
#include <tny-list.h>
#include <tny-iterator.h>

G_BEGIN_DECLS

#define TNY_TYPE_SIMPLE_LIST             (tny_simple_list_get_type ())
#define TNY_SIMPLE_LIST(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_SIMPLE_LIST, TnySimpleList))
#define TNY_SIMPLE_LIST_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_SIMPLE_LIST, TnySimpleListClass))
#define TNY_IS_SIMPLE_LIST(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_SIMPLE_LIST))
#define TNY_IS_SIMPLE_LIST_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_SIMPLE_LIST))
#define TNY_SIMPLE_LIST_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_SIMPLE_LIST, TnySimpleListClass))

struct _TnySimpleList 
{
	GObject parent;
};

struct _TnySimpleListClass 
{
	GObjectClass parent;
};

GType tny_simple_list_get_type (void);
TnyList* tny_simple_list_new (void);

G_END_DECLS

#endif
