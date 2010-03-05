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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <tny-mime-part.h>
#include <tny-camel-shared.h>
#include <tny-list.h>
#include <tny-camel-bs-msg.h>
#include <tny-camel-mem-stream.h>

#include <tny-stream-camel.h>
#include <tny-camel-stream.h>
#include <tny-camel-folder.h>
#include <tny-fs-stream.h>

static GObjectClass *parent_class = NULL;

#include "camel/camel-string-utils.h"
#include "camel/camel-data-wrapper.h"
#include "camel/camel-exception.h"
#include "camel/camel-mime-filter-basic.h"
#include "camel/camel-mime-filter-crlf.h"
#include "camel/camel-mime-filter-windows.h"
#include "camel/camel-private.h"
#include "camel/camel-stream-filter.h"
#include "camel/camel-stream.h"
#include "camel/camel-stream-mem.h"
#include "camel/camel-stream-null.h"
#include "camel/camel-mime-filter-charset.h"
#include "camel/camel-folder.h"

#include "tny-camel-queue-priv.h"
#include "tny-camel-folder-priv.h"
#include "tny-camel-account-priv.h"
#include "tny-camel-store-account-priv.h"
#include "tny-camel-bs-mime-part-priv.h"
#include "tny-camel-bs-msg-priv.h"
#include "tny-camel-bs-msg-header-priv.h"
#include "tny-session-camel-priv.h"

#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API



static ssize_t
decode_to_stream (CamelStream *from_stream, CamelStream *stream, const gchar *encoding, gboolean text)
{
	CamelMimeFilter *filter;
	CamelStream *fstream;
	ssize_t ret;
	CamelTransferEncoding etype;

	if (encoding) 
		etype = camel_transfer_encoding_from_string (encoding);
	else 
		etype = CAMEL_TRANSFER_ENCODING_DEFAULT;

	fstream = (CamelStream *) camel_stream_filter_new_with_stream (stream);

	switch (etype) {
	case CAMEL_TRANSFER_ENCODING_BASE64:
		filter = (CamelMimeFilter *) camel_mime_filter_basic_new_type (CAMEL_MIME_FILTER_BASIC_BASE64_DEC);
		camel_stream_filter_add (CAMEL_STREAM_FILTER (fstream), filter);
		camel_object_unref (filter);
		break;
	case CAMEL_TRANSFER_ENCODING_QUOTEDPRINTABLE:
		filter = (CamelMimeFilter *) camel_mime_filter_basic_new_type (CAMEL_MIME_FILTER_BASIC_QP_DEC);
		camel_stream_filter_add (CAMEL_STREAM_FILTER (fstream), filter);
		camel_object_unref (filter);
		break;
	case CAMEL_TRANSFER_ENCODING_UUENCODE:
		filter = (CamelMimeFilter *) camel_mime_filter_basic_new_type (CAMEL_MIME_FILTER_BASIC_UU_DEC);
		camel_stream_filter_add (CAMEL_STREAM_FILTER (fstream), filter);
		camel_object_unref (filter);
		break;
	default:
		break;
	}

	if (text) {
		filter = camel_mime_filter_crlf_new (CAMEL_MIME_FILTER_CRLF_DECODE,
						     CAMEL_MIME_FILTER_CRLF_MODE_CRLF_ONLY);
		camel_stream_filter_add (CAMEL_STREAM_FILTER (fstream), filter);
		camel_object_unref (filter);
	}

	camel_stream_reset (fstream);
	camel_stream_reset (from_stream);
	ret = camel_stream_write_to_stream (from_stream, fstream);

	camel_stream_flush (fstream);
	camel_object_unref (fstream);

	return ret;
}

static gssize
bs_camel_stream_format_text (CamelStream *from_stream, CamelStream *stream, const gchar *charset, const gchar *encoding)
{
	CamelStreamFilter *filter_stream;
	CamelMimeFilterCharset *filter;
	CamelMimeFilterWindows *windows = NULL;
	gssize bytes_written = -1;
		
	if (g_ascii_strncasecmp (charset, "iso-8859-", 9) == 0) {
		CamelStream *null;

		/* Since a few Windows mailers like to claim they sent
		* out iso-8859-# encoded text when they really sent
		* out windows-cp125#, do some simple sanity checking
		* before we move on... */

		null = camel_stream_null_new();
		filter_stream = camel_stream_filter_new_with_stream(null);
		camel_object_unref(null);

		windows = (CamelMimeFilterWindows *)camel_mime_filter_windows_new(charset);
		camel_stream_filter_add (filter_stream, (CamelMimeFilter *)windows);

		decode_to_stream (from_stream, (CamelStream *)filter_stream, encoding, TRUE);
		camel_stream_flush ((CamelStream *)filter_stream);
		camel_stream_reset (from_stream);
		camel_object_unref (filter_stream);

		charset = camel_mime_filter_windows_real_charset (windows);
	}

	filter_stream = camel_stream_filter_new_with_stream (stream);

	if ((filter = camel_mime_filter_charset_new_convert (charset, "UTF-8"))) {
		camel_stream_filter_add (filter_stream, (CamelMimeFilter *) filter);
		camel_object_unref (filter);
	}

	bytes_written = (gssize) decode_to_stream (from_stream, (CamelStream *)filter_stream, encoding, TRUE);
	camel_stream_flush ((CamelStream *)filter_stream);
	camel_object_unref (filter_stream);

	if (windows)
		camel_object_unref(windows);

	return bytes_written;
}

