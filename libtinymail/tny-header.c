/* libtinymail - The Tiny Mail base library
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

/**
 * TnyHeader: 
 *
 * A summary item, envelope or group of E-mail headers. Depending on how you
 * like to call it.
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#ifdef DBC
#include <string.h>
#endif

#include <tny-header.h>
#include <tny-folder.h>


/**
 * tny_header_set_replyto:
 * @self: a #TnyHeader
 * @to: (null-ok): the reply-to header or NULL
 * 
 * Set the reply-to header
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_header_set_replyto (TnyHeader *self, const gchar *to)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->set_replyto!= NULL);
#endif

	TNY_HEADER_GET_IFACE (self)->set_replyto(self, to);

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_header_dup_replyto:
 * @self: a #TnyHeader
 * 
 * Get the reply-to header
 * 
 * returns: (null-ok) (caller-owns): reply-to header or NULL
 * since: 1.0
 * audience: application-developer
 **/
gchar*
tny_header_dup_replyto (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->dup_replyto!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->dup_replyto(self);
}

/**
 * tny_header_set_bcc:
 * @self: a #TnyHeader
 * @bcc: (null-ok): the BCC header in a comma separated list or NULL
 * 
 * Set the BCC header. Look at the To header for more information
 * about formatting.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_header_set_bcc (TnyHeader *self, const gchar *bcc)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->set_bcc!= NULL);
#endif

	TNY_HEADER_GET_IFACE (self)->set_bcc(self, bcc);

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_header_set_cc:
 * @self: a #TnyHeader
 * @cc: (null-ok): the CC header in a comma separated list or NULL
 * 
 * Set the CC header. Look at the To header for more information
 * about formatting.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_header_set_cc (TnyHeader *self, const gchar *cc)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->set_cc!= NULL);
#endif

	TNY_HEADER_GET_IFACE (self)->set_cc(self, cc);

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_header_set_from:
 * @self: a #TnyHeader
 * @from: (null-ok): the from header or NULL
 * 
 * Set the from header
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_header_set_from (TnyHeader *self, const gchar *from)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->set_from!= NULL);
#endif

	TNY_HEADER_GET_IFACE (self)->set_from(self, from);

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_header_set_subject:
 * @self: a #TnyHeader
 * @subject: (null-ok): the subject header or NULL
 * 
 * Set the subject header
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_header_set_subject (TnyHeader *self, const gchar *subject)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->set_subject!= NULL);
#endif

	TNY_HEADER_GET_IFACE (self)->set_subject(self, subject);

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_header_set_to:
 * @self: a #TnyHeader
 * @to: (null-ok): the To header in a comma separated list or NULL
 * 
 * Set the To header.
 *
 * The format is a comma separated list like this:
 * Full name &gt;user@domain&lt;, Full name &gt;user@domain&lt;
 *
 * There are no quotes nor anything special. Just commas.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_header_set_to (TnyHeader *self, const gchar *to)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->set_to!= NULL);
#endif

	TNY_HEADER_GET_IFACE (self)->set_to(self, to);

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_header_dup_cc:
 * @self: a #TnyHeader
 * 
 * Get the CC header. If not NULL, the returned value must be freed.
 * 
 * returns: (null-ok) (caller-owns): CC header as a alloc'd string or NULL
 * since: 1.0
 * audience: application-developer
 **/
gchar*
tny_header_dup_cc (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->dup_cc!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->dup_cc(self);
}

/**
 * tny_header_dup_bcc:
 * @self: a #TnyHeader
 * 
 * Get the BCC header. If not NULL, the returned value must be freed.
 * 
 * returns: (null-ok) (caller-owns): BCC header as an alloc'd string or NULL
 * since: 1.0
 * audience: application-developer
 **/
gchar*
tny_header_dup_bcc (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->dup_bcc!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->dup_bcc(self);
}

/**
 * tny_header_get_date_received:
 * @self: a #TnyHeader
 * 
 * Get the Date Received header as a time_t
 * 
 * returns: date received header
 * since: 1.0
 * audience: application-developer
 **/
time_t
tny_header_get_date_received (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->get_date_received!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->get_date_received(self);
}

