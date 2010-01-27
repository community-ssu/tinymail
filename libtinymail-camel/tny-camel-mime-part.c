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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <errno.h>

#include <tny-mime-part.h>
#include <tny-camel-mime-part.h>
#include <tny-camel-stream.h>
#include <tny-stream-camel.h>
#include <camel/camel-stream-mem.h>
#include <camel/camel-data-wrapper.h>
#include <tny-camel-shared.h>
#include <tny-list.h>
#include <tny-simple-list.h>
#include <tny-camel-msg.h>
#include <tny-error.h>

static GObjectClass *parent_class = NULL;

#include "tny-camel-mime-part-priv.h"
#include "tny-camel-msg-header-priv.h"
#include "tny-camel-msg-priv.h"

#include <camel/camel-url.h>
#include <camel/camel-stream.h>
#include <camel/camel-stream-mem.h>
#include <camel/camel-multipart.h>
#include <camel/camel-multipart-encrypted.h>
#include <camel/camel-multipart-signed.h>
#include <camel/camel-medium.h>
#include <camel/camel-mime-message.h>
#include <camel/camel-gpg-context.h>
#include <camel/camel-smime-context.h>
#include <camel/camel-string-utils.h>
#include <camel/camel-stream-filter.h>
#include <camel/camel-stream-null.h>
#include <camel/camel-mime-filter-charset.h>
#include <camel/camel-mime-filter-windows.h>

#include <libedataserver/e-iconv.h>

static ssize_t camel_stream_format_text (CamelDataWrapper *dw, CamelStream *stream);




static void 
tny_camel_mime_part_set_header_pair (TnyMimePart *self, const gchar *name, const gchar *value)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->set_header_pair(self, name, value);
	return;
}

static void 
tny_camel_mime_part_set_header_pair_default (TnyMimePart *self, const gchar *name, const gchar *value)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	camel_medium_add_header (CAMEL_MEDIUM (priv->part), name, value);

	return;
}

static void 
tny_camel_mime_part_get_header_pairs (TnyMimePart *self, TnyList *list)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->get_header_pairs(self, list);
	return;
}

static void 
tny_camel_mime_part_get_header_pairs_default (TnyMimePart *self, TnyList *list)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	guint i = 0;
	GArray *headers = camel_medium_get_headers (CAMEL_MEDIUM (priv->part));
	
	for (i=0; i < headers->len; i++)
	{
		CamelMediumHeader *header = &g_array_index (headers, CamelMediumHeader, i);
		TnyPair *pair = tny_pair_new (header->name, header->value);
		tny_list_append (list, G_OBJECT (pair));
		g_object_unref (pair);
	}

	camel_medium_free_headers (CAMEL_MEDIUM (priv->part), headers);

	return;
}

static void
tny_camel_mime_part_get_parts (TnyMimePart *self, TnyList *list)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->get_parts(self, list);
	return;
}

static void
tny_camel_mime_part_get_parts_default (TnyMimePart *self, TnyList *list)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelDataWrapper *containee;
	gboolean is_related = FALSE;
	CamelContentType *content_type = NULL;
	gboolean has_attachments = FALSE;

	g_assert (TNY_IS_LIST (list));

	g_mutex_lock (priv->part_lock);

	containee = camel_medium_get_content_object (CAMEL_MEDIUM (priv->part));
	
	if (G_UNLIKELY (containee == NULL)) {
		g_mutex_unlock (priv->part_lock);
		return;
	}

	content_type = camel_mime_part_get_content_type (priv->part);
	if (content_type != NULL) {
		if ((content_type->type && strcmp (content_type->type, "multipart")==0) &&
		    (content_type->subtype && strcmp (content_type->subtype, "related") == 0)) {
			is_related = TRUE;
		}
	}

	if (CAMEL_IS_MULTIPART (containee))
	{
		guint i, parts = camel_multipart_get_number (CAMEL_MULTIPART (containee));
		for (i = 0; i < parts; i++) 
		{
			CamelMimePart *tpart = camel_multipart_get_part (CAMEL_MULTIPART (containee), i);
			TnyMimePart *newpart=NULL;
			CamelContentType *type;
			const gchar *disposition;

			if (!tpart || !CAMEL_IS_MIME_PART (tpart))
				continue;

			disposition = camel_mime_part_get_disposition (tpart);
			if (disposition && camel_strstrcase (disposition, "attachment") != NULL)
				has_attachments = TRUE;

			type = camel_mime_part_get_content_type (tpart);
			if (CAMEL_IS_MIME_MESSAGE (tpart))
			{
				TnyHeader *nheader = NULL;

				newpart = TNY_MIME_PART (tny_camel_msg_new ());
				_tny_camel_mime_part_set_part (TNY_CAMEL_MIME_PART (newpart), CAMEL_MIME_PART (tpart));

				nheader = _tny_camel_msg_header_new (CAMEL_MIME_MESSAGE (tpart), NULL, 
					camel_mime_message_get_date_received (CAMEL_MIME_MESSAGE (tpart), NULL));
				_tny_camel_msg_set_header (TNY_CAMEL_MSG (newpart), nheader);

				g_object_unref (nheader);
			}
			else if (type && camel_content_type_is (type, "message", "rfc822"))
			{
				CamelDataWrapper *c = camel_medium_get_content_object (CAMEL_MEDIUM (tpart));

				if (c && CAMEL_IS_MIME_PART (c) && CAMEL_IS_MIME_MESSAGE (c)) 
				{
					TnyHeader *nheader = NULL;

					newpart = TNY_MIME_PART (tny_camel_msg_new ());
					_tny_camel_mime_part_set_part (TNY_CAMEL_MIME_PART (newpart), CAMEL_MIME_PART (c));
					nheader = _tny_camel_msg_header_new (CAMEL_MIME_MESSAGE (c), NULL, 
						camel_mime_message_get_date_received (CAMEL_MIME_MESSAGE (c), NULL));
					_tny_camel_msg_set_header (TNY_CAMEL_MSG (newpart), nheader);

					g_object_unref (nheader);
				}

			} else {
				newpart = tny_camel_mime_part_new_with_part (tpart);
				if (is_related && (camel_mime_part_get_disposition (tpart) == NULL)) {
					camel_mime_part_set_disposition (tpart, "inline");
				}
			}

			tny_list_append (list, G_OBJECT (newpart));
			g_object_unref (G_OBJECT (newpart));
		}
	}

	if (TNY_IS_MSG (self)) {
		TnyHeader *header;

		header = tny_msg_get_header (TNY_MSG (self));
		if (header) {
			if (has_attachments && 
			    !(tny_header_get_flags (header) & TNY_HEADER_FLAG_ATTACHMENTS))
				tny_header_set_flag (header, TNY_HEADER_FLAG_ATTACHMENTS);
			g_object_unref (header);
		}
	}

	g_mutex_unlock (priv->part_lock);

	return;
}