static gssize 
decode_from_stream_to (TnyMimePart *self, TnyStream *from_stream, TnyStream *stream, gboolean binary, gboolean decode_text)
{
	gssize bytes_written = -1;
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);

	if (decode_text &&
	    camel_strcase_equal (priv->bodystructure->content.type, "TEXT") &&
	    camel_strcase_equal (priv->bodystructure->content.subtype, "PLAIN")) 
	{
		gchar *encoding = NULL;
		gchar *charset = (gchar *) mimeparam_get_value_for (priv->bodystructure->content.params, "CHARSET");
		CamelStream *cfrom_stream = tny_stream_camel_new (from_stream);
		CamelStream *cto_stream = tny_stream_camel_new (stream);

		if (!binary)
			encoding = priv->bodystructure->encoding;

		if (!charset)
			charset = "UTF-8";

		bytes_written = (gssize) bs_camel_stream_format_text (cfrom_stream, cto_stream, charset, encoding);

		camel_object_unref (cfrom_stream);
		camel_object_unref (cto_stream);
	} else {
		if (binary)
			bytes_written = tny_stream_write_to_stream (from_stream, stream);
		else {
			CamelStream *cfrom_stream = tny_stream_camel_new (from_stream);
			CamelStream *cto_stream = tny_stream_camel_new (stream);
			gchar *encoding = priv->bodystructure->encoding;

			bytes_written = (gssize) decode_to_stream (cfrom_stream, cto_stream, encoding, FALSE);

			camel_object_unref (cfrom_stream);
			camel_object_unref (cto_stream);
		}
	}
	return bytes_written;
}

static gssize 
fetch_part (TnyMimePart *self, TnyStream *stream, gboolean decode_text)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	GError *err = NULL;
	gboolean binary = FALSE;
	TnyStream *from_stream;
	gssize return_value = 0;

	binary = !camel_strcase_equal (priv->bodystructure->content.type, "TEXT");
	from_stream = tny_camel_bs_msg_receive_strategy_start_receiving_part (
		priv->strat, priv->folder, TNY_CAMEL_BS_MIME_PART (self), &binary, &err);

	if (err) {
		g_warning ("Error while fetching part: %s", err->message);
		g_error_free (err);
		return_value = -1;
	} else if (from_stream)
		return_value = decode_from_stream_to (self, from_stream, stream, binary, decode_text);

	if (from_stream)
		g_object_unref (from_stream);


	return return_value;
}

static void 
tny_camel_bs_mime_part_set_header_pair (TnyMimePart *self, const gchar *name, const gchar *value)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->set_header_pair(self, name, value);
	return;
}

static void 
tny_camel_bs_mime_part_set_header_pair_default (TnyMimePart *self, const gchar *name, const gchar *value)
{
	g_warning ("Writing to this MIME part is not supported\n");
}

static void 
tny_camel_bs_mime_part_get_header_pairs (TnyMimePart *self, TnyList *list)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->get_header_pairs(self, list);
	return;
}

static gchar *
create_content_type_string (bodystruct_t *bs)
{
	GString *buffer;
	mimeparam_t *current;
	
	buffer = g_string_new ("");
	g_string_append_printf (buffer, "%s/%s", 
				bs->content.type,
				bs->content.subtype);
	buffer = g_string_ascii_down (buffer);

	for (current = bs->content.params; current != NULL; current = current->next) {
		gchar *down;
		buffer = g_string_append (buffer, "; ");
		down = g_ascii_strdown (current->name, -1);
		buffer = g_string_append (buffer, down);
		g_free (down);
		buffer = g_string_append (buffer, "=");
		buffer = g_string_append (buffer, current->value);
	}
 
	if (bs->content.cid != NULL ) {
		g_string_append_printf (buffer, "; cid=%s", bs->content.cid);
	}
	if (bs->content.md5 != NULL ) {
		g_string_append_printf (buffer, "; md5=%s", bs->content.md5);
	}
 
	return g_string_free (buffer, FALSE);
}
 
static gchar *
create_disposition_string (bodystruct_t *bs)
{
	GString *buffer;
	mimeparam_t *current;
 
	buffer = g_string_new (bs->disposition.type);
 
	for (current = bs->disposition.params; current != NULL; current = current->next) {
		gchar *down;
		buffer = g_string_append (buffer, "; ");
		down = g_ascii_strdown (current->name, -1);
		buffer = g_string_append (buffer, down);
		g_free (down);
		buffer = g_string_append (buffer, "=");
		buffer = g_string_append (buffer, current->value);
	}
 
	if (bs->octets > 0) {
		g_string_append_printf (buffer, "; size=%d", bs->octets);
	}
 
	return g_string_free (buffer, FALSE);
}

