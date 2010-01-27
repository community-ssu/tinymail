/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* camel-imap-command.c: IMAP command sending/parsing routines */

/*
 *  Authors:
 *    Dan Winship <danw@ximian.com>
 *    Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2000, 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <glib/gi18n-lib.h>

#include "camel-debug.h"
#include "camel-exception.h"
#include "camel-private.h"
#include "camel-session.h"
#include "camel-utf8.h"

#include "camel-imap-command.h"
#include "camel-imap-folder.h"
#include "camel-imap-store-summary.h"
#include "camel-imap-store.h"
#include "camel-imap-store-priv.h"
#include "camel-imap-utils.h"
#include "camel-imap-summary.h"

#include "camel-string-utils.h"
#include "camel-stream-buffer.h"

extern int camel_verbose_debug;

static gboolean imap_command_start (CamelImapStore *store, CamelFolder *folder,
				    const char *cmd, CamelException *ex);
static CamelImapResponse *imap_read_response (CamelImapStore *store,
					      CamelException *ex);
static char *imap_read_untagged (CamelImapStore *store, char *line,
				 CamelException *ex);
static char *imap_read_untagged_opp (CamelImapStore *store, char *line,
				 CamelException *ex, int len);
static char *imap_command_strdup_vprintf (CamelImapStore *store,
					  const char *fmt, va_list ap);
static char *imap_command_strdup_printf (CamelImapStore *store,
					 const char *fmt, ...);

#ifdef NOTUSED
static int
uid_compar (const void *va, const void *vb)
{
	const char **sa = (const char **)va, **sb = (const char **)vb;
	unsigned long a, b;

	a = strtoul (*sa, NULL, 10);
	b = strtoul (*sb, NULL, 10);
	if (a < b)
		return -1;
	else if (a == b)
		return 0;
	else
		return 1;
}

#endif

/**
 * camel_imap_command:
 * @store: the IMAP store
 * @folder: The folder to perform the operation in (or %NULL if not
 * relevant).
 * @ex: a CamelException
 * @fmt: a sort of printf-style format string, followed by arguments
 *
 * This function calls camel_imap_command_start() to send the
 * command, then reads the complete response to it using
 * camel_imap_command_response() and returns a CamelImapResponse
 * structure.
 *
 * As a special case, if @fmt is %NULL, it will just select @folder
 * and return the response from doing so.
 *
 * See camel_imap_command_start() for details on @fmt.
 *
 * On success, the store's connect_lock will be locked. It will be freed
 * when you call camel_imap_response_free. (The lock is recursive, so
 * callers can grab and release it themselves if they need to run
 * multiple commands atomically.)
 *
 * Return value: %NULL if an error occurred (in which case @ex will
 * be set). Otherwise, a CamelImapResponse describing the server's
 * response, which the caller must free with camel_imap_response_free().
 **/
#define UID_SET_LIMIT  (768)

CamelImapResponse *
camel_imap_command (CamelImapStore *store, CamelFolder *folder,
		    CamelException *ex, const char *fmt, ...)
{
	va_list ap;
	char *cmd = NULL;

	camel_imap_store_stop_idle_connect_lock (store);

	if (fmt) {
		va_start (ap, fmt);
		cmd = imap_command_strdup_vprintf (store, fmt, ap);
		va_end (ap);
	} else {

		char *modseq = NULL;

		/* camel_object_ref(folder);
		if (store->current_folder && CAMEL_IS_OBJECT (store->current_folder))
			camel_object_unref(store->current_folder); */

		if (!folder) {
			store->last_folder = store->current_folder;
			if (store->last_folder)
				camel_object_hook_event (store->last_folder, "finalize",
							 _camel_imap_store_last_folder_finalize, store);
		} else {
			modseq = camel_imap_folder_get_highestmodseq (CAMEL_IMAP_FOLDER (folder));
			if (store->last_folder)
				camel_object_unhook_event (store->last_folder, "finalize",
							   _camel_imap_store_last_folder_finalize, store);
			store->last_folder = NULL;
		}
		
		if (store->current_folder)
			camel_object_unhook_event (store->current_folder, "finalize",
						   _camel_imap_store_current_folder_finalize, store);
		store->current_folder = folder;
		if (store->current_folder)
			camel_object_hook_event (store->current_folder, "finalize",
						 _camel_imap_store_current_folder_finalize, store);
		
		if (modseq && (store->capabilities & IMAP_CAPABILITY_QRESYNC))
		{
			CamelImapSummary *imap_summary = CAMEL_IMAP_SUMMARY (folder->summary);

			cmd = imap_command_strdup_printf (store,
				"SELECT %F (QRESYNC (%d %s))",
				folder->full_name,
				imap_summary->validity, modseq);

		} else if (folder) {
			if (store->capabilities & IMAP_CAPABILITY_CONDSTORE)
				cmd = imap_command_strdup_printf (store, "SELECT %F (CONDSTORE)", folder->full_name);
			else
				cmd = imap_command_strdup_printf (store, "SELECT %F", folder->full_name);
		}

		if (modseq)
			g_free (modseq);
	}

	if (!imap_command_start (store, folder, cmd, ex)) {
		g_free (cmd);
		camel_imap_store_connect_unlock_start_idle (store);
		return NULL;
	}
	g_free (cmd);

	return imap_read_response (store, ex);
}


