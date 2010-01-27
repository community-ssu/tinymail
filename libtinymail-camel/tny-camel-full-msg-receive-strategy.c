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
#include <glib.h>
#include <glib/gi18n-lib.h>

#include <tny-camel-full-msg-receive-strategy.h>

#include <tny-camel-folder.h>
#include <tny-error.h>
#include <tny-msg.h>
#include <tny-header.h>
#include <tny-camel-msg.h>

#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-service.h>
#include <camel/camel-store.h>

#include <tny-session-camel.h>
#include <tny-account.h>
#include <tny-camel-account.h>

#include <tny-status.h>
#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#include "tny-camel-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include "tny-camel-account-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-msg-header-priv.h"
#include "tny-camel-msg-priv.h"
#include "tny-camel-mime-part-priv.h"

static GObjectClass *parent_class = NULL;

static TnyMsg *
tny_camel_full_msg_receive_strategy_perform_get_msg (TnyMsgReceiveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err)
{
	return TNY_CAMEL_FULL_MSG_RECEIVE_STRATEGY_GET_CLASS (self)->perform_get_msg(self, folder, header, err);
}

static TnyMsg *
tny_camel_full_msg_receive_strategy_perform_get_msg_default (TnyMsgReceiveStrategy *self, TnyFolder *folder, TnyHeader *header, GError **err)
{
	TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (folder);
	TnyMsg *message = NULL;
	CamelMimeMessage *camel_message = NULL;
	gchar *id;
	CamelException ex = CAMEL_EXCEPTION_INITIALISER;
	CamelFolderReceiveType mtype = CAMEL_FOLDER_RECEIVE_FULL;

	g_assert (TNY_IS_HEADER (header));


	if (!priv->strict_retrieval)
		mtype = CAMEL_FOLDER_RECEIVE_ANY_OR_FULL;

	id = tny_header_dup_uid (TNY_HEADER (header));
	message = NULL;
	camel_message = camel_folder_get_message (priv->folder, (const char *) id, mtype, -1, &ex);
	g_free (id);

	if (camel_exception_is_set (&ex))
	{
		_tny_camel_exception_to_tny_error (&ex, err);
		camel_exception_clear (&ex);
	} else 
	{
		if (camel_message && CAMEL_IS_OBJECT (camel_message))
		{
			TnyHeader *nheader = NULL;

			nheader = _tny_camel_msg_header_new (CAMEL_MIME_MESSAGE (camel_message), 
				folder, tny_header_get_date_received (header));
			_tny_camel_msg_header_set_decorated (TNY_CAMEL_MSG_HEADER (nheader), header, FALSE);

			message = tny_camel_msg_new ();
			_tny_camel_msg_set_received (TNY_CAMEL_MSG (message), 
				tny_header_get_date_received (header));
			_tny_camel_msg_set_folder (TNY_CAMEL_MSG (message), folder);
			TNY_CAMEL_MSG_HEADER (nheader)->old_uid = tny_header_dup_uid (header);
			_tny_camel_msg_set_header (TNY_CAMEL_MSG (message), nheader);
			_tny_camel_mime_part_set_part (TNY_CAMEL_MIME_PART (message), 
						CAMEL_MIME_PART (camel_message)); 
			tny_header_set_flag (nheader, TNY_HEADER_FLAG_CACHED);
			tny_header_unset_flag (nheader, TNY_HEADER_FLAG_PARTIAL);
			g_object_unref (nheader);
			tny_header_set_flag (header, TNY_HEADER_FLAG_CACHED);
			tny_header_unset_flag (header, TNY_HEADER_FLAG_PARTIAL);
		}
	}

	if (camel_message && CAMEL_IS_OBJECT (camel_message))
		camel_object_unref (CAMEL_OBJECT (camel_message));

	return message;
}

static void
tny_camel_full_msg_receive_strategy_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
tny_msg_receive_strategy_init (TnyMsgReceiveStrategyIface *klass)
{
	klass->perform_get_msg= tny_camel_full_msg_receive_strategy_perform_get_msg;
}


/**
 * tny_camel_full_msg_receive_strategy_new:
 * 
 * A message receiver that fetches full messages
 *
 * Return value: A new #TnyMsgReceiveStrategy instance implemented for Camel
 **/
TnyMsgReceiveStrategy* 
tny_camel_full_msg_receive_strategy_new (void)
{
	TnyCamelFullMsgReceiveStrategy *self = g_object_new (TNY_TYPE_CAMEL_FULL_MSG_RECEIVE_STRATEGY, NULL);

	return TNY_MSG_RECEIVE_STRATEGY (self);
}


static void
tny_camel_full_msg_receive_strategy_class_init (TnyCamelFullMsgReceiveStrategyClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;

	klass->perform_get_msg= tny_camel_full_msg_receive_strategy_perform_get_msg_default;

	object_class->finalize = tny_camel_full_msg_receive_strategy_finalize;
}

static void 
tny_camel_full_msg_receive_strategy_instance_init (GTypeInstance *instance, gpointer g_class)
{
	return;
}


static gpointer
tny_camel_full_msg_receive_strategy_register_type (gpointer notused)
{
	GType type = 0;
	static const GTypeInfo info = 
		{
			sizeof (TnyCamelFullMsgReceiveStrategyClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_full_msg_receive_strategy_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelFullMsgReceiveStrategy),
			0,      /* n_preallocs */
			tny_camel_full_msg_receive_strategy_instance_init,    /* instance_init */
			NULL
		};


	static const GInterfaceInfo tny_msg_receive_strategy_info = 
		{
			(GInterfaceInitFunc) tny_msg_receive_strategy_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelFullMsgReceiveStrategy",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_MSG_RECEIVE_STRATEGY,
				     &tny_msg_receive_strategy_info);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_camel_full_msg_receive_strategy_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_camel_full_msg_receive_strategy_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
