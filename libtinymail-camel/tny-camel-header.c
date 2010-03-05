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

#include <tny-header.h>
#include <tny-camel-folder.h>

#include "tny-camel-common-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-header-priv.h"

#include <tny-camel-shared.h>

#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>
#include <camel/camel-stream-null.h>

static GObjectClass *parent_class = NULL;


void 
_tny_camel_header_set_camel_message_info (TnyCamelHeader *self, CamelMessageInfo *camel_message_info, gboolean knowit)
{
	if (!knowit && G_UNLIKELY (self->info))
		g_warning ("Strange behaviour: Overwriting existing message info");

	if (self->info)
		camel_message_info_free (self->info);

	camel_message_info_ref (camel_message_info);
	self->info = camel_message_info;

	return;
}

void 
_tny_camel_header_set_as_memory (TnyCamelHeader *self, CamelMessageInfo *info)
{
	if (G_UNLIKELY (self->info))
		g_warning ("Strange behaviour: Overwriting existing message info");

	if (self->info)
		camel_message_info_free (self->info);

	self->info = info;

	return;
}


static gchar*
tny_camel_header_dup_replyto (TnyHeader *self)
{
	return NULL;
}


static void
tny_camel_header_set_bcc (TnyHeader *self, const gchar *bcc)
{
	g_warning ("tny_header_set_bcc: This is a summary header instance. You can't modify it.\n");
	return;
}

static void
tny_camel_header_set_cc (TnyHeader *self, const gchar *cc)
{
	g_warning ("tny_header_set_cc: This is a summary header instance. You can't modify it.\n");
	return;
}

static void
tny_camel_header_set_from (TnyHeader *self, const gchar *from)
{
	g_warning ("tny_header_set_from: This is a summary header instance. You can't modify it.\n");
	return;
}

static void
tny_camel_header_set_subject (TnyHeader *self, const gchar *subject)
{
	g_warning ("tny_header_set_subject: This is a summary header instance. You can't modify it.\n");
	return;
}

static void
tny_camel_header_set_to (TnyHeader *self, const gchar *to)
{
	g_warning ("tny_header_set_to: This is a summary header instance. You can't modify it.\n");
	return;
}


static void
tny_camel_header_set_replyto (TnyHeader *self, const gchar *replyto)
{
	g_warning ("tny_header_set_replyto: This is a summary header instance. You can't modify it.\n");
	return;
}


static gchar*
tny_camel_header_dup_cc (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	gchar *retval = NULL;

	camel_folder_summary_lock ();
	retval = g_strdup (camel_message_info_cc (me->info));
	camel_folder_summary_unlock ();

	return retval;
}

static gchar*
tny_camel_header_dup_bcc (TnyHeader *self)
{
	return NULL;
}

static TnyHeaderFlags
tny_camel_header_get_flags (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	TnyHeaderFlags retval = 0;

	/* This is only legal because the flags between CamelLite and Tinymail are equalized */
	retval = camel_message_info_flags (me->info);

	return retval;
}

static void
tny_camel_header_set_flag (TnyHeader *self, TnyHeaderFlags mask)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	CamelMessageInfoBase *info = (CamelMessageInfoBase *) me->info;
	gboolean doit=FALSE;
	TnyCamelFolderPriv *fpriv = NULL;

	if ( ( (!(info->flags & CAMEL_MESSAGE_SEEN) && (mask & TNY_HEADER_FLAG_SEEN)) ||
	   ((info->flags & CAMEL_MESSAGE_SEEN) && !(mask & TNY_HEADER_FLAG_SEEN)) ) 
	   && me->folder) doit = TRUE;

	if (me->folder)
	{
		fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (me->folder);
		fpriv->handle_changes = FALSE;
	}

	/* This is only legal because the flags between CamelLite and Tinymail are equalized */
	camel_message_info_set_flags (me->info, mask, ~0);

	if (fpriv)
		fpriv->handle_changes = TRUE;

	if (doit)
		_tny_camel_folder_check_unread_count (me->folder);

	return;
}

