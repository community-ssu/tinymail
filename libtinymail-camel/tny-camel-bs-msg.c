/* libtinymail-camel_bs - The Tiny Mail base library for CamelBs
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
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <tny-list.h>
#include <tny-iterator.h>
#include <tny-status.h>
#include <tny-header.h>
#include <tny-simple-list.h>

#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include <tny-camel-shared.h>

#include "camel/camel-folder.h"
#include "camel/camel-service.h"

#include <tny-camel-folder.h>
#include <tny-camel-bs-msg.h>
#include <tny-camel-bs-mime-part.h>

#include "tny-camel-folder-priv.h"
#include "tny-camel-account-priv.h"
#include "tny-camel-bs-msg-priv.h"
#include "tny-camel-bs-mime-part-priv.h"

static GObjectClass *parent_class = NULL;

#define TNY_CAMEL_BS_MSG_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_CAMEL_BS_MSG, TnyCamelBsMsgPriv))


static TnyFolder* 
tny_camel_bs_msg_get_folder (TnyMsg *self)
{
	return TNY_CAMEL_BS_MSG_GET_CLASS (self)->get_folder(self);
}

static TnyFolder* 
tny_camel_bs_msg_get_folder_default (TnyMsg *self)
{
	TnyCamelBsMsgPriv *priv = TNY_CAMEL_BS_MSG_GET_PRIVATE (self);
	TnyCamelBsMimePartPriv *mpriv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	TnyFolder *retval;

	g_mutex_lock (priv->folder_lock);
	retval = mpriv->folder;
	if (retval)
		g_object_ref (retval);
	g_mutex_unlock (priv->folder_lock);

	return retval;
}


static TnyHeader*
tny_camel_bs_msg_get_header (TnyMsg *self)
{
	return TNY_CAMEL_BS_MSG_GET_CLASS (self)->get_header(self);
}

void 
_tny_camel_bs_msg_set_header (TnyCamelBsMsg *self, TnyHeader *header)
{
	TnyCamelBsMsgPriv *priv = TNY_CAMEL_BS_MSG_GET_PRIVATE (self);
	g_mutex_lock (priv->header_lock);
	if (priv->header)
		g_object_unref (priv->header);
	priv->header = TNY_HEADER (g_object_ref (header));
	g_mutex_unlock (priv->header_lock);
	return;
}

static gchar* 
tny_camel_bs_msg_get_url_string (TnyMsg *self)
{
	return TNY_CAMEL_BS_MSG_GET_CLASS (self)->get_url_string(self);
}

static gchar* 
tny_camel_bs_msg_get_url_string_default (TnyMsg *self)
{
	TnyCamelBsMsgPriv *priv = TNY_CAMEL_BS_MSG_GET_PRIVATE (self);
	gchar *retval = NULL;
	TnyFolder *folder;

	TnyHeader *header = tny_msg_get_header (self);
	gchar *uid = tny_header_dup_uid (header);

	if (priv->folder) {
		folder = g_object_ref (priv->folder);
	} else if (header) {
		folder = tny_header_get_folder (header);
	} else {
		folder = NULL;
	}

	if (uid && folder) {
		TnyCamelFolderPriv *fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (priv->folder);
		if (fpriv->iter && fpriv->iter->uri) {
			retval = g_strdup_printf ("%s/%s", fpriv->iter->uri, uid);
			
		} else if (fpriv->account) {
			TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (fpriv->account);
			if (apriv->service) {
				char *urls = camel_service_get_url (apriv->service);
				CamelFolder *cfol = _tny_camel_folder_get_camel_folder (TNY_CAMEL_FOLDER (priv->folder));
				const char *foln = camel_folder_get_full_name (cfol);
				retval = g_strdup_printf ("%s/%s/%s", urls, foln, uid);
				g_free (urls);
			}
		}
		g_free (uid);
	}
	
	g_object_unref (header);
	if (folder) g_object_unref (folder);

	return retval;
}

static void
tny_camel_bs_msg_rewrite_cache_default (TnyMsg *self)
{
	/* Purging in TnyCamelBsMimePart is immediate */
	return;
}

static void
tny_camel_bs_msg_rewrite_cache (TnyMsg *self)
{
	TNY_CAMEL_BS_MSG_GET_CLASS (self)->rewrite_cache(self);
	return;
}

static gboolean
tny_camel_bs_msg_get_allow_external_images_default (TnyMsg *self)
{
	TnyCamelBsMsgPriv *priv = TNY_CAMEL_BS_MSG_GET_PRIVATE (self);
	gboolean allow = FALSE;
	
	if (priv->header) {
		gchar *uid;
		TnyFolder *folder;
		uid = tny_header_dup_uid (priv->header);
		if (priv->folder) {
			folder = g_object_ref (priv->folder);
		} else {
			folder = tny_header_get_folder (priv->header);
		}
		if (folder) {
			allow = _tny_camel_folder_get_allow_external_images (TNY_CAMEL_FOLDER(folder),
									     uid);
			g_object_unref (folder);
		}
		g_free (uid);
	}
	return allow;
}

static gboolean
tny_camel_bs_msg_get_allow_external_images (TnyMsg *self)
{
	return TNY_CAMEL_BS_MSG_GET_CLASS (self)->get_allow_external_images (self);
}

