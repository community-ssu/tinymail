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
 * TnyMimePart:
 *
 * A part of a message, like the text of the message or like an attachment
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#ifdef DBC
#include <string.h>
#endif

#include <tny-mime-part.h>
#include <tny-list.h>

/* TNY TODO: Check MIME RFC to evaluate whether NULL string values should be
 * allowed and strengthen contracts if not */


/**
 * tny_mime_part_set_header_pair:
 * @self: a #TnyMimePart
 * @name: the name of the header
 * @value: (null-ok): the value of the header or NULL to unset
 * 
 * Set a header pair (name: value) or delete a header (use NULL as value).
 *
 * Note that not all #TnyMimePart instances are writable. Only when creating
 * a new message will the instance be guaranteed to be writable. This is a
 * writing operation.
 *
 * Example:
 * <informalexample><programlisting>
 * TnyMsg *message = ...
 * tny_mime_part_set_header_pair (TNY_MIME_PART (message),
 *        "X-MS-Has-Attach", "yes");
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_mime_part_set_header_pair (TnyMimePart *self, const gchar *name, const gchar *value)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (name);
	g_assert (strlen (name) > 0);
	g_assert (TNY_MIME_PART_GET_IFACE (self)->set_header_pair!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->set_header_pair(self, name, value);
	return;
}

/**
 * tny_mime_part_get_header_pairs:
 * @self: a #TnyMimePart
 * @list: a #TnyList to which the header pairs will be prepended
 * 
 * Get a read-only list of header pairs in @self.
 *
 * Example:
 * <informalexample><programlisting>
 * TnyMsg *message = ...
 * TnyList *pairs = tny_simple_list_new ();
 * tny_mime_part_get_header_pairs (TNY_MIME_PART (message), pairs);
 * iter = tny_list_create_iterator (pairs);
 * while (!tny_iterator_is_done (iter))
 * {
 *      TnyPair *pair = TNY_PAIR (tny_iterator_get_current (iter));
 *      g_print (%s: %s", tny_pair_get_name (pair), 
 *           tny_pair_get_value (pair));
 *      g_object_unref (pair);
 *      tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * g_object_unref (pairs);
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_mime_part_get_header_pairs (TnyMimePart *self, TnyList *list)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (list);
	g_assert (TNY_IS_LIST (list));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->get_header_pairs!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->get_header_pairs(self, list);
	return;
}


/**
 * tny_mime_part_get_parts:
 * @self: a #TnyMimePart
 * @list: a #TnyList to which the parts will be prepended
 * 
 * Get a read-only list of child MIME parts in @self.
 *
 * Example:
 * <informalexample><programlisting>
 * TnyMsg *message = ...
 * TnyList *parts = tny_simple_list_new ();
 * tny_mime_part_get_parts (TNY_MIME_PART (message), parts);
 * iter = tny_list_create_iterator (parts);
 * while (!tny_iterator_is_done (iter))
 * {
 *      TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
 *      g_object_unref (part);
 *      tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * g_object_unref (parts);
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_mime_part_get_parts (TnyMimePart *self, TnyList *list)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (list);
	g_assert (TNY_IS_LIST (list));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->get_parts!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->get_parts(self, list);
	return;
}



/**
 * tny_mime_part_add_part:
 * @self: a #TnyMimePart object
 * @part: a #TnyMimePart to add to @self
 * 
 * Add a mime-part to @self. It's not guaranteed that once the part is added to
 * @self, when getting a list of parts in @self with tny_mime_part_get_parts(),
 * that the same instance for the part will be the one you just added. Therefore
 * it's recommended to unreference @part after using this API.
 *
 * Likewise it's recommended to alter @part completely the way you want to alter
 * it before adding it to @self. Not after you added it.
 *
 * Note that not all #TnyMimePart instances are writable. Only when creating
 * a new message will the instance be guaranteed to be writable. This is a
 * writing operation.
 *
 * returns: The id of the added mime-part
 * since: 1.0
 * audience: application-developer
 **/
gint
tny_mime_part_add_part (TnyMimePart *self, TnyMimePart *part)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (part);
	g_assert (TNY_IS_MIME_PART (part));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->add_part!= NULL);
#endif

	return TNY_MIME_PART_GET_IFACE (self)->add_part(self, part);
}

