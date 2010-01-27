/* libtinymail-camel - The Tiny Mail base library for Camel
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

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <string.h>
#include <time.h>
#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-camel-queue-priv.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <tny-camel-shared.h>
#include <tny-camel-account-priv.h>
#include <tny-status.h>

static GObjectClass *parent_class = NULL;

#define TNY_CAMEL_QUEUE_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_CAMEL_QUEUE, TnyCamelQueuePriv))

static void account_finalized (TnyCamelQueue *queue, GObject *finalized_account);

static void
tny_camel_queue_finalize (GObject *object)
{
	TnyCamelQueue *self = (TnyCamelQueue*) object;

	self->stopped = TRUE;

	g_mutex_lock (self->mutex);
	if (self->account)
		g_object_weak_unref (G_OBJECT (self->account), (GWeakNotify) account_finalized, self);
	g_mutex_unlock (self->mutex);

	g_static_rec_mutex_lock (self->lock);
	g_list_free (self->list);
	self->list = NULL;
	g_static_rec_mutex_unlock (self->lock);

	g_cond_free (self->condition);
	g_mutex_free (self->mutex);

	/* g_static_rec_mutex_free (self->lock); */
	g_free (self->lock);
	self->lock = NULL;

	(*parent_class->finalize) (object);

	return;
}

static void
account_finalized (TnyCamelQueue *queue, GObject *finalized_account)
{
	g_mutex_lock (queue->mutex);
	queue->account = NULL;
	queue->stopped = TRUE;
	if (queue->is_waiting) {
		g_cond_broadcast (queue->condition);
	}
	g_mutex_unlock (queue->mutex);
}

/**
 * _tny_camel_queue_new
 * @account: the account
 *
 * Internal, non-public API documentation of Tinymail
 *
 * Make a new queue for @account.
 **/
TnyCamelQueue*
_tny_camel_queue_new (TnyCamelAccount *account)
{
	TnyCamelQueue *self = g_object_new (TNY_TYPE_CAMEL_QUEUE, NULL);

	self->account = account;
	g_object_weak_ref (G_OBJECT (account), (GWeakNotify) account_finalized, self);

	return TNY_CAMEL_QUEUE (self);
}

typedef struct
{
	GThreadFunc func;
	GSourceFunc callback;
	GDestroyNotify destroyer;
	GSourceFunc cancel_callback;
	GDestroyNotify cancel_destroyer;
	gpointer data;
	gsize data_size;
	TnyCamelQueueItemFlags flags;
	const gchar *name;
	gboolean *cancel_field;
	gboolean deleted;
} QueueItem;

static gboolean
perform_callback (gpointer user_data)
{
	QueueItem *item = (QueueItem *) user_data;
	gboolean retval = FALSE;

	if (item->callback)
		retval = item->callback (item->data);

	return retval;
}

static void
perform_destroyer (gpointer user_data)
{
	QueueItem *item = (QueueItem *) user_data;
	TnyCamelQueueable *info = (TnyCamelQueueable * ) item->data;

	if (item->destroyer)
		item->destroyer (item->data);

	if (info->condition) {
		g_mutex_lock (info->mutex);
		g_cond_broadcast (info->condition);
		info->had_callback = TRUE;
		g_mutex_unlock (info->mutex);
	}

	return;
}


static gboolean
perform_cancel_callback (gpointer user_data)
{
	QueueItem *item = (QueueItem *) user_data;
	gboolean retval = FALSE;

	if (item->cancel_callback)
		retval = item->cancel_callback (item->data);

	return retval;
}

static void
perform_cancel_destroyer (gpointer user_data)
{
	QueueItem *item = (QueueItem *) user_data;
	TnyCamelQueueable *info = (TnyCamelQueueable * ) item->data;

	if (item->cancel_destroyer)
		item->cancel_destroyer (item->data);

	if (info->condition) {
		g_mutex_lock (info->mutex);
		g_cond_broadcast (info->condition);
		info->had_callback = TRUE;
		g_mutex_unlock (info->mutex);
	}

	return;
}