static void
tny_camel_bs_msg_set_allow_external_images_default (TnyMsg *self, gboolean allow)
{
	TnyCamelBsMsgPriv *priv = TNY_CAMEL_BS_MSG_GET_PRIVATE (self);

	if (priv->header) {
		gchar *uid;
		TnyFolder *folder;
		uid = tny_header_dup_uid (priv->header);
		if (priv->folder) {
			folder = g_object_ref (priv->folder);
		} else {
			folder = tny_header_get_folder (priv->header);
		}
		if (folder) {
			_tny_camel_folder_set_allow_external_images (TNY_CAMEL_FOLDER(folder),
								     uid, allow);
			g_object_unref (folder);
		}
		g_free (uid);
	}
	return;
}

static void
tny_camel_bs_msg_set_allow_external_images (TnyMsg *self, gboolean allow)
{
	TNY_CAMEL_BS_MSG_GET_CLASS (self)->set_allow_external_images (self, allow);
	return;
}

static TnyHeader*
tny_camel_bs_msg_get_header_default (TnyMsg *self)
{
	TnyCamelBsMsgPriv *priv = TNY_CAMEL_BS_MSG_GET_PRIVATE (self);
	TnyHeader *retval;

	g_mutex_lock (priv->header_lock);
	retval = TNY_HEADER (g_object_ref (priv->header));
	g_mutex_unlock (priv->header_lock);

	return retval;
}


static void 
tny_camel_bs_msg_uncache_attachments (TnyMsg *self)
{
	TNY_CAMEL_BS_MSG_GET_CLASS (self)->uncache_attachments(self);
}

static void 
tny_camel_bs_msg_uncache_attachments_default (TnyMsg *self)
{
	TnyList *list = tny_simple_list_new ();
	TnyIterator *iter;

	tny_mime_part_get_parts (TNY_MIME_PART (self), list);
	iter = tny_list_create_iterator (list);
	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
		if (tny_mime_part_is_attachment (part))
			tny_mime_part_set_purged (part);
		g_object_unref (part);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_object_unref (list);

	return;
}

static void
tny_camel_bs_msg_finalize (GObject *object)
{
	TnyCamelBsMsg *self = (TnyCamelBsMsg*) object;
	TnyCamelBsMsgPriv *priv = TNY_CAMEL_BS_MSG_GET_PRIVATE (self);

	g_mutex_lock (priv->message_lock);
	g_mutex_lock (priv->header_lock);

	if (priv->header)
		g_object_unref (priv->header);
	priv->header = NULL;

	if (priv->folder)
		g_object_unref (priv->folder);
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

TnyMsg*
_tny_camel_bs_msg_new (bodystruct_t *bodystructure, const gchar *uid, TnyCamelBsMimePart *parent)
{
	TnyCamelBsMsg *self = g_object_new (TNY_TYPE_CAMEL_BS_MSG, NULL);
	TnyCamelBsMimePartPriv *mpriv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);

	if (parent)
		mpriv->parent = TNY_CAMEL_BS_MIME_PART (g_object_ref (parent));

	mpriv->uid = g_strdup (uid);
	mpriv->bodystructure = bodystructure;

	return TNY_MSG (self);
}



static void
tny_msg_init (gpointer g, gpointer iface_data)
{
	TnyMsgIface *klass = (TnyMsgIface *)g;

	klass->get_header= tny_camel_bs_msg_get_header;
	klass->get_folder= tny_camel_bs_msg_get_folder;
	klass->get_url_string= tny_camel_bs_msg_get_url_string;
	klass->uncache_attachments= tny_camel_bs_msg_uncache_attachments;
	klass->rewrite_cache= tny_camel_bs_msg_rewrite_cache;
	klass->get_allow_external_images = tny_camel_bs_msg_get_allow_external_images;
	klass->set_allow_external_images = tny_camel_bs_msg_set_allow_external_images;

	return;
}

static void 
tny_camel_bs_msg_class_init (TnyCamelBsMsgClass *class)
{
	GObjectClass *object_class;
	
	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;
	
	class->get_header= tny_camel_bs_msg_get_header_default;
	class->get_folder= tny_camel_bs_msg_get_folder_default;
	class->get_url_string= tny_camel_bs_msg_get_url_string_default;
	class->uncache_attachments= tny_camel_bs_msg_uncache_attachments_default;
	class->rewrite_cache= tny_camel_bs_msg_rewrite_cache_default;
	class->get_allow_external_images = tny_camel_bs_msg_get_allow_external_images_default;
	class->set_allow_external_images = tny_camel_bs_msg_set_allow_external_images_default;

	object_class->finalize = tny_camel_bs_msg_finalize;

	g_type_class_add_private (object_class, sizeof (TnyCamelBsMsgPriv));

	return;
}


static void
tny_camel_bs_msg_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelBsMsg *self = (TnyCamelBsMsg*)instance;
	TnyCamelBsMsgPriv *priv = TNY_CAMEL_BS_MSG_GET_PRIVATE (self);

	priv->header = NULL;
	priv->message_lock = g_mutex_new ();
	priv->parts_lock = g_mutex_new ();
	priv->header_lock = g_mutex_new ();
	priv->folder_lock = g_mutex_new ();

	return;
}

static gpointer
tny_camel_bs_msg_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelBsMsgClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_bs_msg_class_init, /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelBsMsg),
			0,      /* n_preallocs */
			tny_camel_bs_msg_instance_init,    /* instance_init */
			NULL
		};
	
	static const GInterfaceInfo tny_msg_info = 
		{
			(GInterfaceInitFunc) tny_msg_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (TNY_TYPE_CAMEL_BS_MIME_PART,
				       "TnyCamelBsMsg",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_MSG,
				     &tny_msg_info);

	return GSIZE_TO_POINTER (type);
}

/**
 * tny_camel_bs_msg_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_bs_msg_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_bs_msg_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