typedef struct {
	GObject *self, *stream;
	TnyMimePartCallback callback;
	gpointer user_data;
	GError *err;
} DecodeAsyncInfo;


static void
decode_async_destroyer (gpointer user_data)
{
	DecodeAsyncInfo *info = (DecodeAsyncInfo *) user_data;
	/* thread reference */
	g_object_unref (info->self);
	g_object_unref (info->stream);
	if (info->err)
		g_error_free (info->err);
	g_slice_free (DecodeAsyncInfo, info);
	return;
}

static gboolean
decode_async_callback (gpointer user_data)
{
	DecodeAsyncInfo *info = (DecodeAsyncInfo *) user_data;
	if (info->callback) { 
		/* TODO: tny_lockable_lock (priv->ui_locker); */
		info->callback (TNY_MIME_PART (info->self), 
			FALSE, TNY_STREAM (info->stream), info->err, info->user_data);
		/* TODO: tny_lockable_unlock (priv->ui_locker); */
	}
	return FALSE;
}

static gpointer
decode_to_stream_async_thread (gpointer userdata)
{
	DecodeAsyncInfo *info = (DecodeAsyncInfo *) userdata;

	tny_mime_part_decode_to_stream (info->self, info->stream, &(info->err));

	g_idle_add_full (G_PRIORITY_HIGH, 
			 decode_async_callback, 
			 info, decode_async_destroyer);
	return NULL;

}

static void
tny_camel_mime_part_decode_to_stream_async (TnyMimePart *self, TnyStream *stream, TnyMimePartCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->decode_to_stream_async(self, stream, callback, status_callback, user_data);
	return;
}

static void
tny_camel_mime_part_decode_to_stream_async_default (TnyMimePart *self, TnyStream *stream, TnyMimePartCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
	DecodeAsyncInfo *info = g_slice_new0 (DecodeAsyncInfo);

	info->self = g_object_ref (self);
	info->stream = g_object_ref (stream);
	info->callback = callback;
	info->user_data = user_data;
	info->err = NULL;

	if (g_thread_create (decode_to_stream_async_thread, info, FALSE, &(info->err)) == FALSE) {
		g_critical ("%s failed: couldn't create the working thread", __FUNCTION__);
	}
		
	return;
}


static gint
tny_camel_mime_part_add_part (TnyMimePart *self, TnyMimePart *part)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->add_part(self, part);
}

