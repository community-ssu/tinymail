/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
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


#ifndef CAMEL_FILE_UTILS_H
#define CAMEL_FILE_UTILS_H 1

#include <glib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

#include <camel/camel-store.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif


#define IDLE_READ_TIMEOUT 30
#define IDLE_WRITE_TIMEOUT 30

#define BLOCKING_READ_TIMEOUT 30
#define BLOCKING_WRITE_TIMEOUT 30
#define WRITE_CHUNK_SIZE 128

#define NONBLOCKING_READ_TIMEOUT 0
#define NONBLOCKING_WRITE_TIMEOUT 0

#define CONNECT_TIMEOUT 15


G_BEGIN_DECLS

int camel_file_util_encode_fixed_int32 (FILE *out, gint32);
int camel_file_util_decode_fixed_int32 (FILE *in, gint32 *);
int camel_file_util_encode_uint32 (FILE *out, guint32);
int camel_file_util_decode_uint32 (FILE *in, guint32 *);

int camel_file_util_encode_time_t (FILE *out, time_t);
int camel_file_util_decode_time_t (FILE *in, time_t *);
int camel_file_util_encode_off_t (FILE *out, off_t);
int camel_file_util_decode_off_t (FILE *in, off_t *);
int camel_file_util_encode_size_t (FILE *out, size_t);
int camel_file_util_decode_size_t (FILE *in, size_t *);
int camel_file_util_encode_string (FILE *out, const char *);
int camel_file_util_decode_string (FILE *in, char **);
int camel_file_util_encode_fixed_string (FILE *out, const char *str, size_t len);
int camel_file_util_decode_fixed_string (FILE *in, char **str, size_t len);


unsigned char* camel_file_util_mmap_decode_time_t(unsigned char *start, time_t *dest);
unsigned char* camel_file_util_mmap_decode_off_t(unsigned char *start, off_t *dest);
unsigned char* camel_file_util_mmap_decode_size_t(unsigned char *start, size_t *dest);
unsigned char* camel_file_util_mmap_decode_uint32 (unsigned char *start, guint32 *dest, gboolean is_string);

char *camel_file_util_safe_filename (const char *name);

/* Code that intends to be portable to Win32 should use camel_read()
 * and camel_write() only on file descriptors returned from open(),
 * creat(), pipe() or fileno(). On Win32 camel_read() and
 * camel_write() calls will not be cancellable. For sockets, use
 * camel_read_socket() and camel_write_socket(). These are cancellable
 * also on Win32.
 */
ssize_t camel_read (int fd, char *buf, size_t n);
ssize_t camel_write (int fd, const char *buf, size_t n);

ssize_t camel_write_socket (int fd, const char *buf, size_t n);

ssize_t camel_read_socket (int fd, char *buf, size_t n);
ssize_t camel_read_socket_nb (int fd, char *buf, size_t n);

char *camel_file_util_savename(const char *filename);
void camel_file_util_read_counts (const gchar *spath, CamelFolderInfo *fi);

void camel_file_util_read_counts_2 (const gchar *spath,
	guint32 *version, guint32 *flags,
	guint32 *nextuid, time_t *time, guint32 *saved_count,
	guint32 *unread_count, guint32 *deleted_count,
	guint32 *junk_count);

#define get_unaligned_u32(p) ((((unsigned char*)(p))[0]) | \
	(((unsigned char*)(p))[1] << 8) | \
	(((unsigned char*)(p))[2] << 16) | \
	(((unsigned char*)(p))[3] << 24))

G_END_DECLS

#endif /* CAMEL_FILE_UTILS_H */
