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
#include <string.h>
#include <ctype.h>

#include <tny-camel-account.h>
#include <tny-session-camel.h>

#include "tny-session-camel-priv.h"
#include "tny-camel-common-priv.h"
#include "tny-camel-account-priv.h"

#include <tny-folder-store-query.h>

static void remove_quotes (gchar *buffer);
static gchar **split_recipients (gchar *buffer);


/* TODOL Rename to tny_camel_session_check_operation. */
/** 
 * _tny_session_check_operation:
 * @session: A camel session.
 * @err: A pointer to a #GError *, which will be set if the session is not ready. 
 * This should be freed with g_error_free().
 * @domain The error domain for the GError, if necessary.
 * @code The error code for the GError if necessary.
 *
 * Check that the session is ready to be used, and create a GError with the specified 
 * domain and code if the session is not ready.
 *
 * Returns: %TRUE if the session is ready to be used.
 **/
gboolean 
_tny_session_check_operation (TnySessionCamel *session, TnyAccount *account, GError **err, GQuark domain, gint code)
{
	TnySessionCamel *in = (TnySessionCamel *) session;
	gboolean is_connecting = FALSE;

	if (in == NULL || !CAMEL_IS_SESSION (in))
	{
		g_set_error (err, domain, code,
			"Operating can't continue: account not ready. "
			"This problem indicates a bug in the software.");
		return FALSE;
	}

	if (account && TNY_IS_CAMEL_ACCOUNT (account))
	{
		TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (account);
		is_connecting = apriv->is_connecting;
	} else 
		is_connecting = in->priv->is_connecting;

	if (is_connecting)
	{
		g_set_error (err, domain, code,
			"Operating can't continue: connecting in progress. "
			"This problem indicates a bug in the software.");
		return FALSE;
	}

	in->priv->is_inuse = TRUE; /* Not yet used */

	return TRUE;
}

void 
_tny_session_stop_operation (TnySessionCamel *session)
{
	TnySessionCamel *in = (TnySessionCamel *) session;
	if (in)
		in->priv->is_inuse = FALSE;
}


gboolean 
_tny_folder_store_query_passes (TnyFolderStoreQuery *query, CamelFolderInfo *finfo)
{
	gboolean retval = FALSE;
	TnyList *items;

	if (!query)
		return TRUE;

	items = tny_folder_store_query_get_items (query);

	if (query && (tny_list_get_length (items) > 0)) {
		/* TNY TODO: Make this cope with AND constructs */
		TnyIterator *iterator;
		iterator = tny_list_create_iterator (items);

		while (!tny_iterator_is_done (iterator))
		{
			TnyFolderStoreQueryItem *item = (TnyFolderStoreQueryItem*) tny_iterator_get_current (iterator);
			TnyFolderStoreQueryOption options = tny_folder_store_query_item_get_options (item);

			if ((options & TNY_FOLDER_STORE_QUERY_OPTION_SUBSCRIBED) &&
				finfo->flags & CAMEL_FOLDER_SUBSCRIBED)
					retval = TRUE;

			if ((options & TNY_FOLDER_STORE_QUERY_OPTION_UNSUBSCRIBED) &&
				!(finfo->flags & CAMEL_FOLDER_SUBSCRIBED))
					retval = TRUE;

			if (options & TNY_FOLDER_STORE_QUERY_OPTION_PATTERN_IS_REGEX)
			{
				regex_t *regex = (regex_t *) tny_folder_store_query_item_get_regex (item);

				if (regex && (options & TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_NAME)) {
					if (regexec (regex, finfo->name, 0, NULL, 0) == 0)
						retval = TRUE;
				}

				if (regex && (options & TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_ID)) {
					if (regexec (regex, finfo->full_name, 0, NULL, 0) == 0)
						retval = TRUE;
				}

			} else {
				const gchar *pattern = tny_folder_store_query_item_get_pattern (item);

				if (pattern && (options & TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_NAME)) 
				{
					if (options & TNY_FOLDER_STORE_QUERY_OPTION_PATTERN_IS_CASE_INSENSITIVE)
					{
						if (g_strcasecmp (finfo->name, pattern) == 0)
							retval = TRUE;
					} else {
						if (strcmp (finfo->name, pattern) == 0)
							retval = TRUE;
					}
				}

				if (pattern && (options & TNY_FOLDER_STORE_QUERY_OPTION_MATCH_ON_ID)) 
				{
					if (options & TNY_FOLDER_STORE_QUERY_OPTION_PATTERN_IS_CASE_INSENSITIVE)
					{
						if (g_strcasecmp (finfo->full_name, pattern) == 0)
							retval = TRUE;
					} else {
						if (strcmp (finfo->full_name, pattern) == 0)
							retval = TRUE;
					}
				}
			}

			g_object_unref (item);
			tny_iterator_next (iterator);
		}
		 
		g_object_unref (iterator);
	} else
		retval = TRUE;

	g_object_unref (items);

	return retval;
}

