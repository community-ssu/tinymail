/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Authors:
 *   Michael Zucchi <notzed@ximian.com>
 *   Jeffrey Stedfast <fejj@ximian.com>
 *   Dan Winship <danw@ximian.com>
 *
 * Copyright (C) 2000, 2003 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>

#ifdef G_OS_WIN32
#include <winsock2.h>
#define EWOULDBLOCK EAGAIN
#endif

#include <libedataserver/e-data-server-util.h>

#include "camel-file-utils.h"
#include "camel-operation.h"
#include "camel-url.h"



/**
 * camel_file_util_encode_fixed_int32:
 * @out: file to output to
 * @value: value to output
 *
 * Encode a gint32, performing no compression, but converting
 * to network order.
 *
 * Return value: 0 on success, -1 on error.
 **/
int
camel_file_util_encode_fixed_int32 (FILE *out, gint32 value)
{
	guint32 save;

	save = g_htonl (value);
	if (fwrite (&save, sizeof (save), 1, out) != 1)
		return -1;
	return 0;
}

void
camel_file_util_read_counts (const gchar *spath, CamelFolderInfo *fi)
{
	/* This code reads the beginning of the summary.mmap file for
	 * the length and unread-count of the folder. It makes it
	 * possible to read the total and unread values of the
	 * CamelFolderInfo structure without having to read the entire
	 * folder in (mmap it)
	 * It's called by get_folder_info_offline and therefore also by
	 * get_folder_info_online for each node in the tree. */

	FILE *f = fopen (spath, "r");
	if (f) {

		gint tsize = ((sizeof (guint32) * 5) + sizeof (time_t));
		char *buffer = malloc (tsize), *ptr;
		guint32 version, a;
		a = fread (buffer, 1, tsize, f);
		if (a == tsize)
		{
			ptr = buffer;
			version = g_ntohl(get_unaligned_u32(ptr));
			ptr += 16;
			if (fi->total == -1)
				fi->total = g_ntohl(get_unaligned_u32(ptr));
			ptr += 4;
			if (fi->unread == -1 && (version < 0x100 && version >= 13))
				fi->unread = g_ntohl(get_unaligned_u32(ptr));
		}
		g_free (buffer);
		fclose (f);
	} else {
		fi->unread = 0;
		fi->total = 0;
	}
}


void
camel_file_util_read_counts_2 (const gchar *spath,
	guint32 *version, guint32 *flags,
	guint32 *nextuid, time_t *time, guint32 *saved_count,
	guint32 *unread_count, guint32 *deleted_count,
	guint32 *junk_count)
{
	/* This code reads the beginning of the summary.mmap file for
	 * the length and unread-count of the folder. It makes it
	 * possible to read the total and unread values of the
	 * CamelFolderInfo structure without having to read the entire
	 * folder in (mmap it)
	 * It's called by get_folder_info_offline and therefore also by
	 * get_folder_info_online for each node in the tree. */

	FILE *f = fopen (spath, "r");
	if (f) {

		gint tsize = ((sizeof (guint32) * 7) + sizeof (time_t));
		char *buffer = malloc (tsize), *ptr;
		guint32 a;

		a = fread (buffer, 1, tsize, f);
		if (a == tsize)
		{
			guint32 v = 0;
			ptr = buffer;
			v = g_ntohl(get_unaligned_u32(ptr));
			*version = v;
			ptr += 4;
			*flags = g_ntohl(get_unaligned_u32(ptr));
			ptr += 4;
			*nextuid = g_ntohl(get_unaligned_u32(ptr));
			ptr += 4;
			*time = g_ntohl(get_unaligned_u32(ptr));
			ptr += 4;
			*saved_count = g_ntohl(get_unaligned_u32(ptr));

			if (v < 0x100 && v >= 13) {
				ptr += 4;
				*unread_count = g_ntohl(get_unaligned_u32(ptr));
				ptr += 4;
				*deleted_count = g_ntohl(get_unaligned_u32(ptr));
				ptr += 4;
				*junk_count = g_ntohl(get_unaligned_u32(ptr));
			}
		}
		g_free (buffer);
		fclose (f);
	}
}

