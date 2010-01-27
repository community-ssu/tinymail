#ifndef TNY_CAMEL_SEND_QUEUE_H
#define TNY_CAMEL_SEND_QUEUE_H

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

#include <tny-send-queue.h>
#include <tny-msg.h>
#include <tny-camel-transport-account.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_SEND_QUEUE             (tny_camel_send_queue_get_type ())
#define TNY_CAMEL_SEND_QUEUE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_SEND_QUEUE, TnyCamelSendQueue))
#define TNY_CAMEL_SEND_QUEUE_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_SEND_QUEUE, TnyCamelSendQueueClass))
#define TNY_IS_CAMEL_SEND_QUEUE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_SEND_QUEUE))
#define TNY_IS_CAMEL_SEND_QUEUE_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_SEND_QUEUE))
#define TNY_CAMEL_SEND_QUEUE_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_SEND_QUEUE, TnyCamelSendQueueClass))

typedef struct _TnyCamelSendQueue TnyCamelSendQueue;
typedef struct _TnyCamelSendQueueClass TnyCamelSendQueueClass;

struct _TnyCamelSendQueue
{
	GObject parent;
};

struct _TnyCamelSendQueueClass 
{
	GObjectClass parent;

	/* virtual methods */
	void (*add) (TnySendQueue *self, TnyMsg *msg, GError **err);
	TnyFolder* (*get_sentbox) (TnySendQueue *self);
	TnyFolder* (*get_outbox) (TnySendQueue *self);
	void (*cancel) (TnySendQueue *self, TnySendQueueCancelAction cancel_action, GError **err);
	void (*add_async) (TnySendQueue *self, TnyMsg *msg, TnySendQueueAddCallback callback, TnyStatusCallback status_callback, gpointer user_data);
};

GType tny_camel_send_queue_get_type (void);

TnySendQueue* tny_camel_send_queue_new (TnyCamelTransportAccount *trans_account);

TnySendQueue* tny_camel_send_queue_new_with_folders (TnyCamelTransportAccount *trans_account, TnyFolder *outbox, TnyFolder *sentbox);

void tny_camel_send_queue_flush (TnyCamelSendQueue *self);

TnyCamelTransportAccount* tny_camel_send_queue_get_transport_account (TnyCamelSendQueue *self);

void tny_camel_send_queue_set_transport_account (TnyCamelSendQueue *self,
						 TnyCamelTransportAccount *trans_account);


G_END_DECLS

#endif

