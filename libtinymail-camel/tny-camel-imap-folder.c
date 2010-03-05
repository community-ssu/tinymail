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

/* TODO: Refactor to a TnyIMAPStoreAccount, TnyIMAPStoreAccount and 
TnyNNTPStoreAccount. Maybe also add a TnyExchangeStoreAccount? This file can 
stay as an abstract TnyStoreAccount type. */

#include <config.h>
#include <glib/gi18n-lib.h>
#include <glib.h>
#include <string.h>


#include <tny-folder.h>
#include <tny-folder-store.h>
#include <tny-camel-folder.h>
#include <tny-camel-imap-folder.h>

#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-store.h>

#include <tny-folder.h>

#include <tny-status.h>
#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include <tny-camel-bs-msg-receive-strategy.h>

#include "tny-camel-account-priv.h"
#include "tny-camel-store-account-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-common-priv.h"

#include <tny-camel-shared.h>
#include <tny-account-store.h>


static GObjectClass *parent_class = NULL;

TnyFolder*
_tny_camel_imap_folder_new (void)
{
	TnyCamelIMAPFolder *self = g_object_new (TNY_TYPE_CAMEL_IMAP_FOLDER, NULL);

	return TNY_FOLDER (self);
}

static void
tny_camel_imap_folder_finalize (GObject *object)
{

	(*parent_class->finalize) (object);

	return;
}

static void 
tny_camel_imap_folder_class_init (TnyCamelIMAPFolderClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_camel_imap_folder_finalize;

	return;
}


static void
tny_camel_imap_folder_instance_init (GTypeInstance *instance, gpointer g_class)
{
#ifdef IMAP_PART_FETCH
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (instance);

	/* By default disabled until after the first release */

	if (priv->receive_strat)
		g_object_unref (priv->receive_strat);

	priv->receive_strat = tny_camel_bs_msg_receive_strategy_new ();
#endif

	return;
}

static gpointer 
tny_camel_imap_folder_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelIMAPFolderClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_imap_folder_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelIMAPFolder),
			0,      /* n_preallocs */
			tny_camel_imap_folder_instance_init    /* instance_init */
		};
	
	type = g_type_register_static (TNY_TYPE_CAMEL_FOLDER,
				       "TnyCamelIMAPFolder",
				       &info, 0);	    

	return GSIZE_TO_POINTER (type);
}

/**
 * tny_camel_imap_folder_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_imap_folder_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}
       
	g_once (&once, tny_camel_imap_folder_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