/**
 * camel_file_util_decode_fixed_int32:
 * @in: file to read from
 * @dest: pointer to a variable to store the value in
 *
 * Retrieve a gint32.
 *
 * Return value: 0 on success, -1 on error.
 **/
int
camel_file_util_decode_fixed_int32 (FILE *in, gint32 *dest)
{
	guint32 save;

	if (fread (&save, sizeof (save), 1, in) == 1) {
		*dest = g_ntohl (save);
		return 0;
	} else {
		return -1;
	}
}


/**
 * camel_file_util_encode_uint32:
 * @out: file to output to
 * @value: value to output
 *
 * Utility function to save an uint32 to a file.
 *
 * Return value: 0 on success, -1 on error.
 **/
int
camel_file_util_encode_uint32 (FILE *out, guint32 value)
{
	return camel_file_util_encode_fixed_int32 (out, value);
}


/**
 * camel_file_util_decode_uint32:
 * @in: file to read from
 * @dest: pointer to a variable to store the value in
 *
 * Retrieve an encoded uint32 from a file.
 *
 * Return value: 0 on success, -1 on error.  @*dest will contain the
 * decoded value.
 **/
int
camel_file_util_decode_uint32 (FILE *in, guint32 *dest)
{
	return camel_file_util_decode_fixed_int32 (in, (gint32*)dest);
}


unsigned char*
camel_file_util_mmap_decode_uint32 (unsigned char *start, guint32 *dest, gboolean is_string)
{
	guint32 value = 0;
	value = g_ntohl(get_unaligned_u32(start)); start += 4;
	*dest = value;
	return start;
}


/* This casting should be okay, because it also gets BOTH written and read from
   the file using a 4 byte integer. */

#define CFU_ENCODE_T(type)						  \
int									  \
camel_file_util_encode_##type(FILE *out, type value)			  \
{									  \
	return camel_file_util_encode_fixed_int32 (out, (guint32) value); \
}

#define CFU_DECODE_T(type)						  \
int									  \
camel_file_util_decode_##type(FILE *in, type *dest)			  \
{									  \
	return camel_file_util_decode_fixed_int32 (in, (gint32*) dest);  \
}


#define MMAP_DECODE_T(type)						    \
unsigned char*								    \
camel_file_util_mmap_decode_##type(unsigned char *start, type *dest)	    \
{									    \
	return camel_file_util_mmap_decode_uint32 (start, (guint32*) dest, FALSE); \
}


/**
 * camel_file_util_encode_time_t:
 * @out: file to output to
 * @value: value to output
 *
 * Encode a time_t value to the file.
 *
 * Return value: 0 on success, -1 on error.
 **/
CFU_ENCODE_T(time_t)

/**
 * camel_file_util_decode_time_t:
 * @in: file to read from
 * @dest: pointer to a variable to store the value in
 *
 * Decode a time_t value.
 *
 * Return value: 0 on success, -1 on error.
 **/
CFU_DECODE_T(time_t)
MMAP_DECODE_T(time_t)

/**
 * camel_file_util_encode_off_t:
 * @out: file to output to
 * @value: value to output
 *
 * Encode an off_t type.
 *
 * Return value: 0 on success, -1 on error.
 **/
CFU_ENCODE_T(off_t)


/**
 * camel_file_util_decode_off_t:
 * @in: file to read from
 * @dest: pointer to a variable to put the value in
 *
 * Decode an off_t type.
 *
 * Return value: 0 on success, -1 on failure.
 **/
CFU_DECODE_T(off_t)
MMAP_DECODE_T(off_t)

/**
 * camel_file_util_encode_size_t:
 * @out: file to output to
 * @value: value to output
 *
 * Encode an size_t type.
 *
 * Return value: 0 on success, -1 on error.
 **/
