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

#include <tny-gtk-header-list-model.h>

static GObjectClass *parent_class = NULL;

#include "tny-gtk-header-list-iterator-priv.h"

GType _tny_gtk_header_list_iterator_get_type (void);



static void 
_tny_gtk_header_list_iterator_set_model (TnyGtkHeaderListIterator *self, TnyGtkHeaderListModel *model)
{
	if (self->model)
		g_object_unref (self->model);

	self->model = g_object_ref (model);

	/* It's not a list_copy, so don't free this list when 
	   destructing this iterator. Current is used as a ptr
	   to the 'current' GList node. 

	   When the iterator starts, it points to 'start', or,
	   the first node in the list. */

	self->current = 0;

	return;
}



TnyIterator*
_tny_gtk_header_list_iterator_new (TnyGtkHeaderListModel *model)
{
	TnyGtkHeaderListIterator *self = g_object_new (TNY_TYPE_GTK_HEADER_LIST_ITERATOR, NULL);

	_tny_gtk_header_list_iterator_set_model (self, model);

	return TNY_ITERATOR (self);
}

static void
tny_gtk_header_list_iterator_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyGtkHeaderListIterator *self = (TnyGtkHeaderListIterator *)instance;

	self->model = NULL;
	self->current = 0;

	return;
}

static void
tny_gtk_header_list_iterator_finalize (GObject *object)
{
	TnyGtkHeaderListIterator *self = (TnyGtkHeaderListIterator *) object;

	if (self->model)
		g_object_unref (self->model);

	(*parent_class->finalize) (object);

	return;
}


void 
_tny_gtk_header_list_iterator_next_nl (TnyGtkHeaderListIterator *me)
{
	me->current++;
	return;
}

static void 
tny_gtk_header_list_iterator_next (TnyIterator *self)
{
	TnyGtkHeaderListIterator *me = (TnyGtkHeaderListIterator*) self;
	TnyGtkHeaderListModelPriv *mpriv;

	if (G_UNLIKELY (!me || !me->model))
		return;

	mpriv = TNY_GTK_HEADER_LIST_MODEL_GET_PRIVATE (me->model);

	/* Move the iterator to the next node */

	g_static_rec_mutex_lock (mpriv->iterator_lock);
	me->current++;
	g_static_rec_mutex_unlock (mpriv->iterator_lock);

	return;
}

void
_tny_gtk_header_list_iterator_prev_nl (TnyGtkHeaderListIterator *me)
{
	me->current--;
	return;
}

static void
tny_gtk_header_list_iterator_prev (TnyIterator *self)
{
	TnyGtkHeaderListIterator *me = (TnyGtkHeaderListIterator*) self;
	TnyGtkHeaderListModelPriv *mpriv;

	if (G_UNLIKELY (!me || !me->model))
		return;

	/* Move the iterator to the previous node */

	mpriv = TNY_GTK_HEADER_LIST_MODEL_GET_PRIVATE (me->model);

	g_static_rec_mutex_lock (mpriv->iterator_lock);
	me->current--;
	g_static_rec_mutex_unlock (mpriv->iterator_lock);

	return;
}

gboolean 
_tny_gtk_header_list_iterator_is_done_nl (TnyGtkHeaderListIterator *me)
{
	TnyGtkHeaderListModelPriv *mpriv;

	if (G_UNLIKELY (!me  || !me->model))
		return TRUE;

	mpriv = TNY_GTK_HEADER_LIST_MODEL_GET_PRIVATE (me->model);

	return (/*me->current < 0 || */me->current >= mpriv->items->len);
}

static gboolean 
tny_gtk_header_list_iterator_is_done (TnyIterator *self)
{
	TnyGtkHeaderListIterator *me = (TnyGtkHeaderListIterator*) self;
	TnyGtkHeaderListModelPriv *mpriv;
	gboolean retval = FALSE;

	if (G_UNLIKELY (!me || !me->model))
		return FALSE;

	mpriv = TNY_GTK_HEADER_LIST_MODEL_GET_PRIVATE (me->model);
	g_static_rec_mutex_lock (mpriv->iterator_lock);
	retval = (/* me->current < 0 || */me->current >= mpriv->items->len);
	g_static_rec_mutex_unlock (mpriv->iterator_lock);

	return retval;
}