static TnyMimePart * 
recreate_part (TnyMimePart *orig)
{
	TnyMimePart *retval, *piece;
	gboolean piece_needs_unref = FALSE;
	TnyList *list = tny_simple_list_new ();
	TnyIterator *iter;
	const gchar *type = tny_mime_part_get_content_type (orig);
	TnyStream *in_stream = tny_mime_part_get_decoded_stream (orig);
/*
	TnyList *header_pairs = tny_simple_list_new ();
*/
	if (TNY_IS_MSG (orig)) {
		TnyHeader *hdr = NULL;
		TnyHeader *dest_header;
		gchar *str;

		hdr = tny_msg_get_header (TNY_MSG (orig));
		retval = TNY_MIME_PART (tny_camel_msg_new ());
 		dest_header = tny_msg_get_header (TNY_MSG (retval));
		if (str = tny_header_dup_bcc (hdr)) {
			tny_header_set_bcc (dest_header, str);
			g_free (str);
		}
		if (str = tny_header_dup_cc (hdr)) {
			tny_header_set_cc (dest_header, str);
			g_free (str);
		}
		if (str = tny_header_dup_from (hdr)) {
			tny_header_set_from (dest_header, str);
			g_free (str);
		}
		if (str = tny_header_dup_replyto (hdr)) {
			tny_header_set_replyto (dest_header, str);
			g_free (str);
		}
		if (str = tny_header_dup_subject (hdr)) {
			tny_header_set_subject (dest_header, str);
			g_free (str);
		}
		if (str = tny_header_dup_to (hdr)) {
			tny_header_set_to (dest_header, str);
			g_free (str);
		}
		/* tny_header_set_priority (dest_header, tny_header_get_priority (hdr)); */
		g_object_unref (hdr);
		g_object_unref (dest_header);
		piece = tny_camel_mime_part_new ();
		piece_needs_unref = TRUE;
		if (!g_ascii_strcasecmp (type, "message/rfc822"))
			type = NULL;
	} else {
		piece = tny_camel_mime_part_new ();
		retval = piece;
	}

	tny_mime_part_construct (piece, in_stream, type, tny_mime_part_get_transfer_encoding (orig));

	if (tny_mime_part_get_description (orig))
		tny_mime_part_set_description (piece, tny_mime_part_get_description (orig));

	tny_mime_part_set_content_id (piece, tny_mime_part_get_content_id (orig));

	tny_mime_part_set_transfer_encoding (piece, tny_mime_part_get_transfer_encoding (orig));

	if (tny_mime_part_get_content_location (orig))
		tny_mime_part_set_content_location (piece, tny_mime_part_get_content_location (orig));

	if (tny_mime_part_is_attachment (orig))
		tny_mime_part_set_filename (piece, tny_mime_part_get_filename (orig));
/*
	tny_mime_part_get_header_pairs (orig, header_pairs);
	iter = tny_list_create_iterator (header_pairs);
	while (!tny_iterator_is_done (iter)) {
		TnyPair *pair = TNY_PAIR (tny_iterator_get_current (iter));
		tny_mime_part_set_header_pair (piece, 
			tny_pair_get_name (pair), 
			tny_pair_get_value (pair));
		g_object_unref (pair);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_object_unref (header_pairs);
*/

	g_object_unref (in_stream);

	tny_mime_part_get_parts (orig, list);
	iter = tny_list_create_iterator (list);
	while (!tny_iterator_is_done (iter)) {
		TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
		TnyMimePart *add_part;

		add_part = recreate_part (part);

		tny_mime_part_add_part (piece, add_part);
		g_object_unref (add_part);

		g_object_unref (part);
		tny_iterator_next (iter);
	}
	g_object_unref (iter);
	g_object_unref (list);

	if (piece_needs_unref) {
		tny_mime_part_add_part (retval, piece);
		g_object_unref (piece);
	}

	return retval;
}

static const gchar* 
tny_camel_mime_part_get_transfer_encoding (TnyMimePart *self)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->get_transfer_encoding(self);
}

static const gchar* 
tny_camel_mime_part_get_transfer_encoding_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelTransferEncoding encoding =  camel_mime_part_get_encoding (priv->part);
	const char *text = camel_transfer_encoding_to_string (encoding);
	return (const gchar *) text;
}

static gint
tny_camel_mime_part_add_part_default (TnyMimePart *self, TnyMimePart *part)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelMedium *medium;
	CamelDataWrapper *containee;
	CamelMultipart *body;
	gint curl = 0, retval = 0;
	TnyMimePart *actual_part = part;
	CamelMimePart *cpart;

	g_assert (TNY_IS_MIME_PART (part));

	if (!TNY_IS_CAMEL_MIME_PART (part))
		actual_part = recreate_part (part);
	else 
		g_object_ref (actual_part);

	g_mutex_lock (priv->part_lock);

	medium = CAMEL_MEDIUM (priv->part);
	containee = camel_medium_get_content_object (medium);

	/* Warp it into a multipart */
	if (!containee || !CAMEL_IS_MULTIPART (containee))
	{
		CamelContentType *type;
		gchar *applied_type = NULL;

		/* camel_medium_set_content_object does this ...
		if (containee)
			camel_object_unref (containee); */

		curl = 0;
		type = camel_mime_part_get_content_type (priv->part);

		if (type && !g_ascii_strcasecmp (type->type, "multipart"))
			applied_type = g_strdup_printf ("%s/%s", type->type, type->subtype);
		else
			applied_type = g_strdup ("multipart/mixed");

		body = camel_multipart_new ();
		camel_data_wrapper_set_mime_type (CAMEL_DATA_WRAPPER (body),
						applied_type);
		g_free (applied_type);
		camel_multipart_set_boundary (body, NULL);
		camel_medium_set_content_object (medium, CAMEL_DATA_WRAPPER (body));
		camel_object_unref (body);
	} else
		body = CAMEL_MULTIPART (containee);

	cpart = tny_camel_mime_part_get_part (TNY_CAMEL_MIME_PART (actual_part));

	/* Generate a content-id */
	camel_mime_part_set_content_id (cpart, NULL);

	if (cpart && CAMEL_IS_MIME_MESSAGE (cpart)) {
		CamelMimePart *message_part = camel_mime_part_new ();
		const gchar *subject;
		gchar *description;
		gboolean freedescup = FALSE;

		subject = camel_mime_message_get_subject (CAMEL_MIME_MESSAGE (cpart));

		if (subject) {
			freedescup = TRUE;
			description = g_strdup_printf ("Forwarded message: %s", subject);
		} else
			description = "Forwarded message";

		camel_mime_part_set_description (message_part, description);

		if (freedescup)
			g_free (description);

		camel_mime_part_set_disposition (message_part, "inline");
		camel_medium_set_content_object (CAMEL_MEDIUM (message_part), 
						 CAMEL_DATA_WRAPPER (cpart));
		camel_mime_part_set_content_type (message_part, "message/rfc822");
		camel_multipart_add_part (body, message_part);
		camel_object_unref (message_part);
	} else if (cpart)
		camel_multipart_add_part (body, cpart);

	if (cpart)
		camel_object_unref (cpart);

	retval = camel_multipart_get_number (body);

	g_mutex_unlock (priv->part_lock);

	g_object_unref (actual_part);

	return retval;
}