static void
remove_quotes (gchar *buffer)
{
	gchar *tmp = buffer;
	gboolean first_is_quote = FALSE;

	if (buffer == NULL)
		return;

	/* First we remove the first quote */
	first_is_quote = (buffer[0] == '\"');
	while (*tmp != '\0') {
		if ((tmp[1] == '\"') && (tmp[2] == '\0'))
			tmp[1] = '\0';
		if (first_is_quote)
			tmp[0] = tmp[1];
		tmp++;
	}

	if ((tmp > buffer) && (*(tmp-1) == '\"'))
		*(tmp-1) = '\0';

}

static gchar **
split_recipients (gchar *buffer)
{
	gchar *tmp, *start;
	gboolean is_quoted = FALSE;
	GPtrArray *array = g_ptr_array_new ();

	start = tmp = buffer;

	if (buffer == NULL) {
		g_ptr_array_add (array, NULL);
		return (gchar **) g_ptr_array_free (array, FALSE);
	}

	while (*tmp != '\0') {
		if (*tmp == '\"') {
			if (is_quoted) {
				gchar *next_at = strchr (tmp, '@');
				if (next_at) {
					gchar *next_quote = strchr (tmp+1, '"');
					if (!next_quote || (next_quote > next_at))
						is_quoted = !is_quoted;
				}
			} else {
				is_quoted = !is_quoted;
			}
		}
		if (*tmp == '\\')
			tmp++;
		if ((!is_quoted) && ((*tmp == ',') || (*tmp == ';'))) {
			gchar *part;
			part = g_strndup (start, tmp - start);
			part = g_strstrip (part);
			g_ptr_array_add (array, part);
			start = tmp+1;
		}

		tmp++;
	}

	if (start != tmp)
		g_ptr_array_add (array, g_strstrip (g_strdup (start)));

	g_ptr_array_add (array, NULL);
	return (gchar **) g_ptr_array_free (array, FALSE);
}


void
_string_to_camel_inet_addr (gchar *tok, CamelInternetAddress *target)
{
	char *stfnd = NULL;
	
	stfnd = strchr (tok, '<');
	
	if (G_LIKELY (stfnd))
	{
		char *name = (char*)tok, *lname = NULL;
		char *email = stfnd+1, *gtfnd = NULL;

		if (stfnd != tok)
			lname = stfnd-1;

		gtfnd = strchr (stfnd, '>');
	
		if (G_UNLIKELY (!gtfnd))
		{
			g_warning (_("Invalid e-mail address in field"));
			return;
		}
	
		*stfnd = '\0';
		*gtfnd = '\0';
	
		if (G_LIKELY (*name == ' '))
			name++;
	
		if (G_LIKELY (lname && *lname == ' '))
			*lname-- = '\0';
		remove_quotes (name);
		camel_internet_address_add (target, name, email);
	} else {
		
		char *name = (char*)tok;
		char *lname = name;

		lname += (strlen (name)-1);

		if (G_LIKELY (*name == ' '))
			name++;
	
		if (G_LIKELY (*lname == ' '))
			*lname-- = '\0';
		camel_internet_address_add (target, NULL, name);
	}
}


void
_foreach_email_add_to_inet_addr (const gchar *emails, CamelInternetAddress *target)
{
	char *dup = g_strdup (emails);
	gchar **parts, **current;

	if (!emails)
		return;

	parts = split_recipients (dup);
	current = parts;

	while (G_LIKELY (*current != NULL))
	{
		
		_string_to_camel_inet_addr (*current, target);

		current++;
	}

	g_strfreev (parts);
	g_free (dup);

	return;
}

