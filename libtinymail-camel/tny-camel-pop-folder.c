/* libtinymail-camel - The Tiny Mail base library for Camel
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
#include <glib/gi18n-lib.h>
#include <glib.h>
#include <string.h>


#include <tny-folder.h>
#include <tny-folder-store.h>
#include <tny-camel-folder.h>
#include <tny-camel-pop-folder.h>

#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-store.h>

#include <tny-folder.h>
#include <tny-camel-pop-store-account.h>

#include <tny-status.h>
#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include "tny-camel-account-priv.h"
#include "tny-camel-store-account-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-common-priv.h"

#include "tny-camel-pop-folder-priv.h"
#include "tny-camel-pop-store-account-priv.h"

#include <tny-camel-shared.h>
#include <tny-account-store.h>
#include <tny-error.h>

static GObjectClass *parent_class = NULL;


TnyFolder*
_tny_camel_pop_folder_new (void)
{
	TnyCamelPOPFolder *self = g_object_new (TNY_TYPE_CAMEL_POP_FOLDER, NULL);

	return TNY_FOLDER (self);
}

static void
tny_camel_pop_folder_finalize (GObject *object)
{

	(*parent_class->finalize) (object);

	return;
}


static void 
tny_camel_pop_folder_remove_folder (TnyFolderStore *self, TnyFolder *folder, GError **err)
{
	g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNSUPPORTED,
			"You can't use the tny_folder_store_remove_folder "
			"API on POP accounts. This problem indicates a bug in "
			"the software.");

	return;
}

static TnyFolder* 
tny_camel_pop_folder_create_folder (TnyFolderStore *self, const gchar *name, GError **err)
{
	g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNSUPPORTED,
			"You can't use the tny_folder_store_create_folder "
			"API on POP accounts. This problem indicates a "
			"bug in the software.");

	return NULL;
}


static void 
tny_camel_pop_folder_class_init (TnyCamelPOPFolderClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	TNY_CAMEL_FOLDER_CLASS (class)->remove_folder= tny_camel_pop_folder_remove_folder;
	TNY_CAMEL_FOLDER_CLASS (class)->create_folder= tny_camel_pop_folder_create_folder;

	object_class->finalize = tny_camel_pop_folder_finalize;

	return;
}


static void
tny_camel_pop_folder_instance_init (GTypeInstance *instance, gpointer g_class)
{
	return;
}

static gpointer
tny_camel_pop_folder_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelPOPFolderClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_pop_folder_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelPOPFolder),
			0,      /* n_preallocs */
			tny_camel_pop_folder_instance_init    /* instance_init */
		};
	
	type = g_type_register_static (TNY_TYPE_CAMEL_FOLDER,
				       "TnyCamelPOPFolder",
				       &info, 0);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_camel_pop_folder_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_pop_folder_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_pop_folder_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