/* TODO: camel_mime_message_set_date(msg, time(0), 930); */

static void 
tny_camel_mime_part_del_part (TnyMimePart *self,  TnyMimePart *part)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->del_part(self, part);
	return;
}

static void 
tny_camel_mime_part_del_part_default (TnyMimePart *self, TnyMimePart *part)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelDataWrapper *containee;
	CamelMimePart *cpart;

	/* Yes, indeed (I don't yet support non TnyCamelMimePart mime part 
	   instances, and I know I should. Feel free to implement the copying
	   if you really need it) */

	g_assert (TNY_IS_CAMEL_MIME_PART (part));

	g_mutex_lock (priv->part_lock);

	containee = camel_medium_get_content_object (CAMEL_MEDIUM (priv->part));

	if (containee && CAMEL_IS_MULTIPART (containee))
	{
		cpart = tny_camel_mime_part_get_part (TNY_CAMEL_MIME_PART (part));
		camel_multipart_remove_part (CAMEL_MULTIPART (containee), cpart);
		camel_object_unref (CAMEL_OBJECT (cpart));
	}

	g_mutex_unlock (priv->part_lock);

	return;
}



static gboolean 
tny_camel_mime_part_is_attachment (TnyMimePart *self)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->is_attachment(self);
}


static gboolean 
tny_camel_mime_part_is_attachment_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelDataWrapper *dw = NULL;
	CamelMedium *medium = (CamelMedium *)priv->part;
	const gchar *contdisp = camel_medium_get_header (medium, "content-disposition");

	/* Content-Disposition is excellent for this, of course (but we might
	 * not actually have this header, as not all E-mail clients add it) */

	if (contdisp) {
		if (camel_strstrcase (contdisp, "attachment"))
			return TRUE;
		if (camel_strstrcase (contdisp, "inline") && (camel_strstrcase (contdisp, "filename=") == NULL))
			return FALSE;
	}

	/* Check the old fashioned way */
	dw = camel_medium_get_content_object(medium);

	if (dw) {
		return !(camel_content_type_is(dw->mime_type, "application", "x-pkcs7-mime")
			 || camel_content_type_is(dw->mime_type, "application", "pkcs7-mime")
			 || camel_content_type_is(dw->mime_type, "application", "x-inlinepgp-signed")
			 || camel_content_type_is(dw->mime_type, "application", "x-inlinepgp-encrypted")
			 || (camel_mime_part_get_filename(priv->part) == NULL));
	}

	return FALSE;
}

static gssize
tny_camel_mime_part_write_to_stream (TnyMimePart *self, TnyStream *stream, GError **err)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->write_to_stream(self, stream, err);
}

static gssize
tny_camel_mime_part_write_to_stream_default (TnyMimePart *self, TnyStream *stream, GError **err)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelDataWrapper *wrapper;
	CamelMedium *medium;
	CamelStream *cstream;
	gssize bytes = -1;

	g_assert (TNY_IS_STREAM (stream));

	cstream = tny_stream_camel_new (stream);

	g_mutex_lock (priv->part_lock);
	medium = CAMEL_MEDIUM (priv->part);
	camel_object_ref (medium);
	g_mutex_unlock (priv->part_lock);

	wrapper = camel_medium_get_content_object (medium);

	if (!wrapper) {
		g_warning (_("Mime part does not yet have a source stream, use "
			     "tny_mime_part_construct first"));
		camel_object_unref (cstream);
		g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_MIME_ERROR_STATE,
				_("Mime part does not yet have a source stream, use "
				"tny_mime_part_construct first"));
		return bytes;
	}

	/* This should work but doesn't . . .
	camel_data_wrapper_write_to_stream (wrapper, cstream); */

	camel_stream_reset (wrapper->stream);
	bytes = (gssize) camel_stream_write_to_stream (wrapper->stream, cstream);

	camel_object_unref (cstream);
	camel_object_unref (medium);

	if (bytes < 0) {
		g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_IO_ERROR_WRITE,
				strerror (errno));
	}
	
	return bytes;
}



static ssize_t
camel_stream_format_text (CamelDataWrapper *dw, CamelStream *stream)
{
	/* Stolen from evolution, evil evil me!! moehahah */

	CamelStreamFilter *filter_stream;
	CamelMimeFilterCharset *filter;
	const char *charset = "UTF-8"; /* I default to UTF-8, like it or not */
	CamelMimeFilterWindows *windows = NULL;
	ssize_t bytes = -1;

	if (dw->mime_type && (charset = camel_content_type_param 
			(dw->mime_type, "charset")) && 
		g_ascii_strncasecmp(charset, "iso-8859-", 9) == 0) 
	{
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
		camel_data_wrapper_decode_to_stream (dw, (CamelStream *)filter_stream);
		camel_stream_flush ((CamelStream *)filter_stream);
		camel_object_unref (filter_stream);
		charset = camel_mime_filter_windows_real_charset (windows);
	}

	filter_stream = camel_stream_filter_new_with_stream (stream);

	if ((filter = camel_mime_filter_charset_new_convert (charset, "UTF-8"))) {
		camel_stream_filter_add (filter_stream, (CamelMimeFilter *) filter);
		camel_object_unref (filter);
	}

	bytes = camel_data_wrapper_decode_to_stream (dw, (CamelStream *)filter_stream);
	camel_stream_flush ((CamelStream *)filter_stream);
	camel_object_unref (filter_stream);

	if (windows)
		camel_object_unref(windows);

	return bytes;
}

