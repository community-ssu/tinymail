#ifndef TNY_CAMEL_BS_MSG_H
#define TNY_CAMEL_BS_MSG_H

/* libtinymail-camel_bs - The Tiny Mail base library for CamelBs
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

#include <tny-msg.h>
#include <tny-mime-part.h>
#include <tny-stream.h>
#include <tny-header.h>
#include <tny-camel-bs-mime-part.h>

G_BEGIN_DECLS

#define TNY_TYPE_CAMEL_BS_MSG             (tny_camel_bs_msg_get_type ())
#define TNY_CAMEL_BS_MSG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_CAMEL_BS_MSG, TnyCamelBsMsg))
#define TNY_CAMEL_BS_MSG_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_CAMEL_BS_MSG, TnyCamelBsMsgClass))
#define TNY_IS_CAMEL_BS_MSG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_CAMEL_BS_MSG))
#define TNY_IS_CAMEL_BS_MSG_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_CAMEL_BS_MSG))
#define TNY_CAMEL_BS_MSG_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_CAMEL_BS_MSG, TnyCamelBsMsgClass))

typedef struct _TnyCamelBsMsg TnyCamelBsMsg;
typedef struct _TnyCamelBsMsgClass TnyCamelBsMsgClass;

struct _TnyCamelBsMsg
{
	TnyCamelBsMimePart parent;
};

struct _TnyCamelBsMsgClass 
{
	TnyCamelBsMimePartClass parent;

	/* virtual methods */
	TnyHeader* (*get_header) (TnyMsg *self);
	TnyFolder* (*get_folder) (TnyMsg *self);
	gchar* (*get_url_string) (TnyMsg *self);
	void (*uncache_attachments) (TnyMsg *self);
	void (*rewrite_cache) (TnyMsg *self);
	gboolean (*get_allow_external_images) (TnyMsg *self);
	void (*set_allow_external_images) (TnyMsg *self, gboolean allow);
};

GType tny_camel_bs_msg_get_type (void);



G_END_DECLS

#endif

