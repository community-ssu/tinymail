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

/* TODO: Refactor to a TnyNNTPStoreAccount, TnyPOPStoreAccount and 
TnyNNTPStoreAccount. Maybe also add a TnyExchangeStoreAccount? This file can 
stay as an abstract TnyStoreAccount type. */

#include <config.h>
#include <glib/gi18n-lib.h>
#include <glib.h>
#include <string.h>

#include <tny-list.h>
#include <tny-account.h>
#include <tny-store-account.h>
#include <tny-camel-store-account.h>
#include <tny-camel-nntp-store-account.h>

#include <tny-folder.h>
#include <tny-folder-store.h>
#include <tny-camel-folder.h>

#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-store.h>

#ifndef CAMEL_FOLDER_TYPE_SENT
#define CAMEL_FOLDER_TYPE_SENT (5 << 10)
#endif

#include <tny-folder.h>
#include <tny-status.h>
#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include "tny-camel-account-priv.h"
#include "tny-camel-store-account-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-common-priv.h"
#include "tny-camel-nntp-folder-priv.h"

#include <tny-camel-shared.h>
#include <tny-account-store.h>
#include <tny-error.h>


static GObjectClass *parent_class = NULL;




static void 
tny_camel_nntp_store_account_remove_folder (TnyFolderStore *self, TnyFolder *folder, GError **err)
{
	g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNSUPPORTED,
			"You can't use the tny_folder_store_remove_folder API "
			"on NNTP accounts. This problem indicates a bug in the "
			"software.");

	return;
}

static TnyFolder* 
tny_camel_nntp_store_account_create_folder (TnyFolderStore *self, const gchar *name, GError **err)
{
	g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNSUPPORTED,
			"You can't use the tny_folder_store_create_folder "
			"API on NNTP accounts. This problem indicates a "
			"bug in the software.");

	return NULL;
}


static TnyFolder * 
tny_camel_nntp_store_account_factor_folder (TnyCamelStoreAccount *self, const gchar *full_name, gboolean *was_new)
{
	TnyCamelStoreAccountPriv *priv = TNY_CAMEL_STORE_ACCOUNT_GET_PRIVATE (self);
	TnyCamelFolder *folder = NULL;

	GList *copy = priv->managed_folders;
	while (copy)
	{
		TnyFolder *fnd = (TnyFolder*) copy->data;
		const gchar *name = tny_folder_get_id (fnd);
		if (!strcmp (name, full_name))
		{
			folder = TNY_CAMEL_FOLDER (g_object_ref (G_OBJECT (fnd)));
			*was_new = FALSE;
			break;
		}
		copy = g_list_next (copy);
	}

	if (!folder) {
		folder = TNY_CAMEL_FOLDER (_tny_camel_nntp_folder_new ());
		*was_new = TRUE;
	}

	return (TnyFolder *) folder;
}

/**
 * tny_camel_nntp_store_account_new:
 * 
 * Create a new NNTP #TnyStoreAccount instance implemented for Camel
 *
 * Return value: A new NNTP #TnyStoreAccount instance implemented for Camel
 **/
TnyStoreAccount*
tny_camel_nntp_store_account_new (void)
{
	TnyCamelNNTPStoreAccount *self = g_object_new (TNY_TYPE_CAMEL_NNTP_STORE_ACCOUNT, NULL);

	return TNY_STORE_ACCOUNT (self);
}

static void
tny_camel_nntp_store_account_finalize (GObject *object)
{

	/* The abstract CamelStoreAccount finalizes everything correctly */

	(*parent_class->finalize) (object);

	return;
}

static void 
tny_camel_nntp_store_account_class_init (TnyCamelNNTPStoreAccountClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	TNY_CAMEL_STORE_ACCOUNT_CLASS (class)->remove_folder= tny_camel_nntp_store_account_remove_folder;
	TNY_CAMEL_STORE_ACCOUNT_CLASS (class)->create_folder= tny_camel_nntp_store_account_create_folder;

	/* Protected override */
	TNY_CAMEL_STORE_ACCOUNT_CLASS (class)->factor_folder= tny_camel_nntp_store_account_factor_folder;

	object_class->finalize = tny_camel_nntp_store_account_finalize;

	return;
}


static void
tny_camel_nntp_store_account_instance_init (GTypeInstance *instance, gpointer g_class)
{
	/* The abstract CamelStoreAccount initializes everything correctly */

	return;
}

static gpointer
tny_camel_nntp_store_account_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelNNTPStoreAccountClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_nntp_store_account_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelNNTPStoreAccount),
			0,      /* n_preallocs */
			tny_camel_nntp_store_account_instance_init    /* instance_init */
		};
	
	type = g_type_register_static (TNY_TYPE_CAMEL_STORE_ACCOUNT,
				       "TnyCamelNNTPStoreAccount",
				       &info, 0);

	return GSIZE_TO_POINTER (type);
}

/**
 * tny_camel_nntp_store_account_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_nntp_store_account_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_nntp_store_account_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
