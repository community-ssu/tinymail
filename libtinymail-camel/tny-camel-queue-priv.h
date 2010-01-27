#ifndef TNY_CAMEL_QUEUE_H
#define TNY_CAMEL_QUEUE_H

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

#include <glib.h>
#include <glib-object.h>

#include <tny-camel-store-account.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_QUEUE             (tny_camel_queue_get_type ())
#define TNY_CAMEL_QUEUE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_QUEUE, TnyCamelQueue))
#define TNY_CAMEL_QUEUE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_QUEUE, TnyCamelQueueClass))
#define TNY_IS_CAMEL_QUEUE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_QUEUE))
#define TNY_IS_CAMEL_QUEUE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_QUEUE))
#define TNY_CAMEL_QUEUE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_QUEUE, TnyCamelQueueClass))

typedef struct _TnyCamelQueue TnyCamelQueue;
typedef struct _TnyCamelQueueable TnyCamelQueueable;
typedef struct _TnyCamelQueueClass TnyCamelQueueClass;

struct _TnyCamelQueue
{
	GObject parent;

	TnyCamelAccount *account;
	GList *list;
	GThread *thread;
	GCond *condition;
	GMutex *mutex;
	gboolean is_waiting;
	GStaticRecMutex *lock;
	gboolean stopped, next_uncancel;
	gpointer current;
};

struct _TnyCamelQueueClass 
{
	GObjectClass parent;
};

struct _TnyCamelQueueable
{
	GCond* condition;
	gboolean had_callback;
	GMutex *mutex;
};

typedef enum {
	TNY_CAMEL_QUEUE_NORMAL_ITEM = 1<<0,
	TNY_CAMEL_QUEUE_RECONNECT_ITEM = 1<<1,
	TNY_CAMEL_QUEUE_CANCELLABLE_ITEM = 1<<2,
	TNY_CAMEL_QUEUE_PRIORITY_ITEM = 1<<3,
	TNY_CAMEL_QUEUE_GET_HEADERS_ITEM = 1<<4,
	TNY_CAMEL_QUEUE_SYNC_ITEM = 1<<5, 
	TNY_CAMEL_QUEUE_REFRESH_ITEM = 1<<6,
	TNY_CAMEL_QUEUE_AUTO_CANCELLABLE_ITEM = 1<<7,
	TNY_CAMEL_QUEUE_CONNECT_ITEM = 1<<8,
} TnyCamelQueueItemFlags;

GType tny_camel_queue_get_type (void);

TnyCamelQueue* _tny_camel_queue_new (TnyCamelAccount *account);
void _tny_camel_queue_launch_wflags (TnyCamelQueue *queue, GThreadFunc func, GSourceFunc callback, GDestroyNotify destroyer, GSourceFunc cancel_callback, GDestroyNotify cancel_destroyer, gboolean *cancel_field, gpointer data, gsize data_size, TnyCamelQueueItemFlags flags, const gchar *name);
void _tny_camel_queue_launch (TnyCamelQueue *queue, GThreadFunc func, GSourceFunc callback, GDestroyNotify destroyer, GSourceFunc cancel_callback, GDestroyNotify cancel_destroyer, gboolean *cancel_field, gpointer data, gsize data_size, const gchar *name);
void _tny_camel_queue_remove_items (TnyCamelQueue *queue, TnyCamelQueueItemFlags flags);
void _tny_camel_queue_cancel_remove_items (TnyCamelQueue *queue, TnyCamelQueueItemFlags flags);
gboolean _tny_camel_queue_has_items (TnyCamelQueue *queue, TnyCamelQueueItemFlags flags);

G_END_DECLS

#endif

