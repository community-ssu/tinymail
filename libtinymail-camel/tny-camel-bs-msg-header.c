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
#include <ctype.h>

#include <tny-header.h>

#include "tny-camel-common-priv.h"
#include "tny-camel-bs-msg-header-priv.h"

#include <tny-camel-shared.h>

#include <camel/camel-mime-utils.h>
#include <libedataserver/e-iconv.h>

static GObjectClass *parent_class = NULL;

char *
_tny_camel_bs_decode_raw_header (TnyCamelBsMsgHeader *me, const char *str)
{
	const gchar *charset;

	if (!str)
		return NULL;

	charset = me->charset;
	
	if (charset && (g_ascii_strcasecmp(charset, "us-ascii") == 0))
		charset = NULL;

	charset = charset ? e_iconv_charset_name (charset) : NULL;

	while (isspace ((unsigned) *str))
		str++;

	return camel_header_decode_string (str, charset);
}



static gchar*
tny_camel_bs_msg_header_dup_replyto (TnyHeader *self)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (self);
	return _tny_camel_bs_decode_raw_header (me, me->envelope->reply_to);
}


static void
tny_camel_bs_msg_header_set_bcc (TnyHeader *self, const gchar *bcc)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return;
}

static void
tny_camel_bs_msg_header_set_cc (TnyHeader *self, const gchar *cc)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return;
}

static void
tny_camel_bs_msg_header_set_from (TnyHeader *self, const gchar *from)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return;
}

static void
tny_camel_bs_msg_header_set_subject (TnyHeader *self, const gchar *subject)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return;
}

static void
tny_camel_bs_msg_header_set_to (TnyHeader *self, const gchar *to)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return;
}


static void
tny_camel_bs_msg_header_set_replyto (TnyHeader *self, const gchar *replyto)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return;
}


static gchar*
tny_camel_bs_msg_header_dup_cc (TnyHeader *self)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (self);
	return _tny_camel_bs_decode_raw_header (me, me->envelope->cc);
}

static gchar*
tny_camel_bs_msg_header_dup_bcc (TnyHeader *self)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (self);
	return _tny_camel_bs_decode_raw_header (me, me->envelope->bcc);
}

static TnyHeaderFlags
tny_camel_bs_msg_header_get_flags (TnyHeader *self)
{
	return TNY_HEADER_FLAG_CACHED;
}

static void
tny_camel_bs_msg_header_set_flag (TnyHeader *self, TnyHeaderFlags mask)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return;
}

static void
tny_camel_bs_msg_header_unset_flag (TnyHeader *self, TnyHeaderFlags mask)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return;
}

static gboolean
tny_camel_bs_msg_header_get_user_flag (TnyHeader *self, const gchar *id)
{
	return FALSE;
}

static void
tny_camel_bs_msg_header_set_user_flag (TnyHeader *self, const gchar *id)
{
	return;
}

static void
tny_camel_bs_msg_header_unset_user_flag (TnyHeader *self, const gchar *id)
{
	return;
}

static gboolean
tny_camel_bs_msg_header_support_user_flags (TnyHeader *self)
{
	return TNY_HEADER_SUPPORT_FLAGS_NONE;
}

static time_t
tny_camel_bs_msg_header_get_date_received (TnyHeader *self)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (self);
	return camel_header_decode_date (me->envelope->date, NULL);
}

static time_t
tny_camel_bs_msg_header_get_date_sent (TnyHeader *self)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (self);
	return camel_header_decode_date (me->envelope->date, NULL);
}

static gchar*
tny_camel_bs_msg_header_dup_from (TnyHeader *self)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (self);
	return _tny_camel_bs_decode_raw_header (me, me->envelope->from);
}

static gchar*
tny_camel_bs_msg_header_dup_subject (TnyHeader *self)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (self);
	return _tny_camel_bs_decode_raw_header (me, me->envelope->subject);
}


static gchar*
tny_camel_bs_msg_header_dup_to (TnyHeader *self)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (self);
	return _tny_camel_bs_decode_raw_header (me, me->envelope->to);
}