/**
 * camel_imap_command_start:
 * @store: the IMAP store
 * @folder: The folder to perform the operation in (or %NULL if not
 * relevant).
 * @ex: a CamelException
 * @fmt: a sort of printf-style format string, followed by arguments
 *
 * This function makes sure that @folder (if non-%NULL) is the
 * currently-selected folder on @store and then sends the IMAP command
 * specified by @fmt and the following arguments.
 *
 * @fmt can include the following %-escapes ONLY:
 *	%s, %d, %%: as with printf
 *	%S: an IMAP "string" (quoted string or literal)
 *	%F: an IMAP folder name
 *	%G: an IMAP folder name, with namespace already prepended
 *
 * %S strings will be passed as literals if the server supports LITERAL+
 * and quoted strings otherwise. (%S does not support strings that
 * contain newlines.)
 *
 * %F will have the imap store's namespace prepended; %F and %G will then
 * be converted to UTF-7 and processed like %S.
 *
 * On success, the store's connect_lock will be locked. It will be
 * freed when %CAMEL_IMAP_RESPONSE_TAGGED or %CAMEL_IMAP_RESPONSE_ERROR
 * is returned from camel_imap_command_response(). (The lock is
 * recursive, so callers can grab and release it themselves if they
 * need to run multiple commands atomically.)
 *
 * Return value: %TRUE if the command was sent successfully, %FALSE if
 * an error occurred (in which case @ex will be set).
 **/
gboolean
camel_imap_command_start (CamelImapStore *store, CamelFolder *folder,
			  CamelException *ex, const char *fmt, ...)
{
	va_list ap;
	char *cmd;
	gboolean ok;

	va_start (ap, fmt);
	cmd = imap_command_strdup_vprintf (store, fmt, ap);
	va_end (ap);

	camel_imap_store_stop_idle_connect_lock (store);

	ok = imap_command_start (store, folder, cmd, ex);
	g_free (cmd);

	if (!ok)
		camel_imap_store_connect_unlock_start_idle (store);

	return ok;
}

static gboolean
imap_command_start (CamelImapStore *store, CamelFolder *folder,
		    const char *cmd, CamelException *ex)
{
	ssize_t nwritten;
	ssize_t nread;
	gchar *resp = NULL;
	CamelException myex = CAMEL_EXCEPTION_INITIALISER;
	gchar *full_cmd = NULL;
	guint len = 0;
	gboolean fetching_message = (folder && (folder->parent_store != (CamelStore *) store));

	if (store->ostream == NULL || ((CamelObject *)store->ostream)->ref_count <= 0)
	{
		if (store->has_login && (store->not_recon || !camel_service_connect ((CamelService*)store, ex)))
		{
			camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_UNAVAILABLE,
				     _("You must be working online to "
				       "complete this operation"));
			return FALSE;
		}
	}

	/* g_mutex_lock (store->stream_lock); */

	if (store->ostream==NULL) {
		camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_UNAVAILABLE,
				     _("You must be working online to "
				       "complete this operation"));
		 return FALSE;
	}

	if (store->istream==NULL) {
		camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_UNAVAILABLE,
				     _("You must be working online to "
				       "complete this operation"));
		return FALSE;
	}

	/* g_mutex_unlock (store->stream_lock);*/


	/* Also read imap_update_summary and all of the IDLE crap. The fetching
	 * message and the LOGOUT exception are for camel_imap_folder_fetch_data's
	 * case (I know this is a hack): In those cases, store is not the
	 * connection where the IDLE is happening, so there's no need to stop it
	 * either! On top of that is stopping the IDLE in case of LOGOUT handled
	 * elsewere too */

	if (!store->dontdistridlehack && !fetching_message && strcmp (cmd, "LOGOUT"))
	{
		gboolean wasnull = FALSE;
		if (!store->current_folder) {
			store->current_folder = store->last_folder;
			if (store->current_folder)
				camel_object_hook_event (store->current_folder, "finalize",
							 _camel_imap_store_current_folder_finalize, store);
			wasnull = TRUE;
		}
		camel_imap_store_stop_idle (store);
		if (wasnull) {
			if (store->current_folder) {
				camel_object_unhook_event (store->current_folder, "finalize",
							   _camel_imap_store_current_folder_finalize, store);
				store->current_folder = NULL;
			}
		}
	}

	/* Check for current folder */
	if (folder && folder != store->current_folder)
	{
		CamelImapResponse *response;
		CamelException internal_ex = CAMEL_EXCEPTION_INITIALISER;

		response = camel_imap_command (store, folder, ex, NULL);
		if (!response)
			return FALSE;

		/* It'll not be the same instances when called for camel_imap_folder_fetch_data.
		 * That's because a all-new connection will be used for getting messages */

		if (!fetching_message)
			camel_imap_folder_selected (folder, response, &internal_ex, FALSE);

		camel_imap_response_free (store, response);

		if (camel_exception_is_set (&internal_ex)) {
			camel_exception_xfer (ex, &internal_ex);
			return FALSE;
		}
	}

	/* Send the command */
	if (camel_verbose_debug) {
		const char *mask;

		if (!strncmp ("LOGIN \"", cmd, 7))
			mask = "LOGIN \"xxx\" xxx";
		else if (!strncmp ("LOGIN {", cmd, 7))
			mask = "LOGIN {N+}\r\nxxx {N+}\r\nxxx";
		else if (!strncmp ("LOGIN ", cmd, 6))
			mask = "LOGIN xxx xxx";
		else
			mask = cmd;

		fprintf (stderr, "sending : %c%.5u %s\r\n", store->tag_prefix, store->command, mask);
	}

	/* Read away whatever we got */
	while ((nread = camel_imap_store_readline_nb (store, &resp, &myex)) > 0)
	{
#ifdef IMAP_DEBUG
		gchar *debug_resp;
		gchar *debug_resp_escaped;
		imap_debug ("unsolitcited: ");
		debug_resp = g_strndup (resp, nread);
		debug_resp_escaped = g_strescape (debug_resp, "");
		g_free (debug_resp);
		imap_debug (debug_resp_escaped);
		g_free (debug_resp_escaped);
		imap_debug ("\n");
#endif

		g_free (resp);
		resp=NULL;
	}
	if (resp)
		g_free (resp);

	full_cmd = g_strdup_printf ("%c%.5u %s\r\n", store->tag_prefix,
		store->command++, cmd);
	len = strlen (full_cmd);

	nwritten = camel_stream_write (store->ostream, full_cmd, len);

	imap_debug ("(%d, %d) -> %s\n", len, nwritten, full_cmd);

	g_free (full_cmd);

	/* g_mutex_unlock (store->stream_lock); */

	if (nwritten != len)
	{
		CamelException mex = CAMEL_EXCEPTION_INITIALISER;

		if (errno == EINTR) {
			camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL,
					     _("Operation cancelled"));
		} else
			camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_LOST_CONNECTION,
					     g_strerror (errno));


		camel_imap_recon (store, &mex, FALSE);
		imap_debug ("Recon in start: %s\n", camel_exception_get_description (&mex));

		camel_exception_clear (&mex);
		return FALSE;
	}

	return TRUE;
}

