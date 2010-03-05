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

#include <config.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-simple-list.h>

#include "tny-simple-list-priv.h"
#include "tny-simple-list-iterator-priv.h"

static GObjectClass *parent_class = NULL;


void 
_tny_simple_list_iterator_set_model (TnySimpleListIterator *self, TnySimpleList *model)
{
	TnySimpleListPriv *lpriv;

	if (self->model)
		g_object_unref (self->model);

	self->model = g_object_ref (model);

	lpriv = TNY_SIMPLE_LIST_GET_PRIVATE (self->model);

	g_mutex_lock (lpriv->iterator_lock);
	self->current = lpriv->first;
	g_mutex_unlock (lpriv->iterator_lock);

	return;
}



TnyIterator*
_tny_simple_list_iterator_new (TnySimpleList *model)
{
	TnySimpleListIterator *self = g_object_new (TNY_TYPE_SIMPLE_LIST_ITERATOR, NULL);

	_tny_simple_list_iterator_set_model (self, model);

	return TNY_ITERATOR (self);
}

static void
tny_simple_list_iterator_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnySimpleListIterator *self = (TnySimpleListIterator *)instance;

	self->model = NULL;
	self->current = NULL;

	return;
}

static void
tny_simple_list_iterator_finalize (GObject *object)
{
	TnySimpleListIterator *self = (TnySimpleListIterator *) object;

	if (self->model)
		g_object_unref (self->model);

	(*parent_class->finalize) (object);

	return;
}


static void 
tny_simple_list_iterator_next (TnyIterator *self)
{
	TnySimpleListIterator *me = (TnySimpleListIterator*) self;
	TnySimpleListPriv *lpriv;

	if (G_UNLIKELY (!me || !me->current || !me->model))
		return;

	lpriv = TNY_SIMPLE_LIST_GET_PRIVATE (me->model);

	g_mutex_lock (lpriv->iterator_lock);
	me->current = g_list_next (me->current);
	g_mutex_unlock (lpriv->iterator_lock);

	return;
}

static void 
tny_simple_list_iterator_prev (TnyIterator *self)
{
	TnySimpleListIterator *me = (TnySimpleListIterator*) self;
	TnySimpleListPriv *lpriv;

	if (G_UNLIKELY (!me || !me->current || !me->model))
		return;

	lpriv = TNY_SIMPLE_LIST_GET_PRIVATE (me->model);

	g_mutex_lock (lpriv->iterator_lock);
	me->current = g_list_previous (me->current);
	g_mutex_unlock (lpriv->iterator_lock);

	return;
}

static void 
tny_simple_list_iterator_first (TnyIterator *self)
{
	TnySimpleListIterator *me = (TnySimpleListIterator*) self;
	TnySimpleListPriv *lpriv;

	if (G_UNLIKELY (!me || !me->current || !me->model))
		return;

	lpriv = TNY_SIMPLE_LIST_GET_PRIVATE (me->model);

	g_mutex_lock (lpriv->iterator_lock);
	me->current = lpriv->first;
	g_mutex_unlock (lpriv->iterator_lock);

	return;
}


static gboolean 
tny_simple_list_iterator_is_done (TnyIterator *self)
{
	TnySimpleListIterator *me = (TnySimpleListIterator*) self;

	return me->current == NULL;
}



static void
tny_simple_list_iterator_nth (TnyIterator *self, guint nth)
{
	TnySimpleListIterator *me = (TnySimpleListIterator*) self;
	TnySimpleListPriv *lpriv;

	if (G_UNLIKELY (!me || !me->current || !me->model))
		return;

	lpriv = TNY_SIMPLE_LIST_GET_PRIVATE (me->model);

	/* Move the iterator to the nth node. We'll count from zero,
	   so we start with the first node of which we know the model
	   stored a reference. */

	g_mutex_lock (lpriv->iterator_lock);
	me->current = g_list_nth (lpriv->first, nth);
	g_mutex_unlock (lpriv->iterator_lock);

	return;
}


static GObject* 
tny_simple_list_iterator_get_current (TnyIterator *self)
{
	TnySimpleListIterator *me = (TnySimpleListIterator*) self;
	gpointer retval;
	TnySimpleListPriv *lpriv;

	if (G_UNLIKELY (!me || !me->model))
		return NULL;

	lpriv = TNY_SIMPLE_LIST_GET_PRIVATE (me->model);

	g_mutex_lock (lpriv->iterator_lock);
	retval = (G_UNLIKELY (me->current)) ? me->current->data : NULL;
	g_mutex_unlock (lpriv->iterator_lock);

	if (retval) 
		g_object_ref (G_OBJECT(retval));

	return (GObject*)retval;
}


static TnyList* 
tny_simple_list_iterator_get_list (TnyIterator *self)
{
	TnySimpleListIterator *me = (TnySimpleListIterator*) self;

	/* Return the list */

	if (G_UNLIKELY (!me || !me->model))
		return NULL;

    	g_object_ref (G_OBJECT (me->model));
			  
	return TNY_LIST (me->model);
}

static void
tny_iterator_init (TnyIteratorIface *klass)
{

	klass->next= tny_simple_list_iterator_next;
	klass->prev= tny_simple_list_iterator_prev;
	klass->first= tny_simple_list_iterator_first;
	klass->nth= tny_simple_list_iterator_nth;
	klass->get_current= tny_simple_list_iterator_get_current;
	klass->get_list= tny_simple_list_iterator_get_list;
	klass->is_done = tny_simple_list_iterator_is_done;
	
	return;
}

static void 
tny_simple_list_iterator_class_init (TnySimpleListIteratorClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	object_class->finalize = tny_simple_list_iterator_finalize;

	return;
}

static gpointer
_tny_simple_list_iterator_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnySimpleListIteratorClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_simple_list_iterator_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnySimpleListIterator),
			0,      /* n_preallocs */
			tny_simple_list_iterator_instance_init,    /* instance_init */
			NULL
		};
	
	static const GInterfaceInfo tny_iterator_info = 
		{
			(GInterfaceInitFunc) tny_iterator_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnySimpleListIterator",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_ITERATOR, 
				     &tny_iterator_info);

	return GSIZE_TO_POINTER (type);
}

GType 
_tny_simple_list_iterator_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, _tny_simple_list_iterator_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
