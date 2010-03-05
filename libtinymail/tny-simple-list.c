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

/**
 * TnySimpleList:
 *
 * A simple list
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-simple-list.h>


static GObjectClass *parent_class;

#include "tny-simple-list-priv.h"
#include "tny-simple-list-iterator-priv.h"


static void
tny_simple_list_append (TnyList *self, GObject* item)
{
	TnySimpleListPriv *priv = TNY_SIMPLE_LIST_GET_PRIVATE (self);

	g_mutex_lock (priv->iterator_lock);
	g_object_ref (G_OBJECT (item));
	priv->first = g_list_append (priv->first, item);
	g_mutex_unlock (priv->iterator_lock);

	return;
}

static void
tny_simple_list_prepend (TnyList *self, GObject* item)
{
	TnySimpleListPriv *priv = TNY_SIMPLE_LIST_GET_PRIVATE (self);

	g_mutex_lock (priv->iterator_lock);
	g_object_ref (G_OBJECT (item));
	priv->first = g_list_prepend (priv->first, item);
	g_mutex_unlock (priv->iterator_lock);

	return;
}


static guint
tny_simple_list_get_length (TnyList *self)
{
	TnySimpleListPriv *priv = TNY_SIMPLE_LIST_GET_PRIVATE (self);
	guint retval = 0;

	g_mutex_lock (priv->iterator_lock);
	retval = priv->first?g_list_length (priv->first):0;
	g_mutex_unlock (priv->iterator_lock);

	return retval;
}

static void
tny_simple_list_remove (TnyList *self, GObject* item)
{
	TnySimpleListPriv *priv = TNY_SIMPLE_LIST_GET_PRIVATE (self);
	GList *link;

	g_mutex_lock (priv->iterator_lock);
	link = g_list_find (priv->first, item);
	if (link) {
		priv->first = g_list_delete_link (priv->first, link);
		g_object_unref (G_OBJECT (item));
	}
	g_mutex_unlock (priv->iterator_lock);

	return;
}

static TnyIterator*
tny_simple_list_create_iterator (TnyList *self)
{
	return _tny_simple_list_iterator_new (TNY_SIMPLE_LIST (self));
}

static TnyList*
tny_simple_list_copy_the_simple_list (TnyList *self)
{
	TnySimpleList *copy = g_object_new (TNY_TYPE_SIMPLE_LIST, NULL);
	TnySimpleListPriv *priv = TNY_SIMPLE_LIST_GET_PRIVATE (self);
	TnySimpleListPriv *cpriv = TNY_SIMPLE_LIST_GET_PRIVATE (copy);
	GList *list_copy = NULL;

	g_mutex_lock (priv->iterator_lock);
	list_copy = g_list_copy (priv->first);
	g_list_foreach (list_copy, (GFunc)g_object_ref, NULL);
	cpriv->first = list_copy;
	g_mutex_unlock (priv->iterator_lock);

	return TNY_LIST (copy);
}

static void 
tny_simple_list_foreach_in_the_simple_list (TnyList *self, GFunc func, gpointer user_data)
{
	TnySimpleListPriv *priv = TNY_SIMPLE_LIST_GET_PRIVATE (self);

	g_mutex_lock (priv->iterator_lock);
	g_list_foreach (priv->first, func, user_data);
	g_mutex_unlock (priv->iterator_lock);

	return;
}

static void
tny_list_init (TnyListIface *klass)
{
	klass->get_length= tny_simple_list_get_length;
	klass->prepend= tny_simple_list_prepend;
	klass->append= tny_simple_list_append;
	klass->remove= tny_simple_list_remove;
	klass->create_iterator= tny_simple_list_create_iterator;
	klass->copy= tny_simple_list_copy_the_simple_list;
	klass->foreach= tny_simple_list_foreach_in_the_simple_list;

	return;
}

static void
destroy_item (gpointer item, gpointer user_data)
{
	if (item && G_IS_OBJECT (item))
		g_object_unref (G_OBJECT (item));
}

static void
tny_simple_list_finalize (GObject *object)
{
	TnySimpleListPriv *priv = TNY_SIMPLE_LIST_GET_PRIVATE (object);

	g_mutex_lock (priv->iterator_lock);

	if (priv->first)
	{
		g_list_foreach (priv->first, destroy_item, NULL);
		g_list_free (priv->first);
		priv->first = NULL;
	}
	g_mutex_unlock (priv->iterator_lock);

	g_mutex_free (priv->iterator_lock);
	priv->iterator_lock = NULL;

	parent_class->finalize (object);

	return;
}


static void
tny_simple_list_class_init (TnySimpleListClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *)klass;
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = tny_simple_list_finalize;
	g_type_class_add_private (object_class, sizeof (TnySimpleListPriv));

	return;
}

static void
tny_simple_list_init (TnySimpleList *self)
{
	TnySimpleListPriv *priv = TNY_SIMPLE_LIST_GET_PRIVATE (self);

	priv->iterator_lock = g_mutex_new ();
	priv->first = NULL;

	return;
}


/**
 * tny_simple_list_new:
 * 
 * Create a general purpose #TnyList instance
 *
 * returns: (caller-owns): A general purpose #TnyList instance
 **/
TnyList*
tny_simple_list_new (void)
{
	TnySimpleList *self = g_object_new (TNY_TYPE_SIMPLE_LIST, NULL);

	return TNY_LIST (self);
}

static gpointer
tny_simple_list_register_type (gpointer notused)
{
	GType object_type = 0;

	static const GTypeInfo object_info = 
		{
			sizeof (TnySimpleListClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) tny_simple_list_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (TnySimpleList),
			0,              /* n_preallocs */
			(GInstanceInitFunc) tny_simple_list_init,
			NULL
		};
	
	static const GInterfaceInfo tny_list_info = {
		(GInterfaceInitFunc) tny_list_init,
		NULL,
		NULL
	};
	
	object_type = g_type_register_static (G_TYPE_OBJECT, 
					      "TnySimpleList", &object_info, 0);
	
	g_type_add_interface_static (object_type, TNY_TYPE_LIST,
				     &tny_list_info);

	return GSIZE_TO_POINTER (object_type);
}

GType
tny_simple_list_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_simple_list_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