/**
 * camel_imap_command_continuation:
 * @store: the IMAP store
 * @cmd: buffer containing the response/request data
 * @cmdlen: command length
 * @ex: a CamelException
 *
 * This method is for sending continuing responses to the IMAP server
 * after camel_imap_command() or camel_imap_command_response() returns
 * a continuation response.
 *
 * This function assumes you have an exclusive lock on the imap stream.
 *
 * Return value: as for camel_imap_command(). On failure, the store's
 * connect_lock will be released.
 **/
CamelImapResponse *
camel_imap_command_continuation (CamelImapStore *store, const char *cmd,
				 size_t cmdlen, CamelException *ex)
{
	if (!camel_disco_store_check_online ((CamelDiscoStore*)store, ex))
		return NULL;

	g_return_val_if_fail(store->ostream!=NULL, NULL);
	g_return_val_if_fail(store->istream!=NULL, NULL);

	if (camel_stream_write (store->ostream, cmd, cmdlen) == -1 ||
	    camel_stream_write (store->ostream, "\r\n", 2) == -1) {
		if (errno == EINTR) {
			CamelException mex = CAMEL_EXCEPTION_INITIALISER;
			camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL,
					     _("Operation cancelled"));
			camel_imap_recon (store, &mex, TRUE);
			imap_debug ("Recon in cont: %s\n", camel_exception_get_description (&mex));
			camel_imap_store_connect_unlock_start_idle (store);
			camel_exception_clear (&mex);
			return NULL;
		} else
			camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_LOST_CONNECTION,
					     g_strerror (errno));
		camel_service_disconnect (CAMEL_SERVICE (store), FALSE, NULL);

		camel_imap_store_connect_unlock_start_idle (store);
		return NULL;
	}

	return imap_read_response (store, ex);
}


/**
 * camel_imap_command_response:
 * @store: the IMAP store
 * @response: a pointer to pass back the response data in
 * @ex: a CamelException
 *
 * This reads a single tagged, untagged, or continuation response from
 * @store into *@response. The caller must free the string when it is
 * done with it.
 *
 * Return value: One of %CAMEL_IMAP_RESPONSE_CONTINUATION,
 * %CAMEL_IMAP_RESPONSE_UNTAGGED, %CAMEL_IMAP_RESPONSE_TAGGED, or
 * %CAMEL_IMAP_RESPONSE_ERROR. If either of the last two, @store's
 * command lock will be unlocked.
 **/