static gchar*
tny_camel_bs_msg_header_dup_message_id (TnyHeader *self)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (self);
	return g_strdup (me->envelope->message_id);
}



static guint
tny_camel_bs_msg_header_get_message_size (TnyHeader *self)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (self);
	return me->msg_size;
}

static gchar*
tny_camel_bs_msg_header_dup_uid (TnyHeader *self)
{
	g_warning ("tny_header_dup_uid: This is a header instance for a RFC822 message. "
		"The uid of it is therefore not available. This indicates a problem "
		"in the software.");

	return NULL;
}


static void
tny_camel_bs_msg_header_finalize (GObject *object)
{
	TnyCamelBsMsgHeader *me = TNY_CAMEL_BS_MSG_HEADER (object);

	g_free (me->charset);

	(*parent_class->finalize) (object);
	return;
}

static TnyFolder*
tny_camel_bs_msg_header_get_folder (TnyHeader *self)
{
	return NULL;
}

TnyHeader*
_tny_camel_bs_msg_header_new (envelope_t *envelope, gint msg_size,
			      const gchar *charset)
{
	TnyCamelBsMsgHeader *self = g_object_new (TNY_TYPE_CAMEL_BS_MSG_HEADER, NULL);

	self->envelope = envelope; 
	self->msg_size = msg_size;
	self->charset = g_strdup (charset);

	return (TnyHeader*) self;
}

static void
tny_header_init (gpointer g, gpointer iface_data)
{
	TnyHeaderIface *klass = (TnyHeaderIface *)g;

	klass->dup_from= tny_camel_bs_msg_header_dup_from;
	klass->dup_message_id= tny_camel_bs_msg_header_dup_message_id;
	klass->get_message_size= tny_camel_bs_msg_header_get_message_size;
	klass->dup_to= tny_camel_bs_msg_header_dup_to;
	klass->dup_subject= tny_camel_bs_msg_header_dup_subject;
	klass->get_date_received= tny_camel_bs_msg_header_get_date_received;
	klass->get_date_sent= tny_camel_bs_msg_header_get_date_sent;
	klass->dup_cc= tny_camel_bs_msg_header_dup_cc;
	klass->dup_bcc= tny_camel_bs_msg_header_dup_bcc;
	klass->dup_replyto= tny_camel_bs_msg_header_dup_replyto;
	klass->dup_uid= tny_camel_bs_msg_header_dup_uid;
	klass->get_folder= tny_camel_bs_msg_header_get_folder;
	klass->set_bcc= tny_camel_bs_msg_header_set_bcc;
	klass->set_cc= tny_camel_bs_msg_header_set_cc;
	klass->set_to= tny_camel_bs_msg_header_set_to;
	klass->set_from= tny_camel_bs_msg_header_set_from;
	klass->set_subject= tny_camel_bs_msg_header_set_subject;
	klass->set_replyto= tny_camel_bs_msg_header_set_replyto;
	klass->set_flag= tny_camel_bs_msg_header_set_flag;
	klass->unset_flag= tny_camel_bs_msg_header_unset_flag;
	klass->get_flags= tny_camel_bs_msg_header_get_flags;
	klass->set_user_flag= tny_camel_bs_msg_header_set_user_flag;
	klass->unset_user_flag= tny_camel_bs_msg_header_unset_user_flag;
	klass->get_user_flag= tny_camel_bs_msg_header_get_user_flag;
	klass->support_user_flags = tny_camel_bs_msg_header_support_user_flags;

	return;
}


static void 
tny_camel_bs_msg_header_class_init (TnyCamelBsMsgHeaderClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_camel_bs_msg_header_finalize;

	return;
}


static gpointer 
tny_camel_bs_msg_header_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelBsMsgHeaderClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_bs_msg_header_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelBsMsgHeader),
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
				       "TnyCamelBsMsgHeader",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_HEADER, 
				     &tny_header_info);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_camel_bs_msg_header_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_bs_msg_header_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_bs_msg_header_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
