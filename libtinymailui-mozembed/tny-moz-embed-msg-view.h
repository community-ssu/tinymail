#ifndef TNY_MOZ_EMBED_MSG_VIEW_H
#define TNY_MOZ_EMBED_MSG_VIEW_H

/* libtinymailui-mozembed - The Tiny Mail UI library for Mozembed
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

#include <gtk/gtk.h>
#include <glib-object.h>
#include <tny-shared.h>

#include <tny-gtk-msg-view.h>

G_BEGIN_DECLS

#define TNY_TYPE_MOZ_EMBED_MSG_VIEW             (tny_moz_embed_msg_view_get_type ())
#define TNY_MOZ_EMBED_MSG_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_MOZ_EMBED_MSG_VIEW, TnyMozEmbedMsgView))
#define TNY_MOZ_EMBED_MSG_VIEW_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_MOZ_EMBED_MSG_VIEW, TnyMozEmbedMsgViewClass))
#define TNY_IS_MOZ_EMBED_MSG_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_MOZ_EMBED_MSG_VIEW))
#define TNY_IS_MOZ_EMBED_MSG_VIEW_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_MOZ_EMBED_MSG_VIEW))
#define TNY_MOZ_EMBED_MSG_VIEW_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_MOZ_EMBED_MSG_VIEW, TnyMozEmbedMsgViewClass))

typedef struct _TnyMozEmbedMsgView TnyMozEmbedMsgView;
typedef struct _TnyMozEmbedMsgViewClass TnyMozEmbedMsgViewClass;

struct _TnyMozEmbedMsgView
{
	TnyGtkMsgView parent;
};

struct _TnyMozEmbedMsgViewClass
{
	TnyGtkMsgViewClass parent_class;

	/* virtual methods */
	TnyMimePartView* (*create_mime_part_view_for) (TnyMsgView *self, TnyMimePart *part);
	TnyMsgView* (*create_new_inline_viewer) (TnyMsgView *self);
};

GType tny_moz_embed_msg_view_get_type (void);
TnyMsgView* tny_moz_embed_msg_view_new (void);

G_END_DECLS

#endif