static void 
tny_camel_bs_mime_part_get_header_pairs_default (TnyMimePart *self, TnyList *list)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	CamelFolderPartState state;
	CamelFolder *cfolder = _tny_camel_folder_get_camel_folder (TNY_CAMEL_FOLDER (priv->folder));
	gchar *part_uid;

	if (priv->bodystructure && priv->bodystructure->parent) {
		/* this means we have to read the cached header of a subpart */
		part_uid = g_strconcat (priv->uid, "_", priv->bodystructure->part_spec, NULL);
	} else {
		part_uid = g_strdup (priv->uid);
	}
	gchar *pos_filename = camel_folder_get_cache_filename (cfolder, 
							       part_uid, "HEADER", &state);
	g_free (part_uid);
 
	if (pos_filename && !priv->parent && TNY_IS_MSG (self)) {
		int fd = open (pos_filename, O_RDONLY);
		if (fd != -1) {
			CamelMimeParser *mp;
			struct _camel_header_raw *headers;
			char *buf;
			size_t len;
			int err;

			mp = camel_mime_parser_new ();
			camel_mime_parser_init_with_fd (mp, fd);

			camel_mime_parser_step (mp, &buf, &len);
			headers = camel_mime_parser_headers_raw (mp);

			while (headers) {
				TnyPair *pair;
				pair = tny_pair_new (headers->name, headers->value);
				tny_list_append (list, (GObject *) pair);
				g_object_unref (pair);
				headers = headers->next;
			}

			err = camel_mime_parser_errno (mp);
			
			if (err != 0) {
				camel_object_unref (mp);
				mp = NULL;
			}
			
			camel_mime_parser_drop_step (mp);
                       
			close (fd);
		}
		g_free (pos_filename);
	} else {
		/* extract from bodystructure the retrieved headers */
		TnyPair *pair;
		gchar *disposition_string;
		gchar *content_type_string;
		
		if (priv->bodystructure->description) {
			pair = tny_pair_new ("Description", priv->bodystructure->description);
			tny_list_append (list, (GObject *) pair);
			g_object_unref (pair);
		}
		disposition_string = create_disposition_string (priv->bodystructure);
		if (disposition_string) {
			pair = tny_pair_new ("Content-Disposition", disposition_string);
			tny_list_append (list, (GObject *) pair);
			g_object_unref (pair);
			g_free (disposition_string);
		}
		content_type_string = create_content_type_string (priv->bodystructure);
		if (content_type_string) {
			pair = tny_pair_new ("Content-Type", content_type_string);
			tny_list_append (list, (GObject *) pair);
			g_object_unref (pair);
			g_free (content_type_string);
		}
		
	}

	return;
}

static void
tny_camel_bs_mime_part_get_parts (TnyMimePart *self, TnyList *list)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->get_parts(self, list);
	return;
}

static void
tny_camel_bs_mime_part_get_parts_default (TnyMimePart *self, TnyList *list)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	bodystruct_t *part = priv->bodystructure;

	g_mutex_lock (priv->part_lock);

	if (part->content.type && !strcasecmp (part->content.type, "multipart")) {
		part = part->subparts;
		while (part != NULL) {
			TnyMimePart *mpart;

			if (!strcasecmp (part->content.type, "message") && !strcasecmp (part->content.subtype, "rfc822")) {
				TnyHeader *header = _tny_camel_bs_msg_header_new (part->envelope, part->octets, 
										  mimeparam_get_value_for (part->content.params, "CHARSET"));
				mpart = (TnyMimePart *) _tny_camel_bs_msg_new (part->subparts,
					priv->uid, TNY_CAMEL_BS_MIME_PART (self));
				_tny_camel_bs_msg_set_header (TNY_CAMEL_BS_MSG (mpart), header);
				g_object_unref (header);
			} else 
				mpart =_tny_camel_bs_mime_part_new (part, priv->uid, TNY_CAMEL_BS_MIME_PART (self));

			_tny_camel_bs_mime_part_set_folder (TNY_CAMEL_BS_MIME_PART (mpart), priv->folder);
			_tny_camel_bs_mime_part_set_strat (TNY_CAMEL_BS_MIME_PART (mpart), priv->strat);

			tny_list_prepend (list, (GObject *) mpart);
			g_object_unref (mpart);
			part = part->next;
		}
	}

	g_mutex_unlock (priv->part_lock);

	return;
}



static gint
tny_camel_bs_mime_part_add_part (TnyMimePart *self, TnyMimePart *part)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->add_part(self, part);
}

static gint
tny_camel_bs_mime_part_add_part_default (TnyMimePart *self, TnyMimePart *part)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return -1;
}


static void 
tny_camel_bs_mime_part_del_part (TnyMimePart *self,  TnyMimePart *part)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->del_part(self, part);
	return;
}

static void 
tny_camel_bs_mime_part_del_part_default (TnyMimePart *self, TnyMimePart *part)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return;
}



static gboolean 
tny_camel_bs_mime_part_is_attachment (TnyMimePart *self)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->is_attachment(self);
}


