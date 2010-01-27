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
 * TnyMsg:
 *
 * A special kind of #TnyMimePart that has a header
 *
 * free-function: g_object_free
 **/

#include <config.h>

#ifdef DBC
#include <string.h>
#endif

#include <tny-msg.h>
#include <tny-header.h>
#include <tny-folder.h>


/**
 * tny_msg_uncache_attachments:
 * @self: a #TnyMsg
 * 
 * API WARNING: This API might change
 * 
 * Uncache the attachments of @self.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_msg_uncache_attachments (TnyMsg *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MSG (self));
	g_assert (TNY_MSG_GET_IFACE (self)->uncache_attachments!= NULL);
#endif

	TNY_MSG_GET_IFACE (self)->uncache_attachments(self);

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_msg_rewrite_cache:
 * @self: a #TnyMsg
 * 
 * API WARNING: This API might change
 * 
 * Rewrite the message to cache, purging mime parts marked for purge.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_msg_rewrite_cache (TnyMsg *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MSG (self));
	g_assert (TNY_MSG_GET_IFACE (self)->rewrite_cache!= NULL);
#endif

	TNY_MSG_GET_IFACE (self)->rewrite_cache(self);

#ifdef DBC /* ensure */
#endif

	return;
}

/**
 * tny_msg_set_allow_external_images:
 * @self: a #TnyMsg
 * @allow: a #gboolean
 * 
 * API WARNING: This API might change
 * 
 * Set if views should fetch external images referenced
 * in this message or not.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_msg_set_allow_external_images (TnyMsg *self, gboolean allow)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MSG (self));
	g_assert (TNY_MSG_GET_IFACE (self)->get_allow_external_images != NULL);
#endif

	TNY_MSG_GET_IFACE (self)->set_allow_external_images (self, allow);

#ifdef DBC /* ensure */
#endif

	return;
}


/**
 * tny_msg_get_allow_external_images:
 * @self: a #TnyMsg
 * 
 * API WARNING: This API might change
 * 
 * Get if views should fetch external images into message.
 *
 * Returns: a #gboolean
 *
 * since: 1.0
 * audience: application-developer
 **/
gboolean
tny_msg_get_allow_external_images (TnyMsg *self)
{
	gboolean retval;
#ifdef DBC /* require */
	g_assert (TNY_IS_MSG (self));
	g_assert (TNY_MSG_GET_IFACE (self)->get_allow_external_images != NULL);
#endif

	retval = TNY_MSG_GET_IFACE (self)->get_allow_external_images (self);

#ifdef DBC /* ensure */
#endif

	return retval;
}

/**
 * tny_msg_get_url_string:
 * @self: a #TnyMsg
 * 
 * Get the url string for @self or NULL if it's impossible to determine the url 
 * string of @self. If not NULL, the returned value must be freed after use.
 *
 * The url string is specified in RFC 1808 and looks for example like this:
 * imap://user@hostname/INBOX/folder/000 where 000 is the UID of the message on
 * the IMAP server. Note that it doesn't necessarily contain the password of the
 * IMAP account.
 * 
 * returns: (null-ok): The url string or NULL.
 * since: 1.0
 * audience: application-developer
 **/
gchar* 
tny_msg_get_url_string (TnyMsg *self)
{
	gchar *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MSG (self));
	g_assert (TNY_MSG_GET_IFACE (self)->get_url_string!= NULL);
#endif

	retval = TNY_MSG_GET_IFACE (self)->get_url_string(self);

#ifdef DBC /* ensure */
	if (retval) {
		g_assert (strlen (retval) > 0);
		/*g_assert (strstr (retval, "://") != NULL);*/
	}
#endif

	return retval;
}

/**
 * tny_msg_get_folder:
 * @self: a #TnyMsg
 * 
 * Get the parent folder of @self or NULL if @self is not contained in a folder.
 * If not NULL, the returned value must be unreferenced after use.
 *
 * returns: (null-ok) (caller-owns): The parent folder of this message or NULL
 * since: 1.0
 * audience: application-developer
 **/
TnyFolder* 
tny_msg_get_folder (TnyMsg *self)
{
	TnyFolder *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MSG (self));
	g_assert (TNY_MSG_GET_IFACE (self)->get_folder!= NULL);
#endif

	retval = TNY_MSG_GET_IFACE (self)->get_folder(self);

#ifdef DBC /* ensure */
	if (retval)
		g_assert (TNY_IS_FOLDER (retval));
#endif

	return retval;
}




/**
 * tny_msg_get_header:
 * @self: a #TnyMsg
 * 
 * Get the header of @self. The returned header object must be unreferenced 
 * after use. You can't use the returned instance with #TnyFolder operations
 * like tny_folder_transfer_msgs() and tny_folder_transfer_msgs_async()!
 *
 * If @self is a writable message, you can write to the returned #TnyHeader
 * too.
 *
 * returns: (caller-owns): header of the message
 * since: 1.0
 * audience: application-developer
 **/
TnyHeader*
tny_msg_get_header (TnyMsg *self)
{
	TnyHeader *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MSG (self));
	g_assert (TNY_MSG_GET_IFACE (self)->get_header!= NULL);
#endif

	retval = TNY_MSG_GET_IFACE (self)->get_header(self);


#ifdef DBC /* ensure */
	g_assert (TNY_IS_HEADER (retval));
#endif

	return retval;
}


static void
tny_msg_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_msg_register_type (gpointer notused)
{
	GType type = 0;
	
	static const GTypeInfo info = 
		{
			sizeof (TnyMsgIface),
			tny_msg_base_init,   /* base_init */
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
				       "TnyMsg", &info, 0);
	
	g_type_interface_add_prerequisite (type, TNY_TYPE_MIME_PART); 
	g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_msg_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_msg_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