static gssize
tny_camel_mime_part_decode_to_stream (TnyMimePart *self, TnyStream *stream, GError **err)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->decode_to_stream(self, stream, err);
}

static gssize
tny_camel_mime_part_decode_to_stream_default (TnyMimePart *self, TnyStream *stream, GError **err)
{

	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelDataWrapper *wrapper;
	CamelMedium *medium;
	CamelStream *cstream;
	gssize bytes = -1;

	g_assert (TNY_IS_STREAM (stream));

	cstream = tny_stream_camel_new (stream);

	g_mutex_lock (priv->part_lock);
	medium = CAMEL_MEDIUM (priv->part);
	camel_object_ref (CAMEL_OBJECT (medium));
	g_mutex_unlock (priv->part_lock);

	wrapper = camel_medium_get_content_object (medium);
	camel_object_ref (wrapper);

	if (G_UNLIKELY (!wrapper)) {
		g_warning (_("Mime part does not yet have a source stream, use "
			     "tny_mime_part_construct first"));
		camel_object_unref (CAMEL_OBJECT (cstream));
		return -1;
	}
	
	if (camel_content_type_is (wrapper->mime_type, "text", "plain"))
		bytes = (gssize) camel_stream_format_text (wrapper, cstream);
	else
		bytes = (gssize) camel_data_wrapper_decode_to_stream (wrapper, cstream);

	camel_object_unref (wrapper);
	camel_object_unref (cstream);
	camel_object_unref (medium);
	
	if (bytes < 0) {
		g_set_error (err, TNY_ERROR_DOMAIN,
				TNY_IO_ERROR_WRITE,
				strerror (errno));
	}

	return bytes;
}

static gint
tny_camel_mime_part_construct (TnyMimePart *self, TnyStream *stream, const gchar *mime_type, const gchar *transfer_encoding)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->construct(self, stream, mime_type, transfer_encoding);
}

static void 
tny_camel_mime_part_set_transfer_encoding (TnyMimePart *self, const gchar *transfer_encoding)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->set_transfer_encoding(self, transfer_encoding);
}

static void 
tny_camel_mime_part_set_transfer_encoding_default (TnyMimePart *self, const gchar *transfer_encoding)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelTransferEncoding encoding;
	encoding = camel_transfer_encoding_from_string (transfer_encoding);
	camel_mime_part_set_encoding (priv->part, encoding);
	return;
}

static gint
tny_camel_mime_part_construct_default (TnyMimePart *self, TnyStream *stream, const gchar *mime_type, const gchar *transfer_encoding)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelDataWrapper *wrapper;
	gint retval = -1;
	CamelMedium *medium;
	CamelStream *cstream;
	CamelTransferEncoding encoding;

	g_assert (TNY_IS_STREAM (stream));

	cstream = tny_stream_camel_new (stream);

	g_mutex_lock (priv->part_lock);

	encoding = camel_transfer_encoding_from_string (transfer_encoding);
	camel_mime_part_set_encoding (priv->part, encoding);

	medium = CAMEL_MEDIUM (priv->part);
	camel_object_ref (CAMEL_OBJECT (medium));
	g_mutex_unlock (priv->part_lock);

	wrapper = camel_medium_get_content_object (medium);

	if (wrapper)
		camel_object_unref (CAMEL_OBJECT (wrapper));

	if (mime_type && g_ascii_strcasecmp (mime_type, "message/rfc822") == 0)
		wrapper = (CamelDataWrapper *) camel_mime_message_new ();
	else
		wrapper = camel_data_wrapper_new ();

	retval = camel_data_wrapper_construct_from_stream (wrapper, cstream);
	if (mime_type)
		camel_data_wrapper_set_mime_type (wrapper, mime_type);

	camel_medium_set_content_object(medium, wrapper);

	camel_mime_part_set_content_id (priv->part, NULL);

	camel_object_unref (cstream);
	camel_object_unref (medium);
	camel_object_unref (wrapper);

	return retval;
}

static TnyStream* 
tny_camel_mime_part_get_stream (TnyMimePart *self)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->get_stream(self);
}

static TnyStream* 
tny_camel_mime_part_get_stream_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	TnyStream *retval = NULL;
	CamelDataWrapper *wrapper;
	CamelMedium *medium;
	CamelStream *stream = camel_stream_mem_new ();

	g_mutex_lock (priv->part_lock);
	medium =  CAMEL_MEDIUM (priv->part);
	camel_object_ref (medium);
	g_mutex_unlock (priv->part_lock);

	wrapper = camel_medium_get_content_object (medium);

	if (!wrapper) {
		wrapper = camel_data_wrapper_new (); 
		camel_medium_set_content_object (medium, wrapper);
		camel_object_unref (wrapper);
	} 

	if (wrapper->stream) {
		camel_stream_reset (wrapper->stream);
		camel_stream_write_to_stream (wrapper->stream, stream);
	}

	retval = TNY_STREAM (tny_camel_stream_new (stream));
	camel_object_unref (stream);

	tny_stream_reset (retval);
	camel_object_unref (medium);

	return retval;
}




static TnyStream* 
tny_camel_mime_part_get_decoded_stream (TnyMimePart *self)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->get_decoded_stream(self);
}