static gboolean 
tny_camel_bs_mime_part_is_attachment_default (TnyMimePart *self)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	const gchar *contdisp = priv->bodystructure->disposition.type;

	/* Content-Disposition is excellent for this, of course (but we might
	 * not actually have this header, as not all E-mail clients add it) */

	if (contdisp) {
		if (camel_strstrcase (contdisp, "attachment"))
			return TRUE;
		if (camel_strstrcase (contdisp, "inline") && mimeparam_get_value_for (priv->bodystructure->disposition.params, "FILENAME") == NULL)
			return FALSE;
	}

	return !((camel_strcase_equal (priv->bodystructure->content.type, "application") && 
		 camel_strcase_equal (priv->bodystructure->content.subtype, "x-pkcs7-mime")) ||
		(camel_strcase_equal (priv->bodystructure->content.type, "application") && 
 		camel_strcase_equal (priv->bodystructure->content.subtype, "x-pkcs7-mime")) ||
		(camel_strcase_equal (priv->bodystructure->content.type, "application") && 
 		camel_strcase_equal (priv->bodystructure->content.subtype, "x-inlinepgp-signed")) ||
		(camel_strcase_equal (priv->bodystructure->content.type, "application") && 
 		camel_strcase_equal (priv->bodystructure->content.subtype, "x-inlinepgp-encrypted")) ||
		(mimeparam_get_value_for (priv->bodystructure->disposition.params, "FILENAME") == NULL));

}

static gssize
tny_camel_bs_mime_part_write_to_stream (TnyMimePart *self, TnyStream *stream, GError **err)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->write_to_stream(self, stream, err);
}

static gssize
tny_camel_bs_mime_part_write_to_stream_default (TnyMimePart *self, TnyStream *stream, GError **err)
{
	return fetch_part (self, stream, FALSE);
}



typedef struct {
	TnyCamelQueueable parent;

	GObject *self, *stream;
	TnyMimePartCallback callback;
	TnyStatusCallback status_callback;
	gpointer user_data;
	GError *err;
	gboolean binary;
	TnyStream *from_stream;
	TnyIdleStopper *stopper;
	TnySessionCamel *session;
	gboolean cancelled;
} DecodeAsyncInfo;


static void
decode_async_destroyer (gpointer user_data)
{
	DecodeAsyncInfo *info = (DecodeAsyncInfo *) user_data;
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (info->self);
	TnyCamelFolderPriv *fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (priv->folder);

	_tny_camel_folder_unreason (fpriv);

	g_object_unref (info->stream);
	g_object_unref (info->self);
	if (info->from_stream)
		g_object_unref (info->from_stream);
	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	
	return;
}

static gboolean
decode_async_callback (gpointer user_data)
{
	DecodeAsyncInfo *info = (DecodeAsyncInfo *) user_data;

	tny_lockable_lock (info->session->priv->ui_lock);

	if (info->from_stream) {
		decode_from_stream_to (TNY_MIME_PART (info->self), 
			TNY_STREAM (info->from_stream), TNY_STREAM (info->stream),
			 info->binary, TRUE);
	}

	if (info->callback) { 
		info->callback (TNY_MIME_PART (info->self),  
			info->cancelled, TNY_STREAM (info->stream), 
			info->err, info->user_data);
	}

	tny_lockable_unlock (info->session->priv->ui_lock);

	return FALSE;
}



static void
decode_async_cancelled_destroyer (gpointer user_data)
{
	DecodeAsyncInfo *info = (DecodeAsyncInfo *) user_data;
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (info->self);
	TnyCamelFolderPriv *fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (priv->folder);

	_tny_camel_folder_unreason (fpriv);
	g_object_unref (info->stream);
	g_object_unref (info->self);
	if (info->err)
		g_error_free (info->err);

	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	
	return;
}


static gboolean
decode_async_cancelled_callback (gpointer user_data)
{
	DecodeAsyncInfo *info = (DecodeAsyncInfo *) user_data;
	if (info->callback) { 
		tny_lockable_lock (info->session->priv->ui_lock);
		info->callback (TNY_MIME_PART (info->self), TRUE,
			TNY_STREAM (info->stream), info->err, 
			info->user_data);
		tny_lockable_unlock (info->session->priv->ui_lock);
	}
	return FALSE;
}

static void
decode_async_status (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *user_data)
{
	DecodeAsyncInfo *oinfo = user_data;
	TnyProgressInfo *info = NULL;

	info = tny_progress_info_new (G_OBJECT (oinfo->self), oinfo->status_callback, 
		TNY_FOLDER_STATUS, TNY_FOLDER_STATUS_CODE_GET_MSG, what, sofar, 
		oftotal, oinfo->stopper, oinfo->session->priv->ui_lock, oinfo->user_data);

	g_idle_add_full (G_PRIORITY_HIGH,
		tny_progress_info_idle_func, info,
		tny_progress_info_destroy);

	return;
}


