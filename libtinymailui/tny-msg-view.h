#ifndef TNY_MSG_VIEW_H
#define TNY_MSG_VIEW_H

/* libtinymailui - The Tiny Mail UI library
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

#include <tny-mime-part-view.h>

G_BEGIN_DECLS

#define TNY_TYPE_MSG_VIEW             (tny_msg_view_get_type ())
#define TNY_MSG_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_MSG_VIEW, TnyMsgView))
#define TNY_IS_MSG_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_MSG_VIEW))
#define TNY_MSG_VIEW_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), TNY_TYPE_MSG_VIEW, TnyMsgViewIface))

typedef struct _TnyMsgView TnyMsgView;
typedef struct _TnyMsgViewIface TnyMsgViewIface;


struct _TnyMsgViewIface
{
	GTypeInterface parent;

	TnyMsg* (*get_msg) (TnyMsgView *self);
	void (*set_msg) (TnyMsgView *self, TnyMsg *msg);
	void (*set_unavailable) (TnyMsgView *self);
	void (*clear) (TnyMsgView *self);
	TnyMimePartView* (*create_mime_part_view_for) (TnyMsgView *self, TnyMimePart *part);
	TnyMsgView* (*create_new_inline_viewer) (TnyMsgView *self);
};

GType tny_msg_view_get_type (void);

TnyMsg* tny_msg_view_get_msg (TnyMsgView *self);
void  tny_msg_view_set_msg (TnyMsgView *self, TnyMsg *msg);
void  tny_msg_view_clear (TnyMsgView *self);
void  tny_msg_view_set_unavailable (TnyMsgView *self);
TnyMimePartView* tny_msg_view_create_mime_part_view_for (TnyMsgView *self, TnyMimePart *part);
TnyMsgView* tny_msg_view_create_new_inline_viewer (TnyMsgView *self);

G_END_DECLS

#endif