static TnyStream* 
tny_camel_mime_part_get_decoded_stream_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	TnyStream *retval = NULL;
	CamelDataWrapper *wrapper;
	CamelMedium *medium;
	CamelStream *stream = camel_stream_mem_new ();
	gssize bytes = -1;

	g_mutex_lock (priv->part_lock);
	medium =  CAMEL_MEDIUM (priv->part);
	camel_object_ref (medium);
	g_mutex_unlock (priv->part_lock);

	wrapper = camel_medium_get_content_object (medium);

	if (!wrapper) {
		wrapper = camel_data_wrapper_new (); 
		camel_medium_set_content_object (medium, wrapper);
		camel_object_unref (wrapper);
	} 

	if (wrapper->stream) {
		camel_stream_reset (wrapper->stream);

		if (camel_content_type_is (wrapper->mime_type, "text", "plain"))
			bytes = camel_stream_format_text (wrapper, stream);
		else
			bytes = camel_data_wrapper_decode_to_stream (wrapper, stream);
	}

	if (bytes >= 0) {
		retval = TNY_STREAM (tny_camel_stream_new (stream));
		tny_stream_reset (retval);
	}

	camel_object_unref (stream);
	camel_object_unref (medium);

	return retval;
}


static const gchar* 
tny_camel_mime_part_get_content_type (TnyMimePart *self)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->get_content_type(self);
}

static const gchar* 
tny_camel_mime_part_get_content_type_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	if (!priv->cached_content_type) {
		CamelContentType *type;
		g_mutex_lock (priv->part_lock);
		type = camel_mime_part_get_content_type (priv->part);
		if (type)
			priv->cached_content_type = g_strdup_printf ("%s/%s", type->type, type->subtype);
		g_mutex_unlock (priv->part_lock);
	}

	return priv->cached_content_type;
}

static gboolean
tny_camel_mime_part_is_purged (TnyMimePart *self)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->is_purged(self);
}

static gboolean
tny_camel_mime_part_is_purged_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	const gchar *disposition = camel_mime_part_get_disposition (priv->part);
	return (disposition != NULL) && (!strcmp (disposition, "purged"));
}

static gboolean 
tny_camel_mime_part_content_type_is (TnyMimePart *self, const gchar *type)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->content_type_is(self, type);
}

static gboolean 
tny_camel_mime_part_content_type_is_default (TnyMimePart *self, const gchar *type)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelContentType *ctype;
	gchar *dup, *str1, *str2, *ptr;
	gboolean retval = FALSE;

	g_mutex_lock (priv->part_lock);
	ctype = camel_mime_part_get_content_type (priv->part);
	g_mutex_unlock (priv->part_lock);

	if (!ctype)
		return FALSE;

	/* Whoooo, pointer hocus .. */

	dup = g_strdup (type);
	ptr = strchr (dup, '/');
	ptr++; str2 = g_strdup (ptr);
	ptr--; *ptr = '\0'; str1 = dup;

	/* pocus ! */

	retval = camel_content_type_is (ctype, (const char *)str1, 
			(const char *) str2);

	g_free (dup);
	g_free (str2);

	return retval;
}


void
_tny_camel_mime_part_set_part (TnyCamelMimePart *self, CamelMimePart *part)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	g_mutex_lock (priv->part_lock);

	if (priv->cached_filename)
		g_free (priv->cached_filename);
	priv->cached_filename = NULL;

	if (priv->cached_content_type)
		g_free (priv->cached_content_type);
	priv->cached_content_type = NULL;

	if (priv->part)
		camel_object_unref (priv->part);

	camel_object_ref (part);
	priv->part = part;

	g_mutex_unlock (priv->part_lock);

	return;
}

/**
 * tny_camel_mime_part_get_part:
 * @self: The #TnyCamelMimePart instance
 * 
 * Get the #CamelMimePart instance that is being proxied by @self.
 *
 * Return value: The #CamelMimePart instance
 **/
CamelMimePart*
tny_camel_mime_part_get_part (TnyCamelMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	CamelMimePart *retval;

	g_mutex_lock (priv->part_lock);
	retval = priv->part;
	if (retval)
		camel_object_ref (retval);
	g_mutex_unlock (priv->part_lock);

	return retval;
}


static const gchar*
tny_camel_mime_part_get_filename (TnyMimePart *self)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->get_filename(self);
}

static const gchar*
tny_camel_mime_part_get_filename_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	g_mutex_lock (priv->part_lock);
	if (!priv->cached_filename) {
		const char *str = camel_mime_part_get_filename (priv->part);

		if (str) {
			if (!g_utf8_validate (str, -1, NULL))
				priv->cached_filename = _tny_camel_decode_raw_header (priv->part, str, FALSE);
			else
				priv->cached_filename = g_strdup (str);
		}
	}
	g_mutex_unlock (priv->part_lock);

	return priv->cached_filename;
}

static const gchar*
tny_camel_mime_part_get_content_id (TnyMimePart *self)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->get_content_id(self);
}

static const gchar*
tny_camel_mime_part_get_content_id_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	const gchar *retval;

	g_mutex_lock (priv->part_lock);
	retval = camel_mime_part_get_content_id (priv->part);
	g_mutex_unlock (priv->part_lock);

	return retval;
}

static const gchar*
tny_camel_mime_part_get_description (TnyMimePart *self)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->get_description(self);
}

static const gchar*
tny_camel_mime_part_get_description_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	const gchar *retval;

	g_mutex_lock (priv->part_lock);
	retval = camel_mime_part_get_description (priv->part);
	g_mutex_unlock (priv->part_lock);

	return retval;
}

static const gchar*
tny_camel_mime_part_get_content_location (TnyMimePart *self)
{
	return TNY_CAMEL_MIME_PART_GET_CLASS (self)->get_content_location(self);
}