/**
 * tny_mime_part_del_part:
 * @self: a #TnyMimePart
 * @part: a #TnyMimePart to delete
 * 
 * Delete a mime-part from @self. You should only attempt to remove parts
 * that you got from @self using the tny_mime_part_get_parts() API.
 *
 * Note that not all #TnyMimePart instances are writable. Only when creating
 * a new message will the instance be guaranteed to be writable. This is a
 * writing operation.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_mime_part_del_part (TnyMimePart *self, TnyMimePart *part)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (part);
	g_assert (TNY_IS_MIME_PART (part));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->del_part!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->del_part(self, part);
	return;
}

/**
 * tny_mime_part_is_attachment:
 * @self: a #TnyMimePart
 * 
 * Figures out whether or not a mime part is an attachment. An attachment
 * is typically something with a original filename. Examples are attached
 * files. An example that will return FALSE is a PGP signatures.
 *
 * The algorithm that figures out whether @self is indeed an attachment
 * might not return its result according to your personal definition of what
 * an attachment really is. There are many personal opinions on that.
 *
 * For example the Content-Disposition and the availability of a filename
 * header are used to determine this.
 * 
 * Example:
 * <informalexample><programlisting>
 * TnyMsg *message = ...
 * TnyList *parts = tny_simple_list_new ();
 * tny_mime_part_get_parts (TNY_MIME_PART (message), parts);
 * iter = tny_list_create_iterator (parts);
 * while (!tny_iterator_is_done (iter))
 * {
 *      TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
 *      if (tny_mime_part_is_attachment (part))
 *      {
 *          g_print ("Found an attachment (%s)\n",
 *               tny_mime_part_get_filename (part));
 *      }
 *      g_object_unref (part);
 *      tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * g_object_unref (parts);
 * </programlisting></informalexample>
 *
 * returns: TRUE if it's an attachment, else FALSE
 * since: 1.0
 * audience: application-developer
 **/
gboolean 
tny_mime_part_is_attachment (TnyMimePart *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->is_attachment!= NULL);
#endif
	return TNY_MIME_PART_GET_IFACE (self)->is_attachment(self);
}


/**
 * tny_mime_part_set_content_location:
 * @self: a #TnyMimePart
 * @content_location: (null-ok): the location or NULL 
 * 
 * Set the content location of @self or NULL to unset it.
 *
 * Note that not all #TnyMimePart instances are writable. Only when creating
 * a new message will the instance be guaranteed to be writable. This is a
 * writing operation.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_mime_part_set_content_location (TnyMimePart *self, const gchar *content_location)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->set_content_location!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->set_content_location(self, content_location);
	return;
}

/**
 * tny_mime_part_set_description:
 * @self: a #TnyMimePart
 * @description: (null-ok): the description or NULL
 * 
 * Set the description of @self or NULL to unset it.
 *
 * Note that not all TnyMimePart instances are writable. Only when creating
 * a new message will the instance be guaranteed to be writable. This is a
 * writing operation.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_mime_part_set_description (TnyMimePart *self, const gchar *description)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->set_description!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->set_description(self, description);
	return;
}

/**
 * tny_mime_part_set_content_id:
 * @self: a #TnyMimePart
 * @content_id: (null-ok): the content id or NULL
 * 
 * Set the content id of @self or NULL to unset it.
 *
 * Note that not all #TnyMimePart instances are writable. Only when creating
 * a new message will the instance be guaranteed to be writable. This is a
 * writing operation.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_mime_part_set_content_id (TnyMimePart *self, const gchar *content_id)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->set_content_id!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->set_content_id(self, content_id);
	return;
}

/**
 * tny_mime_part_set_purged:
 * @self: a #TnyMimePart
 * 
 * Set the message as purged in cache.
 *
 * This is not a writing operation. Although it might change the content of a
 * message for a user who's not connected with the server where @self originates
 * from.
 *
 * Using the tny_msg_rewrite_cache() API on a message's instance will rewrite 
 * its purged mime parts with an empty body (saving storage space). The storage 
 * space is recovered after using tny_msg_rewrite_cache(). Only setting a mime 
 * part to purged might not completely remove it locally.
 *
 * There is no guarantee about what happens with a purged mime part internally 
 * (it might get destroyed or become unuseable more early than the call to
 * tny_msg_rewrite_cache).
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_mime_part_set_purged (TnyMimePart *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->set_purged!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->set_purged(self);
	return;
}

/**
 * tny_mime_part_set_filename:
 * @self: a #TnyMimePart
 * @filename: (null-ok): the filename or NULL
 * 
 * Set the filename of @self or NULL to unset it.
 *
 * Note that not all #TnyMimePart instances are writable. Only when creating
 * a new message will the instance be guaranteed to be writable. This is a
 * writing operation.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_mime_part_set_filename (TnyMimePart *self, const gchar *filename)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->set_filename!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->set_filename(self, filename);
	return;
}

/**
 * tny_mime_part_set_content_type:
 * @self: a #TnyMimePart
 * @contenttype: (null-ok): the content_type or NULL
 * 
 * Set the content type of @self, Formatted as "type/subtype" or NULL to unset
 * it (will default to text/plain then).
 *
 * Note that not all #TnyMimePart instances are writable. Only when creating
 * a new message will the instance be guaranteed to be writable. This is a
 * writing operation.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_mime_part_set_content_type (TnyMimePart *self, const gchar *contenttype)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->set_content_type!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->set_content_type(self, contenttype);
	return;
}


/**
 * tny_mime_part_get_filename:
 * @self: a #TnyMimePart
 * 
 * Get the filename of @self if it's an attachment or NULL otherwise. The
 * returned value should not be freed.
 *
 * returns: (null-ok): the filename of the part as a read-only string or NULL
 * since: 1.0
 * audience: application-developer
 **/