static gpointer 
tny_camel_queue_thread_main_func (gpointer user_data)
{
	TnyCamelQueue *queue = user_data;
	TnyCamelAccountPriv *apriv;

	while (!queue->stopped)
	{
		GList *first = NULL;
		QueueItem *item = NULL;
		gboolean deleted = FALSE, wait = FALSE;

		g_static_rec_mutex_lock (queue->lock);

		if (queue->next_uncancel)
		{
			g_mutex_lock (queue->mutex);
			if (queue->account) {
				g_object_ref (queue->account);
				g_mutex_unlock (queue->mutex);
				_tny_camel_account_actual_uncancel (TNY_CAMEL_ACCOUNT (queue->account));
				g_object_unref (queue->account);
			} else {
				g_mutex_unlock (queue->mutex);
			}
			queue->next_uncancel = FALSE;
		}

		first = g_list_first (queue->list);
		if (first) {
			item = first->data;
			if (item)
				deleted = item->deleted;
			queue->current = item;
		} else
			wait = TRUE;
		/* If no next item is scheduled then we can go idle after finishing operation */
		apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (queue->account);
		if (apriv->service)
			camel_service_can_idle (apriv->service, !first || !first->next);
		g_static_rec_mutex_unlock (queue->lock);

		if (item) {
			TnyCamelQueueable *info = (TnyCamelQueueable * ) item->data;

			if (!deleted) {
				tny_debug ("TnyCamelQueue: %s is on the stage, now performing\n", item->name);
				item->func (item->data);
			}

			tny_debug ("TnyCamelQueue: user callback starts\n");

			info->mutex = g_mutex_new ();
			info->condition = g_cond_new ();
			info->had_callback = FALSE;

			if (deleted) {

				g_idle_add_full (G_PRIORITY_LOW, 
					perform_cancel_callback, item, 
					perform_cancel_destroyer);
			} else {

				g_idle_add_full (G_PRIORITY_LOW, 
					perform_callback, item, 
					perform_destroyer);
			}
			/* Wait on the queue for the mainloop callback to be finished */
			g_mutex_lock (info->mutex);
			if (!info->had_callback)
				g_cond_wait (info->condition, info->mutex);
			g_mutex_unlock (info->mutex);

			tny_debug ("TnyCamelQueue: user callback finished\n");

			g_mutex_free (info->mutex);
			g_cond_free (info->condition);

			g_slice_free1 (item->data_size, item->data);

			if (!deleted)
				tny_debug ("TnyCamelQueue: %s is off the stage, done performing\n", item->name);
		}

		g_static_rec_mutex_lock (queue->lock);
		if (first)
			queue->list = g_list_delete_link (queue->list, first);
		queue->current = NULL;

		if (g_list_length (queue->list) == 0)
			wait = TRUE;
		g_static_rec_mutex_unlock (queue->lock);

		if (item)
			g_slice_free (QueueItem, item);

		if (wait) {
			g_mutex_lock (queue->mutex);
			queue->is_waiting = TRUE;
			g_cond_wait (queue->condition, queue->mutex);
			queue->is_waiting = FALSE;
			g_mutex_unlock (queue->mutex);
		}
	}

	queue->thread = NULL;
	queue->stopped = TRUE;

	g_object_unref (queue);

	return NULL;
}


/**
 * _tny_camel_queue_remove_items
 * @queue: the queue
 * @flags: flags
 *
 * Internal, non-public API documentation of Tinymail
 *
 * Remove all items from the queue where their flags match @flags. As items get
 * removed will their callback and destroyer happen and will their cancel_field
 * be set to TRUE.
 **/
void 
_tny_camel_queue_remove_items (TnyCamelQueue *queue, TnyCamelQueueItemFlags flags)
{
	GList *copy = NULL;

	g_static_rec_mutex_lock (queue->lock);
	copy = queue->list;
	while (copy) 
	{
		QueueItem *item = copy->data;

		if (queue->current != item)
		{
			if (item && (item->flags & flags)) 
			{
				tny_debug ("TnyCamelQueue: %s 's performance is removed\n", item->name);

				if (item->cancel_field)
					*item->cancel_field = TRUE;

				item->deleted = TRUE;
			}
		}
		copy = g_list_next (copy);
	}
	g_static_rec_mutex_unlock (queue->lock);
}