void
_tny_gtk_header_list_iterator_first_nl (TnyGtkHeaderListIterator *me)
{
	me->current = 0;

	return;
}

static void
tny_gtk_header_list_iterator_first (TnyIterator *self)
{
	TnyGtkHeaderListIterator *me = (TnyGtkHeaderListIterator*) self;
	TnyGtkHeaderListModelPriv *mpriv;

	if (G_UNLIKELY (!me || !me->current || !me->model))
		return;

	mpriv = TNY_GTK_HEADER_LIST_MODEL_GET_PRIVATE (me->model);

	g_static_rec_mutex_lock (mpriv->iterator_lock);
	me->current = 0;
	g_static_rec_mutex_unlock (mpriv->iterator_lock);

	return;
}


void
_tny_gtk_header_list_iterator_nth_nl (TnyGtkHeaderListIterator *me, guint nth)
{
	me->current = nth;
	return;
}

static void 
tny_gtk_header_list_iterator_nth (TnyIterator *self, guint nth)
{
	TnyGtkHeaderListIterator *me = (TnyGtkHeaderListIterator*) self;
	TnyGtkHeaderListModelPriv *mpriv;

	if (G_UNLIKELY (!me || !me->current || !me->model))
		return;

	mpriv = TNY_GTK_HEADER_LIST_MODEL_GET_PRIVATE (me->model);

	g_static_rec_mutex_lock (mpriv->iterator_lock);
	me->current = nth;
	g_static_rec_mutex_unlock (mpriv->iterator_lock);

	return;
}

/* exception: don't ref */
gpointer 
_tny_gtk_header_list_iterator_get_current_nl (TnyGtkHeaderListIterator *me)
{
	TnyGtkHeaderListModelPriv *mpriv = TNY_GTK_HEADER_LIST_MODEL_GET_PRIVATE (me->model);
	return mpriv->items->pdata[me->current];
}

static GObject* 
tny_gtk_header_list_iterator_get_current (TnyIterator *self)
{
	TnyGtkHeaderListIterator *me = (TnyGtkHeaderListIterator*) self;
	TnyGtkHeaderListModelPriv *mpriv;
	gpointer retval;

	if (G_UNLIKELY (!me || !me->model))
		return NULL;

	/* Give the data of the current node */

	mpriv = TNY_GTK_HEADER_LIST_MODEL_GET_PRIVATE (me->model);

	g_static_rec_mutex_lock (mpriv->iterator_lock);
	retval = mpriv->items->pdata[me->current];
	if (retval)
		g_object_ref (retval);
	g_static_rec_mutex_unlock (mpriv->iterator_lock);


	return (GObject*) retval;
}


static TnyList* 
tny_gtk_header_list_iterator_get_list (TnyIterator *self)
{
	TnyGtkHeaderListIterator *me = (TnyGtkHeaderListIterator*) self;

	/* Return the list */

	if (G_UNLIKELY (!me || !me->model))
		return NULL;

	g_object_ref (me->model);

	return TNY_LIST (me->model);
}

static void
tny_iterator_init (TnyIteratorIface *klass)
{
	klass->next= tny_gtk_header_list_iterator_next;
	klass->prev= tny_gtk_header_list_iterator_prev;
	klass->first= tny_gtk_header_list_iterator_first;
	klass->nth= tny_gtk_header_list_iterator_nth;
	klass->get_current= tny_gtk_header_list_iterator_get_current;
	klass->get_list= tny_gtk_header_list_iterator_get_list;
	klass->is_done = tny_gtk_header_list_iterator_is_done;

	return;
}

static void 
tny_gtk_header_list_iterator_class_init (TnyGtkHeaderListIteratorClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	object_class->finalize = tny_gtk_header_list_iterator_finalize;

	return;
}

static gpointer 
_tny_gtk_header_list_iterator_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkHeaderListIteratorClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_header_list_iterator_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkHeaderListIterator),
		  0,      /* n_preallocs */
		  tny_gtk_header_list_iterator_instance_init,    /* instance_init */
		  NULL
		};

	static const GInterfaceInfo tny_iterator_info = 
		{
		  (GInterfaceInitFunc) tny_iterator_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyGtkHeaderListIterator",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_ITERATOR, 
				     &tny_iterator_info);

	return GUINT_TO_POINTER (type);
}

GType 
_tny_gtk_header_list_iterator_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, _tny_gtk_header_list_iterator_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
