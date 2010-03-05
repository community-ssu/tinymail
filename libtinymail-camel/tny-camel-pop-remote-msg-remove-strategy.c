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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string.h>

#include <tny-error.h>

#include <tny-camel-pop-remote-msg-remove-strategy.h>
#include <tny-camel-folder.h>

#include <tny-session-camel.h>
#include <tny-account.h>
#include <tny-camel-account.h>

#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-service.h>
#include <camel/camel-store.h>

#include <tny-session-camel.h>
#include <tny-account.h>
#include <tny-camel-account.h>

#include <camel/providers/pop3/camel-pop3-store.h>

#include <tny-status.h>
#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API


#include "tny-camel-account-priv.h"
#include "tny-camel-folder-priv.h"

#include "tny-camel-account-priv.h"
#include "tny-camel-folder-priv.h"

static GObjectClass *parent_class = NULL;


static void
tny_camel_pop_remote_msg_remove_strategy_perform_remove (TnyMsgRemoveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err)
{
	TNY_CAMEL_POP_REMOTE_MSG_REMOVE_STRATEGY_GET_CLASS (self)->perform_remove(self, folder, header, err);
	return;
}

static void
tny_camel_pop_remote_msg_remove_strategy_perform_remove_default (TnyMsgRemoveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err)
{
	TnyCamelFolderPriv *fpriv = NULL;
	TnyAccount *acc = NULL;
	gchar *uid;
	CamelFolder *cfolder;

	g_assert (TNY_IS_CAMEL_FOLDER (folder));
	g_assert (TNY_IS_HEADER (header));

	uid = tny_header_dup_uid (header);
	fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (folder);
	acc = fpriv->account;

	if (acc && uid)
	{
		const CamelService *service = _tny_camel_account_get_service (TNY_CAMEL_ACCOUNT (acc));
		CamelPOP3Store *pop3_store = (CamelPOP3Store *) service;
		gchar *expunged_path = NULL;
		FILE *file = NULL;

		/* camel-pop3-folder.c:455 + 490 is also interesting code! */
		expunged_path = g_strdup_printf ("%s/%s.expunged", pop3_store->storage_path, uid);
		cfolder = _tny_camel_folder_get_folder (TNY_CAMEL_FOLDER (folder));
		camel_folder_delete_message (cfolder, uid);
		camel_object_unref (CAMEL_OBJECT (cfolder));
		file = fopen (expunged_path, "w");
		g_free (expunged_path);
		if (file != NULL)
		{
			fprintf (file, "%s", uid);
			fclose (file);
		}
	}
	g_free (uid);

	return;
}


/**
 * tny_camel_pop_remote_msg_remove_strategy_new:
 *
 * Create a remove strategy for TnyCamelPOPFolder instances. This strategy will
 * remote the message remotely after doing a tny_folder_sync. The default remove
 * strategy of Tinymail wont do this for POP folders (although it will do this 
 * for other E-mail service types, like IMAP). Using this strategy you can easily
 * change this behaviour.
 *
 * Return value: a new #TnyPopRemoteMsgRemoveStrategy instance implemented for a #TnyCamelFolder
 **/
TnyMsgRemoveStrategy*
tny_camel_pop_remote_msg_remove_strategy_new (void)
{
	TnyCamelPopRemoteMsgRemoveStrategy *self = g_object_new (TNY_TYPE_CAMEL_POP_REMOTE_MSG_REMOVE_STRATEGY, NULL);

	return TNY_MSG_REMOVE_STRATEGY (self);
}

static void
tny_camel_pop_remote_msg_remove_strategy_instance_init (GTypeInstance *instance, gpointer g_class)
{
	return;
}

static void
tny_camel_pop_remote_msg_remove_strategy_finalize (GObject *object)
{
	(*parent_class->finalize) (object);

	return;
}

static void
tny_camel_pop_remote_msg_remove_strategy_init (gpointer g, gpointer iface_data)
{
	TnyMsgRemoveStrategyIface *klass = (TnyMsgRemoveStrategyIface *)g;

	klass->perform_remove= tny_camel_pop_remote_msg_remove_strategy_perform_remove;

	return;
}

static void 
tny_camel_pop_remote_msg_remove_strategy_class_init (TnyCamelPopRemoteMsgRemoveStrategyClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->perform_remove= tny_camel_pop_remote_msg_remove_strategy_perform_remove_default;

	object_class->finalize = tny_camel_pop_remote_msg_remove_strategy_finalize;

	return;
}

static gpointer 
tny_camel_pop_remote_msg_remove_strategy_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelPopRemoteMsgRemoveStrategyClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_pop_remote_msg_remove_strategy_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelPopRemoteMsgRemoveStrategy),
			0,      /* n_preallocs */
			tny_camel_pop_remote_msg_remove_strategy_instance_init,    /* instance_init */
			NULL
		};
	
	static const GInterfaceInfo tny_camel_pop_remote_msg_remove_strategy_info = 
		{
			(GInterfaceInitFunc) tny_camel_pop_remote_msg_remove_strategy_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelPopRemoteMsgRemoveStrategy",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_MSG_REMOVE_STRATEGY, 
				     &tny_camel_pop_remote_msg_remove_strategy_info);

	return GSIZE_TO_POINTER (type);
}

GType 
tny_camel_pop_remote_msg_remove_strategy_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_camel_pop_remote_msg_remove_strategy_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
