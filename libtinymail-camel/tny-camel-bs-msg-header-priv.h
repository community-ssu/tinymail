#ifndef TNY_CAMEL_BS_MSG_HEADER_PRIV_H
#define TNY_CAMEL_BS_MSG_HEADER_PRIV_H

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
#include <tny-header.h>

#include "bs/bodystruct.h"

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_BS_MSG_HEADER             (tny_camel_bs_msg_header_get_type ())
#define TNY_CAMEL_BS_MSG_HEADER(obj)             ((TnyCamelBsMsgHeader*)obj)
#define TNY_CAMEL_BS_MSG_HEADER_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_BS_MSG_HEADER, TnyCamelBsMsgHeaderClass))
#define TNY_IS_CAMEL_BS_MSG_HEADER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_BS_MSG_HEADER))
#define TNY_IS_CAMEL_BS_MSG_HEADER_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_BS_MSG_HEADER))
#define TNY_CAMEL_BS_MSG_HEADER_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_BS_MSG_HEADER, TnyCamelBsMsgHeaderClass))

typedef struct _TnyCamelBsMsgHeader TnyCamelBsMsgHeader;
typedef struct _TnyCamelBsMsgHeaderClass TnyCamelBsMsgHeaderClass;


struct _TnyCamelBsMsgHeader
{
	GObject parent;
	envelope_t *envelope;
	gint msg_size;
	gchar *content_type;
	gchar *content_subtype;
	gchar *charset;
};

struct _TnyCamelBsMsgHeaderClass 
{
	GObjectClass parent_class;
};

GType tny_camel_bs_msg_header_get_type (void);

TnyHeader* _tny_camel_bs_msg_header_new (envelope_t *envelope, gint msg_size, const gchar *charset);

G_END_DECLS

#endif
