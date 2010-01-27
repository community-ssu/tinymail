#ifndef TNY_CONNECTION_POLICY_H
#define TNY_CONNECTION_POLICY_H

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

#define TNY_TYPE_CONNECTION_POLICY             (tny_connection_policy_get_type ())
#define TNY_CONNECTION_POLICY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CONNECTION_POLICY, TnyConnectionPolicy))
#define TNY_IS_CONNECTION_POLICY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CONNECTION_POLICY))
#define TNY_CONNECTION_POLICY_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_CONNECTION_POLICY, TnyConnectionPolicyIface))

#ifndef TNY_SHARED_H
typedef struct _TnyConnectionPolicy TnyConnectionPolicy;
typedef struct _TnyConnectionPolicyIface TnyConnectionPolicyIface;
#endif

struct _TnyConnectionPolicyIface
{
	GTypeInterface parent;

	void (*on_connect) (TnyConnectionPolicy *self, TnyAccount *account);
	void (*on_connection_broken) (TnyConnectionPolicy *self, TnyAccount *account);
	void (*on_disconnect) (TnyConnectionPolicy *self, TnyAccount *account);
	void (*set_current) (TnyConnectionPolicy *self, TnyAccount *account, TnyFolder *folder);

};

GType tny_connection_policy_get_type (void);

void tny_connection_policy_on_connect (TnyConnectionPolicy *self, TnyAccount *account);
void tny_connection_policy_on_disconnect (TnyConnectionPolicy *self, TnyAccount *account);
void tny_connection_policy_on_connection_broken (TnyConnectionPolicy *self, TnyAccount *account);
void tny_connection_policy_set_current (TnyConnectionPolicy *self, TnyAccount *account, TnyFolder *folder);

G_END_DECLS

#endif