/**
 * _tny_camel_queue_cancel_remove_items
 * @queue: the queue
 * @flags: flags
 *
 * Internal, non-public API documentation of Tinymail
 *
 * Remove all items from the queue where their flags match @flags. As items get
 * removed will their callback and destroyer happen and will their cancel_field
 * be set to TRUE.
 *
 * Also cancel the current item (make the up-next read() of the operation fail).
 * Note that the current item's @callback and @destroyer will not be called as
 * as soon as the item's work func has been launched, it's considered to be 
 * never cancelled. The current-item cancel just means that the operation will
 * be set to fail at its next read() or write operation. The developer of the
 * work func must deal with this himself.
 **/
void 
_tny_camel_queue_cancel_remove_items (TnyCamelQueue *queue, TnyCamelQueueItemFlags flags)
{
	QueueItem *item = NULL;

	g_static_rec_mutex_lock (queue->lock);
	item = queue->current;

	/* Remove all the cancellables */
	_tny_camel_queue_remove_items (queue, flags);

	/* Cancel the current */
	if (item) {
		if (item->flags & TNY_CAMEL_QUEUE_CANCELLABLE_ITEM) {
			if (!(item->flags & TNY_CAMEL_QUEUE_SYNC_ITEM)) {
				if (queue->account)
					_tny_camel_account_actual_cancel (TNY_CAMEL_ACCOUNT (queue->account));
				queue->next_uncancel = TRUE;
			}
		}
	}

	g_static_rec_mutex_unlock (queue->lock);

	return;
}

/**
 * _tny_camel_queue_launch_wflags
 * @queue: the queue
 * @func: the work function
 * @callback: the callback for when finished
 * @destroyer: the destroyer for the @callback
 * @cancel_callback: for in case of a cancellation, can be NULL
 * @cancel_destroyer: for in case of a cancellation, can be NULL
 * @cancel_field: a byref location of a gboolean that will be set to TRUE in case of a cancellation
 * @data: data that will be passed to @func, @callback and @destroyer
 * @data_size: size of the @data pointer
 * @flags: flags of this item
 * @name: a name for this item for debugging (__FUNCTION__ will do)
 *
 * Internal, non-public API documentation of Tinymail
 *
 * Queue a new item. The contract is that @queue will invoke @func in future if
 * it doesn't get cancelled. If it does get cancelled and @callback is not NULL,
 * @callback will be called in the GMainLoop with @destroyer as GDestroyNotify.
 * A cancelled item's @cancel_field will also be set to TRUE.
 **/
void 
_tny_camel_queue_launch_wflags (TnyCamelQueue *queue, GThreadFunc func, GSourceFunc callback, GDestroyNotify destroyer, GSourceFunc cancel_callback, GDestroyNotify cancel_destroyer, gboolean *cancel_field, gpointer data, gsize data_size, TnyCamelQueueItemFlags flags, const gchar *name)
{
	QueueItem *item = g_slice_new (QueueItem);
	TnyCamelAccountPriv *apriv;

	if (!g_thread_supported ())
		g_thread_init (NULL);

	item->func = func;
	item->callback = callback;
	item->destroyer = destroyer;
	item->cancel_callback = cancel_callback;
	item->cancel_destroyer = cancel_destroyer;
	item->data = data;
	item->data_size = data_size;
	item->flags = flags;
	item->name = name;
	item->cancel_field = cancel_field;
	item->deleted = FALSE;

	g_static_rec_mutex_lock (queue->lock);

	if (queue->account == NULL)
		g_assert ("We should never be running tny_camel_queue_launch_wflags if account was unreferenced");

	if (flags & TNY_CAMEL_QUEUE_PRIORITY_ITEM) 
	{
		/* Preserve the order for prioritized items */
		gboolean stop = FALSE; gint cnt = 0;
		GList *first = g_list_first (queue->list);
		while (first && !stop) {
			QueueItem *item = first->data;
			if (item)
				if (!(item->flags & TNY_CAMEL_QUEUE_PRIORITY_ITEM)) {
					stop = TRUE;
					cnt--;
				}
			cnt++;
			first = g_list_next (first);
		}
		queue->list = g_list_insert (queue->list, item, cnt);
	} else /* Normal items simply get appended */
		queue->list = g_list_append (queue->list, item);

	/* If no next item is scheduled then we can go idle after finishing operation */
	apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (queue->account);
	if (apriv->service)
		camel_service_can_idle (apriv->service, !queue->list || !queue->list->next);

	if (queue->stopped) 
	{
		GError *err = NULL;
		queue->stopped = FALSE;
		g_object_ref (queue);
		queue->thread = g_thread_create (tny_camel_queue_thread_main_func, 
			queue, FALSE, &err);
		if (err) {
			queue->stopped = TRUE;
		}
	} else {
		g_mutex_lock (queue->mutex);
		if (queue->is_waiting)
			g_cond_broadcast (queue->condition);
		g_mutex_unlock (queue->mutex);
	}

	g_static_rec_mutex_unlock (queue->lock);

}

