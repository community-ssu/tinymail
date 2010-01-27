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

#include <ctype.h>
#include <glib.h>
#include <string.h>

#include <tny-header.h>
#include <tny-camel-folder.h>

#include "tny-camel-common-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-msg-header-priv.h"

#include <libedataserver/e-iconv.h>

#include <tny-camel-shared.h>

#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>
#include <camel/camel-stream-null.h>

static GObjectClass *parent_class = NULL;



static gchar*
tny_camel_msg_header_dup_replyto (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	const gchar *enc;

	if (!me->reply_to) {
		enc = camel_medium_get_header (CAMEL_MEDIUM (me->msg), "reply-to");
		me->reply_to = _tny_camel_decode_raw_header ((CamelMimePart *) me->msg, enc, TRUE);
	}

	return g_strdup ((const gchar *) me->reply_to);
}


static void
tny_camel_msg_header_set_bcc (TnyHeader *self, const gchar *bcc)
{
	TnyCamelMsgHeader *me;
	CamelInternetAddress *addr;

	me = TNY_CAMEL_MSG_HEADER (self);
	addr = camel_internet_address_new ();

	_foreach_email_add_to_inet_addr (bcc, addr);

	camel_mime_message_set_recipients (me->msg, 
		CAMEL_RECIPIENT_TYPE_BCC, addr);

	camel_object_unref (CAMEL_OBJECT (addr));

	return;
}

static void
tny_camel_msg_header_set_cc (TnyHeader *self, const gchar *cc)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);    
	CamelInternetAddress *addr = camel_internet_address_new ();

	_foreach_email_add_to_inet_addr (cc, addr);

	camel_mime_message_set_recipients (me->msg, 
		CAMEL_RECIPIENT_TYPE_CC, addr);

	camel_object_unref (CAMEL_OBJECT (addr));

	return;
}

static void
tny_camel_msg_header_set_from (TnyHeader *self, const gchar *from)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);    
	CamelInternetAddress *addr = camel_internet_address_new ();
	gchar *dup;

	dup = g_strdup (from);
	_string_to_camel_inet_addr (dup, addr);
	g_free (dup);

	camel_mime_message_set_from (me->msg, addr);
	camel_object_unref (CAMEL_OBJECT (addr));

	return;
}

static void
tny_camel_msg_header_set_subject (TnyHeader *self, const gchar *subject)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);    

	camel_mime_message_set_subject (me->msg, subject);

	return;
}

static void
tny_camel_msg_header_set_to (TnyHeader *self, const gchar *to)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);    
	CamelInternetAddress *addr = camel_internet_address_new ();
	gchar *dup;

	dup = g_strdup (to);
	_foreach_email_add_to_inet_addr (dup, addr);
	g_free (dup);

	camel_mime_message_set_recipients (me->msg, 
		CAMEL_RECIPIENT_TYPE_TO, addr);

	camel_object_unref (CAMEL_OBJECT (addr));

	return;
}


static void
tny_camel_msg_header_set_replyto (TnyHeader *self, const gchar *replyto)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);    
	CamelInternetAddress *addr = camel_internet_address_new ();
	gchar *dup;

	dup = g_strdup (replyto);
	_foreach_email_add_to_inet_addr (dup, addr);
	g_free (dup);

	camel_mime_message_set_reply_to (me->msg, addr);

	camel_object_unref (CAMEL_OBJECT (addr));

	return;
}


static gchar*
tny_camel_msg_header_dup_cc (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	const gchar *enc;

	if (!me->cc) {
		enc = camel_medium_get_header (CAMEL_MEDIUM (me->msg), "cc");
		me->cc = _tny_camel_decode_raw_header ((CamelMimePart *) me->msg, enc, TRUE);
	}

	return g_strdup ((const gchar *) me->cc);
}

static gchar*
tny_camel_msg_header_dup_bcc (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	const gchar *enc;

	if (!me->bcc) {
		enc = camel_medium_get_header (CAMEL_MEDIUM (me->msg), "bcc");
		me->bcc = _tny_camel_decode_raw_header ((CamelMimePart *) me->msg, enc, TRUE);
	}

	return g_strdup ((const gchar *) me->bcc);
}