const gchar*
tny_mime_part_get_filename (TnyMimePart *self)
{
	const gchar *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->get_filename!= NULL);
#endif

	retval = TNY_MIME_PART_GET_IFACE (self)->get_filename(self);

#ifdef DBC /* ensure */
	g_assert (retval == NULL || strlen (retval) > 0);
#endif

	return retval;
}

/**
 * tny_mime_part_get_content_id:
 * @self: a #TnyMimePart
 * 
 * Get the content-id of @self. The returned value should not be freed.
 *
 * returns: (null-ok): the content-id of the part as a read-only string or NULL
 * since: 1.0
 * audience: application-developer
 **/
const gchar*
tny_mime_part_get_content_id (TnyMimePart *self)
{
	const gchar *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->get_content_id!= NULL);
#endif

	retval = TNY_MIME_PART_GET_IFACE (self)->get_content_id(self);

#ifdef DBC /* ensure */
	g_assert (retval == NULL || strlen (retval) > 0);
#endif

	return retval;
}

/**
 * tny_mime_part_is_purged:
 * @self: a #TnyMimePart
 * 
 * Get if this attachment has been purged from cache.
 *
 * returns: if purged TRUE, else FALSE
 * since: 1.0
 * audience: application-developer
 **/
gboolean
tny_mime_part_is_purged (TnyMimePart *self)
{
	gboolean retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->is_purged!= NULL);
#endif

	retval = TNY_MIME_PART_GET_IFACE (self)->is_purged(self);

	return retval;
}

/**
 * tny_mime_part_get_description:
 * @self: a #TnyMimePart
 * 
 * Get the description of @self. The returned value should not be freed.
 *
 * returns: (null-ok): the description of the part as a read-only string or NULL
 * since: 1.0
 * audience: application-developer
 **/
const gchar*
tny_mime_part_get_description (TnyMimePart *self)
{
	const gchar *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->get_description!= NULL);
#endif

	retval = TNY_MIME_PART_GET_IFACE (self)->get_description(self);

#ifdef DBC /* ensure */
	g_assert (retval == NULL || strlen (retval) > 0);
#endif

	return retval;
}

/**
 * tny_mime_part_get_content_location:
 * @self: a #TnyMimePart
 * 
 * Get the content location of @self. The returned value should not be freed.
 *
 * returns: (null-ok): the content-location of the part as a read-only string or NULL
 * since: 1.0
 * audience: application-developer
 **/
const gchar*
tny_mime_part_get_content_location (TnyMimePart *self)
{
	const gchar *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->get_content_location!= NULL);
