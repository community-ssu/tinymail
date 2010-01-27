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

/**
 * TnyList:
 * 
 * A list of things
 *
 * free-function: g_object_unref
 * type-parameter: G
 */

#include <config.h>

#include <tny-list.h>

/**
 * tny_list_get_length:
 * @self: A #TnyList
 *
 * Get the amount of items in @self.
 *
 * returns: the length of the list
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
guint
tny_list_get_length (TnyList *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_LIST (self));
	g_assert (TNY_LIST_GET_IFACE (self)->get_length!= NULL);
#endif

	return TNY_LIST_GET_IFACE (self)->get_length(self);
}

/**
 * tny_list_prepend:
 * @self: A #TnyList
 * @item: (type-parameter G): the item to prepend
 *
 * Prepends an item to a list. You can only prepend items that inherit from the
 * GObject base item. That's because Tinymail's list infrastructure does 
 * reference counting. Consider using a doubly linked list, a pointer array or 
 * any other list-type available on your development platform for non-GObject
 * lists.
 *
 * The @item you add will get a reference added by @self, with @self claiming 
 * ownership of @item. When @self is finalised, that reference is taken from
 * @item.
 *
 * Prepending an item invalidates all existing iterators and puts them in an 
 * unspecified state. You'll need to recreate the iterator(s) when prepending 
 * items.
 *
 * When implementing, if you have to choose, make this one the fast one
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void
tny_list_prepend (TnyList *self, GObject* item)
{
#ifdef DBC /* require */
	gint length = tny_list_get_length (self);
	g_assert (TNY_IS_LIST (self));
	g_assert (item);
	g_assert (G_IS_OBJECT (item));
	g_assert (TNY_LIST_GET_IFACE (self)->prepend!= NULL);
#endif

	TNY_LIST_GET_IFACE (self)->prepend(self, item);

#ifdef DBC /* ensure */
	g_assert (tny_list_get_length (self) == length + 1);
#endif

	return;
}

/**
 * tny_list_append:
 * @self: A #TnyList
 * @item: (type-parameter G): the item to prepend
 *
 * Appends an item to a list. You can only prepend items that inherit from the
 * GObject base item. That's because Tinymail's list infrastructure does 
 * reference counting. Consider using a doubly linked list, a pointer array or 
 * any other list-type available on your development platform for non-GObject
 * lists.
 *
 * The @item you add will get a reference added by @self, with @self claiming 
 * ownership of @item. When @self is finalised, that reference is taken from
 * @item.
 *
 * Appending an item invalidates all existing iterators and puts them in an 
 * unspecified state. You'll need to recreate the iterator(s) when appending 
 * items.
 *
 * When implementing, if you have to choose, make the prepend one the fast one
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
 void 
tny_list_append (TnyList *self, GObject* item)
{
#ifdef DBC /* require */
	gint length = tny_list_get_length (self);
	g_assert (TNY_IS_LIST (self));
	g_assert (item);
	g_assert (G_IS_OBJECT (item));
	g_assert (TNY_LIST_GET_IFACE (self)->append!= NULL);
#endif

	TNY_LIST_GET_IFACE (self)->append(self, item); 

#ifdef DBC /* ensure */
	g_assert (tny_list_get_length (self) == length + 1);
#endif

	return;
}