CamelImapResponseType
camel_imap_command_response (CamelImapStore *store, char **response,
			     CamelException *ex)
{
	CamelImapResponseType type;
	char *respbuf;
	int len = -1;

	if (camel_imap_store_readline (store, &respbuf, ex) < 0) {
		camel_imap_store_connect_unlock_start_idle (store);
		return CAMEL_IMAP_RESPONSE_ERROR;
	}

	char *ptr = strchr (respbuf, '{');
	if (ptr) {
		ptr++;
		len = strtoul (ptr, NULL, 10);
	}

	imap_debug ("(.., ..) <- %s\n", respbuf);

	switch (*respbuf) {
	case '*':
		if (!g_ascii_strncasecmp (respbuf, "* BYE", 5)) {
			/* Connection was lost, no more data to fetch */
			camel_service_disconnect (CAMEL_SERVICE (store), FALSE, NULL);
			camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_LOST_CONNECTION,
					      _("Server unexpectedly disconnected: %s"),
					      _("Unknown error")); /* g_strerror (104));  FIXME after 1.0 is released */
			store->connected = FALSE;
			g_free (respbuf);
			respbuf = NULL;
			type = CAMEL_IMAP_RESPONSE_ERROR;
			break;
		}

		/* Read the rest of the response. */
		type = CAMEL_IMAP_RESPONSE_UNTAGGED;

		if (len != -1)
			respbuf = imap_read_untagged_opp (store, respbuf, ex, len);
		else
			respbuf = imap_read_untagged (store, respbuf, ex);

		if (!respbuf)
			type = CAMEL_IMAP_RESPONSE_ERROR;
		else if (!g_ascii_strncasecmp (respbuf, "* OK [ALERT]", 12)
			 || !g_ascii_strncasecmp (respbuf, "* NO [ALERT]", 12)
			 || !g_ascii_strncasecmp (respbuf, "* BAD [ALERT]", 13)) {
			char *msg;

			/* for imap ALERT codes, account user@host */
			/* we might get a ']' from a BAD response since we +12, but who cares? */
			msg = g_strdup_printf(_("Alert from IMAP server %s@%s:\n%s"),
					      ((CamelService *)store)->url->user, ((CamelService *)store)->url->host, respbuf+12);
			camel_session_alert_user_generic(((CamelService *)store)->session,
				CAMEL_SESSION_ALERT_WARNING, msg, FALSE, ((CamelService *)store));
			g_free(msg);
		}

		break;
	case '+':
		type = CAMEL_IMAP_RESPONSE_CONTINUATION;
		break;
	default:
		type = CAMEL_IMAP_RESPONSE_TAGGED;
		break;
	}
	*response = respbuf;

	if (type == CAMEL_IMAP_RESPONSE_ERROR ||
	    type == CAMEL_IMAP_RESPONSE_TAGGED)
		camel_imap_store_connect_unlock_start_idle (store);

	return type;
}


CamelImapResponseType
camel_imap_command_response_idle (CamelImapStore *store, char **response,
			     CamelException *ex)
{
	CamelImapResponseType type;
	char *respbuf;

	if (camel_imap_store_readline_nl (store, &respbuf, ex) < 0)
		return CAMEL_IMAP_RESPONSE_ERROR;

	imap_debug ("(.., ..) <- %s (IDLE response)\n", respbuf);

	switch (*respbuf) {
	case '*':
		if (!g_ascii_strncasecmp (respbuf, "* BYE", 5)) {
			/* Connection was lost, no more data to fetch */
			camel_service_disconnect (CAMEL_SERVICE (store), FALSE, NULL);
			camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_LOST_CONNECTION,
					      _("Server unexpectedly disconnected: %s"),
					      _("Unknown error")); /* g_strerror (104));  FIXME after 1.0 is released */
			store->connected = FALSE;
			g_free (respbuf);
			respbuf = NULL;
			type = CAMEL_IMAP_RESPONSE_ERROR;
			break;
		}

		/* Read the rest of the response. */
		type = CAMEL_IMAP_RESPONSE_UNTAGGED;
		respbuf = imap_read_untagged (store, respbuf, ex);
		if (!respbuf)
			type = CAMEL_IMAP_RESPONSE_ERROR;
		else if (!g_ascii_strncasecmp (respbuf, "* OK [ALERT]", 12)
			 || !g_ascii_strncasecmp (respbuf, "* NO [ALERT]", 12)
			 || !g_ascii_strncasecmp (respbuf, "* BAD [ALERT]", 13)) {
			char *msg;

			/* for imap ALERT codes, account user@host */
			/* we might get a ']' from a BAD response since we +12, but who cares? */
			msg = g_strdup_printf(_("Alert from IMAP server %s@%s:\n%s"),
					      ((CamelService *)store)->url->user, ((CamelService *)store)->url->host, respbuf+12);
			camel_session_alert_user_generic(((CamelService *)store)->session,
				CAMEL_SESSION_ALERT_WARNING, msg, FALSE, ((CamelService *)store));
			g_free(msg);
		} else if (!g_ascii_strncasecmp (respbuf, "* BAD Invalid tag",17))
			type = CAMEL_IMAP_RESPONSE_ERROR;
		break;
	case '+':
		type = CAMEL_IMAP_RESPONSE_CONTINUATION;
		break;
	default:
		if (camel_strstrcase (respbuf, "OK") != NULL ||
			camel_strstrcase (respbuf, "NO") != NULL ||
			camel_strstrcase (respbuf, "BAD") != NULL) {

			type = CAMEL_IMAP_RESPONSE_TAGGED;

		} else
			type = CAMEL_IMAP_RESPONSE_UNTAGGED;
		break;
	}
	*response = respbuf;

	return type;
}