#endif

	retval = TNY_MIME_PART_GET_IFACE (self)->get_content_location(self);

#ifdef DBC /* ensure */
	g_assert (retval == NULL || strlen (retval) > 0);
#endif

	return retval;
}

/**
 * tny_mime_part_write_to_stream:
 * @self: a #TnyMimePart
 * @stream: a #TnyStream
 * 
 * Efficiently write the content of @self to @stream. This will not read the 
 * data of the part in a memory buffer. In stead it will read the part data while
 * already writing it to @stream efficiently. Although there is no hard guarantee
 * about the memory usage either (just that it's the most efficient way).
 *
 * You probably want to utilise the tny_mime_part_decode_to_stream()
 * method in stead of this one. This method will not attempt to decode the
 * mime part. Mime parts are usually encoded in E-mails.
 *
 * When the mime part was received in BINARY mode from an IMAP server, then this
 * API has mostly the same effect as the tny_mime_part_decode_to_stream(): You 
 * will get a non-encoded version of the data. A small difference will be that
 * the tny_mime_part_decode_to_stream() will decode certain special characters in
 * TEXT mime parts (character set encoding) to UTF-8.
 *
 * However. A larger difference happens with binary mime parts that where not
 * retrieved using BINARY. For those this API will give you the encoded data
 * as is. This means that you will get a stream spitting out for example BASE64
 * data.
 * 
 * Example:
 * <informalexample><programlisting>
 * int fd = open ("/tmp/attachment.png.base64", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
 * TnyMimePart *part = ...
 * if (fd != -1)
 * {
 *      TnyFsStream *stream = tny_fs_stream_new (fd);
 *      tny_mime_part_write_to_stream (part, TNY_STREAM (stream));
 *      g_object_unref (stream);
 * }
 * </programlisting></informalexample>
 *
 * returns: Returns %-1 on error, or the number of bytes succesfully copied.
 * since: 1.0
 * audience: application-developer
 **/
gssize
tny_mime_part_write_to_stream (TnyMimePart *self, TnyStream *stream, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (stream);
	g_assert (TNY_IS_STREAM (stream));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->write_to_stream!= NULL);
#endif

	return TNY_MIME_PART_GET_IFACE (self)->write_to_stream(self, stream, err);
}


/**
 * tny_mime_part_decode_to_stream:
 * @self: a #TnyMimePart object
 * @stream: a #TnyMsgStream stream
 * 
 * Efficiently decode @self to @stream. This will not read the data of the
 * part in a memory buffer. In stead it will read the part data while already
 * writing it to the stream efficiently. Although there is no hard guarantee
 * about the memory usage either (just that it's the most efficient way).
 *
 * You will always get the decoded version of the data of @self. When your part
 * got received in BINARY from an IMAP server, then nothing will really happen
 * with your data (it will be streamed to you the way it got received). If we
 * received using FETCH BODY and the data is encoded in a known encoding
 * (like BASE 64, QUOTED-PRINTED, UUENCODED, etc), the data will be decoded
 * before delivered to @stream. TEXT mime parts will also enjoy character set
 * decoding.
 *
 * In short will this API prepare the cookie and deliver it to your @stream.
 * It's therefore most likely the one that you want to use. If you are planning
 * to nevertheless use the tny_mime_part_write_to_stream() API, then please know
 * and understand what you are doing.
 *
 * It's possible that this API receives information from the service. If you 
 * don't want to block on this, use tny_mime_part_decode_to_stream_async().
 *
 * Example:
 * <informalexample><programlisting>
 * int fd = open ("/tmp/attachment.png", O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
 * TnyMimePart *part = ...
 * if (fd != -1)
 * {
 *      TnyFsStream *stream = tny_fs_stream_new (fd);
 *      tny_mime_part_decode_to_stream (part, TNY_STREAM (stream));
 *      g_object_unref (G_OBJECT (stream));
 * }
 * </programlisting></informalexample>
 *
 * returns: Returns %-1 on error, or the number of bytes succesfully copied.
 * since: 1.0
 * audience: application-developer
 **/
gssize
tny_mime_part_decode_to_stream (TnyMimePart *self, TnyStream *stream, GError **err)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (stream);
	g_assert (TNY_IS_STREAM (stream));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->decode_to_stream!= NULL);
