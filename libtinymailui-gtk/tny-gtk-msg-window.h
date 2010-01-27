#ifndef TNY_GTK_MSG_WINDOW_H
#define TNY_GTK_MSG_WINDOW_H

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
#include <tny-msg-window.h>
#include <tny-header.h>
#include <tny-msg.h>
#include <tny-stream.h>
#include <tny-mime-part.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_MSG_WINDOW             (tny_gtk_msg_window_get_type ())
#define TNY_GTK_MSG_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_MSG_WINDOW, TnyGtkMsgWindow))
#define TNY_GTK_MSG_WINDOW_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_MSG_WINDOW, TnyGtkMsgWindowClass))
#define TNY_IS_GTK_MSG_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_MSG_WINDOW))
#define TNY_IS_GTK_MSG_WINDOW_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_MSG_WINDOW))
#define TNY_GTK_MSG_WINDOW_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_MSG_WINDOW, TnyGtkMsgWindowClass))

typedef struct _TnyGtkMsgWindow TnyGtkMsgWindow;
typedef struct _TnyGtkMsgWindowClass TnyGtkMsgWindowClass;

struct _TnyGtkMsgWindow
{
	GtkWindow parent;

};

struct _TnyGtkMsgWindowClass
{
	GtkWindowClass parent_class;

	/* virtual methods */
	TnyMsg* (*get_msg) (TnyMsgView *self);
	void (*set_msg) (TnyMsgView *self, TnyMsg *msg);
	void (*set_unavailable) (TnyMsgView *self);
	void (*clear) (TnyMsgView *self);
	TnyMimePartView* (*create_mime_part_view_for) (TnyMsgView *self, TnyMimePart *part);
	TnyMsgView* (*create_new_inline_viewer) (TnyMsgView *self);
	TnyMimePart* (*get_part) (TnyMimePartView *self);
	void (*set_part) (TnyMimePartView *self, TnyMimePart *part);
};

GType tny_gtk_msg_window_get_type (void);
TnyMsgWindow* tny_gtk_msg_window_new (TnyMsgView *msgview);
void tny_gtk_msg_window_set_view (TnyGtkMsgWindow *self, TnyMsgView *view);

G_END_DECLS

#endif