static gpointer 
decode_async_thread (gpointer user_data)
{
	DecodeAsyncInfo *info = (DecodeAsyncInfo *) user_data;
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (info->self);
	CamelOperation *cancel;

	/* To disable parallel getting of messages while summary is being retreived,
	 * restore this lock (A) */
	/* g_static_rec_mutex_lock (priv->folder_lock); */

	cancel = camel_operation_new (decode_async_status, info);

	if (priv->folder && TNY_IS_CAMEL_FOLDER (priv->folder)) {
		TnyCamelFolderPriv *fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (priv->folder);
		if (fpriv->account && TNY_IS_CAMEL_ACCOUNT (fpriv->account)) {
			TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (fpriv->account);
			camel_operation_ref (cancel);
			apriv->getmsg_cancel = cancel;
		}
	}

	camel_operation_register (cancel);
	camel_operation_start (cancel, (char *) "Getting message part");

	info->binary = !camel_strcase_equal (priv->bodystructure->content.type, "TEXT");
	info->from_stream = tny_camel_bs_msg_receive_strategy_start_receiving_part (
		priv->strat, priv->folder, TNY_CAMEL_BS_MIME_PART (info->self), 
		&info->binary, &info->err);

	info->cancelled = camel_operation_cancel_check (cancel);

	if (info->err != NULL) {
		if (camel_strstrcase (info->err->message, "cancel") != NULL)
			info->cancelled = TRUE;
	}

	camel_operation_unregister (cancel);
	camel_operation_end (cancel);
	if (CAMEL_IS_OBJECT (cancel))
		camel_operation_unref (cancel);


	if (priv->folder && TNY_IS_CAMEL_FOLDER (priv->folder)) {
		TnyCamelFolderPriv *fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (priv->folder);
		if (fpriv->account && TNY_IS_CAMEL_ACCOUNT (fpriv->account)) {
			TnyCamelAccountPriv *apriv = TNY_CAMEL_ACCOUNT_GET_PRIVATE (fpriv->account);
			apriv->getmsg_cancel = NULL;
			camel_operation_unref (cancel);
		}
	}

	/* To disable parallel getting of messages while summary is being retreived,
	 * restore this lock (B) */
	/* g_static_rec_mutex_unlock (priv->folder_lock);  */

	return NULL;
}


static void
tny_camel_bs_mime_part_decode_to_stream_async (TnyMimePart *self, TnyStream *stream, TnyMimePartCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->decode_to_stream_async(self, stream, callback, status_callback, user_data);
	return;
}


static void
tny_camel_bs_mime_part_decode_to_stream_async_default (TnyMimePart *self, TnyStream *stream, TnyMimePartCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	DecodeAsyncInfo *info = g_slice_new0 (DecodeAsyncInfo);
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	TnyCamelFolderPriv *fpriv = TNY_CAMEL_FOLDER_GET_PRIVATE (priv->folder);

	info->err = NULL;
	info->session = TNY_FOLDER_PRIV_GET_SESSION (fpriv);
	info->self = g_object_ref (self);
	info->stream = g_object_ref (stream);
	info->callback = callback;
	info->status_callback = status_callback;
	info->user_data = user_data;
	info->stopper = tny_idle_stopper_new ();
	info->cancelled = FALSE;

	_tny_camel_folder_reason (fpriv);

	_tny_camel_queue_launch (TNY_FOLDER_PRIV_GET_QUEUE (fpriv), 
		decode_async_thread, 
		decode_async_callback,
		decode_async_destroyer, 
		decode_async_cancelled_callback,
		decode_async_cancelled_destroyer, 
		&info->cancelled,
		info, sizeof (DecodeAsyncInfo), __FUNCTION__);

	return;
}


static gssize
tny_camel_bs_mime_part_decode_to_stream (TnyMimePart *self, TnyStream *stream, GError **err)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->decode_to_stream(self, stream, err);
}

static gssize
tny_camel_bs_mime_part_decode_to_stream_default (TnyMimePart *self, TnyStream *stream, GError **err)
{
	return fetch_part (self, stream, TRUE);
}

static gint
tny_camel_bs_mime_part_construct (TnyMimePart *self, TnyStream *stream, const gchar *mime_type, const gchar *transfer_encoding)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->construct(self, stream, mime_type, transfer_encoding);
}

static gint
tny_camel_bs_mime_part_construct_default (TnyMimePart *self, TnyStream *stream, const gchar *mime_type, const gchar* transfer_encoding)
{
	return -1;
}

static const gchar* 
tny_camel_bs_mime_part_get_transfer_encoding (TnyMimePart *self)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->get_transfer_encoding(self);
}

static const gchar* 
tny_camel_bs_mime_part_get_transfer_encoding_default (TnyMimePart *self)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	return priv->bodystructure->encoding;
}

static void
tny_camel_bs_mime_part_set_transfer_encoding (TnyMimePart *self, const gchar *transfer_encoding)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->set_transfer_encoding(self, transfer_encoding);
}

static void
tny_camel_bs_mime_part_set_transfer_encoding_default (TnyMimePart *self, const gchar *transfer_encoding)
{
	return;
}

static TnyStream* 
tny_camel_bs_mime_part_get_stream (TnyMimePart *self)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->get_stream(self);
}

static TnyStream* 
tny_camel_bs_mime_part_get_stream_default (TnyMimePart *self)
{
	TnyStream *retval = tny_camel_mem_stream_new ();
	fetch_part (self, retval, FALSE);
	return retval;
}


static TnyStream* 
tny_camel_bs_mime_part_get_decoded_stream (TnyMimePart *self)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->get_decoded_stream(self);
}

static TnyStream* 
tny_camel_bs_mime_part_get_decoded_stream_default (TnyMimePart *self)
{
	TnyStream *retval = tny_camel_mem_stream_new ();
	fetch_part (self, retval, TRUE);
	tny_stream_reset (retval);
	return retval;
}

