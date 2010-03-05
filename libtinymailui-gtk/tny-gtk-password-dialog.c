/* tinymail - Tiny Mail
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

/**
 * TnyGtkPasswordDialog:
 *
 * A #TnyPasswordGetter that will ask the user to enter the password in Gtk+
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <string.h>
#include <sys/mman.h>

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include <tny-gtk-password-dialog.h>

#include <tny-account.h>

static GObjectClass *parent_class = NULL;
static GHashTable *passwords = NULL;

static const gchar*
tny_gtk_password_dialog_get_password (TnyPasswordGetter *self, const gchar *aid, const gchar *prompt, gboolean *cancel)
{
	const gchar *accountid = aid;
	const gchar *retval = NULL;

	if (G_UNLIKELY (!passwords))
		passwords = g_hash_table_new (g_str_hash, g_str_equal);
	retval = g_hash_table_lookup (passwords, accountid);

	if (G_UNLIKELY (!retval))
	{
		GtkDialog *dialog = NULL;
		GtkEntry *pwd_entry;
		GtkLabel *prompt_label;

		dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (_("Password input"), NULL,
			GTK_DIALOG_MODAL, 
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, 
			NULL));

		gtk_window_set_title (GTK_WINDOW (dialog), _("Password input"));

		/* TODO: Add key icon or something */

		pwd_entry = GTK_ENTRY (gtk_entry_new ());
		prompt_label = GTK_LABEL (gtk_label_new (""));

		gtk_entry_set_visibility (pwd_entry, FALSE);

		gtk_widget_show (GTK_WIDGET (pwd_entry));
		gtk_widget_show (GTK_WIDGET (prompt_label));

		gtk_box_pack_start (GTK_BOX (dialog->vbox), 
			GTK_WIDGET (prompt_label), TRUE, TRUE, 0);

		gtk_box_pack_start (GTK_BOX (dialog->vbox), 
			GTK_WIDGET (pwd_entry), TRUE, TRUE, 0);

		gtk_label_set_text (prompt_label, prompt);

		if (G_LIKELY (gtk_dialog_run (dialog) == GTK_RESPONSE_OK))
		{
			const gchar *pwd = gtk_entry_get_text (pwd_entry);
			retval = g_strdup (pwd);
			mlock (pwd, strlen (pwd));
			mlock (retval, strlen (retval));
			*cancel = FALSE;
		} else {
			*cancel = TRUE;
		}

		gtk_widget_destroy (GTK_WIDGET (dialog));

		while (gtk_events_pending ())
			gtk_main_iteration ();
	} else {
			*cancel = FALSE;
	}

	return retval;
}

static void 
tny_gtk_password_dialog_forget_password (TnyPasswordGetter *self, const gchar *aid)
{
	if (G_LIKELY (passwords))
	{
		const gchar *accountid = aid;

		gchar *pwd = g_hash_table_lookup (passwords, accountid);

		if (G_LIKELY (pwd))
		{
			memset (pwd, 0, strlen (pwd));
			/* g_free (pwd); uhm, crashed once */
			g_hash_table_remove (passwords, accountid);
		}

	}
}

/**
 * tny_gtk_password_dialog_new:
 * 
 * Create a dialog window that will ask the user for a password
 *
 * returns: (caller-owns): A new #GtkDialog password dialog
 * since: 1.0
 * audience: application-developer
 **/
TnyPasswordGetter*
tny_gtk_password_dialog_new (void)
{
	TnyGtkPasswordDialog *self = g_object_new (TNY_TYPE_GTK_PASSWORD_DIALOG, NULL);

	return TNY_PASSWORD_GETTER (self);
}

static void
tny_gtk_password_dialog_instance_init (GTypeInstance *instance, gpointer g_class)
{
	return;
}

static void
tny_gtk_password_dialog_finalize (GObject *object)
{
	(*parent_class->finalize) (object);

	return;
}



static void
tny_password_getter_init (gpointer g, gpointer iface_data)
{
	TnyPasswordGetterIface *klass = (TnyPasswordGetterIface *)g;
	klass->forget_password= tny_gtk_password_dialog_forget_password;
	klass->get_password= tny_gtk_password_dialog_get_password;
}


static void 
tny_gtk_password_dialog_class_init (TnyGtkPasswordDialogClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	object_class->finalize = tny_gtk_password_dialog_finalize;

	return;
}

static gpointer 
tny_gtk_password_dialog_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyGtkPasswordDialogClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_gtk_password_dialog_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyGtkPasswordDialog),
		  0,      /* n_preallocs */
		  tny_gtk_password_dialog_instance_init    /* instance_init */
		};

	static const GInterfaceInfo tny_password_getter_info = 
		{
		  (GInterfaceInitFunc) tny_password_getter_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyGtkPasswordDialog", &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_PASSWORD_GETTER, 
				     &tny_password_getter_info);

	return GSIZE_TO_POINTER (type);
}

GType 
tny_gtk_password_dialog_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gtk_password_dialog_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
