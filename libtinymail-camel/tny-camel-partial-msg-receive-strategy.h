#ifndef TNY_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY_H
#define TNY_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY_H

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

#include <glib-object.h>

#include <tny-msg-receive-strategy.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY             (tny_camel_partial_msg_receive_strategy_get_type ())
#define TNY_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY, TnyCamelPartialMsgReceiveStrategy))
#define TNY_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY, TnyCamelPartialMsgReceiveStrategyClass))
#define TNY_IS_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY))
#define TNY_IS_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY))
#define TNY_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_PARTIAL_MSG_RECEIVE_STRATEGY, TnyCamelPartialMsgReceiveStrategyClass))

typedef struct _TnyCamelPartialMsgReceiveStrategy TnyCamelPartialMsgReceiveStrategy;
typedef struct _TnyCamelPartialMsgReceiveStrategyClass TnyCamelPartialMsgReceiveStrategyClass;

struct _TnyCamelPartialMsgReceiveStrategy
{
	GObject parent;

};

struct _TnyCamelPartialMsgReceiveStrategyClass
{
	GObjectClass parent_class;

	/* virtual methods */
	TnyMsg * (*perform_get_msg) (TnyMsgReceiveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err);
};

GType tny_camel_partial_msg_receive_strategy_get_type (void);
TnyMsgReceiveStrategy* tny_camel_partial_msg_receive_strategy_new (void);

G_END_DECLS

#endif