static const gchar* 
tny_camel_bs_mime_part_get_content_type (TnyMimePart *self)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->get_content_type(self);
}

static const gchar* 
tny_camel_bs_mime_part_get_content_type_default (TnyMimePart *self)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);

	if (!priv->cached_content_type) {
		gchar *tmp;
		g_mutex_lock (priv->part_lock);
		tmp = g_strdup_printf ("%s/%s",
				       priv->bodystructure->content.type,
				       priv->bodystructure->content.subtype);
		priv->cached_content_type = g_ascii_strdown (tmp, -1);
		g_free (tmp);
		g_mutex_unlock (priv->part_lock);
	}

	return priv->cached_content_type;
}

static gboolean
tny_camel_bs_mime_part_is_purged (TnyMimePart *self)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->is_purged(self);
}

static gboolean
tny_camel_bs_mime_part_is_purged_default (TnyMimePart *self)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	CamelFolderPartState state;
	CamelFolder *cfolder = _tny_camel_folder_get_camel_folder (TNY_CAMEL_FOLDER (priv->folder));
	gchar *pos_filename = camel_folder_get_cache_filename (cfolder, 
		priv->uid, priv->bodystructure->part_spec, &state);
	gboolean retval = FALSE;

	if (pos_filename) {
		retval = FALSE;
		g_free (pos_filename);
	}

	return retval;
}

static gboolean 
tny_camel_bs_mime_part_content_type_is (TnyMimePart *self, const gchar *type)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->content_type_is(self, type);
}

static gboolean 
tny_camel_bs_mime_part_content_type_is_default (TnyMimePart *self, const gchar *type)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	gchar *dup, *str1, *str2, *ptr;
	gboolean retval = FALSE;

	/* Whoooo, pointer hocus .. */

	dup = g_strdup (type);
	ptr = strchr (dup, '/');
	ptr++; str2 = g_strdup (ptr);
	ptr--; *ptr = '\0'; str1 = dup;

	/* pocus ! */

	if (!strcmp (str2, "*")) {
		if (camel_strcase_equal (priv->bodystructure->content.type, str1))
				retval = TRUE;
	} else {
		if (camel_strcase_equal (priv->bodystructure->content.type, str1) &&
			camel_strcase_equal (priv->bodystructure->content.subtype, str2))
				retval = TRUE;
	}

	g_free (dup);
	g_free (str2);

	return retval;
}


static const gchar*
tny_camel_bs_mime_part_get_filename (TnyMimePart *self)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->get_filename(self);
}

static const gchar*
tny_camel_bs_mime_part_get_filename_default (TnyMimePart *self)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);

	if (!priv->cached_filename) {
		const gchar *raw_value;
		const gchar *charset;

		g_mutex_lock (priv->part_lock);
		raw_value = mimeparam_get_value_for (priv->bodystructure->disposition.params, "FILENAME");
		charset = mimeparam_get_value_for (priv->bodystructure->content.params, "CHARSET");

		if (!raw_value) {
			priv->cached_filename = NULL;
		} else {
	
			if (charset && (g_ascii_strcasecmp(charset, "us-ascii") == 0))
				charset = NULL;

			charset = charset ? (const gchar *) e_iconv_charset_name (charset) : NULL;

			while (isspace ((unsigned) *raw_value))
				raw_value++;

			priv->cached_filename = camel_header_decode_string (raw_value, charset);
		}
		g_mutex_unlock (priv->part_lock);
	}

	return priv->cached_filename;
}

static const gchar*
tny_camel_bs_mime_part_get_content_id (TnyMimePart *self)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->get_content_id(self);
}

static const gchar*
tny_camel_bs_mime_part_get_content_id_default (TnyMimePart *self)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	const gchar *retval;

	g_mutex_lock (priv->part_lock);
	retval = priv->bodystructure->content.cid;
	g_mutex_unlock (priv->part_lock);

	return retval;
}

static const gchar*
tny_camel_bs_mime_part_get_description (TnyMimePart *self)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->get_description(self);
}

static const gchar*
tny_camel_bs_mime_part_get_description_default (TnyMimePart *self)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	const gchar *retval;

	g_mutex_lock (priv->part_lock);
	retval = priv->bodystructure->description;
	g_mutex_unlock (priv->part_lock);

	return retval;
}

static const gchar*
tny_camel_bs_mime_part_get_content_location (TnyMimePart *self)
{
	return TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->get_content_location(self);
}

static const gchar*
tny_camel_bs_mime_part_get_content_location_default (TnyMimePart *self)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	const gchar *retval;

	g_mutex_lock (priv->part_lock);
	retval = priv->bodystructure->content.loc;
	g_mutex_unlock (priv->part_lock);

	return retval;
}


static void 
tny_camel_bs_mime_part_set_content_location (TnyMimePart *self, const gchar *content_location)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->set_content_location(self, content_location);
	return;
}

static void 
tny_camel_bs_mime_part_set_content_location_default (TnyMimePart *self, const gchar *content_location)
{
	g_warning ("Writing to this MIME part is not supported\n");
}

static void 
tny_camel_bs_mime_part_set_description (TnyMimePart *self, const gchar *description)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->set_description(self, description);
	return;
}

