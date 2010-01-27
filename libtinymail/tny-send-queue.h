#ifndef TNY_SEND_QUEUE_H
#define TNY_SEND_QUEUE_H

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

#include <glib.h>
#include <glib-object.h>

#include <tny-shared.h>
#include <tny-msg.h>

G_BEGIN_DECLS

#define TNY_TYPE_SEND_QUEUE             (tny_send_queue_get_type ())
#define TNY_SEND_QUEUE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_SEND_QUEUE, TnySendQueue))
#define TNY_IS_SEND_QUEUE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_SEND_QUEUE))
#define TNY_SEND_QUEUE_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_SEND_QUEUE, TnySendQueueIface))

#ifndef TNY_SHARED_H
typedef struct _TnySendQueue TnySendQueue;
typedef struct _TnySendQueueIface TnySendQueueIface;
#endif

#define TNY_TYPE_SEND_QUEUE_SIGNAL (tny_send_queue_signal_get_type ())

enum _TnySendQueueSignal
{
	TNY_SEND_QUEUE_MSG_SENDING,
	TNY_SEND_QUEUE_MSG_SENT,
	TNY_SEND_QUEUE_ERROR_HAPPENED,
	TNY_SEND_QUEUE_START,
	TNY_SEND_QUEUE_STOP,
	TNY_SEND_QUEUE_LAST_SIGNAL
};

typedef enum
{
	TNY_SEND_QUEUE_CANCEL_ACTION_SUSPEND,
	TNY_SEND_QUEUE_CANCEL_ACTION_REMOVE
} TnySendQueueCancelAction;

extern guint tny_send_queue_signals[TNY_SEND_QUEUE_LAST_SIGNAL];


struct _TnySendQueueIface
{
	GTypeInterface parent;
	
	/* Signals */
	void (*msg_sending) (TnySendQueue *self, TnyHeader *header, TnyMsg *msg, guint nth, guint total);
	void (*msg_sent) (TnySendQueue *self, TnyHeader *header, TnyMsg *msg, guint nth, guint total);
	void (*error_happened) (TnySendQueue *self, TnyHeader *header, TnyMsg *msg, GError *err);
	void (*queue_start) (TnySendQueue *self);
	void (*queue_stop) (TnySendQueue *self);

	/* methods */
	void (*add) (TnySendQueue *self, TnyMsg *msg, GError **err);
	void (*add_async) (TnySendQueue *self, TnyMsg *msg, TnySendQueueAddCallback callback, TnyStatusCallback status_callback, gpointer user_data);
	TnyFolder* (*get_sentbox) (TnySendQueue *self);
	TnyFolder* (*get_outbox) (TnySendQueue *self);
	void (*cancel) (TnySendQueue *self, TnySendQueueCancelAction cancel_action, GError **err);

};

GType tny_send_queue_get_type (void);

#ifndef TNY_DISABLE_DEPRECATED
void tny_send_queue_add (TnySendQueue *self, TnyMsg *msg, GError **err);
#endif

void tny_send_queue_add_async (TnySendQueue *self, TnyMsg *msg, TnySendQueueAddCallback callback, TnyStatusCallback status_callback, gpointer user_data);
TnyFolder* tny_send_queue_get_sentbox (TnySendQueue *self);
TnyFolder* tny_send_queue_get_outbox (TnySendQueue *self);
void tny_send_queue_cancel (TnySendQueue *self, TnySendQueueCancelAction cancel_action, GError **err);

G_END_DECLS

#endif