/**
 * tny_header_get_date_sent:
 * @self: a #TnyHeader
 * 
 * Get the Date Sent header as a time_t
 * 
 * returns: date sent header
 * since: 1.0
 * audience: application-developer
 **/
time_t
tny_header_get_date_sent (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->get_date_sent!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->get_date_sent(self);
}


/**
 * tny_header_dup_uid:
 * @self: a #TnyHeader
 * 
 * Get an unique id of the message of which @self is a message header. The UID
 * corresponds to the UID in IMAP and the UIDL in POP if UIDL is supported or 
 * the UID if not. If not NULL, the returned value must be freed.
 * 
 * returns: (null-ok) (caller-owns): unique follow-up uid as an alloc'd string or NULL
 * since: 1.0
 * audience: application-developer
 **/
gchar*
tny_header_dup_uid (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->dup_uid!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->dup_uid(self);
}

/**
 * tny_header_dup_message_id:
 * @self: a #TnyHeader
 * 
 * Get an unique id of the message of which self is a message header. The 
 * message-id corresponds to the message-id header in the MIME message.
 * If not NULL, the returned value must be freed.
 * 
 * returns: (null-ok) (caller-owns): message-id header as an alloc'd string or NULL
 * since: 1.0
 * audience: application-developer
 **/
gchar*
tny_header_dup_message_id (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->dup_message_id!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->dup_message_id(self);
}


/**
 * tny_header_get_message_size:
 * @self: a #TnyHeader
 * 
 * Get the expected message's size (the accuracy of the size prediction depends 
 * on the availability of an accurate total size of the message at the service).
 * 
 * returns: expected message size
 * since: 1.0
 * audience: application-developer
 **/
guint
tny_header_get_message_size (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->get_message_size!= NULL);
#endif


	return TNY_HEADER_GET_IFACE (self)->get_message_size(self);
}


/**
 * tny_header_dup_from:
 * @self: a #TnyHeader
 * 
 * Get the from header. If not NULL, the returned value must be freed.
 * 
 * returns: (null-ok) (caller-owns): from header as an alloc'd string or NULL
 * since: 1.0
 * audience: application-developer
 **/
gchar* 
tny_header_dup_from (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->dup_from!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->dup_from(self);
}

/**
 * tny_header_dup_subject:
 * @self: a #TnyHeader
 * 
 * Get the subject header. If not NULL, the returned value must be freed.
 * 
 * returns: (null-ok) (caller-owns): subject header as an alloc'd string or NULL
 * since: 1.0
 * audience: application-developer
 **/
gchar*
tny_header_dup_subject (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->dup_subject!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->dup_subject(self);
}


/**
 * tny_header_dup_to:
 * @self: a #TnyHeader
 * 
 * Get the to header. If not NULL, the returned value must be freed.
 * 
 * returns: (null-ok) (caller-owns): to header as an alloc'd string or NULL
 * since: 1.0
 * audience: application-developer
 **/
gchar* 
tny_header_dup_to (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->dup_to!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->dup_to(self);
}

/**
 * tny_header_get_folder:
 * @self: a #TnyHeader
 * 
 * Get the parent folder where this message header is located. This method can
 * return NULL in case the folder isn't know. The returned value, if not NULL,
 * must be unreferenced after use.
 * 
 * returns: (null-ok) (caller-owns): The folder of the message header or NULL
 * since: 1.0
 * audience: application-developer
 **/
TnyFolder* 
tny_header_get_folder (TnyHeader *self)
{
	TnyFolder *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->get_folder!= NULL);
#endif

	retval = TNY_HEADER_GET_IFACE (self)->get_folder(self);

#ifdef DBC /* ensure */
	if (retval)
		g_assert (TNY_IS_FOLDER (retval));
#endif

	return retval;
}



/**
 * tny_header_get_flags:
 * @self: a #TnyHeader object
 * 
 * Get message's information flags like the message's read state and whether
 * it's detected that it has attachments.
 * 
 * Note: don't use this method to get priority settings.
 * Use tny_header_get_priority in stead.
 *
 * returns: flags bitmask
 * since: 1.0
 * audience: application-developer
 **/
TnyHeaderFlags
tny_header_get_flags (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->get_flags!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->get_flags(self);
}