static TnyHeaderFlags
tny_camel_msg_header_get_flags (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	const gchar *priority_string = NULL;
	const gchar *attachments_string = NULL;
	TnyHeaderFlags result = 0;
	TnyHeaderFlags decorated_flags;

	result |= TNY_HEADER_FLAG_CACHED;

	priority_string = camel_medium_get_header (CAMEL_MEDIUM (me->msg), "X-Priority");
	attachments_string = camel_medium_get_header (CAMEL_MEDIUM (me->msg), "X-MS-Has-Attach");
	if (priority_string != NULL) {
		if (g_strrstr (priority_string, "1") != NULL ||
		    g_strrstr (priority_string, "2") != NULL)
			result |= TNY_HEADER_FLAG_HIGH_PRIORITY;
		else if (g_strrstr (priority_string, "4") != NULL ||
			 g_strrstr (priority_string, "5") != NULL)
			result |= TNY_HEADER_FLAG_LOW_PRIORITY;
		else 
			result |= TNY_HEADER_FLAG_NORMAL_PRIORITY;
	}

	if (attachments_string != NULL)
		result |= TNY_HEADER_FLAG_ATTACHMENTS;

	if (me->partial)
		result |= TNY_HEADER_FLAG_PARTIAL;

	if (me->decorated) {
		decorated_flags = tny_header_get_flags (me->decorated);
		result |= decorated_flags;
	}

	return result;
}

static void
set_prio_mask (TnyCamelMsgHeader *me, TnyHeaderFlags mask)
{
	TnyHeaderFlags prio_mask = mask;
	prio_mask &= TNY_HEADER_FLAG_PRIORITY_MASK;

	camel_medium_remove_header (CAMEL_MEDIUM (me->msg), "X-MSMail-Priority");
	camel_medium_remove_header (CAMEL_MEDIUM (me->msg), "X-Priority");

	if (prio_mask == TNY_HEADER_FLAG_HIGH_PRIORITY) {
		camel_medium_add_header (CAMEL_MEDIUM (me->msg), "X-MSMail-Priority", "High");
		camel_medium_add_header (CAMEL_MEDIUM (me->msg), "X-Priority", "1");
	} else if (prio_mask == TNY_HEADER_FLAG_LOW_PRIORITY) {
		camel_medium_add_header (CAMEL_MEDIUM (me->msg), "X-MSMail-Priority", "Low");
		camel_medium_add_header (CAMEL_MEDIUM (me->msg), "X-Priority", "5");
	} else if (prio_mask == TNY_HEADER_FLAG_NORMAL_PRIORITY) {
		camel_medium_add_header (CAMEL_MEDIUM (me->msg), "X-MSMail-Priority", "Normal");
		camel_medium_add_header (CAMEL_MEDIUM (me->msg), "X-Priority", "3");
	}

	return;
}

static void
tny_camel_msg_header_set_flag (TnyHeader *self, TnyHeaderFlags mask)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);

	if (me->decorated) {
		tny_header_set_flag (me->decorated, mask);
	}

	if (mask & TNY_HEADER_FLAG_CACHED || mask & TNY_HEADER_FLAG_PARTIAL) {
		if (mask & TNY_HEADER_FLAG_PARTIAL)
			me->partial = TRUE;
		else
			me->partial = FALSE;
	}

	/* Set priority only if no other flags are found.
	   Normal priority is 00 so there's no other way to detect it */
	if (!(mask & ~TNY_HEADER_FLAG_PRIORITY_MASK)) {
		set_prio_mask (me, mask);
	}

	if (mask & TNY_HEADER_FLAG_ATTACHMENTS) {
		camel_medium_remove_header (CAMEL_MEDIUM (me->msg), "X-MS-Has-Attach");
		camel_medium_add_header (CAMEL_MEDIUM (me->msg), "X-MS-Has-Attach", "Yes");
	}

	return;
}

static void
tny_camel_msg_header_unset_flag (TnyHeader *self, TnyHeaderFlags mask)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);

	if (me->decorated) {
		tny_header_set_flag (me->decorated, mask);
	}

	if (mask & TNY_HEADER_FLAG_ATTACHMENTS)
		camel_medium_remove_header (CAMEL_MEDIUM (me->msg), "X-MS-Has-Attach");

	if (mask & TNY_HEADER_FLAG_PARTIAL)
		me->partial = FALSE;

	return;
}

static void
tny_camel_msg_header_set_user_flag (TnyHeader *self, const gchar *id)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);

	if (me->decorated) {
		tny_header_set_user_flag (me->decorated, id);
	}

	return;
}

static void
tny_camel_msg_header_unset_user_flag (TnyHeader *self, const gchar *id)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);

	if (me->decorated) {
		tny_header_unset_user_flag (me->decorated, id);
	}

	return;
}