CFU_ENCODE_T(size_t)


/**
 * camel_file_util_decode_size_t:
 * @in: file to read from
 * @dest: pointer to a variable to put the value in
 *
 * Decode an size_t type.
 *
 * Return value: 0 on success, -1 on failure.
 **/
CFU_DECODE_T(size_t)
MMAP_DECODE_T(size_t)


/**
 * camel_file_util_encode_string:
 * @out: file to output to
 * @str: value to output
 *
 * Encode a normal string and save it in the output file.
 *
 * Return value: 0 on success, -1 on error.
 **/
int
camel_file_util_encode_string (FILE *out, const char *str)
{
	register int lena, len;

	if (str == NULL) {
		if (camel_file_util_encode_uint32 (out, 0) == -1)
			return -1;

		return 0;
	}

	lena = len = strlen (str) + 1;

	if (len > 65536)
		lena = len = 65536;

	if (lena % G_MEM_ALIGN)
		lena += G_MEM_ALIGN - (lena % G_MEM_ALIGN);

	if (camel_file_util_encode_uint32 (out, lena) == -1)
		return -1;

	if (fwrite (str, len, 1, out) == 1) {
		if (lena > len) {
			if (fwrite ("\0\0\0\0\0\0\0\0", lena-len, 1, out) == 1)
				return 0;
			else return -1;
		} else return 0;
	}

	return -1;
}


/**
 * camel_file_util_decode_string:
 * @in: file to read from
 * @str: pointer to a variable to store the value in
 *
 * Decode a normal string from the input file.
 *
 * Return value: %0 on success, %-1 on error.
 **/
int
camel_file_util_decode_string (FILE *in, char **str)
{
	guint32 len;
	register char *ret;

	if (camel_file_util_decode_uint32 (in, &len) == -1) {
		*str = NULL;
		return -1;
	}

	if (len > 65536) {
		*str = NULL;
		return -1;
	}

	ret = g_malloc (len+1);
	if (len > 0 && fread (ret, len, 1, in) != 1) {
		g_free (ret);
		*str = NULL;
		return -1;
	}

	ret[len] = 0;
	*str = ret;

	return 0;
}

/**
 * camel_file_util_encode_fixed_string:
 * @out: file to output to
 * @str: value to output
 * @len: total-len of str to store
 *
 * Encode a normal string and save it in the output file.
 * Unlike @camel_file_util_encode_string, it pads the
 * @str with "NULL" bytes, if @len is > strlen(str)
 *
 * Return value: 0 on success, -1 on error.
 **/
int
camel_file_util_encode_fixed_string (FILE *out, const char *str, size_t len)
{
	char *buff = (char *) alloca (len);

	/* Don't allow empty strings to be written */
	if (len < 1)
		return -1;

	/* Max size is 64K */
	if (len > 65536)
		len = 65536;

	memset(buff, 0x00, len);
	g_strlcpy(buff, str, len);

	if (fwrite (buff, len, 1, out) == len)
		return 0;

	return -1;
}


/**
 * camel_file_util_decode_fixed_string:
 * @in: file to read from
 * @str: pointer to a variable to store the value in
 * @len: total-len to decode.
 *
 * Decode a normal string from the input file.
 *
 * Return value: 0 on success, -1 on error.
 **/
int
camel_file_util_decode_fixed_string (FILE *in, char **str, size_t len)
{
	register char *ret;

	if (len > 65536) {
		*str = NULL;
		return -1;
	}

	ret = g_malloc (len+1);
	if (len > 0 && fread (ret, len, 1, in) != 1) {
		g_free (ret);
		*str = NULL;
		return -1;
	}

	ret[len] = 0;
	*str = ret;
	return 0;
}

/**
 * camel_file_util_safe_filename:
 * @name: string to 'flattened' into a safe filename
 *
 * 'Flattens' @name into a safe filename string by hex encoding any
 * chars that may cause problems on the filesystem.
 *
 * Returns a safe filename string.
 **/
