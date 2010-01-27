#ifndef TNY_CAMEL_TRANSPORT_ACCOUNT_H
#define TNY_CAMEL_TRANSPORT_ACCOUNT_H

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

#include <tny-camel-account.h>
#include <tny-transport-account.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_TRANSPORT_ACCOUNT             (tny_camel_transport_account_get_type ())
#define TNY_CAMEL_TRANSPORT_ACCOUNT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_TRANSPORT_ACCOUNT, TnyCamelTransportAccount))
#define TNY_CAMEL_TRANSPORT_ACCOUNT_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_TRANSPORT_ACCOUNT, TnyCamelTransportAccountClass))
#define TNY_IS_CAMEL_TRANSPORT_ACCOUNT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_TRANSPORT_ACCOUNT))
#define TNY_IS_CAMEL_TRANSPORT_ACCOUNT_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_TRANSPORT_ACCOUNT))
#define TNY_CAMEL_TRANSPORT_ACCOUNT_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_TRANSPORT_ACCOUNT, TnyCamelTransportAccountClass))

typedef struct _TnyCamelTransportAccount TnyCamelTransportAccount;
typedef struct _TnyCamelTransportAccountClass TnyCamelTransportAccountClass;

struct _TnyCamelTransportAccount
{
	TnyCamelAccount parent;
};

struct _TnyCamelTransportAccountClass 
{
	TnyCamelAccountClass parent;

	/* virtual methods */
	void (*send) (TnyTransportAccount *self, TnyMsg *msg, GError **err);
};

GType tny_camel_transport_account_get_type (void);
TnyTransportAccount* tny_camel_transport_account_new (void);

const gchar *tny_camel_transport_account_get_from (TnyCamelTransportAccount *self);
void tny_camel_transport_account_set_from (TnyCamelTransportAccount *self, const gchar *from);

G_END_DECLS

#endif

