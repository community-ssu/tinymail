#ifndef TNY_GTK_PASSWORD_DIALOG_H
#define TNY_GTK_PASSWORD_DIALOG_H

/* libtinymailui-gtk - The Tiny Mail base library for Gtk
 * Copyright (C) 2006-2007 Philip Van Hoof <pvanhoof@gtk.org>
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
#include <tny-password-getter.h>

G_BEGIN_DECLS

#define TNY_TYPE_GTK_PASSWORD_DIALOG             (tny_gtk_password_dialog_get_type ())
#define TNY_GTK_PASSWORD_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_GTK_PASSWORD_DIALOG, TnyGtkPasswordDialog))
#define TNY_GTK_PASSWORD_DIALOG_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_GTK_PASSWORD_DIALOG, TnyGtkPasswordDialogClass))
#define TNY_IS_GTK_PASSWORD_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_GTK_PASSWORD_DIALOG))
#define TNY_IS_GTK_PASSWORD_DIALOG_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_GTK_PASSWORD_DIALOG))
#define TNY_GTK_PASSWORD_DIALOG_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_GTK_PASSWORD_DIALOG, TnyGtkPasswordDialogClass))

typedef struct _TnyGtkPasswordDialog TnyGtkPasswordDialog;
typedef struct _TnyGtkPasswordDialogClass TnyGtkPasswordDialogClass;

struct _TnyGtkPasswordDialog
{
	GObject parent;
};

struct _TnyGtkPasswordDialogClass
{
	GObjectClass parent_class;
};

GType tny_gtk_password_dialog_get_type (void);
TnyPasswordGetter* tny_gtk_password_dialog_new (void);

G_END_DECLS

#endif
