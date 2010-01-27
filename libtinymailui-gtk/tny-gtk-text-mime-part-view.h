#ifndef TNY_GTK_TEXT_MIME_PART_VIEW_H
#define TNY_GTK_TEXT_MIME_PART_VIEW_H

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

#include <tny-mime-part-view.h>
#include <tny-stream.h>
#include <tny-mime-part.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_TEXT_MIME_PART_VIEW             (tny_gtk_text_mime_part_view_get_type ())
#define TNY_GTK_TEXT_MIME_PART_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_TEXT_MIME_PART_VIEW, TnyGtkTextMimePartView))
#define TNY_GTK_TEXT_MIME_PART_VIEW_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_TEXT_MIME_PART_VIEW, TnyGtkTextMimePartViewClass))
#define TNY_IS_GTK_TEXT_MIME_PART_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_TEXT_MIME_PART_VIEW))
#define TNY_IS_GTK_TEXT_MIME_PART_VIEW_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_TEXT_MIME_PART_VIEW))
#define TNY_GTK_TEXT_MIME_PART_VIEW_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_TEXT_MIME_PART_VIEW, TnyGtkTextMimePartViewClass))

typedef struct _TnyGtkTextMimePartView TnyGtkTextMimePartView;
typedef struct _TnyGtkTextMimePartViewClass TnyGtkTextMimePartViewClass;

struct _TnyGtkTextMimePartView
{
	GtkTextView parent;

};

struct _TnyGtkTextMimePartViewClass
{
	GtkTextViewClass parent_class;

	/* virtual methods */
	TnyMimePart* (*get_part) (TnyMimePartView *self);
	void (*set_part) (TnyMimePartView *self, TnyMimePart *part);
	void (*clear) (TnyMimePartView *self); 

};

GType tny_gtk_text_mime_part_view_get_type (void);
TnyMimePartView* tny_gtk_text_mime_part_view_new (TnyStatusCallback status_callback, gpointer status_user_data);

G_END_DECLS

#endif
