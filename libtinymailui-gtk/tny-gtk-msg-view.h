#ifndef TNY_GTK_MSG_VIEW_H
#define TNY_GTK_MSG_VIEW_H

/* libtinymailui-gtk - The Tiny Mail UI library for Gtk+
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

#include <tny-msg-view.h>
#include <tny-header-view.h>
#include <tny-header.h>
#include <tny-msg.h>
#include <tny-stream.h>
#include <tny-mime-part.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_MSG_VIEW             (tny_gtk_msg_view_get_type ())
#define TNY_GTK_MSG_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_MSG_VIEW, TnyGtkMsgView))
#define TNY_GTK_MSG_VIEW_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_MSG_VIEW, TnyGtkMsgViewClass))
#define TNY_IS_GTK_MSG_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_MSG_VIEW))
#define TNY_IS_GTK_MSG_VIEW_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_MSG_VIEW))
#define TNY_GTK_MSG_VIEW_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_MSG_VIEW, TnyGtkMsgViewClass))

typedef struct _TnyGtkMsgView TnyGtkMsgView;
typedef struct _TnyGtkMsgViewClass TnyGtkMsgViewClass;

struct _TnyGtkMsgView
{
	GtkBin parent;
	GtkContainer *viewers;
};

struct _TnyGtkMsgViewClass
{
	GtkBinClass parent_class;

	/* virtual methods */
	TnyMimePart* (*get_part) (TnyMimePartView *self);
	void (*set_part) (TnyMimePartView *self, TnyMimePart *part);
	TnyMsg* (*get_msg) (TnyMsgView *self);
	void (*set_msg) (TnyMsgView *self, TnyMsg *msg);
	void (*set_unavailable) (TnyMsgView *self);
	void (*clear) (TnyMsgView *self);
	TnyMimePartView* (*create_mime_part_view_for) (TnyMsgView *self, TnyMimePart *part);
	TnyMsgView* (*create_new_inline_viewer) (TnyMsgView *self);

	TnyHeaderView* (*create_header_view) (TnyGtkMsgView *self);
};

GType tny_gtk_msg_view_get_type (void);
TnyMsgView* tny_gtk_msg_view_new (void);

void tny_gtk_msg_view_set_display_html (TnyGtkMsgView *self, gboolean setting);
void tny_gtk_msg_view_set_display_rfc822 (TnyGtkMsgView *self, gboolean setting);
void tny_gtk_msg_view_set_display_attachments (TnyGtkMsgView *self, gboolean setting);
void tny_gtk_msg_view_set_display_plain (TnyGtkMsgView *self, gboolean setting);

gboolean tny_gtk_msg_view_get_display_html (TnyGtkMsgView *self);
gboolean tny_gtk_msg_view_get_display_rfc822 (TnyGtkMsgView *self);
gboolean tny_gtk_msg_view_get_display_attachments (TnyGtkMsgView *self);
gboolean tny_gtk_msg_view_get_display_plain (TnyGtkMsgView *self);


void tny_gtk_msg_view_set_status_callback (TnyGtkMsgView *self, TnyStatusCallback status_callback, gpointer status_user_data);
void tny_gtk_msg_view_get_status_callback (TnyGtkMsgView *self, TnyStatusCallback *status_callback, gpointer *status_user_data);

void tny_gtk_msg_view_set_parented (TnyGtkMsgView *self, gboolean parented);

G_END_DECLS

#endif
