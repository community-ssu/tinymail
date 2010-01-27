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

#include <time.h>

#include <tny-list.h>
#include <tny-iterator.h>

#include <tny-camel-msg.h>
#include <tny-mime-part.h>
#include <tny-stream.h>
#include <tny-header.h>
#include <tny-camel-mime-part.h>
#include <tny-stream-camel.h>
#include <tny-camel-shared.h>
#include <tny-account.h>
#include <tny-folder.h>

#include <camel/camel-types.h>

#include <tny-camel-msg-remove-strategy.h>
#include <tny-camel-full-msg-receive-strategy.h>
#include <tny-camel-partial-msg-receive-strategy.h>
#include <tny-session-camel.h>

#include <tny-camel-account.h>
#include <tny-session-camel.h>
#include <tny-account-store.h>
#include <tny-folder.h>
#include <tny-camel-folder.h>
#include <tny-error.h>

#include <camel/camel.h>
#include <camel/camel-session.h>
#include <camel/camel-store.h>
#include <camel/camel-offline-folder.h>
#include <camel/camel-offline-store.h>
#include <camel/camel-disco-folder.h>
#include <camel/camel-disco-store.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <tny-camel-shared.h>
#include <tny-status.h>

#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include "tny-camel-account-priv.h"
#include "tny-session-camel-priv.h"
#include "tny-camel-msg-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-mime-part-priv.h"
#include "tny-camel-msg-header-priv.h"

static GObjectClass *parent_class = NULL;

#define TNY_CAMEL_MSG_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_CAMEL_MSG, TnyCamelMsgPriv))


CamelMimeMessage* 
_tny_camel_msg_get_camel_mime_message (TnyCamelMsg *self)
{
	TnyCamelMimePartPriv *ppriv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	return CAMEL_MIME_MESSAGE (ppriv->part);
}


static TnyFolder* 
tny_camel_msg_get_folder (TnyMsg *self)
{
	return TNY_CAMEL_MSG_GET_CLASS (self)->get_folder(self);
}

static TnyFolder* 
tny_camel_msg_get_folder_default (TnyMsg *self)
{
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);
	TnyFolder *retval;

	g_mutex_lock (priv->folder_lock);
	retval = priv->folder;
	if (retval)
		g_object_ref (G_OBJECT (retval));
	g_mutex_unlock (priv->folder_lock);

	return retval;
}



void 
_tny_camel_msg_set_received (TnyCamelMsg *self, time_t received)
{
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);
	priv->received = received;
}

void
_tny_camel_msg_set_folder (TnyCamelMsg *self, TnyFolder* folder)
{
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);

	g_assert (TNY_IS_CAMEL_FOLDER (folder));

	g_mutex_lock (priv->folder_lock);
	priv->folder = (TnyFolder*)folder;
	if (priv->header)
		((TnyCamelMsgHeader *)priv->header)->folder = folder;
	g_object_ref (priv->folder);
	g_mutex_unlock (priv->folder_lock);

	return;
}



static TnyHeader*
tny_camel_msg_get_header (TnyMsg *self)
{
	return TNY_CAMEL_MSG_GET_CLASS (self)->get_header(self);
}

void 
_tny_camel_msg_set_header (TnyCamelMsg *self, TnyHeader *header)
{
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);
	g_mutex_lock (priv->header_lock);
	priv->header = TNY_HEADER (g_object_ref (G_OBJECT (header)));
	g_mutex_unlock (priv->header_lock);
	return;
}

static gchar* 
tny_camel_msg_get_url_string (TnyMsg *self)
{
	return TNY_CAMEL_MSG_GET_CLASS (self)->get_url_string(self);
}

static gchar* 
tny_camel_msg_get_url_string_default (TnyMsg *self)
{
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);
	gchar *retval = NULL;

	if (priv->folder)
	{
		TnyHeader *header = tny_msg_get_header (self);
		gchar *uid = tny_header_dup_uid (header);

		/* This is incorrect, the UID will always be NULL */

		if (uid)
		{
			TnyCamelFolderPriv *fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (priv->folder);
			if (fpriv->iter && fpriv->iter->uri)
			{
				retval = g_strdup_printf ("%s/%s", fpriv->iter->uri, uid);

			} else if (fpriv->account)
			{
				TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (fpriv->account);
				if (apriv->service)
				{
					char *urls = camel_service_get_url (apriv->service);
					const char *foln = camel_folder_get_full_name (fpriv->folder);
					retval = g_strdup_printf ("%s/%s/%s", urls, foln, uid);
					g_free (urls);
				}
			}
			g_free (uid);
		}

		g_object_unref (G_OBJECT (header));
	}

	return retval;
}