static void
tny_camel_header_unset_flag (TnyHeader *self, TnyHeaderFlags mask)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	CamelMessageInfoBase *info = (CamelMessageInfoBase *) me->info;
	gboolean doit=FALSE;

	if ( ( (!(info->flags & CAMEL_MESSAGE_SEEN) && (mask & TNY_HEADER_FLAG_SEEN)) ||
	   ((info->flags & CAMEL_MESSAGE_SEEN) && !(mask & TNY_HEADER_FLAG_SEEN)) ) 
	   && me->folder) doit = TRUE;

	camel_message_info_set_flags (me->info, mask, 0);

	if (doit)
		_tny_camel_folder_check_unread_count (me->folder);

	return;
}

static gboolean
tny_camel_header_get_user_flag (TnyHeader *self, const gchar *id)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	gboolean retval;

	retval = camel_message_info_user_flag (me->info, id);

	return retval;
}

static void
tny_camel_header_set_user_flag (TnyHeader *self, const gchar *id)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);

	camel_message_info_set_user_flag (me->info, id, TRUE);
}

static void
tny_camel_header_unset_user_flag (TnyHeader *self, const gchar *id)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);

	camel_message_info_set_user_flag (me->info, id, FALSE);
}

static TnyHeaderSupportFlags
tny_camel_header_support_user_flags (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	TnyHeaderSupportFlags flags = TNY_HEADER_SUPPORT_FLAGS_NONE;

	if (camel_message_info_user_flag (me->info, CAMEL_MESSAGE_INFO_SUPPORT_FLAGS_ANY_USER_FLAG))
		flags |= TNY_HEADER_SUPPORT_FLAGS_ANY;
	if (camel_message_info_user_flag (me->info, CAMEL_MESSAGE_INFO_SUPPORT_FLAGS_SOME_USER_FLAG))
		flags |= TNY_HEADER_SUPPORT_FLAGS_SOME;
	if (camel_message_info_user_flag (me->info, CAMEL_MESSAGE_INFO_SUPPORT_PERMANENT_FLAGS_ANY_USER_FLAG))
		flags |= TNY_HEADER_SUPPORT_PERSISTENT_FLAGS_ANY;
	if (camel_message_info_user_flag (me->info, CAMEL_MESSAGE_INFO_SUPPORT_PERMANENT_FLAGS_SOME_USER_FLAG))
		flags |= TNY_HEADER_SUPPORT_PERSISTENT_FLAGS_SOME;

	return flags;
}

static time_t
tny_camel_header_get_date_received (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	time_t retval = 0;

	retval = camel_message_info_date_received (me->info);

	return retval;
}

static time_t
tny_camel_header_get_date_sent (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	time_t retval = 0;

	retval = camel_message_info_date_sent (me->info);

	return retval;
}
	
static gchar*
tny_camel_header_dup_from (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	gchar *retval = NULL;

	camel_folder_summary_lock ();
	retval = g_strdup (camel_message_info_from (me->info));
	camel_folder_summary_unlock ();

	return retval;
}

static gchar*
tny_camel_header_dup_subject (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	gchar *retval = NULL;

	camel_folder_summary_lock ();
	retval = g_strdup (camel_message_info_subject (me->info));
	camel_folder_summary_unlock ();

	return retval;
}


static gchar*
tny_camel_header_dup_to (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	gchar *retval = NULL;

	camel_folder_summary_lock ();
	retval = g_strdup (camel_message_info_to (me->info));
	camel_folder_summary_unlock ();

	return retval;
}

static gchar*
tny_camel_header_dup_message_id (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	gchar *retval = NULL;

	camel_folder_summary_lock ();
	retval = (gchar*) camel_message_info_message_id (me->info);
	camel_folder_summary_unlock ();

	return retval;
}



