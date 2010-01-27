#ifndef TNY_MSG_REMOVE_STRATEGY_H
#define TNY_MSG_REMOVE_STRATEGY_H

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

G_BEGIN_DECLS

#define TNY_TYPE_MSG_REMOVE_STRATEGY             (tny_msg_remove_strategy_get_type ())
#define TNY_MSG_REMOVE_STRATEGY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_MSG_REMOVE_STRATEGY, TnyMsgRemoveStrategy))
#define TNY_IS_MSG_REMOVE_STRATEGY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_MSG_REMOVE_STRATEGY))
#define TNY_MSG_REMOVE_STRATEGY_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_MSG_REMOVE_STRATEGY, TnyMsgRemoveStrategyIface))

#ifndef TNY_SHARED_H
typedef struct _TnyMsgRemoveStrategy TnyMsgRemoveStrategy;
typedef struct _TnyMsgRemoveStrategyIface TnyMsgRemoveStrategyIface;
#endif

struct _TnyMsgRemoveStrategyIface
{
	GTypeInterface parent;

	void (*perform_remove) (TnyMsgRemoveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err);
};

GType tny_msg_remove_strategy_get_type (void);
void tny_msg_remove_strategy_perform_remove (TnyMsgRemoveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err);

G_END_DECLS

#endif