CamelImapResponse *
imap_read_response (CamelImapStore *store, CamelException *ex)
{
	CamelImapResponse *response;
	CamelImapResponseType type;
	char *respbuf, *p;

	/* Get another lock so that when we reach the tagged
	 * response and camel_imap_command_response unlocks,
	 * we're still locked. This lock is owned by response
	 * and gets unlocked when response is freed.
	 */

	camel_imap_store_stop_idle_connect_lock (store);

	response = g_new0 (CamelImapResponse, 1);
	if (store->current_folder && camel_disco_store_status (CAMEL_DISCO_STORE (store)) != CAMEL_DISCO_STORE_RESYNCING) {
		response->folder = store->current_folder;
		camel_object_ref (CAMEL_OBJECT (response->folder));
	}

	response->untagged = g_ptr_array_new ();
	while ((type = camel_imap_command_response (store, &respbuf, ex))
	       == CAMEL_IMAP_RESPONSE_UNTAGGED)
		g_ptr_array_add (response->untagged, respbuf);

	if (type == CAMEL_IMAP_RESPONSE_ERROR) {
		camel_imap_response_free_without_processing (store, response);
		return NULL;
	}

	response->status = respbuf;

	/* Check for OK or continuation response. */
	if (*respbuf == '+')
		return response;
	p = strchr (respbuf, ' ');
	if (p && !g_ascii_strncasecmp (p, " OK", 3))
		return response;

	/* We should never get BAD, or anything else but +, OK, or NO
	 * for that matter.  Well, we could get BAD, treat as NO.
	 */
	if (!p || (g_ascii_strncasecmp(p, " NO", 3) != 0 && g_ascii_strncasecmp(p, " BAD", 4)) ) {
		g_warning ("Unexpected response from IMAP server: %s",
			   respbuf);
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
				      _("Unexpected response from IMAP "
					"server: %s"), respbuf);
		camel_imap_response_free_without_processing (store, response);
		return NULL;
	}

	p += 3;
	if (!*p++)
		p = NULL;
	camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
			      _("IMAP command failed: %s"),
			      p ? p : _("Unknown error"));
	camel_imap_response_free_without_processing (store, response);
	return NULL;
}

/* Given a line that is the start of an untagged response, read and
 * return the complete response, which may include an arbitrary number
 * of literals.
 */
static char *
imap_read_untagged_opp (CamelImapStore *store, char *line, CamelException *ex, int len)
{
	int fulllen, ldigits, nread, n, i, sexp = 0;
	unsigned int length;
	GPtrArray *data;
	GString *str;
	char *end, *p, *s, *d;

	p = strrchr (line, '{');
	if (!p)
		return line;

	data = g_ptr_array_new ();
	fulllen = 0;

	while (1) {
		str = g_string_new (line);
		g_free (line);
		fulllen += str->len;
		g_ptr_array_add (data, str);

		if (!(p = strrchr (str->str, '{')) || p[1] == '-')
			break;

		/* HACK ALERT: We scan the non-literal part of the string, looking for possible s expression braces.
		   This assumes we're getting s-expressions, which we should be.
		   This is so if we get a blank line after a literal, in an s-expression, we can keep going, since
		   we do no other parsing at this level.
		   TODO: handle quoted strings? */
		for (s=str->str; s<p; s++) {
			if (*s == '(')
				sexp++;
			else if (*s == ')')
				sexp--;
		}

		length = strtoul (p + 1, &end, 10);
		if (*end != '}' || *(end + 1) || end == p + 1 || length >= UINT_MAX - 2)
			break;
		ldigits = end - (p + 1);

		/* Read the literal */
		str = g_string_sized_new (length + 2);
		str->str[0] = '\n';
		nread = 0;

		do {
			if ((n = camel_stream_buffer_read_opp (store->istream, str->str + nread + 1, length - nread, len)) == -1) {

				if (errno == EINTR)
				{
					CamelException mex = CAMEL_EXCEPTION_INITIALISER;
					camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL,
							     _("Operation cancelled"));
					camel_imap_recon (store, &mex, TRUE);
					imap_debug ("Recon in untagged: %s\n", camel_exception_get_description (&mex));
					camel_exception_clear (&mex);
				} else {
					camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_LOST_CONNECTION,
							     g_strerror (errno));
					camel_service_disconnect (CAMEL_SERVICE (store), FALSE, NULL);
				}
				g_string_free (str, TRUE);
				goto lose;
			}

			nread += n;
		} while (n > 0 && nread < length);

		if (nread < length) {
			if (errno == EINTR) {
				CamelException mex = CAMEL_EXCEPTION_INITIALISER;
				camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL,
						     _("Operation cancelled"));
				camel_imap_recon (store, &mex, TRUE);
				imap_debug ("Recon in untagged idle: %s\n", camel_exception_get_description (&mex));
				camel_exception_clear (&mex);
			} else {
				camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_LOST_CONNECTION,
					     _("Server response ended too soon."));
				camel_service_disconnect (CAMEL_SERVICE (store), FALSE, NULL);
			}
			g_string_free (str, TRUE);
			goto lose;
		}
		str->str[length + 1] = '\0';

		if (camel_debug("imap")) {
			printf("Literal: -->");
			fwrite(str->str+1, 1, length, stdout);
			printf("<--\n");
		}

		/* Fix up the literal, turning CRLFs into LF. Also, if
		 * we find any embedded NULs, strip them. This is
		 * dubious, but:
		 *   - The IMAP grammar says you can't have NULs here
		 *     anyway, so this will not affect our behavior
		 *     against any completely correct server.
		 *   - WU-imapd 12.264 (at least) will cheerily pass
		 *     NULs along if they are embedded in the message
		 */

		s = d = str->str + 1;
		end = str->str + 1 + length;
		while (s < end) {
			while (s < end && *s == '\0') {
				s++;
				length--;
			}
			if (*s == '\r' && *(s + 1) == '\n') {
				s++;
				length--;
			}
			*d++ = *s++;
		}
		*d = '\0';
		str->len = length + 1;

		/* p points to the "{" in the line that starts the
		 * literal. The length of the CR-less response must be
		 * less than or equal to the length of the response
		 * with CRs, therefore overwriting the old value with
		 * the new value cannot cause an overrun. However, we
		 * don't want it to be shorter either, because then the
		 * GString's length would be off...
		 */
		sprintf (p, "{%0*u}", ldigits, length);

		fulllen += str->len;
		g_ptr_array_add (data, str);

		/* Read the next line. */
		do {
			if (camel_imap_store_readline (store, &line, ex) < 0)
				goto lose;

			/* MAJOR HACK ALERT, gropuwise sometimes sends an extra blank line after literals, check that here
			   But only do it if we're inside an sexpression */
			if (line[0] == 0 && sexp > 0)
				g_warning("Server sent empty line after a literal, assuming in error");
		} while (line[0] == 0 && sexp > 0);
	}

	/* Now reassemble the data. */
	p = line = g_malloc (fulllen + 1);
	for (i = 0; i < data->len; i++) {
		str = data->pdata[i];
		memcpy (p, str->str, str->len);
		p += str->len;
		g_string_free (str, TRUE);
	}
	*p = '\0';
	g_ptr_array_free (data, TRUE);
	return line;

 lose:
	for (i = 0; i < data->len; i++)
		g_string_free (data->pdata[i], TRUE);
	g_ptr_array_free (data, TRUE);
	return NULL;
}