static gboolean
tny_camel_msg_header_get_user_flag (TnyHeader *self, const gchar *id)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);

	if (me->decorated) {
		return tny_header_get_user_flag (me->decorated, id);
	} else {
		return FALSE;
	}
}

static TnyHeaderSupportFlags
tny_camel_msg_header_support_user_flags (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);

	if (me->decorated) {
		return tny_header_support_user_flags (me->decorated);
	} else {
		return TNY_HEADER_SUPPORT_FLAGS_NONE;
	}
}

static time_t
tny_camel_msg_header_get_date_received (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	time_t retval = 0;
	int tzone;

	if (me->has_received)
		return me->received;

	retval = camel_mime_message_get_date_received (me->msg, &tzone);
	if (retval == CAMEL_MESSAGE_DATE_CURRENT)
		retval = camel_mime_message_get_date (me->msg, &tzone);

	/* return 0 if we really cannot find a date */
	if (retval == CAMEL_MESSAGE_DATE_CURRENT)
		return 0;

	return retval;
}

static time_t
tny_camel_msg_header_get_date_sent (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	time_t retval = 0;
	int tzone;

	/* first try _date, if that doesn't work, use received instead
	 * however, that is set by the *receiver* so will not be totally
	 * accurate
	 *
	 * NOTE: we ignore the timezone, as the camel function already
	 * returns a UTC-normalized time_t
	 * */
	retval = camel_mime_message_get_date (me->msg, &tzone);

	if (retval == CAMEL_MESSAGE_DATE_CURRENT)
		retval = camel_mime_message_get_date_received (me->msg, &tzone);
	if (retval == CAMEL_MESSAGE_DATE_CURRENT)
		return 0;
	else
		return retval;
}

static gchar*
tny_camel_msg_header_dup_from (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	const gchar *enc;

	if (!me->from) {
		enc = camel_medium_get_header (CAMEL_MEDIUM (me->msg), "from");
		me->from = _tny_camel_decode_raw_header ((CamelMimePart *) me->msg, enc, TRUE);
	}

	return g_strdup ((const gchar *) me->from);
}

static gchar*
tny_camel_msg_header_dup_subject (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	const gchar *enc;

	if (!me->subject) {
		enc = camel_medium_get_header (CAMEL_MEDIUM (me->msg), "subject");
		me->subject = _tny_camel_decode_raw_header ((CamelMimePart *) me->msg, enc, FALSE);
	}

	return g_strdup ((const gchar *) me->subject);
}


static gchar*
tny_camel_msg_header_dup_to (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	const gchar *enc;

	if (!me->to) {
		enc = camel_medium_get_header (CAMEL_MEDIUM (me->msg), "to");
		me->to = _tny_camel_decode_raw_header ((CamelMimePart *) me->msg, enc, TRUE);
	}

	return g_strdup ((const gchar *) me->to);

}

static gchar*
tny_camel_msg_header_dup_message_id (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	gchar *retval = NULL;

	retval = g_strdup (camel_mime_message_get_message_id (me->msg));

	return retval;
}



static guint
tny_camel_msg_header_get_message_size (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);
	guint retval;
	CamelStreamNull *sn = (CamelStreamNull *)camel_stream_null_new();

	camel_data_wrapper_write_to_stream((CamelDataWrapper *)me->msg, (CamelStream *)sn);

	retval = (guint) sn->written;
	camel_object_unref((CamelObject *)sn);

	return retval;
}

static gchar*
tny_camel_msg_header_dup_uid (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);

	if (!me->old_uid)
	{
		g_warning ("tny_header_dup_uid: This is a header instance for a new message. "
			"The uid of it is therefore not available. This indicates a problem "
			"in the software.");
	}

	return g_strdup (me->old_uid);
}

static void 
notify_decorated_del (gpointer user_data, GObject *decorated)
{
	TnyCamelMsgHeader *me = (TnyCamelMsgHeader *) user_data;
	me->decorated = NULL;
	me->decorated_has_ref = FALSE;
}

static void
tny_camel_msg_header_finalize (GObject *object)
{
	TnyCamelMsgHeader *me = (TnyCamelMsgHeader *) object;

	if (me->bcc)
		g_free (me->bcc);
	if (me->cc)
		g_free (me->cc);
	if (me->from)
		g_free (me->from);
	if (me->to)
		g_free (me->to);
	if (me->subject)
		g_free (me->subject);
	if (me->reply_to)
		g_free (me->reply_to);

	if (me->old_uid)
		g_free (me->old_uid);

	if (me->decorated) {
		if (me->decorated_has_ref)
			g_object_unref (me->decorated);
		else
			g_object_weak_unref (G_OBJECT (me->decorated), notify_decorated_del, me);
		me->decorated = NULL;
	}


	(*parent_class->finalize) (object);

	return;
}