/**
 * tny_list_remove:
 * @self: A #TnyList
 * @item: (type-parameter G): the item to remove
 *
 * Removes an item from a list.  Removing a item invalidates all existing
 * iterators and puts them in an unspecified state. You'll need to recreate
 * the iterator(s) when removing an item.
 *
 * If you want to clear a list, consider using the tny_list_foreach() or simply
 * destroy the list instance and construct a new one. If you want to remove
 * specific items from a list, consider using a second list. You should not
 * attempt to remove items from a list while an iterator is active on the
 * same list.
 * 
 * Removing @item from @self will decrease the reference count of @item by one.
 *
 * Example (removing even items):
 * <informalexample><programlisting>
 * TnyList *toremovefrom = ...
 * TnyList *removethese = tny_simple_list_new ();
 * TnyIterator *iter = tny_list_create_iterator (toremovefrom);
 * int i = 0;
 * while (!tny_iterator_is_done (iter))
 * {
 *      if (i % 2 == 0)
 *      {
 *           GObject *obj = tny_iterator_get_current (iter);
 *           tny_list_prepend (removethese, obj);
 *           g_object_unref (G_OBJECT (obj));
 *      }
 *      i++;
 *      tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * iter = tny_list_create_iterator (removethese);
 * while (!tny_iterator_is_done (iter))
 * {
 *      GObject *obj = tny_iterator_get_current (iter);
 *      tny_list_remove (toremovefrom, obj);
 *      g_object_unref (G_OBJECT (obj));
 *      tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * g_object_unref (removethese);
 * g_object_unref (toremovefrom);
 * </programlisting></informalexample>
 *
 **/
void 
tny_list_remove (TnyList *self, GObject* item)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_LIST (self));
	g_assert (item);
	g_assert (G_IS_OBJECT (item));
	g_assert (TNY_LIST_GET_IFACE (self)->remove!= NULL);
#endif

	TNY_LIST_GET_IFACE (self)->remove(self, item);

#ifdef DBC /* ensure */
#endif

	return;
}


/**
 * tny_list_remove_matches:
 * @self: A #TnyList
 * @matcher: a #TnyListMatcher to match @match_data against items
 * @match_data: data used by the comparer to remove items
 *
 * Removes items from a list.  Removing items invalidates all existing
 * iterators and put them in an unknown and unspecified state. You'll need to 
 * recreate the iterator(s) when removing items.
 *
 * Example (items that match):
 * <informalexample><programlisting>
 * static void 
 * matcher (TnyList *list, GObject *item, gpointer match_data) {
 *     if (!strcmp (tny_header_get_subject ((TnyHeader *) item), match_data))
 *          return TRUE;
 *     return FALSE;
 * }
 * TnyList *headers = ...
 * tny_list_remove_matches (headers, matcher, "Remove subject");
 * </programlisting></informalexample>
 *
 * There's no guarantee whatsoever that existing iterators of @self will be
 * valid after this method returned. 
 *
 * Note that if you didn't remove the initial reference when putting the item
 * in the list, this remove will not take care of that initial reference either. 
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void 
tny_list_remove_matches (TnyList *self, TnyListMatcher matcher, gpointer match_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_LIST (self));
	g_assert (matcher);
#endif

	if (TNY_LIST_GET_IFACE (self)->remove_matches)
		TNY_LIST_GET_IFACE (self)->remove_matches(self, matcher, match_data);
	else {
		GList *to_remove = NULL, *copy;
		TnyIterator *iter = tny_list_create_iterator (self);

		while (!tny_iterator_is_done (iter)) {
			GObject *item = tny_iterator_get_current (iter);
			if (matcher (self, item, match_data)) {
				to_remove = g_list_prepend (to_remove, 
					g_object_ref (item));
			}
			g_object_unref (item);
			tny_iterator_next (iter);
		}
		g_object_unref (iter);

		copy = to_remove;
		while (to_remove) {
			GObject *item = to_remove->data;
			tny_list_remove (self, item);
			g_object_unref (item);
			to_remove = g_list_next (to_remove);
		}

		if (copy)
			g_list_free (copy);
	}

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_list_create_iterator:
 * @self: A #TnyList
 *
 * Creates a new iterator instance for the list. The initial position of the
 * iterator is the first element.
 *
 * An iterator keeps a position state of a list iteration. The list itself does
 * not keep any position information. Using multiple iterator instances makes
 * it possible to have simultaneous list iterations (i.e. multiple threads or
 * in a loop that uses with multiple position states in a single list).
 *
 * Example:
 * <informalexample><programlisting>
 * TnyList *list = tny_simple_list_new ();
 * TnyIterator *iter1 = tny_list_create_iterator (list);
 * TnyIterator *iter2 = tny_list_create_iterator (list);
 * while (!tny_iterator_is_done (iter1))
 * {
 *      while (!tny_iterator_is_done (iter2))
 *            tny_iterator_next (iter2);
 *      tny_iterator_next (iter1);
 * }
 * g_object_unref (iter1);
 * g_object_unref (iter2);
 * g_object_unref (list);
 * </programlisting></informalexample>
 *
 * The reason why the method isn't called get_iterator is because it's a object
 * creation method (a factory method). Therefore it's not a property. Instead
 * it creates a new instance of an iterator. The returned iterator object should
 * (therefore) be unreferenced after use.
 * 
 * returns: (caller-owns) (type-parameter G): A new iterator for the list
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyIterator* 
tny_list_create_iterator (TnyList *self)
{
	TnyIterator *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_LIST (self));
	g_assert (TNY_LIST_GET_IFACE (self)->create_iterator!= NULL);
#endif

	retval = TNY_LIST_GET_IFACE (self)->create_iterator(self);

#ifdef DBC /* ensure */
	g_assert (TNY_IS_ITERATOR (retval));