static char *
imap_read_untagged (CamelImapStore *store, char *line, CamelException *ex)
{
	int fulllen, ldigits, nread, n, i, sexp = 0;
	unsigned int length;
	GPtrArray *data;
	GString *str;
	char *end, *p, *s, *d;

	p = strrchr (line, '{');
	if (!p)
		return line;

	data = g_ptr_array_new ();
	fulllen = 0;

	while (1) {
		str = g_string_new (line);
		g_free (line);
		fulllen += str->len;
		g_ptr_array_add (data, str);

		if (!(p = strrchr (str->str, '{')) || p[1] == '-')
			break;

		/* HACK ALERT: We scan the non-literal part of the string, looking for possible s expression braces.
		   This assumes we're getting s-expressions, which we should be.
		   This is so if we get a blank line after a literal, in an s-expression, we can keep going, since
		   we do no other parsing at this level.
		   TODO: handle quoted strings? */
		for (s=str->str; s<p; s++) {
			if (*s == '(')
				sexp++;
			else if (*s == ')')
				sexp--;
		}

		length = strtoul (p + 1, &end, 10);
		if (*end != '}' || *(end + 1) || end == p + 1 || length >= UINT_MAX - 2)
			break;
		ldigits = end - (p + 1);

		/* Read the literal */
		str = g_string_sized_new (length + 2);
		str->str[0] = '\n';
		nread = 0;

		do {
			if ((n = camel_stream_read (store->istream, str->str + nread + 1, length - nread)) == -1) {
				if (errno == EINTR)
				{
					CamelException mex = CAMEL_EXCEPTION_INITIALISER;
					camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL,
							     _("Operation cancelled"));
					camel_imap_recon (store, &mex, TRUE);
					imap_debug ("Recon in untagged: %s\n", camel_exception_get_description (&mex));
					camel_exception_clear (&mex);
				} else {
					camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_LOST_CONNECTION,
							     g_strerror (errno));
					camel_service_disconnect (CAMEL_SERVICE (store), FALSE, NULL);
				}
				g_string_free (str, TRUE);
				goto lose;
			}

			nread += n;
		} while (n > 0 && nread < length);

		if (nread < length) {
			if (errno == EINTR) {
				CamelException mex = CAMEL_EXCEPTION_INITIALISER;
				camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL,
						     _("Operation cancelled"));
				camel_imap_recon (store, &mex, TRUE);
				imap_debug ("Recon in untagged idle: %s\n", camel_exception_get_description (&mex));
				camel_exception_clear (&mex);
			} else {
				camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_LOST_CONNECTION,
					     _("Server response ended too soon."));
				camel_service_disconnect (CAMEL_SERVICE (store), FALSE, NULL);
			}
			g_string_free (str, TRUE);
			goto lose;
		}
		str->str[length + 1] = '\0';

		if (camel_debug("imap")) {
			printf("Literal: -->");
			fwrite(str->str+1, 1, length, stdout);
			printf("<--\n");
		}

		/* Fix up the literal, turning CRLFs into LF. Also, if
		 * we find any embedded NULs, strip them. This is
		 * dubious, but:
		 *   - The IMAP grammar says you can't have NULs here
		 *     anyway, so this will not affect our behavior
		 *     against any completely correct server.
		 *   - WU-imapd 12.264 (at least) will cheerily pass
		 *     NULs along if they are embedded in the message
		 */

		s = d = str->str + 1;
		end = str->str + 1 + length;
		while (s < end) {
			while (s < end && *s == '\0') {
				s++;
				length--;
			}
			if (*s == '\r' && *(s + 1) == '\n') {
				s++;
				length--;
			}
			*d++ = *s++;
		}
		*d = '\0';
		str->len = length + 1;

		/* p points to the "{" in the line that starts the
		 * literal. The length of the CR-less response must be
		 * less than or equal to the length of the response
		 * with CRs, therefore overwriting the old value with
		 * the new value cannot cause an overrun. However, we
		 * don't want it to be shorter either, because then the
		 * GString's length would be off...
		 */
		sprintf (p, "{%0*u}", ldigits, length);

		fulllen += str->len;
		g_ptr_array_add (data, str);

		/* Read the next line. */
		do {
			if (camel_imap_store_readline (store, &line, ex) < 0)
				goto lose;

			/* MAJOR HACK ALERT, gropuwise sometimes sends an extra blank line after literals, check that here
			   But only do it if we're inside an sexpression */
			if (line[0] == 0 && sexp > 0)
				g_warning("Server sent empty line after a literal, assuming in error");
		} while (line[0] == 0 && sexp > 0);
	}

	/* Now reassemble the data. */
	p = line = g_malloc (fulllen + 1);
	for (i = 0; i < data->len; i++) {
		str = data->pdata[i];
		memcpy (p, str->str, str->len);
		p += str->len;
		g_string_free (str, TRUE);
	}
	*p = '\0';
	g_ptr_array_free (data, TRUE);
	return line;

 lose:
	for (i = 0; i < data->len; i++)
		g_string_free (data->pdata[i], TRUE);
	g_ptr_array_free (data, TRUE);
	return NULL;
}