static void 
tny_camel_bs_mime_part_set_description_default (TnyMimePart *self, const gchar *description)
{
	g_warning ("Writing to this MIME part is not supported\n");
}

static void 
tny_camel_bs_mime_part_set_content_id (TnyMimePart *self, const gchar *content_id)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->set_content_id(self, content_id);
	return;
}

static void 
tny_camel_bs_mime_part_set_content_id_default (TnyMimePart *self, const gchar *content_id)
{
	g_warning ("Writing to this MIME part is not supported\n");
}

static void 
tny_camel_bs_mime_part_set_filename (TnyMimePart *self, const gchar *filename)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->set_filename(self, filename);
	return;
}

static void 
tny_camel_bs_mime_part_set_filename_default (TnyMimePart *self, const gchar *filename)
{
	g_warning ("Writing to this MIME part is not supported\n");
}


static void 
tny_camel_bs_mime_part_set_content_type (TnyMimePart *self, const gchar *content_type)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->set_content_type(self, content_type);
	return;
}

static void
tny_camel_bs_mime_part_set_purged_default (TnyMimePart *self)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);
	CamelFolderPartState state;
	CamelFolder *cfolder = _tny_camel_folder_get_camel_folder (TNY_CAMEL_FOLDER (priv->folder));
	gchar *pos_filename = camel_folder_get_cache_filename (cfolder, 
		priv->uid, priv->bodystructure->part_spec, &state);

	if (pos_filename) {
		unlink (pos_filename);
		g_free (pos_filename);
	}

	return;
}

static void
tny_camel_bs_mime_part_set_purged (TnyMimePart *self)
{
	TNY_CAMEL_BS_MIME_PART_GET_CLASS (self)->set_purged(self);
	return;
}

static void 
tny_camel_bs_mime_part_set_content_type_default (TnyMimePart *self, const gchar *content_type)
{
	g_warning ("Writing to this MIME part is not supported\n");
	return;
}


void
_tny_camel_bs_mime_part_set_folder (TnyCamelBsMimePart *self, TnyFolder* folder)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);

	if (priv->folder)
		g_object_unref (priv->folder);
	priv->folder = folder;
	if (folder)
		g_object_ref (priv->folder);
	return;
}


void
_tny_camel_bs_mime_part_set_strat (TnyCamelBsMimePart *self, TnyCamelBsMsgReceiveStrategy* strat)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);

	if (priv->strat)
		g_object_unref (priv->strat);
	priv->strat = strat;
	if (strat)
		g_object_ref (priv->strat);
	return;
}

gboolean
tny_camel_bs_mime_part_is_fetched (TnyCamelBsMimePart *part)
{
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (part);
	CamelFolderPartState state;
	CamelFolder *cfolder = _tny_camel_folder_get_camel_folder (TNY_CAMEL_FOLDER (priv->folder));
	gchar *pos_filename = camel_folder_get_cache_filename (cfolder, 
		priv->uid, priv->bodystructure->part_spec, &state);

	if (pos_filename) {
		g_free (pos_filename);
		return TRUE;
	}

	return FALSE;
}


static void
tny_camel_bs_mime_part_finalize (GObject *object)
{
	TnyCamelBsMimePart *self = (TnyCamelBsMimePart*) object;
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);

	g_mutex_lock (priv->part_lock);
	if (priv->cached_content_type)
		g_free (priv->cached_content_type);
	priv->cached_content_type = NULL;
	if (priv->cached_filename)
		g_free (priv->cached_filename);
	priv->cached_filename = NULL;
	g_mutex_unlock (priv->part_lock);

	g_mutex_free (priv->part_lock);
	priv->part_lock = NULL;

	g_free (priv->uid);

	if (priv->parent)
		g_object_unref (priv->parent);

	if (priv->folder)
		g_object_unref (priv->folder);

	if (!priv->bodystructure->parent)
		bodystruct_free (priv->bodystructure);

	(*parent_class->finalize) (object);

	return;
}

TnyMimePart* 
_tny_camel_bs_mime_part_new (bodystruct_t *bodystructure, const gchar *uid, TnyCamelBsMimePart *parent)
{
	TnyCamelBsMimePart *self = g_object_new (TNY_TYPE_CAMEL_BS_MIME_PART, NULL);
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);

	if (parent)
		priv->parent = TNY_CAMEL_BS_MIME_PART (g_object_ref (parent));
	priv->bodystructure = bodystructure;
	priv->uid = g_strdup (uid);

	return TNY_MIME_PART (self);
}