static void
tny_camel_msg_rewrite_cache_default (TnyMsg *self)
{
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);
	CamelMimeMessage *msg;
	
	if (priv->folder && priv->header) {
		gchar *uid;
		msg = _tny_camel_msg_get_camel_mime_message (TNY_CAMEL_MSG (self));
		uid = tny_header_dup_uid (priv->header);
		_tny_camel_folder_rewrite_cache (TNY_CAMEL_FOLDER(priv->folder),
						 uid, msg);
		g_free (uid);
	}
	return;
}

static void
tny_camel_msg_rewrite_cache (TnyMsg *self)
{
	TNY_CAMEL_MSG_GET_CLASS (self)->rewrite_cache(self);
	return;
}

static gboolean
tny_camel_msg_get_allow_external_images_default (TnyMsg *self)
{
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);
	gboolean allow = FALSE;
	
	if (priv->folder && priv->header) {
		gchar *uid;
		uid = tny_header_dup_uid (priv->header);
		allow = _tny_camel_folder_get_allow_external_images (TNY_CAMEL_FOLDER(priv->folder),
								     uid);
		g_free (uid);
	}
	return allow;
}

static gboolean
tny_camel_msg_get_allow_external_images (TnyMsg *self)
{
	return TNY_CAMEL_MSG_GET_CLASS (self)->get_allow_external_images (self);
}

static void
tny_camel_msg_set_allow_external_images_default (TnyMsg *self, gboolean allow)
{
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);

	if (priv->folder && priv->header) {
		gchar *uid;
		uid = tny_header_dup_uid (priv->header);
		_tny_camel_folder_set_allow_external_images (TNY_CAMEL_FOLDER(priv->folder),
							     uid, allow);
		g_free (uid);
	}
	return;
}

static void
tny_camel_msg_set_allow_external_images (TnyMsg *self, gboolean allow)
{
	TNY_CAMEL_MSG_GET_CLASS (self)->set_allow_external_images (self, allow);
	return;
}

static TnyHeader*
tny_camel_msg_get_header_default (TnyMsg *self)
{
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);
	TnyHeader *retval;

	g_mutex_lock (priv->header_lock);

	if (!priv->header)
	{
		CamelMimeMessage *msg;
		msg = _tny_camel_msg_get_camel_mime_message (TNY_CAMEL_MSG (self));
		/* Read _tny_camel_msg_header_new too! */
		priv->header = _tny_camel_msg_header_new (msg, priv->folder, priv->received);
		if (priv->folder)
			((TnyCamelMsgHeader *)priv->header)->folder = priv->folder;
	}

	retval = TNY_HEADER (g_object_ref (G_OBJECT (priv->header)));

	g_mutex_unlock (priv->header_lock);

	return retval;
}


static void 
tny_camel_msg_uncache_attachments (TnyMsg *self)
{
	TNY_CAMEL_MSG_GET_CLASS (self)->uncache_attachments(self);
}

static void 
tny_camel_msg_uncache_attachments_default (TnyMsg *self)
{
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);

	if (priv->folder && priv->header)
	{
		gchar *uid;
		uid = tny_header_dup_uid (priv->header);
		_tny_camel_folder_uncache_attachments (
			TNY_CAMEL_FOLDER (priv->folder), uid);
		g_free (uid);
		/* tny_header_set_flag (priv->header, TNY_HEADER_FLAG_PARTIAL); */
	}
}

static void
tny_camel_msg_finalize (GObject *object)
{
	TnyCamelMsg *self = (TnyCamelMsg*) object;
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);

	g_mutex_lock (priv->message_lock);
	g_mutex_lock (priv->header_lock);

	if (G_LIKELY (priv->header))
		g_object_unref (G_OBJECT (priv->header));
	priv->header = NULL;
	
	if (G_LIKELY (priv->folder))
		g_object_unref (G_OBJECT (priv->folder));
	priv->folder = NULL;

	g_mutex_unlock (priv->header_lock);
	g_mutex_unlock (priv->message_lock);

	g_mutex_free (priv->message_lock);
	g_mutex_free (priv->header_lock);
	g_mutex_free (priv->parts_lock);
	g_mutex_free (priv->folder_lock);

	(*parent_class->finalize) (object);

	return;
}

