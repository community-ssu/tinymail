/* tinymail - Tiny Mail
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

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include <tny-gnome-keyring-password-getter.h>

static GObjectClass *parent_class = NULL;

static const gchar*
tny_gnome_keyring_password_getter_get_password (TnyPasswordGetter *self, const gchar *aid, const gchar *prompt, gboolean *cancel)
{
	gchar *retval = NULL;
	GList *list;
	GnomeKeyringResult keyringret;
	gchar *keyring;

	gnome_keyring_get_default_keyring_sync (&keyring);

	keyringret = gnome_keyring_find_network_password_sync (aid, "Mail", 
		aid /* hostname */,
		"password", aid /* proto */, 
		"PLAIN", 0, &list);

	if ((keyringret != GNOME_KEYRING_RESULT_OK) || (list == NULL))
	{
		gboolean canc = FALSE;

		GnomePasswordDialog *dialog = GNOME_PASSWORD_DIALOG 
				(gnome_password_dialog_new
					(_("Enter password"), prompt,
					NULL, 
					NULL, TRUE));

		gnome_password_dialog_set_domain (dialog, "Mail");
		gnome_password_dialog_set_remember (dialog, 
			GNOME_PASSWORD_DIALOG_REMEMBER_FOREVER);

		gnome_password_dialog_set_readonly_username (dialog, TRUE);
		/* gnome_password_dialog_set_username (dialog, 
			tny_account_get_user (account)); */

		gnome_password_dialog_set_show_username (dialog, FALSE);
		gnome_password_dialog_set_show_remember (dialog, 
			gnome_keyring_is_available ());
		gnome_password_dialog_set_show_domain (dialog, FALSE);
		gnome_password_dialog_set_show_userpass_buttons (dialog, FALSE);

		canc = gnome_password_dialog_run_and_block (dialog);

		if (canc)
		{
			guint32 item_id;
			GnomePasswordDialogRemember r;

			retval = gnome_password_dialog_get_password (dialog);

			mlock (retval, strlen (retval));

			r = gnome_password_dialog_get_remember (dialog);

			if (r == GNOME_PASSWORD_DIALOG_REMEMBER_FOREVER)
			{
				gnome_keyring_set_network_password_sync (keyring,
					aid /* user */,
					"Mail", aid /* hostname */,
					"password", aid /* proto */, 
					"PLAIN", 0, retval, &item_id);
			}
		} else retval = NULL;

		*cancel = (!canc);

		/* this causes warnings, but should be done afaik */
		gtk_object_destroy (GTK_OBJECT (dialog));

		while (gtk_events_pending ())
			gtk_main_iteration ();

	} else {

		GnomeKeyringNetworkPasswordData *pwd_data;
		pwd_data = list->data;
		retval = g_strdup (pwd_data->password);

		*cancel = FALSE;

		gnome_keyring_network_password_list_free (list);
	}

	return retval;
}

static void
tny_gnome_keyring_password_getter_forget_password (TnyPasswordGetter *self, const gchar *aid)
{
	GList *list=NULL;
	GnomeKeyringResult keyringret;
	gchar *keyring;
	GnomeKeyringNetworkPasswordData *pwd_data;

	gnome_keyring_get_default_keyring_sync (&keyring);

	keyringret = gnome_keyring_find_network_password_sync (
		aid /* user */,
		"Mail", aid /* hostname */,
		"password", aid /* proto */, 
		"PLAIN", 0, &list);

	if (keyringret == GNOME_KEYRING_RESULT_OK && list)
	{
		pwd_data = list->data;
		gnome_keyring_item_delete_sync (keyring, pwd_data->item_id);
		gnome_keyring_network_password_list_free (list);
	}
	return;
}

static void
tny_gnome_keyring_password_getter_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
tny_password_getter_init (TnyPasswordGetterIface *klass)
{
	klass->get_password= tny_gnome_keyring_password_getter_get_password;
	klass->forget_password= tny_gnome_keyring_password_getter_forget_password;
}

static void
tny_gnome_keyring_password_getter_class_init (TnyGnomeKeyringPasswordGetterClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = tny_gnome_keyring_password_getter_finalize;
}

static void 
tny_gnome_keyring_password_getter_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


TnyPasswordGetter* tny_gnome_keyring_password_getter_new (void)
{
	TnyGnomeKeyringPasswordGetter *self = g_object_new (TNY_TYPE_GNOME_KEYRING_PASSWORD_GETTER, NULL);

	return TNY_PASSWORD_GETTER (self);
}

static gpointer
tny_gnome_keyring_password_getter_register_type (gpointer notused)
{
	GType type = 0;
	static const GTypeInfo info = 
		{
			sizeof (TnyGnomeKeyringPasswordGetterClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_gnome_keyring_password_getter_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyGnomeKeyringPasswordGetter),
			0,      /* n_preallocs */
			tny_gnome_keyring_password_getter_instance_init,    /* instance_init */
			NULL
		};


	static const GInterfaceInfo tny_password_getter_info = 
		{
			(GInterfaceInitFunc) tny_password_getter_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyGnomeKeyringPasswordGetter",
				       &info, 0);

	g_type_add_interface_static (type, TNY_TYPE_PASSWORD_GETTER,
				     &tny_password_getter_info);

	return GUINT_TO_POINTER (type);
}

GType
tny_gnome_keyring_password_getter_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_gnome_keyring_password_getter_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