/**
 * camel_imap_response_free:
 * @store: the CamelImapStore the response is from
 * @response: a CamelImapResponse
 * @fetching_message: whether we're fetching a message
 *
 * Frees all of the data in @response and processes any untagged
 * EXPUNGE and EXISTS responses in it. Releases @store's connect_lock.
 **/
void
camel_imap_response_free (CamelImapStore *store, CamelImapResponse *response)
{
	int i, number, exists = 0;
	GArray *expunged = NULL;
	char *resp, *p;
	gboolean fetching_message = FALSE;


	if (!response)
		return;

	fetching_message = (response->folder && (response->folder->parent_store != (CamelStore *) store));

	for (i = 0; i < response->untagged->len; i++) {
		resp = response->untagged->pdata[i];

		if (response->folder) {
			/* Check if it's something we need to handle. */
			number = strtoul (resp + 2, &p, 10);
			if (!g_ascii_strcasecmp (p, " EXISTS")) {
				exists = number;

			/* TNY TODO, QRESYNC TODO: add VANISHED HERE */

			} else if (!g_ascii_strcasecmp (p, " EXPUNGE")
				   || !g_ascii_strcasecmp(p, " XGWMOVE")) {
				/* XGWMOVE response is the same as an EXPUNGE response */
				if (!expunged) {
					expunged = g_array_new (FALSE, FALSE,
								sizeof (int));
				}
				g_array_append_val (expunged, number);
			}
		}
		g_free (resp);
	}

	g_ptr_array_free (response->untagged, TRUE);
	g_free (response->status);

	if (response->folder && !fetching_message) {
		if (exists > 0 || expunged) {
			/* Update the summary */
			if (!(store->parameters & IMAP_PARAM_DONT_TOUCH_SUMMARY)){
				camel_imap_folder_changed (response->folder,
						   exists, expunged, NULL);
			}
		}
	}

	if (expunged)
		g_array_free (expunged, TRUE);

	if (response->folder)
		camel_object_unref (CAMEL_OBJECT (response->folder));

	g_free (response);

	camel_imap_store_connect_unlock_start_idle (store);
}

/**
 * camel_imap_response_free_without_processing:
 * @store: the CamelImapStore the response is from.
 * @response: a CamelImapResponse:
 *
 * Frees all of the data in @response without processing any untagged
 * responses. Releases @store's command lock.
 **/
void
camel_imap_response_free_without_processing (CamelImapStore *store,
					     CamelImapResponse *response)
{
	if (!response)
		return;

	if (response->folder) {
		camel_object_unref (CAMEL_OBJECT (response->folder));
		response->folder = NULL;
	}
	camel_imap_response_free (store, response);
}

/**
 * camel_imap_response_extract:
 * @store: the store the response came from
 * @response: the response data returned from camel_imap_command
 * @type: the response type to extract
 * @ex: a CamelException
 *
 * This checks that @response contains a single untagged response of
 * type @type and returns just that response data. If @response
 * doesn't contain the right information, the function will set @ex
 * and return %NULL. Either way, @response will be freed and the
 * store's connect_lock released.
 *
 * Return value: the desired response string, which the caller must free.
 **/