static TnyFolder*
tny_camel_msg_header_get_folder (TnyHeader *self)
{
	TnyCamelMsgHeader *me = TNY_CAMEL_MSG_HEADER (self);

	if (me->folder)
		g_object_ref (G_OBJECT (me->folder));

	return (TnyFolder*) me->folder;
}

TnyHeader*
_tny_camel_msg_header_new (CamelMimeMessage *msg, TnyFolder *folder, time_t received)
{
	TnyCamelMsgHeader *self = g_object_new (TNY_TYPE_CAMEL_MSG_HEADER, NULL);

	/*  For this implementation of TnyHeader: self dies when the TnyMsg dies who
		owns this msg. If this ever changes then we need to add a reference here, 
		and remove it in the finalize. Same for folder. */

	if (received == -1)
		self->has_received = FALSE;
	self->received = received;
	self->old_uid = NULL;
	self->msg = msg; 
	self->folder = folder;
	self->has_received = FALSE;
	self->partial = FALSE;
	self->decorated = NULL;
	self->decorated_has_ref = FALSE;
	self->to = NULL;
	self->from = NULL;
	self->cc = NULL;
	self->bcc = NULL;
	self->subject = NULL;
	self->reply_to = NULL;

	return (TnyHeader*) self;
}


void 
_tny_camel_msg_header_set_decorated (TnyCamelMsgHeader *header, 
				     TnyHeader *decorated,
				     gboolean add_reference)
{
	g_assert (TNY_IS_HEADER (decorated));
	if (header->decorated) {
		if (header->decorated_has_ref)
			g_object_unref (header->decorated);
		else
			g_object_weak_unref (G_OBJECT (header->decorated), notify_decorated_del, header);
	}
	header->decorated_has_ref = add_reference;

	if (add_reference)
		g_object_ref (decorated);
	else
		g_object_weak_ref (G_OBJECT (decorated), notify_decorated_del, header);

	header->decorated = decorated;

}


static void
tny_header_init (gpointer g, gpointer iface_data)
{
	TnyHeaderIface *klass = (TnyHeaderIface *)g;

	klass->dup_from= tny_camel_msg_header_dup_from;
	klass->dup_message_id= tny_camel_msg_header_dup_message_id;
	klass->get_message_size= tny_camel_msg_header_get_message_size;
	klass->dup_to= tny_camel_msg_header_dup_to;
	klass->dup_subject= tny_camel_msg_header_dup_subject;
	klass->get_date_received= tny_camel_msg_header_get_date_received;
	klass->get_date_sent= tny_camel_msg_header_get_date_sent;
	klass->dup_cc= tny_camel_msg_header_dup_cc;
	klass->dup_bcc= tny_camel_msg_header_dup_bcc;
	klass->dup_replyto= tny_camel_msg_header_dup_replyto;
	klass->dup_uid= tny_camel_msg_header_dup_uid;
	klass->get_folder= tny_camel_msg_header_get_folder;
	klass->set_bcc= tny_camel_msg_header_set_bcc;
	klass->set_cc= tny_camel_msg_header_set_cc;
	klass->set_to= tny_camel_msg_header_set_to;
	klass->set_from= tny_camel_msg_header_set_from;
	klass->set_subject= tny_camel_msg_header_set_subject;
	klass->set_replyto= tny_camel_msg_header_set_replyto;
	klass->set_flag= tny_camel_msg_header_set_flag;
	klass->unset_flag= tny_camel_msg_header_unset_flag;
	klass->get_flags= tny_camel_msg_header_get_flags;
	klass->set_user_flag= tny_camel_msg_header_set_user_flag;
	klass->unset_user_flag= tny_camel_msg_header_unset_user_flag;
	klass->get_user_flag= tny_camel_msg_header_get_user_flag;
	klass->support_user_flags= tny_camel_msg_header_support_user_flags;

	return;
}


static void 
tny_camel_msg_header_class_init (TnyCamelMsgHeaderClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_camel_msg_header_finalize;

	return;
}


static gpointer 
tny_camel_msg_header_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelMsgHeaderClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_msg_header_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelMsgHeader),
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
				       "TnyCamelMsgHeader",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_HEADER, 
				     &tny_header_info);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_camel_msg_header_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_msg_header_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_msg_header_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