char *
camel_file_util_safe_filename (const char *name)
{
#ifdef G_OS_WIN32
	const char *unsafe_chars = "/?()'*<>:\"\\|";
#else
	const char *unsafe_chars = "/?()'*";
#endif

	if (name == NULL)
		return NULL;

	return camel_url_encode(name, unsafe_chars);
}


/* FIXME: poll() might be more efficient and more portable? */

/**
 * camel_read:
 * @fd: file descriptor
 * @buf: buffer to fill
 * @n: number of bytes to read into @buf
 *
 * Cancellable libc read() replacement.
 *
 * Code that intends to be portable to Win32 should call this function
 * only on file descriptors returned from open(), not on sockets.
 *
 * Returns number of bytes read or -1 on fail. On failure, errno will
 * be set appropriately.
 **/
ssize_t
camel_read (int fd, char *buf, size_t n)
{
	ssize_t nread;
	int cancel_fd;

	if (camel_operation_cancel_check (NULL)) {
		errno = EINTR;
		return -1;
	}
#ifndef G_OS_WIN32
	cancel_fd = camel_operation_cancel_fd (NULL);
#else
	cancel_fd = -1;
#endif
	if (cancel_fd == -1) {
		do {
			nread = read (fd, buf, n);
		} while (nread == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
	} else {
#ifndef G_OS_WIN32
		int errnosav, flags, fdmax;
		fd_set rdset;

		flags = fcntl (fd, F_GETFL);
		fcntl (fd, F_SETFL, flags | O_NONBLOCK);

		do {
			struct timeval tv;
			int res;

			FD_ZERO (&rdset);
			FD_SET (fd, &rdset);
			FD_SET (cancel_fd, &rdset);
			fdmax = MAX (fd, cancel_fd) + 1;
			tv.tv_sec = BLOCKING_READ_TIMEOUT;
			tv.tv_usec = 0;
			nread = -1;

			res = select(fdmax, &rdset, 0, 0, &tv);
			if (res == -1)
				;
			else if (res == 0)
				errno = ETIMEDOUT;
			else if (FD_ISSET (cancel_fd, &rdset)) {
				errno = EINTR;
				goto failed;
			} else {
				do {
					nread = read (fd, buf, n);
				} while (nread == -1 && errno == EINTR);
			}
		} while (nread == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
	failed:
		errnosav = errno;
		fcntl (fd, F_SETFL, flags);
		errno = errnosav;
#endif
	}

	return nread;
}

static ssize_t
camel_write_shared (int fd, const char *buf, size_t n, gboolean write_in_chunks)
{
	ssize_t w, written = 0;
	int cancel_fd;

	if (camel_operation_cancel_check (NULL)) {
		errno = EINTR;
		return -1;
	}
#ifndef G_OS_WIN32
	cancel_fd = camel_operation_cancel_fd (NULL);
#else
	cancel_fd = -1;
#endif
	if (cancel_fd == -1) {
		do {
			if (write_in_chunks) {
				/* Write in chunks of max WRITE_CHUNK_SIZE bytes */
				ssize_t actual = MIN (n - written, WRITE_CHUNK_SIZE);
				do {
					w = write (fd, buf + written, actual);
				} while (w == -1 && (errno == EINTR || errno == EAGAIN || 
						     errno == EWOULDBLOCK));			
			} else {
				do {
					w = write (fd, buf + written, n - written);
				} while (w == -1 && (errno == EINTR || errno == EAGAIN || 
						     errno == EWOULDBLOCK));		
			}
			if (w > 0)
				written += w;
		} while (w != -1 && written < n);
	} else {
#ifndef G_OS_WIN32
		int errnosav, flags, fdmax;
		fd_set rdset, wrset;

		flags = fcntl (fd, F_GETFL);
		fcntl (fd, F_SETFL, flags | O_NONBLOCK);

		fdmax = MAX (fd, cancel_fd) + 1;


		do {
			struct timeval tv;
			int res;

			FD_ZERO (&rdset);
			FD_ZERO (&wrset);
			FD_SET (fd, &wrset);
			FD_SET (cancel_fd, &rdset);
			tv.tv_sec = BLOCKING_WRITE_TIMEOUT;
			tv.tv_usec = 0;
			w = -1;

			res = select (fdmax, &rdset, &wrset, 0, &tv);
			if (res == -1) {
				if (errno == EINTR)
					w = 0;
			} else if (res == 0)
				errno = ETIMEDOUT;
			else if (FD_ISSET (cancel_fd, &rdset))
				errno = EINTR;
			else {
				if (write_in_chunks) {
					/* Write in chunks of max WRITE_CHUNK_SIZE bytes */
					ssize_t actual = MIN (n - written, WRITE_CHUNK_SIZE);
					do {
						w = write (fd, buf + written, actual);
					} while (w == -1 && (errno == EINTR || errno == EAGAIN || 
							     errno == EWOULDBLOCK));			
				} else {
					do {
						w = write (fd, buf + written, n - written);
					} while (w == -1 && (errno == EINTR || errno == EAGAIN || 
							     errno == EWOULDBLOCK));	
				}
				if (w == -1) {
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						w = 0;
				} else
					written += w;
			}
		} while (w != -1 && written < n);

		errnosav = errno;
		fcntl (fd, F_SETFL, flags);
		errno = errnosav;
#endif
	}

	if (w == -1)
		return -1;

	return written;
}


/**
 * camel_write:
 * @fd: file descriptor
 * @buf: buffer to write
 * @n: number of bytes of @buf to write
 *
 * Cancellable libc write() replacement.
 *
 * Code that intends to be portable to Win32 should call this function
 * only on file descriptors returned from open(), not on sockets.
 *
 * Returns number of bytes written or -1 on fail. On failure, errno will
 * be set appropriately.
 **/
ssize_t
camel_write (int fd, const char *buf, size_t n)
{
	return camel_write_shared (fd, buf, n, FALSE);
}

ssize_t
camel_read_nb (int fd, char *buf, size_t n)
{
	ssize_t nread;

	int errnosav, flags, fdmax;
	fd_set rdset;

	flags = fcntl (fd, F_GETFL);
	fcntl (fd, F_SETFL, flags | O_NONBLOCK);

	do {
		struct timeval tv;
		int res;

		FD_ZERO (&rdset);
		FD_SET (fd, &rdset);
		fdmax = fd + 1;
		tv.tv_sec = NONBLOCKING_READ_TIMEOUT;
		tv.tv_usec = 0;
		nread = -1;

		res = select(fdmax, &rdset, 0, 0, &tv);
		if (res == -1)
			;
		else if (res == 0)
			errno = ETIMEDOUT;
		else {
			do {
				nread = read (fd, buf, n);
			} while (0 && (nread == -1 && errno == EINTR));
		}
	} while (0 && (nread == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)));

	errnosav = errno;
	fcntl (fd, F_SETFL, flags);
	errno = errnosav;

	return nread;
}


/**
 * camel_read_socket:
 * @fd: a socket
 * @buf: buffer to fill
 * @n: number of bytes to read into @buf
 *
 * Cancellable read() replacement for sockets. Code that intends to be
 * portable to Win32 should call this function only on sockets
 * returned from socket(), or accept().
 *
 * Returns number of bytes read or -1 on fail. On failure, errno will
 * be set appropriately. If the socket is nonblocking
 * camel_read_socket() will retry the read until it gets something.
 **/
ssize_t
camel_read_socket (int fd, char *buf, size_t n)
{
#ifndef G_OS_WIN32
	return camel_read (fd, buf, n);
#else
	ssize_t nread;
	int cancel_fd;

	if (camel_operation_cancel_check (NULL)) {
		errno = EINTR;
		return -1;
	}
	cancel_fd = camel_operation_cancel_fd (NULL);

	if (cancel_fd == -1) {
		int fdmax;
		fd_set rdset;
		u_long yes = 1;

		ioctlsocket (fd, FIONBIO, &yes);
		fdmax = fd + 1;
		do {
			struct timeval tv;
			int res;

			FD_ZERO (&rdset);
			FD_SET (fd, &rdset);
			tv.tv_sec = BLOCKING_READ_TIMEOUT;
			tv.tv_usec = 0;
			nread = -1;

			res = select(fdmax, &rdset, 0, 0, &tv);
			if (res == -1)
				;
			else if (res == 0)
				errno = ETIMEDOUT;
			} else {
				nread = recv (fd, buf, n, 0);
			}
		} while ((nread == -1 && WSAGetLastError () == WSAEWOULDBLOCK));

	} else {
		int fdmax;
		fd_set rdset;
		u_long yes = 1;

		ioctlsocket (fd, FIONBIO, &yes);
		fdmax = MAX (fd, cancel_fd) + 1;
		do {
			struct timeval tv;
			int res;

			FD_ZERO (&rdset);
			FD_SET (fd, &rdset);
			FD_SET (cancel_fd, &rdset);
			tv.tv_sec = BLOCKING_READ_TIMEOUT;
			tv.tv_usec = 0;
			nread = -1;

			res = select(fdmax, &rdset, 0, 0, &tv);
			if (res == -1)
				;
			else if (res == 0)
				errno = ETIMEDOUT;
			else if (FD_ISSET (cancel_fd, &rdset)) {
				errno = EINTR;
				goto failed;
			} else {
				nread = recv (fd, buf, n, 0);
			}
		} while (nread == -1 && WSAGetLastError () == WSAEWOULDBLOCK);
	failed:
		;
	}

	return nread;
#endif
}