static const gchar*
tny_camel_mime_part_get_content_location_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	const gchar *retval;

	g_mutex_lock (priv->part_lock);
	retval = camel_mime_part_get_content_location (priv->part);
	g_mutex_unlock (priv->part_lock);

	return retval;
}


static void 
tny_camel_mime_part_set_content_location (TnyMimePart *self, const gchar *content_location)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->set_content_location(self, content_location);
	return;
}

static void 
tny_camel_mime_part_set_content_location_default (TnyMimePart *self, const gchar *content_location)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	g_mutex_lock (priv->part_lock);
	camel_mime_part_set_content_location (priv->part, content_location);
	g_mutex_unlock (priv->part_lock);

	return;
}

static void 
tny_camel_mime_part_set_description (TnyMimePart *self, const gchar *description)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->set_description(self, description);
	return;
}

static void 
tny_camel_mime_part_set_description_default (TnyMimePart *self, const gchar *description)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	g_mutex_lock (priv->part_lock);
	camel_mime_part_set_description (priv->part, description);
	g_mutex_unlock (priv->part_lock);

	return;
}

static void 
tny_camel_mime_part_set_content_id (TnyMimePart *self, const gchar *content_id)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->set_content_id(self, content_id);
	return;
}

static void 
tny_camel_mime_part_set_content_id_default (TnyMimePart *self, const gchar *content_id)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	g_mutex_lock (priv->part_lock);
	camel_mime_part_set_content_id (priv->part, content_id);
	g_mutex_unlock (priv->part_lock);

	return;
}

static void 
tny_camel_mime_part_set_filename (TnyMimePart *self, const gchar *filename)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->set_filename(self, filename);
	return;
}

static void 
tny_camel_mime_part_set_filename_default (TnyMimePart *self, const gchar *filename)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	g_mutex_lock (priv->part_lock);
	camel_mime_part_set_filename (priv->part, filename);
	g_mutex_unlock (priv->part_lock);
	return;
}


static void 
tny_camel_mime_part_set_content_type (TnyMimePart *self, const gchar *content_type)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->set_content_type(self, content_type);
	return;
}

static void
tny_camel_mime_part_set_purged_default (TnyMimePart *self)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);
	const gchar *tmp;
	gchar *filename = NULL;
	g_mutex_lock (priv->part_lock);
	tmp = camel_mime_part_get_filename (priv->part);
	if (tmp != NULL)
		filename = g_strdup (tmp);
	camel_mime_part_set_disposition (priv->part, "purged");
	if (filename) {
		camel_mime_part_set_filename (priv->part, filename);
		g_free (filename);
	}
	g_mutex_unlock (priv->part_lock);
	return;
}

static void
tny_camel_mime_part_set_purged (TnyMimePart *self)
{
	TNY_CAMEL_MIME_PART_GET_CLASS (self)->set_purged(self);
	return;
}

static void 
tny_camel_mime_part_set_content_type_default (TnyMimePart *self, const gchar *content_type)
{
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	g_assert (CAMEL_IS_MEDIUM (priv->part));

	g_mutex_lock (priv->part_lock);
	camel_mime_part_set_content_type (priv->part, content_type);

	if (priv->cached_filename)
		g_free (priv->cached_filename);
	priv->cached_filename = NULL;

	if (priv->cached_content_type)
		g_free (priv->cached_content_type);
	priv->cached_content_type = NULL;

	g_mutex_unlock (priv->part_lock);

	return;
}

static void
tny_camel_mime_part_finalize (GObject *object)
{
	TnyCamelMimePart *self = (TnyCamelMimePart*) object;
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	g_mutex_lock (priv->part_lock);

	if (priv->cached_filename)
		g_free (priv->cached_filename);
	priv->cached_filename = NULL;

	if (priv->cached_content_type)
		g_free (priv->cached_content_type);
	priv->cached_content_type = NULL;

	if (priv->part && CAMEL_IS_OBJECT (priv->part))
		camel_object_unref (CAMEL_OBJECT (priv->part));
	g_mutex_unlock (priv->part_lock);

	g_mutex_free (priv->part_lock);
	priv->part_lock = NULL;

	(*parent_class->finalize) (object);

	return;
}


/**
 * tny_camel_mime_part_new:
 * 
 * Create a new MIME part instance
 * 
 * Return value: A new #TnyMimePart instance implemented for Camel
 **/
TnyMimePart*
tny_camel_mime_part_new (void)
{
	TnyCamelMimePart *self = g_object_new (TNY_TYPE_CAMEL_MIME_PART, NULL);
	CamelMimePart *cpart = camel_mime_part_new ();

	_tny_camel_mime_part_set_part (self, cpart);

	camel_object_unref(cpart);
	return TNY_MIME_PART (self);
}


/**
 * tny_camel_mime_part_new_with_part:
 * @part: a #CamelMimePart object
 * 
 * Create a new MIME part instance that is a proxy for a #CamelMimePart one
 *
 * Return value: A new #TnyMimePart instance implemented for Camel
 **/
TnyMimePart*
tny_camel_mime_part_new_with_part (CamelMimePart *part)
{
	TnyCamelMimePart *self = g_object_new (TNY_TYPE_CAMEL_MIME_PART, NULL);

	_tny_camel_mime_part_set_part (self, part);

	return TNY_MIME_PART (self);
}



