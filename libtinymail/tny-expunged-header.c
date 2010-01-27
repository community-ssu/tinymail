/* libtinymailui-gtk - The Tiny Mail UI library for Gtk+
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

#include <glib.h>

#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

#include <tny-header.h>

static GObjectClass *parent_class = NULL;

static gchar*
tny_expunged_header_dup_uid (TnyHeader *self)
{
	return g_strdup ("...");
}

static gchar*
tny_expunged_header_dup_bcc (TnyHeader *self)
{
	return g_strdup ("Expunged");
}

static gchar*
tny_expunged_header_dup_cc (TnyHeader *self)
{
	return g_strdup ("Expunged");
}

static gchar*
tny_expunged_header_dup_subject (TnyHeader *self)
{
	return g_strdup ("Expunged");
}

static gchar*
tny_expunged_header_dup_to (TnyHeader *self)
{
	return g_strdup ("Expunged");
}

static gchar*
tny_expunged_header_dup_from (TnyHeader *self)
{
	return g_strdup ("Expunged");
}

static gchar*
tny_expunged_header_dup_replyto (TnyHeader *self)
{
	return g_strdup ("Expunged");
}

static gchar*
tny_expunged_header_dup_message_id (TnyHeader *self)
{
	return g_strdup ("Expunged");
}

static guint
tny_expunged_header_get_message_size (TnyHeader *self)
{
	return -1;
}

static time_t
tny_expunged_header_get_date_received (TnyHeader *self)
{
	return -1;
}

static time_t
tny_expunged_header_get_date_sent (TnyHeader *self)
{
	return -1;
}

static void
tny_expunged_header_set_bcc (TnyHeader *self, const gchar *bcc)
{
}

static void
tny_expunged_header_set_cc (TnyHeader *self, const gchar *cc)
{
}

static void
tny_expunged_header_set_from (TnyHeader *self, const gchar *from)
{
}

static void
tny_expunged_header_set_subject (TnyHeader *self, const gchar *subject)
{
}

static void
tny_expunged_header_set_to (TnyHeader *self, const gchar *to)
{
}

static void
tny_expunged_header_set_replyto (TnyHeader *self, const gchar *to)
{
}

static TnyFolder*
tny_expunged_header_get_folder (TnyHeader *self)
{
	return NULL;
}

static TnyHeaderFlags
tny_expunged_header_get_flags (TnyHeader *self)
{
	return TNY_HEADER_FLAG_EXPUNGED;
}

static void
tny_expunged_header_set_flag (TnyHeader *self, TnyHeaderFlags mask)
{
}

static void
tny_expunged_header_unset_flag (TnyHeader *self, TnyHeaderFlags mask)
{
}

static gboolean
tny_expunged_header_get_user_flag (TnyHeader *self, const gchar *id)
{
	return FALSE;
}

static void
tny_expunged_header_set_user_flag (TnyHeader *self, const gchar *id)
{
}

static void
tny_expunged_header_unset_user_flag (TnyHeader *self, const gchar *id)
{
}

static TnyHeaderSupportFlags
tny_expunged_header_support_user_flags (TnyHeader *self)
{
	return TNY_HEADER_SUPPORT_FLAGS_NONE;
}

static void
tny_expunged_header_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
tny_expunged_header_instance_init (GTypeInstance *instance, gpointer g_class)
{
}

static void
tny_header_init (TnyHeaderIface *klass)
{
	klass->dup_uid= tny_expunged_header_dup_uid;
	klass->dup_bcc= tny_expunged_header_dup_bcc;
	klass->dup_cc= tny_expunged_header_dup_cc;
	klass->dup_subject= tny_expunged_header_dup_subject;
	klass->dup_to= tny_expunged_header_dup_to;
	klass->dup_from= tny_expunged_header_dup_from;
	klass->dup_replyto= tny_expunged_header_dup_replyto;
	klass->dup_message_id= tny_expunged_header_dup_message_id;
	klass->get_message_size= tny_expunged_header_get_message_size;
	klass->get_date_received= tny_expunged_header_get_date_received;
	klass->get_date_sent= tny_expunged_header_get_date_sent;
	klass->set_bcc= tny_expunged_header_set_bcc;
	klass->set_cc= tny_expunged_header_set_cc;
	klass->set_from= tny_expunged_header_set_from;
	klass->set_subject= tny_expunged_header_set_subject;
	klass->set_to= tny_expunged_header_set_to;
	klass->set_replyto= tny_expunged_header_set_replyto;
	klass->get_folder= tny_expunged_header_get_folder;
	klass->get_flags= tny_expunged_header_get_flags;
	klass->set_flag= tny_expunged_header_set_flag;
	klass->unset_flag= tny_expunged_header_unset_flag;
	klass->get_user_flag= tny_expunged_header_get_user_flag;
	klass->set_user_flag= tny_expunged_header_set_user_flag;
	klass->unset_user_flag= tny_expunged_header_unset_user_flag;
	klass->support_user_flags= tny_expunged_header_support_user_flags;
}

static void
tny_expunged_header_class_init (TnyExpungedHeaderClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = tny_expunged_header_finalize;
}

TnyHeader*
tny_expunged_header_new (void)
{
	return (TnyHeader *) g_object_new (TNY_TYPE_EXPUNGED_HEADER, NULL);
}

static gpointer
tny_expunged_header_register_type (gpointer notused)
{
	GType type = 0;
	static const GTypeInfo info = 
		{
			sizeof (TnyExpungedHeaderClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_expunged_header_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyExpungedHeader),
			0,      /* n_preallocs */
			tny_expunged_header_instance_init,    /* instance_init */
			NULL
		};
	

	static const GInterfaceInfo tny_header_info = 
		{
			(GInterfaceInitFunc) tny_header_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyExpungedHeader",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_HEADER,
				     &tny_header_info);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_expunged_header_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_expunged_header_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