ssize_t
camel_read_socket_nb (int fd, char *buf, size_t n)
{
#ifndef G_OS_WIN32
	return camel_read_nb (fd, buf, n);
#else
	ssize_t nread;
	int cancel_fd;

	if (camel_operation_cancel_check (NULL)) {
		errno = EINTR;
		return -1;
	}
	cancel_fd = camel_operation_cancel_fd (NULL);

	if (cancel_fd == -1) {

		int fdmax;
		fd_set rdset;
		u_long yes = 1;

		ioctlsocket (fd, FIONBIO, &yes);
		fdmax = fd + 1;
		do {
			struct timeval tv;
			int res;

			FD_ZERO (&rdset);
			FD_SET (fd, &rdset);
			tv.tv_sec = NONBLOCKING_READ_TIMEOUT;
			tv.tv_usec = 0;
			nread = -1;

			res = select(fdmax, &rdset, 0, 0, &tv);
			if (res == -1)
				;
			else if (res == 0)
				errno = ETIMEDOUT;
			} else {
				nread = recv (fd, buf, n, 0);
			}
		} while (0&&(nread == -1 && WSAGetLastError () == WSAEWOULDBLOCK));

	} else {
		int fdmax;
		fd_set rdset;
		u_long yes = 1;

		ioctlsocket (fd, FIONBIO, &yes);
		fdmax = MAX (fd, cancel_fd) + 1;
		do {
			struct timeval tv;
			int res;

			FD_ZERO (&rdset);
			FD_SET (fd, &rdset);
			FD_SET (cancel_fd, &rdset);
			tv.tv_sec = NONBLOCKING_READ_TIMEOUT;
			tv.tv_usec = 0;
			nread = -1;

			res = select(fdmax, &rdset, 0, 0, &tv);
			if (res == -1)
				;
			else if (res == 0)
				errno = ETIMEDOUT;
			else if (FD_ISSET (cancel_fd, &rdset)) {
				errno = EINTR;
				goto failed;
			} else {
				nread = recv (fd, buf, n, 0);
			}
		} while (0&&(nread == -1 && WSAGetLastError () == WSAEWOULDBLOCK));
	failed:
		;
	}

	return nread;
