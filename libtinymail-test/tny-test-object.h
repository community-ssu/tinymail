#ifndef TNY_TEST_OBJECT_H
#define TNY_TEST_OBJECT_H

/* tinymail - Tiny Mail unit test
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

G_BEGIN_DECLS

#define TNY_TYPE_TEST_OBJECT             (tny_test_object_get_type ())
#define TNY_TEST_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_TEST_OBJECT, TnyTestObject))
#define TNY_TEST_OBJECT_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_TEST_OBJECT, TnyTestObjectClass))
#define TNY_IS_TEST_OBJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_TEST_OBJECT))
#define TNY_IS_TEST_OBJECT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_TEST_OBJECT))
#define TNY_TEST_OBJECT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_TEST_OBJECT, TnyTestObjectClass))


typedef struct _TnyTestObject TnyTestObject;
typedef struct _TnyTestObjectClass TnyTestObjectClass;

struct _TnyTestObject {
	GObject parent;
	gchar *str;
};

struct _TnyTestObjectClass 
{
	GObjectClass parent;
};

GObject* tny_test_object_new (gchar *str);

G_END_DECLS

#endif