/**
 * tny_header_set_flag:
 * @self: a #TnyHeader
 * @mask: a #TnyHeaderFlags bitmask flag to set.
 * 
 * Modify message's flag @mask. Modifying the TNY_HEADER_FLAG_SEEN will trigger
 * the notification of folder observers if @self was originated from a folder.
 *
 * Do not use this method to set priority settings. Use tny_header_set_priority()
 * in stead. Don't set more than one flag. Use the labels of the #TnyHeaderFlags
 * as an enum.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_header_set_flag (TnyHeader *self, TnyHeaderFlags mask)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->set_flag!= NULL);
#endif

	TNY_HEADER_GET_IFACE (self)->set_flag(self, mask);

#ifdef DBC /* ensure */
	/* TNY TODO: A check that ensures that all bits in mask are set */
#endif

	return;
}

/**
 * tny_header_set_user_flag:
 * @self: a #TnyHeader
 * @name: name of the flag to set
 * 
 * Set as %TRUE the user flag with name @name.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_header_set_user_flag (TnyHeader *self, const gchar *name)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->set_user_flag!= NULL);
#endif

	TNY_HEADER_GET_IFACE (self)->set_user_flag(self, name);

	return;
}

/**
 * tny_header_unset_user_flag:
 * @self: a #TnyHeader
 * @name: name of the flag to unset
 * 
 * Set as %FALSE (not set) the user flag with name @name. This is
 * the same as removing the flag.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_header_unset_user_flag (TnyHeader *self, const gchar *name)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->unset_user_flag!= NULL);
#endif

	TNY_HEADER_GET_IFACE (self)->unset_user_flag(self, name);

	return;
}

/**
 * tny_header_get_user_flag:
 * @self: a #TnyHeader
 * @name: name of the flag
 * 
 * Get if a user flag with name @name is set.
 *
 * Returns: %TRUE if flag is set, %FALSE otherwise
 *
 * since: 1.0
 * audience: application-developer
 **/
gboolean 
tny_header_get_user_flag (TnyHeader *self, const gchar *name)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->get_user_flag!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->get_user_flag(self, name);
}

/**
 * tny_header_get_priority:
 * @self: a #TnyHeader
 * 
 * Get message's priority
 *
 * returns: priority of the message
 * since: 1.0
 * audience: application-developer
 **/
TnyHeaderFlags 
tny_header_get_priority (TnyHeader *self)
{
	TnyHeaderFlags flags = tny_header_get_flags (self);
	flags &= TNY_HEADER_FLAG_PRIORITY_MASK;
	return flags;
}

/**
 * tny_header_set_priority:
 * @self: a #TnyHeader
 * @priority: A priority setting (high, low or normal)
 * 
 * Set the priority of the message. To unset the priority, you can use 
 * TNY_HEADER_FLAG_NORMAL_PRIORITY as @priority. Don't combine @priority with
 * other flags. Use additional tny_header_set_flag() calls if you want to do 
 * that.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_header_set_priority (TnyHeader *self, TnyHeaderFlags priority)
{
	TnyHeaderFlags flags;
	g_return_if_fail  ((priority & TNY_HEADER_FLAG_PRIORITY_MASK) != priority);

	/* Is this necessary? */
	flags = tny_header_get_flags (self);
	flags &= ~TNY_HEADER_FLAG_PRIORITY_MASK;
	flags |= priority;
	/* -- */

	tny_header_set_flag (self, flags);
	return;
}

/**
 * tny_header_unset_flag:
 * @self: a #TnyHeader
 * @mask: a #TnyHeaderFlags flag to clear.
 * 
 * Reset message flag @mask. Modifying the TNY_HEADER_FLAG_SEEN will trigger the 
 * notification of folder observers if @self was originated from a folder.
 * 
 * Don't attempt to unset the priority. Use tny_header_set_priority() 
 * with TNY_HEADER_FLAG_NORMAL_PRIORITY to set the priority of a message to normal 
 * (which is the same as unsetting it). Don't unset multiple flags.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_header_unset_flag (TnyHeader *self, TnyHeaderFlags mask)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->unset_flag!= NULL);
#endif

	TNY_HEADER_GET_IFACE (self)->unset_flag(self, mask);

#ifdef DBC /* ensure */
	/* TNY TODO: A check that ensures that all bits in mask are unset */
#endif

	return;
}

