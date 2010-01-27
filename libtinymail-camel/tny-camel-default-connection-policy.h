#ifndef TNY_CAMEL_DEFAULT_CONNECTION_POLICY_H
#define TNY_CAMEL_DEFAULT_CONNECTION_POLICY_H

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

#include <tny-connection-policy.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_DEFAULT_CONNECTION_POLICY             (tny_camel_default_connection_policy_get_type ())
#define TNY_CAMEL_DEFAULT_CONNECTION_POLICY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_DEFAULT_CONNECTION_POLICY, TnyCamelDefaultConnectionPolicy))
#define TNY_CAMEL_DEFAULT_CONNECTION_POLICY_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_DEFAULT_CONNECTION_POLICY, TnyCamelDefaultConnectionPolicyClass))
#define TNY_IS_CAMEL_DEFAULT_CONNECTION_POLICY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_DEFAULT_CONNECTION_POLICY))
#define TNY_IS_CAMEL_DEFAULT_CONNECTION_POLICY_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_DEFAULT_CONNECTION_POLICY))
#define TNY_CAMEL_DEFAULT_CONNECTION_POLICY_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_DEFAULT_CONNECTION_POLICY, TnyCamelDefaultConnectionPolicyClass))

typedef struct _TnyCamelDefaultConnectionPolicy TnyCamelDefaultConnectionPolicy;
typedef struct _TnyCamelDefaultConnectionPolicyClass TnyCamelDefaultConnectionPolicyClass;

struct _TnyCamelDefaultConnectionPolicy
{
	GObject parent;

};

struct _TnyCamelDefaultConnectionPolicyClass
{
	GObjectClass parent_class;
};

GType tny_camel_default_connection_policy_get_type (void);
TnyConnectionPolicy* tny_camel_default_connection_policy_new (void);

G_END_DECLS

#endif