static void
tny_mime_part_init (gpointer g, gpointer iface_data)
{
	TnyMimePartIface *klass = (TnyMimePartIface *)g;

	klass->content_type_is= tny_camel_mime_part_content_type_is;
	klass->get_content_type= tny_camel_mime_part_get_content_type;
	klass->get_stream= tny_camel_mime_part_get_stream;
	klass->get_decoded_stream= tny_camel_mime_part_get_decoded_stream;
	klass->write_to_stream= tny_camel_mime_part_write_to_stream;
	klass->construct= tny_camel_mime_part_construct;
	klass->get_filename= tny_camel_mime_part_get_filename;
	klass->get_content_id= tny_camel_mime_part_get_content_id;
	klass->get_description= tny_camel_mime_part_get_description;
	klass->get_content_location= tny_camel_mime_part_get_content_location;
	klass->is_purged= tny_camel_mime_part_is_purged;
	klass->set_content_location= tny_camel_mime_part_set_content_location;
	klass->set_description= tny_camel_mime_part_set_description;
	klass->set_purged= tny_camel_mime_part_set_purged;
	klass->set_content_id= tny_camel_mime_part_set_content_id;
	klass->set_filename= tny_camel_mime_part_set_filename;
	klass->set_content_type= tny_camel_mime_part_set_content_type;
	klass->is_attachment= tny_camel_mime_part_is_attachment;
	klass->decode_to_stream= tny_camel_mime_part_decode_to_stream;
	klass->get_parts= tny_camel_mime_part_get_parts;
	klass->add_part= tny_camel_mime_part_add_part;
	klass->del_part= tny_camel_mime_part_del_part;
	klass->get_header_pairs= tny_camel_mime_part_get_header_pairs;
	klass->set_header_pair= tny_camel_mime_part_set_header_pair;
	klass->decode_to_stream_async= tny_camel_mime_part_decode_to_stream_async;
	klass->get_transfer_encoding= tny_camel_mime_part_get_transfer_encoding;
	klass->set_transfer_encoding= tny_camel_mime_part_set_transfer_encoding;
	return;
}


static void 
tny_camel_mime_part_class_init (TnyCamelMimePartClass *class)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	class->content_type_is= tny_camel_mime_part_content_type_is_default;
	class->get_content_type= tny_camel_mime_part_get_content_type_default;
	class->get_stream= tny_camel_mime_part_get_stream_default;
	class->get_decoded_stream= tny_camel_mime_part_get_decoded_stream_default;
	class->write_to_stream= tny_camel_mime_part_write_to_stream_default;
	class->construct= tny_camel_mime_part_construct_default;
	class->get_filename= tny_camel_mime_part_get_filename_default;
	class->get_content_id= tny_camel_mime_part_get_content_id_default;
	class->get_description= tny_camel_mime_part_get_description_default;
	class->get_content_location= tny_camel_mime_part_get_content_location_default;
	class->is_purged= tny_camel_mime_part_is_purged_default;
	class->set_purged= tny_camel_mime_part_set_purged_default;
	class->set_content_location= tny_camel_mime_part_set_content_location_default;
	class->set_description= tny_camel_mime_part_set_description_default;
	class->set_content_id= tny_camel_mime_part_set_content_id_default;
	class->set_filename= tny_camel_mime_part_set_filename_default;
	class->set_content_type= tny_camel_mime_part_set_content_type_default;
	class->is_attachment= tny_camel_mime_part_is_attachment_default;
	class->decode_to_stream= tny_camel_mime_part_decode_to_stream_default;
	class->get_parts= tny_camel_mime_part_get_parts_default;
	class->add_part= tny_camel_mime_part_add_part_default;
	class->del_part= tny_camel_mime_part_del_part_default;
	class->get_header_pairs= tny_camel_mime_part_get_header_pairs_default;
	class->set_header_pair= tny_camel_mime_part_set_header_pair_default;
	class->decode_to_stream_async= tny_camel_mime_part_decode_to_stream_async_default;
	class->get_transfer_encoding= tny_camel_mime_part_get_transfer_encoding_default;
	class->set_transfer_encoding= tny_camel_mime_part_set_transfer_encoding_default;

	object_class->finalize = tny_camel_mime_part_finalize;

	g_type_class_add_private (object_class, sizeof (TnyCamelMimePartPriv));

	return;
}

static void
tny_camel_mime_part_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyCamelMimePart *self = (TnyCamelMimePart*)instance;
	TnyCamelMimePartPriv *priv = TNY_CAMEL_MIME_PART_GET_PRIVATE (self);

	priv->part_lock = g_mutex_new ();

	priv->cached_filename = NULL;
	priv->cached_content_type = NULL;

	return;
}

static gpointer
tny_camel_mime_part_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyCamelMimePartClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tny_camel_mime_part_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (TnyCamelMimePart),
			0,      /* n_preallocs */
			tny_camel_mime_part_instance_init,    /* instance_init */
			NULL
		};
	
	static const GInterfaceInfo tny_mime_part_info = 
		{
			(GInterfaceInitFunc) tny_mime_part_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};
	
	type = g_type_register_static (G_TYPE_OBJECT,
				       "TnyCamelMimePart",
				       &info, 0);
	
	g_type_add_interface_static (type, TNY_TYPE_MIME_PART, 
				     &tny_mime_part_info);

	return GUINT_TO_POINTER (type);
}

/**
 * tny_camel_mime_part_get_type:
 *
 * GType system helper function
 *
 * Return value: a GType
 **/
GType 
tny_camel_mime_part_get_type (void)
{
	static GOnce once = G_ONCE_INIT;

	if (G_UNLIKELY (!_camel_type_init_done))
	{
		if (!g_thread_supported ()) 
			g_thread_init (NULL);

		camel_type_init ();
		_camel_type_init_done = TRUE;
	}

	g_once (&once, tny_camel_mime_part_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