/**
 * tny_header_support_user_flags:
 * @self: a #TnyHeader
 *
 * Tells if user flags are supported in this header. It depends mostly on the
 * provider implementation.
 *
 * since: 1.0
 * audience: application-developer
 **/
TnyHeaderSupportFlags
tny_header_support_user_flags (TnyHeader *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_HEADER (self));
	g_assert (TNY_HEADER_GET_IFACE (self)->support_user_flags!= NULL);
#endif

	return TNY_HEADER_GET_IFACE (self)->support_user_flags (self);
}

static void
tny_header_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_header_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyHeaderIface),
			tny_header_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,   /* instance_init */
			NULL
		};
	
	type = g_type_register_static (G_TYPE_INTERFACE,
				       "TnyHeader", &info, 0);
	g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_header_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_header_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}

static gpointer
tny_header_flags_register_type (gpointer notused)
{
	GType etype = 0;
	static const GFlagsValue values[] = {
		{ TNY_HEADER_FLAG_ANSWERED, "TNY_HEADER_FLAG_ANSWERED", "answered" },
		{ TNY_HEADER_FLAG_DELETED, "TNY_HEADER_FLAG_DELETED", "deleted" },
		{ TNY_HEADER_FLAG_DRAFT, "TNY_HEADER_FLAG_DRAFT", "draft" },
		{ TNY_HEADER_FLAG_FLAGGED, "TNY_HEADER_FLAG_FLAGGED", "flagged" },
		{ TNY_HEADER_FLAG_SEEN, "TNY_HEADER_FLAG_SEEN", "seen" },
		{ TNY_HEADER_FLAG_ATTACHMENTS, "TNY_HEADER_FLAG_ATTACHMENTS", "attachments" },
		{ TNY_HEADER_FLAG_CACHED, "TNY_HEADER_FLAG_CACHED", "cached" },
		{ TNY_HEADER_FLAG_PARTIAL, "TNY_HEADER_FLAG_PARTIAL", "partial" },
		{ TNY_HEADER_FLAG_EXPUNGED, "TNY_HEADER_FLAG_EXPUNGED", "expunged" },
		{ TNY_HEADER_FLAG_HIGH_PRIORITY, "TNY_HEADER_FLAG_HIGH_PRIORITY", "high-priority" },
		{ TNY_HEADER_FLAG_NORMAL_PRIORITY, "TNY_HEADER_FLAG_NORMAL_PRIORITY", "normal-priority" },
		{ TNY_HEADER_FLAG_LOW_PRIORITY, "TNY_HEADER_FLAG_LOW_PRIORITY", "low-priority" },
		{ TNY_HEADER_FLAG_SUSPENDED, "TNY_HEADER_FLAG_SUSPENDED", "suspended" },
		{ 0, NULL, NULL }
	};
	etype = g_flags_register_static ("TnyHeaderFlags", values);
	return GUINT_TO_POINTER (etype);
}

/**
 * tny_header_flags_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_header_flags_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_header_flags_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}

static gpointer
tny_header_support_flags_register_type (gpointer notused)
{
	GType etype = 0;
	static const GFlagsValue values[] = {
		{ TNY_HEADER_SUPPORT_FLAGS_NONE, "TNY_HEADER_SUPPORT_FLAGS_NONE", "none" },
		{ TNY_HEADER_SUPPORT_FLAGS_ANY, "TNY_HEADER_SUPPORT_FLAGS_ANY", "any" },
		{ TNY_HEADER_SUPPORT_FLAGS_SOME, "TNY_HEADER_SUPPORT_FLAGS_SOME", "some" },
		{ TNY_HEADER_SUPPORT_PERSISTENT_FLAGS_ANY, "TNY_HEADER_SUPPORT_PERSISTENT_FLAGS_ANY", "persistent_any" },
		{ TNY_HEADER_SUPPORT_PERSISTENT_FLAGS_SOME, "TNY_HEADER_SUPPORT_PERSISTENT_FLAGS_SOME", "persistent_some" },
		{ 0, NULL, NULL }
	};
	etype = g_flags_register_static ("TnyHeaderSupportFlags", values);
	return GUINT_TO_POINTER (etype);
}

/**
 * tny_header_support_flags_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_header_support_flags_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_header_support_flags_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