#endif

	return TNY_MIME_PART_GET_IFACE (self)->decode_to_stream(self, stream, err);
}


/** 
 * TnyMimePartCallback:
 * @self: a #TnyFolderStore that caused the callback
 * @cancelled: if the operation got cancelled
 * @stream: the stream you passed
 * @err: (null-ok): if an error occurred
 * @user_data: (null-ok):  user data that was passed to the callbacks
 *
 * A MIME part callback for when a MIME part decode was requested. If allocated,
 * you must cleanup @user_data at the end of your implementation of the callback.
 * All other fields in the parameters of the callback are read-only.
 *
 * When cancelled, @cancelled will be TRUE while @err might nonetheless be NULL.
 * If @err is not NULL, an error occurred that you should handle gracefully.
 * The @stream parameter will contain the stream that you passed but will now be
 * written, if the operation succeeded.
 *
 * since: 1.0
 * audience: application-developer
 **/


/**
 * tny_mime_part_decode_to_stream_async:
 * @self: a #TnyMimePart object
 * @stream: a #TnyMsgStream stream
 * @callback: (null-ok): a #TnyMimePartCallback or NULL
 * @status_callback: (null-ok): a #TnyStatusCallback or NULL
 * @user_data: (null-ok): user data that will be passed to the callbacks
 *
 * This method does the same as tny_mime_part_decode_to_stream(). It just does 
 * everything asynchronous and callsback when finished.
 *
 * since: 1.0
 * audience: application-developer
 */
void
tny_mime_part_decode_to_stream_async (TnyMimePart *self, TnyStream *stream, TnyMimePartCallback callback, TnyStatusCallback status_callback, gpointer user_data)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (stream);
	g_assert (TNY_IS_STREAM (stream));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->decode_to_stream_async!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->decode_to_stream_async(self, stream, callback, status_callback, user_data);
	return;
}



/**
 * tny_mime_part_set_transfer_encoding:
 * @self: a #TnyMimePart
 * @transfer_encoding: the Content-Transfer-Encoding
 * 
 * Set the transfer encoding
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_mime_part_set_transfer_encoding (TnyMimePart *self, const gchar *transfer_encoding)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->set_transfer_encoding!= NULL);
#endif

	TNY_MIME_PART_GET_IFACE (self)->set_transfer_encoding(self, transfer_encoding);

	return;
}

/**
 * tny_mime_part_construct:
 * @self: a #TnyMimePart
 * @stream: a #TnyStream
 * @mime_type: the MIME type like "text/plain"
 * @transfer_encoding: the Content-Transfer-Encoding
 * 
 * Set the stream from which the part will read its content
 *
 * Valid values for @transfer_encoding are "7bit", "8bit", "base64",
 * "quoted-printable", "binary" and "x-uuencode"
 *
 * Example:
 * <informalexample><programlisting>
 * int fd = open ("/tmp/attachment.png", ...);
 * TnyMimePart *part = ...
 * if (fd != -1)
 * {
 *      TnyFsStream *stream = tny_fs_stream_new (fd);
 *      tny_mime_part_construct (part, TNY_STREAM (stream), "text/html", "base64");
 * }
 * </programlisting></informalexample>
 *
 * returns: 0 on success or -1 on failure
 * since: 1.0
 * audience: application-developer
 **/
gint
tny_mime_part_construct (TnyMimePart *self, TnyStream *stream, const gchar *mime_type, const gchar *transfer_encoding)
{
	gint retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_IS_STREAM (stream));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->construct!= NULL);
#endif

	retval = TNY_MIME_PART_GET_IFACE (self)->construct(self, stream, mime_type, transfer_encoding);

#ifdef DBC /* ensure */
	g_assert (retval == 0 || retval == -1);
#endif

	return retval;
}


/**
 * tny_mime_part_get_transfer_encoding:
 * @self: a #TnyMimePart
 * 
 * Get the transfer encoding of @self or NULL if default transfer encoding 7bit.
 * 
 * Returns: (null-ok): transfer encoding
 * since: 1.0
 * audience: application-developer
 **/
const gchar* 
tny_mime_part_get_transfer_encoding (TnyMimePart *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->get_transfer_encoding!= NULL);
#endif

	return TNY_MIME_PART_GET_IFACE (self)->get_transfer_encoding(self);
}


