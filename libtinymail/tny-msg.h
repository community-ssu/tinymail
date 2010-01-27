#ifndef TNY_MSG_H
#define TNY_MSG_H

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
#include <tny-shared.h>
#include <tny-header.h>
#include <tny-stream.h>
#include <tny-mime-part.h>

G_BEGIN_DECLS

#define TNY_TYPE_MSG             (tny_msg_get_type ())
#define TNY_MSG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_MSG, TnyMsg))
#define TNY_IS_MSG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_MSG))
#define TNY_MSG_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_MSG, TnyMsgIface))

#ifndef TNY_SHARED_H
typedef struct _TnyMsg TnyMsg;
typedef struct _TnyMsgIface TnyMsgIface;
#endif

struct _TnyMsgIface
{
	GTypeInterface parent;

	TnyHeader* (*get_header) (TnyMsg *self);
	TnyFolder* (*get_folder) (TnyMsg *self);
	gchar* (*get_url_string) (TnyMsg *self);
	void (*uncache_attachments) (TnyMsg *self);
	void (*rewrite_cache) (TnyMsg *self);
	gboolean (*get_allow_external_images) (TnyMsg *self);
	void (*set_allow_external_images) (TnyMsg *self, gboolean allow);

};

GType tny_msg_get_type (void);

TnyHeader* tny_msg_get_header (TnyMsg *self);
TnyFolder* tny_msg_get_folder (TnyMsg *self);
gchar* tny_msg_get_url_string (TnyMsg *self);
void tny_msg_uncache_attachments (TnyMsg *self);
void tny_msg_rewrite_cache (TnyMsg *self);
gboolean tny_msg_get_allow_external_images (TnyMsg *self);
void tny_msg_set_allow_external_images (TnyMsg *self, gboolean allow);

G_END_DECLS

#endif