#endif
}



/**
 * camel_write_socket:
 * @fd: file descriptor
 * @buf: buffer to write
 * @n: number of bytes of @buf to write
 *
 * Cancellable write() replacement for sockets. Code that intends to
 * be portable to Win32 should call this function only on sockets
 * returned from socket() or accept().
 *
 * Returns number of bytes written or -1 on fail. On failure, errno will
 * be set appropriately.
 **/
ssize_t
camel_write_socket (int fd, const char *buf, size_t n)
{
#ifndef G_OS_WIN32
	return camel_write_shared (fd, buf, n, TRUE);
#else
	ssize_t w, written = 0;
	int cancel_fd;

	if (camel_operation_cancel_check (NULL)) {
		errno = EINTR;
		return -1;
	}

	cancel_fd = camel_operation_cancel_fd (NULL);
	if (cancel_fd == -1) {

		int fdmax;
		fd_set rdset, wrset;
		u_long arg = 1;

		ioctlsocket (fd, FIONBIO, &arg);
		fdmax = fd + 1;
		do {
			struct timeval tv;
			int res;

			FD_ZERO (&rdset);
			FD_ZERO (&wrset);
			FD_SET (fd, &wrset);
			tv.tv_sec = BLOCKING_WRITE_TIMEOUT;
			tv.tv_usec = 0;
			w = -1;

			res = select (fdmax, &rdset, &wrset, 0, &tv);
			if (res == SOCKET_ERROR) {
				/* w still being -1 will catch this */
			} else if (res == 0)
				errno = ETIMEDOUT;
			else {
				w = send (fd, buf + written, n - written, 0);
				if (w == SOCKET_ERROR) {
					if (WSAGetLastError () == WSAEWOULDBLOCK)
						w = 0;
				} else
					written += w;
			}
		} while (w != -1 && written < n);
		arg = 0;
		ioctlsocket (fd, FIONBIO, &arg);

	} else {
		int fdmax;
		fd_set rdset, wrset;
		u_long arg = 1;

		ioctlsocket (fd, FIONBIO, &arg);
		fdmax = MAX (fd, cancel_fd) + 1;
		do {
			struct timeval tv;
			int res;

			FD_ZERO (&rdset);
			FD_ZERO (&wrset);
			FD_SET (fd, &wrset);
			FD_SET (cancel_fd, &rdset);
			tv.tv_sec = BLOCKING_WRITE_TIMEOUT;
			tv.tv_usec = 0;
			w = -1;

			res = select (fdmax, &rdset, &wrset, 0, &tv);
			if (res == SOCKET_ERROR) {
				/* w still being -1 will catch this */
			} else if (res == 0)
				errno = ETIMEDOUT;
			else if (FD_ISSET (cancel_fd, &rdset))
				errno = EINTR;
			else {
				w = send (fd, buf + written, n - written, 0);
				if (w == SOCKET_ERROR) {
					if (WSAGetLastError () == WSAEWOULDBLOCK)
						w = 0;
				} else
					written += w;
			}
		} while (w != -1 && written < n);
		arg = 0;
		ioctlsocket (fd, FIONBIO, &arg);
	}

	if (w == -1)
		return -1;

	return written;
#endif
}


/**
 * camel_file_util_savename:
 * @filename: a pathname
 *
 * Builds a pathname where the basename is of the form ".#" + the
 * basename of @filename, for instance used in a two-stage commit file
 * write.
 *
 * Return value: The new pathname.  It must be free'd with g_free().
 **/
char *
camel_file_util_savename(const char *filename)
{
	char *dirname, *retval;

	dirname = g_path_get_dirname(filename);

	if (strcmp (dirname, ".") == 0) {
		retval = g_strconcat (".#", filename, NULL);
	} else {
		char *basename = g_path_get_basename(filename);
		char *newbasename = g_strconcat (".#", basename, NULL);

		retval = g_build_filename (dirname, newbasename, NULL);

		g_free (newbasename);
		g_free (basename);
	}
	g_free (dirname);

	return retval;
}
