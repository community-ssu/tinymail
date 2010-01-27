#ifndef TNY_CAMEL_RECOVER_CONNECTION_POLICY_H
#define TNY_CAMEL_RECOVER_CONNECTION_POLICY_H

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

#include <glib-object.h>

#include <tny-connection-policy.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_RECOVER_CONNECTION_POLICY             (tny_camel_recover_connection_policy_get_type ())
#define TNY_CAMEL_RECOVER_CONNECTION_POLICY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_RECOVER_CONNECTION_POLICY, TnyCamelRecoverConnectionPolicy))
#define TNY_CAMEL_RECOVER_CONNECTION_POLICY_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_RECOVER_CONNECTION_POLICY, TnyCamelRecoverConnectionPolicyClass))
#define TNY_IS_CAMEL_RECOVER_CONNECTION_POLICY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_RECOVER_CONNECTION_POLICY))
#define TNY_IS_CAMEL_RECOVER_CONNECTION_POLICY_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_RECOVER_CONNECTION_POLICY))
#define TNY_CAMEL_RECOVER_CONNECTION_POLICY_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_RECOVER_CONNECTION_POLICY, TnyCamelRecoverConnectionPolicyClass))

typedef struct _TnyCamelRecoverConnectionPolicy TnyCamelRecoverConnectionPolicy;
typedef struct _TnyCamelRecoverConnectionPolicyClass TnyCamelRecoverConnectionPolicyClass;

struct _TnyCamelRecoverConnectionPolicy
{
	GObject parent;

};

struct _TnyCamelRecoverConnectionPolicyClass
{
	GObjectClass parent_class;
};

GType tny_camel_recover_connection_policy_get_type (void);
TnyConnectionPolicy* tny_camel_recover_connection_policy_new (void);

void tny_camel_recover_connection_policy_set_reconnect_delay (TnyCamelRecoverConnectionPolicy *self, gint milliseconds);
void tny_camel_recover_connection_policy_set_recover_active_folder (TnyCamelRecoverConnectionPolicy *self, gboolean setting);

G_END_DECLS

#endif