static void
tny_mime_part_init (gpointer g, gpointer iface_data)
{
	TnyMimePartIface *klass = (TnyMimePartIface *)g;

	klass->content_type_is= tny_camel_bs_mime_part_content_type_is;
	klass->get_content_type= tny_camel_bs_mime_part_get_content_type;
	klass->get_stream= tny_camel_bs_mime_part_get_stream;
	klass->get_decoded_stream= tny_camel_bs_mime_part_get_decoded_stream;
	klass->write_to_stream= tny_camel_bs_mime_part_write_to_stream;
	klass->construct= tny_camel_bs_mime_part_construct;
	klass->get_filename= tny_camel_bs_mime_part_get_filename;
	klass->get_content_id= tny_camel_bs_mime_part_get_content_id;
	klass->get_description= tny_camel_bs_mime_part_get_description;
	klass->get_content_location= tny_camel_bs_mime_part_get_content_location;
	klass->is_purged= tny_camel_bs_mime_part_is_purged;
	klass->set_content_location= tny_camel_bs_mime_part_set_content_location;
	klass->set_description= tny_camel_bs_mime_part_set_description;
	klass->set_purged= tny_camel_bs_mime_part_set_purged;
	klass->set_content_id= tny_camel_bs_mime_part_set_content_id;
	klass->set_filename= tny_camel_bs_mime_part_set_filename;
	klass->set_content_type= tny_camel_bs_mime_part_set_content_type;
	klass->is_attachment= tny_camel_bs_mime_part_is_attachment;
	klass->decode_to_stream= tny_camel_bs_mime_part_decode_to_stream;
	klass->get_parts= tny_camel_bs_mime_part_get_parts;
	klass->add_part= tny_camel_bs_mime_part_add_part;
	klass->del_part= tny_camel_bs_mime_part_del_part;
	klass->get_header_pairs= tny_camel_bs_mime_part_get_header_pairs;
	klass->set_header_pair= tny_camel_bs_mime_part_set_header_pair;
	klass->decode_to_stream_async= tny_camel_bs_mime_part_decode_to_stream_async;
	klass->get_transfer_encoding= tny_camel_bs_mime_part_get_transfer_encoding;
	klass->set_transfer_encoding= tny_camel_bs_mime_part_set_transfer_encoding;

	return;
}


static void 
tny_camel_bs_mime_part_class_init (TnyCamelBsMimePartClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->content_type_is= tny_camel_bs_mime_part_content_type_is_default;
	class->get_content_type= tny_camel_bs_mime_part_get_content_type_default;
	class->get_stream= tny_camel_bs_mime_part_get_stream_default;
	class->get_decoded_stream= tny_camel_bs_mime_part_get_decoded_stream_default;
	class->write_to_stream= tny_camel_bs_mime_part_write_to_stream_default;
	class->construct= tny_camel_bs_mime_part_construct_default;
	class->get_filename= tny_camel_bs_mime_part_get_filename_default;
	class->get_content_id= tny_camel_bs_mime_part_get_content_id_default;
	class->get_description= tny_camel_bs_mime_part_get_description_default;
	class->get_content_location= tny_camel_bs_mime_part_get_content_location_default;
	class->is_purged= tny_camel_bs_mime_part_is_purged_default;
	class->set_purged= tny_camel_bs_mime_part_set_purged_default;
	class->set_content_location= tny_camel_bs_mime_part_set_content_location_default;
	class->set_description= tny_camel_bs_mime_part_set_description_default;
	class->set_content_id= tny_camel_bs_mime_part_set_content_id_default;
	class->set_filename= tny_camel_bs_mime_part_set_filename_default;
	class->set_content_type= tny_camel_bs_mime_part_set_content_type_default;
	class->is_attachment= tny_camel_bs_mime_part_is_attachment_default;
	class->decode_to_stream= tny_camel_bs_mime_part_decode_to_stream_default;
	class->get_parts= tny_camel_bs_mime_part_get_parts_default;
	class->add_part= tny_camel_bs_mime_part_add_part_default;
	class->del_part= tny_camel_bs_mime_part_del_part_default;
	class->get_header_pairs= tny_camel_bs_mime_part_get_header_pairs_default;
	class->set_header_pair= tny_camel_bs_mime_part_set_header_pair_default;
	class->decode_to_stream_async= tny_camel_bs_mime_part_decode_to_stream_async_default;
	class->get_transfer_encoding= tny_camel_bs_mime_part_get_transfer_encoding_default;
	class->set_transfer_encoding= tny_camel_bs_mime_part_set_transfer_encoding_default;

	object_class->finalize = tny_camel_bs_mime_part_finalize;

	g_type_class_add_private (object_class, sizeof (TnyCamelBsMimePartPriv));

	return;
}

static void
tny_camel_bs_mime_part_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelBsMimePart *self = (TnyCamelBsMimePart*)instance;
	TnyCamelBsMimePartPriv *priv = TNY_CAMEL_BS_MIME_PART_GET_PRIVATE (self);

	priv->folder = NULL;
	priv->uid = NULL;
	priv->cached_content_type = NULL;
	priv->cached_filename = NULL;
	priv->parent = NULL;
	priv->bodystructure = NULL;

	priv->part_lock = g_mutex_new ();

	return;
}

static gpointer 
tny_camel_bs_mime_part_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelBsMimePartClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_bs_mime_part_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelBsMimePart),
			0,      /* n_preallocs */
			tny_camel_bs_mime_part_instance_init,    /* instance_init */
			NULL
		};

	static const GInterfaceInfo tny_mime_part_info = 
		{
			(GInterfaceInitFunc) tny_mime_part_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelBsMimePart",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_MIME_PART, 
				     &tny_mime_part_info);

	return GSIZE_TO_POINTER (type);
}

/**
 * tny_camel_bs_mime_part_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_bs_mime_part_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_bs_mime_part_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