static guint
tny_camel_header_get_message_size (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	guint retval;

	retval = (guint) camel_message_info_size (me->info);

	return retval;


}

static gchar*
tny_camel_header_dup_uid (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	gchar *retval = NULL;

	retval = g_strdup (camel_message_info_uid (me->info));

	return retval;
}

static void
tny_camel_header_finalize (GObject *object)
{
	TnyCamelHeader *self = (TnyCamelHeader*) object;

	if (self->info)
		camel_message_info_free (self->info);

	if (self->folder) {
		TnyCamelFolderPriv *fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (self->folder);
		_tny_camel_folder_unreason (fpriv);
		g_object_unref (self->folder);
	}

	(*parent_class->finalize) (object);

	return;
}

TnyHeader*
_tny_camel_header_new (void)
{
	TnyCamelHeader *self = g_object_new (TNY_TYPE_CAMEL_HEADER, NULL);

	self->info = NULL;
	self->folder = NULL;

	return (TnyHeader*) self;
}

void
_tny_camel_header_set_folder (TnyCamelHeader *self, TnyCamelFolder *folder, TnyCamelFolderPriv *fpriv)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);
	_tny_camel_folder_reason (fpriv);

	if (me->folder)
		g_object_unref (me->folder);
	me->folder = (TnyCamelFolder*) g_object_ref (folder);

	return;
}

static TnyFolder*
tny_camel_header_get_folder (TnyHeader *self)
{
	TnyCamelHeader *me = TNY_CAMEL_HEADER (self);

	if (me->folder)
		g_object_ref (G_OBJECT (me->folder));

	return (TnyFolder*) me->folder;
}

static void
tny_header_init (gpointer g, gpointer iface_data)
{
	TnyHeaderIface *klass = (TnyHeaderIface *)g;

	klass->dup_from= tny_camel_header_dup_from;
	klass->dup_message_id= tny_camel_header_dup_message_id;
	klass->get_message_size= tny_camel_header_get_message_size;
	klass->dup_to= tny_camel_header_dup_to;
	klass->dup_subject= tny_camel_header_dup_subject;
	klass->get_date_received= tny_camel_header_get_date_received;
	klass->get_date_sent= tny_camel_header_get_date_sent;
	klass->dup_cc= tny_camel_header_dup_cc;
	klass->dup_bcc= tny_camel_header_dup_bcc;
	klass->dup_replyto= tny_camel_header_dup_replyto;
	klass->dup_uid= tny_camel_header_dup_uid;
	klass->get_folder= tny_camel_header_get_folder;
	klass->set_bcc= tny_camel_header_set_bcc;
	klass->set_cc= tny_camel_header_set_cc;
	klass->set_to= tny_camel_header_set_to;
	klass->set_from= tny_camel_header_set_from;
	klass->set_subject= tny_camel_header_set_subject;
	klass->set_replyto= tny_camel_header_set_replyto;
	klass->set_flag= tny_camel_header_set_flag;
	klass->unset_flag= tny_camel_header_unset_flag;
	klass->get_flags= tny_camel_header_get_flags;
	klass->set_user_flag = tny_camel_header_set_user_flag;
	klass->get_user_flag = tny_camel_header_get_user_flag;
	klass->unset_user_flag = tny_camel_header_unset_user_flag;
	klass->support_user_flags = tny_camel_header_support_user_flags;

	return;
}


static void 
tny_camel_header_class_init (TnyCamelHeaderClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_camel_header_finalize;

	return;
}

static gpointer 
tny_camel_header_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelHeaderClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_header_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelHeader),
			0,      /* n_preallocs */
			NULL,    /* instance_init */
			NULL
		};
	
	static const GInterfaceInfo tny_header_info = 
		{
			(GInterfaceInitFunc) tny_header_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelHeader",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_HEADER, 
				     &tny_header_info);

	return GSIZE_TO_POINTER (type);
}

/**
 * tny_camel_header_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_header_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_header_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