/**
 * tny_mime_part_get_stream:
 * @self: a #TnyMimePart
 * 
 * Inefficiently get a stream for @self. The entire data of the part will be
 * kept in memory until the returned value is unreferenced.
 *
 * returns: an in-memory stream
 * since: 1.0
 * audience: application-developer
 **/
TnyStream* 
tny_mime_part_get_stream (TnyMimePart *self)
{
	TnyStream *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->get_stream!= NULL);
#endif

	retval = TNY_MIME_PART_GET_IFACE (self)->get_stream(self);

#ifdef DBC /* ensure */
	g_assert (TNY_IS_STREAM (retval));
#endif

	return retval;
}



/**
 * tny_mime_part_get_decoded_stream:
 * @self: a #TnyMimePart
 * 
 * Inefficiently get a stream for @self. The entire data of the part will be
 * kept in memory until the returned value is unreferenced.
 *
 * returns: an in-memory stream
 * since: 1.0
 * audience: application-developer
 **/
TnyStream* 
tny_mime_part_get_decoded_stream (TnyMimePart *self)
{
	TnyStream *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->get_decoded_stream!= NULL);
#endif

	retval = TNY_MIME_PART_GET_IFACE (self)->get_decoded_stream(self);

#ifdef DBC /* ensure */
	g_assert (TNY_IS_STREAM (retval));
#endif

	return retval;
}

/**
 * tny_mime_part_get_content_type:
 * @self: a #TnyMimePart
 * 
 * Get the mime part type in the format "type/subtype".  You shouldn't free the 
 * returned value. In case of NULL, the intended type is usually "text/plain".
 *
 * returns: (null-ok): content-type of a message part as a read-only string or NULL
 * since: 1.0
 * audience: application-developer
 **/
const gchar*
tny_mime_part_get_content_type (TnyMimePart *self)
{
	const gchar *retval;

#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (TNY_MIME_PART_GET_IFACE (self)->get_content_type!= NULL);
#endif

	retval = TNY_MIME_PART_GET_IFACE (self)->get_content_type(self);

#ifdef DBC /* ensure */
	g_assert (retval == NULL || strlen (retval) > 0);
#endif

	return retval;
}

/**
 * tny_mime_part_content_type_is:
 * @self: a #TnyMimePart
 * @type: The content type in the string format "type/subtype"
 * 
 * Efficiently checks whether @self is of type @type. You can use things
 * like "type/ *" for matching. Only '*' works and stands for 'any'. It's not 
 * a regular expression nor is it like a regular expression.
 *
 * Example:
 * <informalexample><programlisting>
 * TnyMsg *message = ...
 * TnyList *parts = tny_simple_list_new ();
 * tny_mime_part_get_parts (TNY_MIME_PART (message), parts);
 * iter = tny_list_create_iterator (parts);
 * while (!tny_iterator_is_done (iter))
 * {
 *      TnyMimePart *part = TNY_MIME_PART (tny_iterator_get_current (iter));
 *      if (tny_mime_part_content_type_is (part, "text/ *"))
 *              g_print ("Found an E-mail body\n");
 *      if (tny_mime_part_content_type_is (part, "text/html"))
 *              g_print ("Found an E-mail HTML body\n"); 
 *      g_object_unref (part);
 *      tny_iterator_next (iter);
 * }
 * g_object_unref (iter);
 * g_object_unref (parts);
 * </programlisting></informalexample>
 *
 * returns: True if the part is the content type, else FALSE
 * since: 1.0
 * audience: application-developer
 **/
gboolean
tny_mime_part_content_type_is (TnyMimePart *self, const gchar *type)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_MIME_PART (self));
	g_assert (type && strlen (type) > 0);
	g_assert (TNY_MIME_PART_GET_IFACE (self)->content_type_is!= NULL);
#endif

	return TNY_MIME_PART_GET_IFACE (self)->content_type_is(self, type);
}



static void
tny_mime_part_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_mime_part_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnyMimePartIface),
			tny_mime_part_base_init,   /* base_init */
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
				       "TnyMimePart", &info, 0);
	
	return GUINT_TO_POINTER (type);
}

GType
tny_mime_part_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_mime_part_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