/**
 * _tny_camel_queue_launch
 * @queue: the queue
 * @func: the work function
 * @callback: the callback for when finished
 * @destroyer: the destroyer for the @callback
 * @cancel_callback: for in case of a cancellation, can be NULL
 * @cancel_destroyer: for in case of a cancellation, can be NULL
 * @cancel_field: a byref location of a gboolean that will be set to TRUE in case of a cancellation
 * @data: data that will be passed to @func, @callback and @destroyer
 * @data_size: size of the @data pointer
 * @name: a name for this item for debugging (__FUNCTION__ will do)
 *
 * Internal, non-public API documentation of Tinymail
 *
 * Queue a new item. The contract is that @queue will invoke @func in future if
 * it doesn't get cancelled. If it does get cancelled and @callback is not NULL,
 * @callback will be called in the GMainLoop with @destroyer as GDestroyNotify.
 * A cancelled item's @cancel_field will also be set to TRUE.
 *
 * The flags of the queue item will be TNY_CAMEL_QUEUE_NORMAL_ITEM.
 **/
void 
_tny_camel_queue_launch (TnyCamelQueue *queue, GThreadFunc func, GSourceFunc callback, GDestroyNotify destroyer, GSourceFunc cancel_callback, GDestroyNotify cancel_destroyer, gboolean *cancel_field, gpointer data, gsize data_size, const gchar *name)
{
	_tny_camel_queue_launch_wflags (queue, func, callback, destroyer, cancel_callback, cancel_destroyer,
		cancel_field, data, data_size, TNY_CAMEL_QUEUE_NORMAL_ITEM, name);
	return;
}

gboolean 
_tny_camel_queue_has_items (TnyCamelQueue *queue, TnyCamelQueueItemFlags flags)
{
	GList *copy = NULL;
	gboolean retval = FALSE;

	g_static_rec_mutex_lock (queue->lock);
	copy = queue->list;
	while (copy)
	{
		QueueItem *item = copy->data;

		if (item && (item->flags & flags)) 
		{
			tny_debug ("TnyCamelQueue: %s found\n", item->name);
			retval = TRUE;
			break;
		}
		
		copy = g_list_next (copy);
	}
	g_static_rec_mutex_unlock (queue->lock);

	return retval;
}

static void 
tny_camel_queue_class_init (TnyCamelQueueClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_camel_queue_finalize;

	return;
}


static void
tny_camel_queue_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelQueue *self = (TnyCamelQueue*)instance;

	self->is_waiting = FALSE;
	self->mutex = g_mutex_new ();
	self->condition = g_cond_new ();
	self->account = NULL;
	self->stopped = TRUE;
	self->list = NULL;

	/* We don't use a GThreadPool because we need control over the queued
	 * items: we must remove them sometimes for example. */

	self->thread = NULL;
	self->lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (self->lock);
	self->current = NULL;
	self->next_uncancel = FALSE;

	return;
}

static gpointer
tny_camel_queue_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelQueueClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_queue_class_init, /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelQueue),
			0,      /* n_preallocs */
			tny_camel_queue_instance_init,    /* instance_init */
			NULL
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelQueue",
				       &info, 0);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_camel_queue_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_queue_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_camel_queue_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