#endif

	return retval;
}

/**
 * tny_list_foreach:
 * @self: A #TnyList
 * @func: the function to call with each element's data.
 * @user_data: user data to pass to the function.
 *
 * Calls @func for each element in a #TnyList. It will use an internal iteration
 * which you don't have to worry about as application developer. 
 *
 * Example:
 * <informalexample><programlisting>
 * static void
 * list_foreach_item (TnyHeader *header, gpointer user_data)
 * {
 *      g_print ("%s\n", tny_header_get_subject (header));
 * }
 * </programlisting></informalexample>
 *
 * <informalexample><programlisting>
 * TnyFolder *folder = ...
 * TnyList *headers = tny_simple_list_new ();
 * tny_folder_get_headers (folder, headers, FALSE);
 * tny_list_foreach (headers, list_foreach_item, NULL);
 * g_object_unref (G_OBJECT (list));
 * </programlisting></informalexample>
 *
 * The purpose of this method is to have a fast foreach iteration. Using this
 * is faster than inventing your own foreach loop using the is_done and next
 * methods. The order is guaranteed to be the first element first, the last 
 * element last. If during the iteration you don't remove items, it's guaranteed
 * that all current items will be iterated.
 *
 * In the func implementation and during the foreach operation you must not
 * append, remove nor prepend items to the list. In multithreaded environments
 * it's advisable to introduce a lock when using this functionality. 
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void 
tny_list_foreach (TnyList *self, GFunc func, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_LIST (self));
	g_assert (func);
	g_assert (TNY_LIST_GET_IFACE (self)->foreach!= NULL);
#endif

	TNY_LIST_GET_IFACE (self)->foreach(self, func, user_data);
	return;
}


/**
 * tny_list_copy:
 * @self: A #TnyList
 *
 * Creates a shallow copy of @self: it doesn't copy the content of the items.
 * It creates a new list with new references to the same items. The items will
 * get an extra reference added for the new list being their second owner,
 * setting their reference count to for example two. Which means that the 
 * returned value must be unreferenced after use.
 *
 * returns: (caller-owns) (type-parameter G): A copy of this list
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyList*
tny_list_copy (TnyList *self)
{
	TnyList *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_LIST (self));
	g_assert (TNY_LIST_GET_IFACE (self)->copy!= NULL);
#endif

	retval = TNY_LIST_GET_IFACE (self)->copy(self);

#ifdef DBC /* ensure */
	g_assert (TNY_IS_LIST (retval));
#endif

	return retval;
}

static void
tny_list_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_list_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyListIface),
			tny_list_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,   /* instance_init */
			NULL
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyList", &info, 0);

	return GUINT_TO_POINTER (type);
}

GType
tny_list_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_list_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