char *
camel_imap_response_extract (CamelImapStore *store,
			     CamelImapResponse *response,
			     const char *type,
			     CamelException *ex)
{
	int len = strlen (type), i;
	char *resp = NULL;

	len = strlen (type);

	for (i = 0; i < response->untagged->len; i++) {
		resp = response->untagged->pdata[i];
		/* Skip "* ", and initial sequence number, if present */
		strtoul (resp + 2, &resp, 10);
		if (*resp == ' ')
			resp = (char *) imap_next_word (resp);

		if (!g_ascii_strncasecmp (resp, type, len))
			break;
	}

	if (i < response->untagged->len) {
		resp = response->untagged->pdata[i];
		g_ptr_array_remove_index (response->untagged, i);
	} else {
		resp = NULL;
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
				      _("IMAP server response did not contain "
					"%s information"), type);
	}

	camel_imap_response_free (store, response);
	return resp;
}

/**
 * camel_imap_response_extract_continuation:
 * @store: the store the response came from
 * @response: the response data returned from camel_imap_command
 * @ex: a CamelException
 *
 * This checks that @response contains a continuation response, and
 * returns just that data. If @response doesn't contain a continuation
 * response, the function will set @ex, release @store's connect_lock,
 * and return %NULL. Either way, @response will be freed.
 *
 * Return value: the desired response string, which the caller must free.
 **/
char *
camel_imap_response_extract_continuation (CamelImapStore *store,
					  CamelImapResponse *response,
					  CamelException *ex)
{
	char *status;

	if (response->status && *response->status == '+') {
		status = response->status;
		response->status = NULL;
		camel_imap_response_free (store, response);
		return status;
	}

	camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_PROTOCOL,
			      _("Unexpected OK response from IMAP server: %s"),
			      response->status);
	camel_imap_response_free (store, response);
	return NULL;
}

static char *
imap_command_strdup_vprintf (CamelImapStore *store, const char *fmt,
			     va_list ap)
{
	GPtrArray *args;
	const char *p, *start;
	char *out, *outptr, *string;
	int num, len, i, arglen = 0;

	args = g_ptr_array_new ();

	/* Determine the length of the data */
	len = strlen (fmt);
	p = start = fmt;
	while (*p) {
		p = strchr (start, '%');
		if (!p)
			break;

		switch (*++p) {
		case 'd':
			num = va_arg (ap, int);
			g_ptr_array_add (args, GINT_TO_POINTER (num));
			start = p + 1;
			len += 10;
			break;
		case 's':
			string = va_arg (ap, char *);
			g_ptr_array_add (args, string);
			start = p + 1;
			len += strlen (string);
			break;
		case 'S':
		case 'F':
		case 'G':
			string = va_arg (ap, char *);
			/* NB: string is freed during output for %F and %G */
			if (*p == 'F') {
				char *s = camel_imap_store_summary_full_from_path(store->summary, string);
				if (s) {
					string = camel_utf8_utf7(s);
					g_free(s);
				} else {
					string = camel_utf8_utf7(string);
				}
			} else if (*p == 'G') {
				string = camel_utf8_utf7(string);
			}

			if (string)
				arglen = strlen (string);
			g_ptr_array_add (args, string);
			if (string && imap_is_atom (string)) {
				len += arglen;
			} else {
				if (store->capabilities & IMAP_CAPABILITY_LITERALPLUS)
					len += arglen + 15;
				else
					len += arglen * 2;
			}
			start = p + 1;
			break;
		case '%':
			start = p;
			break;
		default:
			g_warning ("camel-imap-command is not printf. I don't "
				   "know what '%%%c' means.", *p);
			start = *p ? p + 1 : p;
			break;
		}
	}

	/* Now write out the string */
	outptr = out = g_malloc (len + 1);
	p = start = fmt;
	i = 0;
	while (*p) {
		p = strchr (start, '%');
		if (!p) {
			strcpy (outptr, start);
			break;
		} else {
			strncpy (outptr, start, p - start);
			outptr += p - start;
		}

		switch (*++p) {
		case 'd':
			num = GPOINTER_TO_INT (args->pdata[i++]);
			outptr += sprintf (outptr, "%d", num);
			break;

		case 's':
			string = args->pdata[i++];
			outptr += sprintf (outptr, "%s", string);
			break;
		case 'S':
		case 'F':
		case 'G':
			string = args->pdata[i++];
			if (imap_is_atom (string)) {
				outptr += sprintf (outptr, "%s", string);
			} else {
				len = strlen (string);
				if (len && store->capabilities & IMAP_CAPABILITY_LITERALPLUS) {
					outptr += sprintf (outptr, "{%d+}\r\n%s", len, string);
				} else {
					char *quoted = imap_quote_string (string);

					outptr += sprintf (outptr, "%s", quoted);
					g_free (quoted);
				}
			}

			if (*p == 'F' || *p == 'G')
				g_free (string);
			break;
		default:
			*outptr++ = '%';
			*outptr++ = *p;
		}

		start = *p ? p + 1 : p;
	}
	
	g_ptr_array_free (args, TRUE);
	
	return out;
}

static char *
imap_command_strdup_printf (CamelImapStore *store, const char *fmt, ...)
{
	va_list ap;
	char *result;
	
	va_start (ap, fmt);
	result = imap_command_strdup_vprintf (store, fmt, ap);
	va_end (ap);
	
	return result;
}