void
_tny_camel_exception_to_tny_error (CamelException *ex, GError **err)
{
	if (!err)
		return;

	if (!ex)
		g_clear_error (err);

	switch (camel_exception_get_id (ex)) {

	case CAMEL_EXCEPTION_NONE:
		g_clear_error (err);
	break;

	case CAMEL_EXCEPTION_INVALID_PARAM: 
		/* The From/To address is not well formed while 
		 * sending or NNTP authentication error*/
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_PROTOCOL,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_SYSTEM:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SYSTEM_ERROR_UNKNOWN,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_USER_CANCEL:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SYSTEM_ERROR_CANCEL,
			camel_exception_get_description (ex));
	break;

	/* Usually fs space problems */
	case CAMEL_EXCEPTION_SYSTEM_IO_WRITE:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_IO_ERROR_WRITE,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_SYSTEM_MEMORY:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SYSTEM_ERROR_MEMORY,
			camel_exception_get_description (ex));
	break;

	/* Usually fs corruption problems */
	case CAMEL_EXCEPTION_SYSTEM_IO_READ:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_IO_ERROR_READ,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_FOLDER_UID_NOT_AVAILABLE: /* message not available atm */
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_MESSAGE_NOT_AVAILABLE,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_FOLDER_INVALID_UID: /* message does not exist */
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_NO_SUCH_MESSAGE,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_SERVICE_NOT_SUPPORTED:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNSUPPORTED,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_FOLDER_NULL: /* never used */
	case CAMEL_EXCEPTION_FOLDER_INVALID:
	case CAMEL_EXCEPTION_FOLDER_INVALID_STATE: /* Parent can't have kids? */
	case CAMEL_EXCEPTION_FOLDER_NON_UID:  /* never used */
	case CAMEL_EXCEPTION_FOLDER_INSUFFICIENT_PERMISSION:
	case CAMEL_EXCEPTION_FOLDER_SUMMARY_INVALID: /* destroyed and recreated on server */
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNKNOWN,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_FOLDER_RENAME: /* folder rename error */
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_FOLDER_RENAME,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_FOLDER_NON_EMPTY: /* folder delete error */
	case CAMEL_EXCEPTION_FOLDER_DELETE:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_FOLDER_REMOVE,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_FOLDER_CREATE: /* folder create error */
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_FOLDER_CREATE,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_STORE_NO_FOLDER: /* Folder does not exist */
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_FOLDER_IS_UNKNOWN,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_SERVICE_PROTOCOL:
		/* For example BAD from IMAP server */
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_PROTOCOL,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_STORE_NULL:
	case CAMEL_EXCEPTION_STORE_INVALID: /* unused */
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNKNOWN,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_SERVICE_INVALID:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_PROTOCOL,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_SERVICE_NOT_CONNECTED:
	case CAMEL_EXCEPTION_SERVICE_UNAVAILABLE:
		/* You must be working online */
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNAVAILABLE,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_SERVICE_CONNECT:
	case CAMEL_EXCEPTION_SYSTEM_HOST_LOOKUP_FAILED:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_CONNECT,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_AUTHENTICATE,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_SERVICE_CERTIFICATE:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_CERTIFICATE,
			camel_exception_get_description (ex));
	break;

	case CAMEL_EXCEPTION_SERVICE_NULL:
	case CAMEL_EXCEPTION_SERVICE_URL_INVALID:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SERVICE_ERROR_UNKNOWN,
			camel_exception_get_description (ex));
	break;
	default:
		g_set_error (err, TNY_ERROR_DOMAIN,
			TNY_SYSTEM_ERROR_UNKNOWN,
			camel_exception_get_description (ex));
	break;
	}

	return;
}

char *
_tny_camel_decode_raw_header (CamelMimePart *part, const char *str, gboolean is_addr)
{
	struct _camel_header_raw *h = part->headers;
	const char *content, *charset = NULL;
	CamelContentType *ct = NULL;

	if (!str)
		return NULL;
	
	if ((content = camel_header_raw_find(&h, "Content-Type", NULL))
	     && (ct = camel_content_type_decode(content))
	     && (charset = camel_content_type_param(ct, "charset"))
	     && (g_ascii_strcasecmp(charset, "us-ascii") == 0))
		charset = NULL;

	charset = charset ? e_iconv_charset_name (charset) : NULL;

	while (isspace ((unsigned) *str))
		str++;

	if (is_addr) {
		char *ret;
		struct _camel_header_address *addr;
		addr = camel_header_address_decode (str, charset);
		if (addr) {
			ret = camel_header_address_list_format (addr);
			camel_header_address_list_clear (&addr);
		} else {
			ret = g_strdup (str);
		}

		if (ct)
			camel_content_type_unref (ct);

		return ret;
	}

	if (ct)
		camel_content_type_unref (ct);

	return camel_header_decode_string (str, charset);
}