/**
 * tny_camel_msg_new:
 * 
 * The #TnyCamelMsg implementation is actually a proxy for #CamelMimeMessage (and
 * a few other Camel types)
 *
 * Return value: A new #TnyMsg instance implemented for Camel
 **/
TnyMsg*
tny_camel_msg_new (void)
{
	TnyCamelMsg *self = g_object_new (TNY_TYPE_CAMEL_MSG, NULL);
	CamelMimeMessage *ms = camel_mime_message_new ();

	_tny_camel_mime_part_set_part (TNY_CAMEL_MIME_PART (self), 
		CAMEL_MIME_PART (ms));

	camel_object_unref (ms);

	return TNY_MSG (self);
}


/**
 * tny_camel_msg_new_with_part:
 * @part: a #CamelMimePart object
 *
 * The #TnyMsg implementation is actually a proxy for #CamelMimePart.
 *
 * Return value: A new #TnyMsg instance implemented for Camel
 **/
TnyMsg*
tny_camel_msg_new_with_part (CamelMimePart *part)
{
	TnyCamelMsg *self = g_object_new (TNY_TYPE_CAMEL_MSG, NULL);

	_tny_camel_mime_part_set_part (TNY_CAMEL_MIME_PART (self), 
		CAMEL_MIME_PART (camel_mime_message_new ()));

	return TNY_MSG (self);
}


static void
tny_msg_init (gpointer g, gpointer iface_data)
{
	TnyMsgIface *klass = (TnyMsgIface *)g;

	klass->get_header= tny_camel_msg_get_header;
	klass->get_folder= tny_camel_msg_get_folder;
	klass->get_url_string= tny_camel_msg_get_url_string;
	klass->uncache_attachments= tny_camel_msg_uncache_attachments;
	klass->rewrite_cache= tny_camel_msg_rewrite_cache;
	klass->get_allow_external_images = tny_camel_msg_get_allow_external_images;
	klass->set_allow_external_images = tny_camel_msg_set_allow_external_images;

	return;
}

static void 
tny_camel_msg_class_init (TnyCamelMsgClass *class)
{
	GObjectClass *object_class;
	
	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	
	class->get_header= tny_camel_msg_get_header_default;
	class->get_folder= tny_camel_msg_get_folder_default;
	class->get_url_string= tny_camel_msg_get_url_string_default;
	class->uncache_attachments= tny_camel_msg_uncache_attachments_default;
	class->rewrite_cache= tny_camel_msg_rewrite_cache_default;
	class->get_allow_external_images = tny_camel_msg_get_allow_external_images_default;
	class->set_allow_external_images = tny_camel_msg_set_allow_external_images_default;
	
	object_class->finalize = tny_camel_msg_finalize;
	
	g_type_class_add_private (object_class, sizeof (TnyCamelMsgPriv));

	return;
}


static void
tny_camel_msg_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelMsg *self = (TnyCamelMsg*)instance;
	TnyCamelMsgPriv *priv = TNY_CAMEL_MSG_GET_PRIVATE (self);

	priv->header = NULL;
	priv->message_lock = g_mutex_new ();
	priv->parts_lock = g_mutex_new ();
	priv->header_lock = g_mutex_new ();
	priv->folder_lock = g_mutex_new ();
	priv->received = -1;

	return;
}

static gpointer
tny_camel_msg_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelMsgClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_msg_class_init, /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelMsg),
			0,      /* n_preallocs */
			tny_camel_msg_instance_init,    /* instance_init */
			NULL
		};
	
	static const GInterfaceInfo tny_msg_info = 
		{
			(GInterfaceInitFunc) tny_msg_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (TNY_TYPE_CAMEL_MIME_PART,
				       "TnyCamelMsg",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_MSG,
				     &tny_msg_info);
	

	return GUINT_TO_POINTER (type);
}

/**
 * tny_camel_msg_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_msg_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	
	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_msg_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}

void
tny_camel_msg_parse (TnyMsg *self, TnyStream *stream)
{
	TnyCamelMimePartPriv *ppriv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelMimeParser *parser;
	CamelStream *cstream;

	cstream = tny_stream_camel_new (stream);

	parser = camel_mime_parser_new ();
	camel_mime_parser_init_with_stream (parser, cstream);

	camel_mime_part_construct_from_parser (ppriv->part, parser);

	camel_object_unref (cstream);
	camel_object_unref (parser);
}

