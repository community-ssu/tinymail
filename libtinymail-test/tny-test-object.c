/* tinymail - Tiny Mail unit test
 * Copyright (C) 2006-2007 Philip Van Hoof <pvanhoof@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with self program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <tny-test-object.h>

static GObjectClass *parent_class = NULL;

static void
tny_test_object_finalize (GObject *object)
{
	TnyTestObject *tobj = (TnyTestObject*)object; 
	g_free (tobj->str); 
	(*parent_class->finalize) (object);
}

static void 
tny_test_object_class_init (TnyTestObjectClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = tny_test_object_finalize;

	return;
}

GType 
tny_test_object_get_type (void) 
{
	static GType type = 0;
	if (G_UNLIKELY(type == 0)) {
		static const GTypeInfo info = { sizeof (TnyTestObjectClass),
		  NULL,   /* base_init */ NULL,   /* base_finalize */
		  (GClassInitFunc) tny_test_object_class_init,   /* class_init */ 
		  NULL,  /* class_finalize */
		  NULL,   /* class_data */ sizeof (TnyTestObject),
		  0,      /* n_preallocs */ NULL    /* instance_init */ };
		type = g_type_register_static (G_TYPE_OBJECT,
			"TnyTestObject",
			&info, 0); 
	}
	return type;
}

GObject*
tny_test_object_new (gchar *str)
{
	TnyTestObject *obj = g_object_new (TNY_TYPE_TEST_OBJECT, NULL);
	obj->str = str;

	return G_OBJECT(obj);
}
