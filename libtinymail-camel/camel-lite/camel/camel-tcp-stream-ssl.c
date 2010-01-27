/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2001 Ximian, Inc. (www.ximian.com)
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

/* NOTE: This is the default implementation of CamelTcpStreamSSL,
 * used when the Mozilla NSS libraries are used. If you configured
 * OpenSSL support instead, then this file won't be compiled and
 * the CamelTcpStreamSSL implementation in camel-tcp-stream-openssl.c
 * will be used instead.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_NSS

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <nspr.h>
#include <prio.h>
#include <prerror.h>
#include <prerr.h>
#include <secerr.h>
#include <sslerr.h>
#include "nss.h"    /* Don't use <> here or it will include the system nss.h instead */
#include <ssl.h>
#include <cert.h>
#include <certdb.h>
#include <pk11func.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <libedataserver/md5-utils.h>

#include "camel-certdb.h"
#include "camel-file-utils.h"
#include "camel-operation.h"
#include "camel-private.h"
#include "camel-session.h"
#include "camel-stream-fs.h"
#include "camel-tcp-stream-ssl.h"

static CamelTcpStreamClass *parent_class = NULL;

/* Returns the class for a CamelTcpStreamSSL */
#define CTSS_CLASS(so) CAMEL_TCP_STREAM_SSL_CLASS (CAMEL_OBJECT_GET_CLASS (so))

static ssize_t stream_read (CamelStream *stream, char *buffer, size_t n);
static ssize_t stream_write (CamelStream *stream, const char *buffer, size_t n);
static int stream_flush  (CamelStream *stream);
static int stream_close  (CamelStream *stream);

static PRFileDesc *enable_ssl (CamelTcpStreamSSL *ssl, PRFileDesc *fd);

static int stream_connect    (CamelTcpStream *stream, struct addrinfo *host);
static int stream_getsockopt (CamelTcpStream *stream, CamelSockOptData *data);
static int stream_setsockopt (CamelTcpStream *stream, const CamelSockOptData *data);
static struct sockaddr *stream_get_local_address (CamelTcpStream *stream, socklen_t *len);
static struct sockaddr *stream_get_remote_address (CamelTcpStream *stream, socklen_t *len);
static ssize_t stream_read_nb (CamelTcpStream *stream, char *buffer, size_t n);
static int stream_gettimeout (CamelTcpStream *stream);

static gboolean begin_read (CamelTcpStreamSSL *stream);
static void end_read (CamelTcpStreamSSL *stream);
static gint ssl_close (CamelTcpStreamSSL *stream);


struct _CamelTcpStreamSSLPrivate {
	PRFileDesc *sockfd;

	struct _CamelSession *session;
	char *expected_host;
	gboolean ssl_mode;
	guint32 flags;
	gboolean accepted;
	CamelService *service;
	guint reads;
	GMutex *reads_lock;
	gboolean scheduled_close;
};

/*
 * This method is used for making sure we don't close the ssl socket
 * before all the scheduled reads are finished. It also rejects read
 * if we have a pending close.
 *
 * The return value is FALSE when we didn't accept the request to begin
 * a read operation. In this case we shouldn't call end_read.
 */
static inline gboolean
begin_read (CamelTcpStreamSSL *stream)
{
	gboolean retval = TRUE;

	g_mutex_lock (stream->priv->reads_lock);
	if (stream->priv->scheduled_close) {
		retval = FALSE;
	} else {
		stream->priv->reads ++;
		camel_object_ref (stream);
	}
	g_mutex_unlock (stream->priv->reads_lock);
	return retval;
}

/*
 * This ends a read request started succesfully in begin_read. Then, if that
 * method returns FALSE, we shouldn't call this.
 *
 * It checks if there's a close call delayed because of reads pending. It it's
 * so, then it closes the socket and sets the sock reference to NULL.
 */
static inline void
end_read (CamelTcpStreamSSL *stream)
{
	gboolean close;
	g_mutex_lock (stream->priv->reads_lock);
	stream->priv->reads --;
	close =  ((stream->priv->reads == 0) && (stream->priv->scheduled_close));
	g_mutex_unlock (stream->priv->reads_lock);
	if (close) {
		PR_Shutdown (stream->priv->sockfd, PR_SHUTDOWN_BOTH);
		PR_Close (stream->priv->sockfd);
		stream->priv->sockfd = NULL;
	}
	camel_object_unref (stream);
}


/*
 * Closes the stream ssl socket, in case there are no reads pending, or sets
 * this as requested.
 */
static gint
ssl_close (CamelTcpStreamSSL *stream)
{
	gboolean close;
	g_mutex_lock (stream->priv->reads_lock);
	close = ((stream->priv->reads == 0) && (stream->priv->sockfd != NULL));
	if (stream->priv->sockfd != NULL)
		stream->priv->scheduled_close = TRUE;
	g_mutex_unlock (stream->priv->reads_lock);
	if (close) {
		PR_Shutdown (stream->priv->sockfd, PR_SHUTDOWN_BOTH);
		if (PR_Close (stream->priv->sockfd) == PR_FAILURE) {
			stream->priv->sockfd = NULL;
			return -1;
		}
		stream->priv->sockfd = NULL;
	}
	return 0;
}

static void 
ssl_enable_compress (CamelTcpStream *stream)
{
	/* TODO */ 
	g_print ("NSS COMPRESS enable request: not yet supported");
	return;
}

static void
camel_tcp_stream_ssl_class_init (CamelTcpStreamSSLClass *camel_tcp_stream_ssl_class)
{
	CamelTcpStreamClass *camel_tcp_stream_class =
		CAMEL_TCP_STREAM_CLASS (camel_tcp_stream_ssl_class);
	CamelStreamClass *camel_stream_class =
		CAMEL_STREAM_CLASS (camel_tcp_stream_ssl_class);

	parent_class = CAMEL_TCP_STREAM_CLASS (camel_type_get_global_classfuncs (camel_tcp_stream_get_type ()));

	camel_stream_class->read = stream_read;
	camel_stream_class->write = stream_write;
	camel_stream_class->flush = stream_flush;
	camel_stream_class->close = stream_close;

	camel_tcp_stream_class->enable_compress = ssl_enable_compress;
	camel_tcp_stream_class->gettimeout = stream_gettimeout;
	camel_tcp_stream_class->read_nb = stream_read_nb;
	camel_tcp_stream_class->connect = stream_connect;
	camel_tcp_stream_class->getsockopt = stream_getsockopt;
	camel_tcp_stream_class->setsockopt = stream_setsockopt;
	camel_tcp_stream_class->get_local_address  = stream_get_local_address;
	camel_tcp_stream_class->get_remote_address = stream_get_remote_address;
}

static int
stream_gettimeout (CamelTcpStream *stream)
{
	return 330;
}

static void
camel_tcp_stream_ssl_init (gpointer object, gpointer klass)
{
	CamelTcpStreamSSL *stream = CAMEL_TCP_STREAM_SSL (object);

	stream->priv = g_new0 (struct _CamelTcpStreamSSLPrivate, 1);
	stream->priv->reads_lock = g_mutex_new ();
}

static void
camel_tcp_stream_ssl_finalize (CamelObject *object)
{
	CamelTcpStreamSSL *stream = CAMEL_TCP_STREAM_SSL (object);

	if (stream->priv->sockfd != NULL) {
		ssl_close (stream);
	}

	if (stream->priv->session)
		camel_object_unref(stream->priv->session);

	g_mutex_free (stream->priv->reads_lock);

	g_free (stream->priv->expected_host);

	g_free (stream->priv);
}


CamelType
camel_tcp_stream_ssl_get_type (void)
{
	static CamelType type = CAMEL_INVALID_TYPE;

	if (type == CAMEL_INVALID_TYPE) {
		type = camel_type_register (camel_tcp_stream_get_type (),
					    "CamelTcpStreamSSL",
					    sizeof (CamelTcpStreamSSL),
					    sizeof (CamelTcpStreamSSLClass),
					    (CamelObjectClassInitFunc) camel_tcp_stream_ssl_class_init,
					    NULL,
					    (CamelObjectInitFunc) camel_tcp_stream_ssl_init,
					    (CamelObjectFinalizeFunc) camel_tcp_stream_ssl_finalize);
	}

	return type;
}


/**
 * camel_tcp_stream_ssl_new:
 * @session: an active #CamelSession object
 * @expected_host: host that the stream is expected to connect with
 * @flags: a bitwise combination of any of
 * #CAMEL_TCP_STREAM_SSL_ENABLE_SSL2,
 * #CAMEL_TCP_STREAM_SSL_ENABLE_SSL3 or
 * #CAMEL_TCP_STREAM_SSL_ENABLE_TLS
 *
 * Since the SSL certificate authenticator may need to prompt the
 * user, a #CamelSession is needed. @expected_host is needed as a
 * protection against an MITM attack.
 *
 * Returns a new #CamelTcpStreamSSL stream preset in SSL mode
 **/

static gboolean has_init = FALSE;

CamelStream *
camel_tcp_stream_ssl_new (CamelService *service, const char *expected_host, guint32 flags)
{
	CamelTcpStreamSSL *stream;

	g_assert(CAMEL_IS_SESSION(service->session));

	stream = CAMEL_TCP_STREAM_SSL (camel_object_new (camel_tcp_stream_ssl_get_type ()));

	stream->priv->session = service->session;

	if (!has_init) {
		char *str = camel_session_get_storage_path (stream->priv->session, service, NULL);
		if (!str)
			str = g_strdup (g_get_tmp_dir ());

		if (NSS_InitReadWrite (str) == SECFailure) {
			/* fall back on using volatile dbs? */
			if (NSS_NoDB_Init (str) == SECFailure) {
				g_warning ("Failed to initialize NSS");
				g_free (str);
				stream->priv->session = NULL;
				camel_stream_close (stream);
				camel_object_unref (stream);
				return NULL;
			}
		}
		has_init = TRUE;
		g_free (str);
	}

	camel_object_ref(service->session);
	stream->priv->expected_host = g_strdup (expected_host);
	stream->priv->ssl_mode = TRUE;
	stream->priv->flags = flags;
	stream->priv->service = service;

	return CAMEL_STREAM (stream);
}


/**
 * camel_tcp_stream_ssl_new_raw:
 * @service: an active #CamelService object
 * @expected_host: host that the stream is expected to connect with
 * @flags: a bitwise combination of any of
 * #CAMEL_TCP_STREAM_SSL_ENABLE_SSL2,
 * #CAMEL_TCP_STREAM_SSL_ENABLE_SSL3 or
 * #CAMEL_TCP_STREAM_SSL_ENABLE_TLS
 *
 * Since the SSL certificate authenticator may need to prompt the
 * user, a CamelSession is needed. @expected_host is needed as a
 * protection against an MITM attack.
 *
 * Returns a new #CamelTcpStreamSSL stream not yet toggled into SSL mode
 **/
CamelStream *
camel_tcp_stream_ssl_new_raw (CamelService *service, const char *expected_host, guint32 flags)
{
	CamelTcpStreamSSL *stream;

	g_assert(CAMEL_IS_SESSION(service->session));

	stream = CAMEL_TCP_STREAM_SSL (camel_object_new (camel_tcp_stream_ssl_get_type ()));

	stream->priv->session = service->session;
	camel_object_ref(service->session);
	stream->priv->expected_host = g_strdup (expected_host);
	stream->priv->ssl_mode = FALSE;
	stream->priv->flags = flags;
	stream->priv->service = service;

	return CAMEL_STREAM (stream);
}


static void
set_errno (int code)
{
	/* FIXME: this should handle more. */
	switch (code) {
	case PR_INVALID_ARGUMENT_ERROR:
		errno = EINVAL;
		break;
	case PR_PENDING_INTERRUPT_ERROR:
		errno = EINTR;
		break;
	case PR_IO_PENDING_ERROR:
		errno = EAGAIN;
		break;
#ifdef EWOULDBLOCK
	case PR_WOULD_BLOCK_ERROR:
		errno = EWOULDBLOCK;
		break;
#endif
#ifdef EINPROGRESS
	case PR_IN_PROGRESS_ERROR:
		errno = EINPROGRESS;
		break;
#endif
#ifdef EALREADY
	case PR_ALREADY_INITIATED_ERROR:
		errno = EALREADY;
		break;
#endif
#ifdef EHOSTUNREACH
	case PR_NETWORK_UNREACHABLE_ERROR:
		errno = EHOSTUNREACH;
		break;
#endif
#ifdef ECONNREFUSED
	case PR_CONNECT_REFUSED_ERROR:
		errno = ECONNREFUSED;
		break;
#endif
#ifdef ETIMEDOUT
	case PR_CONNECT_TIMEOUT_ERROR:
	case PR_IO_TIMEOUT_ERROR:
		errno = ETIMEDOUT;
		break;
#endif
#ifdef ENOTCONN
	case PR_NOT_CONNECTED_ERROR:
		errno = ENOTCONN;
		break;
#endif
#ifdef ECONNRESET
	case PR_CONNECT_RESET_ERROR:
		errno = ECONNRESET;
		break;
#endif
	case PR_IO_ERROR:
	default:
		errno = EIO;
		break;
	}
}


/**
 * camel_tcp_stream_ssl_enable_ssl:
 * @ssl: a #CamelTcpStreamSSL object
 *
 * Toggles an ssl-capable stream into ssl mode (if it isn't already).
 *
 * Returns %0 on success or %-1 on fail
 **/
int
camel_tcp_stream_ssl_enable_ssl (CamelTcpStreamSSL *ssl)
{
	PRFileDesc *fd;
	int nonblock;
	PRSocketOptionData sockopts;

	/* get O_NONBLOCK options */
	sockopts.option = PR_SockOpt_Nonblocking;
	PR_GetSocketOption (ssl->priv->sockfd, &sockopts);
	sockopts.option = PR_SockOpt_Nonblocking;
	nonblock = sockopts.value.non_blocking;
	sockopts.value.non_blocking = FALSE;
	PR_SetSocketOption (ssl->priv->sockfd, &sockopts);

	g_return_val_if_fail (CAMEL_IS_TCP_STREAM_SSL (ssl), -1);

	if (ssl->priv->sockfd && !ssl->priv->ssl_mode) {
		if (!(fd = enable_ssl (ssl, NULL))) {
			set_errno (PR_GetError ());
			return -1;
		}

		g_mutex_lock (ssl->priv->reads_lock);
		ssl->priv->sockfd = fd;
		ssl->priv->scheduled_close = FALSE;
		ssl->priv->reads = 0;
		g_mutex_unlock (ssl->priv->reads_lock);

		if (SSL_ResetHandshake (fd, FALSE) == SECFailure) {
			set_errno (PR_GetError ());
			return -1;
		}

		if (SSL_ForceHandshake (fd) == SECFailure) {
			set_errno (PR_GetError ());
			return -1;
		}
	}

	/* restore O_NONBLOCK options */
	sockopts.option = PR_SockOpt_Nonblocking;
	sockopts.value.non_blocking = nonblock;
	PR_SetSocketOption (ssl->priv->sockfd, &sockopts);

	ssl->priv->ssl_mode = TRUE;

	return 0;
}


static ssize_t
stream_read_nb (CamelTcpStream *stream, char *buffer, size_t n)
{
	CamelTcpStreamSSL *tcp_stream_ssl = CAMEL_TCP_STREAM_SSL (stream);
	ssize_t nread;
	PRSocketOptionData sockopts;
	PRPollDesc pollfds[2];
	gboolean nonblock;

	/* get O_NONBLOCK options */
	sockopts.option = PR_SockOpt_Nonblocking;
	PR_GetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);
	sockopts.option = PR_SockOpt_Nonblocking;
	nonblock = sockopts.value.non_blocking;
	sockopts.value.non_blocking = TRUE;
	PR_SetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);

	pollfds[0].fd = tcp_stream_ssl->priv->sockfd;
	pollfds[0].in_flags = PR_POLL_READ;

	do {
		PRInt32 res;

		pollfds[0].out_flags = 0;
		pollfds[1].out_flags = 0;
		nread = -1;

		res = PR_Poll(pollfds, 1, NONBLOCKING_READ_TIMEOUT);
		if (res == -1)
			set_errno(PR_GetError());
		else if (res == 0) {
#ifdef ETIMEDOUT
			errno = ETIMEDOUT;
#else
			errno = EIO;
#endif
			goto failed;
		} else {
			 do {
				nread = -1;
				if (begin_read (tcp_stream_ssl)) {
					if (PR_Available (tcp_stream_ssl->priv->sockfd) != 0)
						nread = PR_Read (tcp_stream_ssl->priv->sockfd, buffer, n);
					end_read (tcp_stream_ssl);
				}
				if (nread == -1)
					set_errno (PR_GetError ());
			 } while (0 && (nread == -1 && PR_GetError () == PR_PENDING_INTERRUPT_ERROR));
		}
	} while (0 && (nread == -1 && (PR_GetError () == PR_PENDING_INTERRUPT_ERROR ||
				 PR_GetError () == PR_IO_PENDING_ERROR ||
				 PR_GetError () == PR_WOULD_BLOCK_ERROR)));
 failed:
	/* restore O_NONBLOCK options */
	sockopts.option = PR_SockOpt_Nonblocking;
	sockopts.value.non_blocking = nonblock;
	PR_SetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);

	return nread;
}

static ssize_t
stream_read (CamelStream *stream, char *buffer, size_t n)
{
	CamelTcpStreamSSL *tcp_stream_ssl = CAMEL_TCP_STREAM_SSL (stream);
	PRFileDesc *cancel_fd;
	ssize_t nread;

	if (camel_operation_cancel_check (NULL)) {
		errno = EINTR;
		return -1;
	}

	cancel_fd = camel_operation_cancel_prfd (NULL);
	if (cancel_fd == NULL) {
		PRSocketOptionData sockopts;
		PRPollDesc pollfds[1];
		gboolean nonblock;
		int error;

		/* get O_NONBLOCK options */
		sockopts.option = PR_SockOpt_Nonblocking;
		PR_GetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);
		sockopts.option = PR_SockOpt_Nonblocking;
		nonblock = sockopts.value.non_blocking;
		sockopts.value.non_blocking = TRUE;
		PR_SetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);

		pollfds[0].fd = tcp_stream_ssl->priv->sockfd;
		pollfds[0].in_flags = PR_POLL_READ;

		do {
			PRInt32 res;

			pollfds[0].out_flags = 0;
			nread = -1;

			res = PR_Poll(pollfds, 1, PR_TicksPerSecond () * BLOCKING_READ_TIMEOUT);

			if (res == -1)
				set_errno(PR_GetError());
			else if (res == 0) {
#ifdef ETIMEDOUT
				errno = ETIMEDOUT;
#else
				errno = EIO;
#endif
				goto failed1;
			} else {
				do {
					nread = -1;
					if (begin_read (tcp_stream_ssl)) {
						nread = PR_Read (tcp_stream_ssl->priv->sockfd, buffer, n);
						if (nread == -1)
							set_errno (PR_GetError ());
						end_read (tcp_stream_ssl);
					}
				} while (nread == -1 && PR_GetError () == PR_PENDING_INTERRUPT_ERROR);
			}
		} while (nread == -1 && (PR_GetError () == PR_PENDING_INTERRUPT_ERROR ||
					 PR_GetError () == PR_IO_PENDING_ERROR ||
					 PR_GetError () == PR_WOULD_BLOCK_ERROR));

	failed1:
		/* restore O_NONBLOCK options */
		error = errno;
		sockopts.option = PR_SockOpt_Nonblocking;
		sockopts.value.non_blocking = nonblock;
		PR_SetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);
		errno = error;

	} else {
		PRSocketOptionData sockopts;
		PRPollDesc pollfds[2];
		gboolean nonblock;
		int error;

		/* get O_NONBLOCK options */
		sockopts.option = PR_SockOpt_Nonblocking;
		PR_GetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);
		sockopts.option = PR_SockOpt_Nonblocking;
		nonblock = sockopts.value.non_blocking;
		sockopts.value.non_blocking = TRUE;
		PR_SetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);

		pollfds[0].fd = tcp_stream_ssl->priv->sockfd;
		pollfds[0].in_flags = PR_POLL_READ;
		pollfds[1].fd = cancel_fd;
		pollfds[1].in_flags = PR_POLL_READ;

		do {
			PRInt32 res;

			pollfds[0].out_flags = 0;
			pollfds[1].out_flags = 0;
			nread = -1;

			res = PR_Poll(pollfds, 2, PR_TicksPerSecond () * BLOCKING_READ_TIMEOUT);

			if (res == -1)
				set_errno(PR_GetError());
			else if (res == 0) {
#ifdef ETIMEDOUT
				errno = ETIMEDOUT;
#else
				errno = EIO;
#endif
				goto failed2;
			} else if (pollfds[1].out_flags == PR_POLL_READ) {
				errno = EINTR;
				goto failed2;
			} else {
				do {
					nread = -1;
					if (begin_read (tcp_stream_ssl)) {
						nread = PR_Read (tcp_stream_ssl->priv->sockfd, buffer, n);
						if (nread == -1)
							set_errno (PR_GetError ());
						end_read (tcp_stream_ssl);
					}
				} while (nread == -1 && PR_GetError () == PR_PENDING_INTERRUPT_ERROR);
			}
		} while (nread == -1 && (PR_GetError () == PR_PENDING_INTERRUPT_ERROR ||
					 PR_GetError () == PR_IO_PENDING_ERROR ||
					 PR_GetError () == PR_WOULD_BLOCK_ERROR));

		/* restore O_NONBLOCK options */
	failed2:
		error = errno;
		sockopts.option = PR_SockOpt_Nonblocking;
		sockopts.value.non_blocking = nonblock;
		PR_SetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);
		errno = error;
	}

	return nread;
}


static ssize_t
stream_write (CamelStream *stream, const char *buffer, size_t n)
{
	CamelTcpStreamSSL *tcp_stream_ssl = CAMEL_TCP_STREAM_SSL (stream);
	ssize_t w, written = 0;
	PRFileDesc *cancel_fd;

	if (camel_operation_cancel_check (NULL)) {
		errno = EINTR;
		return -1;
	}

	cancel_fd = camel_operation_cancel_prfd (NULL);
	if (cancel_fd == NULL) {
		PRSocketOptionData sockopts;
		PRPollDesc pollfds[1];
		gboolean nonblock;
		int error;

		/* get O_NONBLOCK options */
		sockopts.option = PR_SockOpt_Nonblocking;
		PR_GetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);
		sockopts.option = PR_SockOpt_Nonblocking;
		nonblock = sockopts.value.non_blocking;
		sockopts.value.non_blocking = TRUE;
		PR_SetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);

		pollfds[0].fd = tcp_stream_ssl->priv->sockfd;
		pollfds[0].in_flags = PR_POLL_WRITE;

		do {
			PRInt32 res;

			/* Write in chunks of max WRITE_CHUNK_SIZE bytes */
			ssize_t actual = MIN (n - written, WRITE_CHUNK_SIZE);

			pollfds[0].out_flags = 0;
			w = -1;

			res = PR_Poll (pollfds, 1, PR_TicksPerSecond () * BLOCKING_WRITE_TIMEOUT);
			if (res == -1) {
				set_errno(PR_GetError());
				if (PR_GetError () == PR_PENDING_INTERRUPT_ERROR)
					w = 0;
			} else if (res == 0) {
#ifdef ETIMEDOUT
				errno = ETIMEDOUT;
#else
				errno = EIO;
#endif
			} else {
				do {
					w = -1;
					if (begin_read (tcp_stream_ssl)) {
						w = PR_Write (tcp_stream_ssl->priv->sockfd, buffer + written, actual /* n - written */);
						if (w == -1)
							set_errno (PR_GetError ());
						end_read (tcp_stream_ssl);
					} 
				} while (w == -1 && PR_GetError () == PR_PENDING_INTERRUPT_ERROR);

				if (w == -1) {
					if (PR_GetError () == PR_IO_PENDING_ERROR ||
					    PR_GetError () == PR_WOULD_BLOCK_ERROR)
						w = 0;
				} else
					written += w;
			}
		} while (w != -1 && written < n);

		/* restore O_NONBLOCK options */
		error = errno;
		sockopts.option = PR_SockOpt_Nonblocking;
		sockopts.value.non_blocking = nonblock;
		PR_SetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);
		errno = error;

	} else {
		PRSocketOptionData sockopts;
		PRPollDesc pollfds[2];
		gboolean nonblock;
		int error;

		/* get O_NONBLOCK options */
		sockopts.option = PR_SockOpt_Nonblocking;
		PR_GetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);
		sockopts.option = PR_SockOpt_Nonblocking;
		nonblock = sockopts.value.non_blocking;
		sockopts.value.non_blocking = TRUE;
		PR_SetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);

		pollfds[0].fd = tcp_stream_ssl->priv->sockfd;
		pollfds[0].in_flags = PR_POLL_WRITE;
		pollfds[1].fd = cancel_fd;
		pollfds[1].in_flags = PR_POLL_READ;

		do {
			PRInt32 res;

			/* Write in chunks of max WRITE_CHUNK_SIZE bytes */
			ssize_t actual = MIN (n - written, WRITE_CHUNK_SIZE);

			pollfds[0].out_flags = 0;
			pollfds[1].out_flags = 0;
			w = -1;

			res = PR_Poll (pollfds, 2, PR_TicksPerSecond () * BLOCKING_WRITE_TIMEOUT);
			if (res == -1) {
				set_errno(PR_GetError());
				if (PR_GetError () == PR_PENDING_INTERRUPT_ERROR)
					w = 0;
			} else if (res == 0) {
#ifdef ETIMEDOUT
				errno = ETIMEDOUT;
#else
				errno = EIO;
#endif
			} else if (pollfds[1].out_flags == PR_POLL_READ) {
				errno = EINTR;
			} else {
				do {
					w = -1;
					if (begin_read (tcp_stream_ssl)) {
						w = PR_Write (tcp_stream_ssl->priv->sockfd, buffer + written, actual /* n - written */);
						if (w == -1)
							set_errno (PR_GetError ());
						end_read (tcp_stream_ssl);
					}
				} while (w == -1 && PR_GetError () == PR_PENDING_INTERRUPT_ERROR);

				if (w == -1) {
					if (PR_GetError () == PR_IO_PENDING_ERROR ||
					    PR_GetError () == PR_WOULD_BLOCK_ERROR)
						w = 0;
				} else
					written += w;
			}
		} while (w != -1 && written < n);

		/* restore O_NONBLOCK options */
		error = errno;
		sockopts.option = PR_SockOpt_Nonblocking;
		sockopts.value.non_blocking = nonblock;
		PR_SetSocketOption (tcp_stream_ssl->priv->sockfd, &sockopts);
		errno = error;
	}

	if (w == -1)
		return -1;

	return written;
}

static int
stream_flush (CamelStream *stream)
{
	/*return PR_Sync (((CamelTcpStreamSSL *)stream)->priv->sockfd);*/
	return 0;
}

static int
stream_close (CamelStream *stream)
{
	if (((CamelTcpStreamSSL *)stream)->priv->sockfd == NULL) {
		errno = EINVAL;
		return -1;
	}

	ssl_close ((CamelTcpStreamSSL *) stream);

	((CamelTcpStreamSSL *)stream)->priv->sockfd = NULL;

	return 0;
}

#if 0
/* Since this is default implementation, let NSS handle it. */
static SECStatus
ssl_get_client_auth (void *data, PRFileDesc *sockfd,
		     struct CERTDistNamesStr *caNames,
		     struct CERTCertificateStr **pRetCert,
		     struct SECKEYPrivateKeyStr **pRetKey)
{
	SECStatus status = SECFailure;
	SECKEYPrivateKey *privkey;
	CERTCertificate *cert;
	void *proto_win;

	proto_win = SSL_RevealPinArg (sockfd);

	if ((char *) data) {
		cert = PK11_FindCertFromNickname ((char *) data, proto_win);
		if (cert) {
			privKey = PK11_FindKeyByAnyCert (cert, proto_win);
			if (privkey) {
				status = SECSuccess;
			} else {
				CERT_DestroyCertificate (cert);
			}
		}
	} else {
		/* no nickname given, automatically find the right cert */
		CERTCertNicknames *names;
		int i;

		names = CERT_GetCertNicknames (CERT_GetDefaultCertDB (),
					       SEC_CERT_NICKNAMES_USER,
					       proto_win);

		if (names != NULL) {
			for (i = 0; i < names->numnicknames; i++) {
				cert = PK11_FindCertFromNickname (names->nicknames[i],
								  proto_win);
				if (!cert)
					continue;

				/* Only check unexpired certs */
				if (CERT_CheckCertValidTimes (cert, PR_Now (), PR_FALSE) != secCertTimeValid) {
					CERT_DestroyCertificate (cert);
					continue;
				}

				status = NSS_CmpCertChainWCANames (cert, caNames);
				if (status == SECSuccess) {
					privkey = PK11_FindKeyByAnyCert (cert, proto_win);
					if (privkey)
						break;

					status = SECFailure;
					break;
				}

				CERT_FreeNicknames (names);
			}
		}
	}

	if (status == SECSuccess) {
		*pRetCert = cert;
		*pRetKey  = privkey;
	}

	return status;
}
#endif

#if 0
/* Since this is the default NSS implementation, no need for us to use this. */
static SECStatus
ssl_auth_cert (void *data, PRFileDesc *sockfd, PRBool checksig, PRBool is_server)
{
	CERTCertificate *cert;
	SECStatus status;
	void *pinarg;
	char *host;

	cert = SSL_PeerCertificate (sockfd);
	pinarg = SSL_RevealPinArg (sockfd);
	status = CERT_VerifyCertNow ((CERTCertDBHandle *)data, cert,
				     checksig, certUsageSSLClient, pinarg);

	if (status != SECSuccess)
		return SECFailure;

	/* Certificate is OK.  Since this is the client side of an SSL
	 * connection, we need to verify that the name field in the cert
	 * matches the desired hostname.  This is our defense against
	 * man-in-the-middle attacks.
	 */

	/* SSL_RevealURL returns a hostname, not a URL. */
	host = SSL_RevealURL (sockfd);

	if (host && *host) {
		status = CERT_VerifyCertName (cert, host);
	} else {
		PR_SetError (SSL_ERROR_BAD_CERT_DOMAIN, 0);
		status = SECFailure;
	}

	if (host)
		PR_Free (host);

	return secStatus;
}
#endif

static CamelCert *camel_certdb_nss_cert_get(CamelCertDB *certdb, CERTCertificate *cert, CamelSession *session);
static CamelCert *camel_certdb_nss_cert_add(CamelCertDB *certdb, CERTCertificate *cert, CamelSession *session);
static void camel_certdb_nss_cert_set(CamelCertDB *certdb, CamelCert *ccert, CERTCertificate *cert, CamelSession *session);

static char *
cert_fingerprint(CERTCertificate *cert)
{
	unsigned char fp[16];
	SECItem fpitem;
	char *fpstr;
	char *c;

	PK11_HashBuf (SEC_OID_MD5, fp, cert->derCert.data, cert->derCert.len);
	fpitem.data = fp;
	fpitem.len = sizeof (fp);
	fpstr = CERT_Hexify (&fpitem, 1);

	for (c = fpstr; *c != 0; c++) {
#ifdef G_OS_WIN32
		if (*c == ':')
			*c = '_';
#endif
		*c = g_ascii_tolower (*c);
	}

	return fpstr;
}

/* lookup a cert uses fingerprint to index an on-disk file */
static CamelCert *
camel_certdb_nss_cert_get(CamelCertDB *certdb, CERTCertificate *cert, CamelSession *session)
{
	char *fingerprint;
	CamelCert *ccert;

	fingerprint = cert_fingerprint (cert);
	ccert = camel_certdb_get_cert (certdb, fingerprint);
	if (ccert == NULL) {
		g_free (fingerprint);
		return ccert;
	}

	if (ccert->rawcert == NULL) {
		GByteArray *array;
		gchar *filename;
		gchar *contents;
		gsize length;
		GError *error = NULL;

		filename = g_build_filename (
			session->storage_path, ".camel_certs", fingerprint, NULL);
		g_file_get_contents (filename, &contents, &length, &error);
		if (error != NULL) {
			g_warning (
				"Could not load cert %s: %s",
				filename, error->message);
			g_error_free (error);

			camel_certdb_touch (certdb);
			g_free (fingerprint);
			g_free (filename);

			return ccert;
		}
		g_free (filename);

		array = g_byte_array_sized_new (length);
		g_byte_array_append (array, (guint8 *) contents, length);
		g_free (contents);

		ccert->rawcert = array;
	}

	g_free(fingerprint);
	if (ccert->rawcert->len != cert->derCert.len
	    || memcmp(ccert->rawcert->data, cert->derCert.data, cert->derCert.len) != 0) {
		g_warning("rawcert != derCer");
		camel_certdb_touch(certdb);
	}

	return ccert;
}

/* add a cert to the certdb */
static CamelCert *
camel_certdb_nss_cert_add(CamelCertDB *certdb, CERTCertificate *cert, CamelSession *session)
{
	CamelCert *ccert;
	char *fingerprint;

	fingerprint = cert_fingerprint(cert);

	ccert = camel_certdb_cert_new(certdb);
	camel_cert_set_issuer(certdb, ccert, CERT_NameToAscii(&cert->issuer));
	camel_cert_set_subject(certdb, ccert, CERT_NameToAscii(&cert->subject));
	/* hostname is set in caller */
	/*camel_cert_set_hostname(certdb, ccert, ssl->priv->expected_host);*/
	camel_cert_set_fingerprint(certdb, ccert, fingerprint);
	g_free(fingerprint);

	camel_certdb_nss_cert_set(certdb, ccert, cert, session);

	camel_certdb_add(certdb, ccert);

	return ccert;
}

/* set the 'raw' cert (& save it) */
static void
camel_certdb_nss_cert_set(CamelCertDB *certdb, CamelCert *ccert, CERTCertificate *cert, CamelSession *session)
{
	char *dir, *path, *fingerprint;
	CamelStream *stream;
	struct stat st;

	fingerprint = ccert->fingerprint;

	if (ccert->rawcert == NULL)
		ccert->rawcert = g_byte_array_new ();

	g_byte_array_set_size (ccert->rawcert, cert->derCert.len);
	memcpy (ccert->rawcert->data, cert->derCert.data, cert->derCert.len);

	dir = g_build_filename (session->storage_path, ".camel_certs", NULL);

	if (g_stat (dir, &st) == -1 && g_mkdir_with_parents (dir, 0700) == -1) {
		g_warning ("Could not create cert directory '%s': %s", dir, strerror (errno));
		g_free (dir);
		return;
	}

	path = g_strdup_printf ("%s/%s", dir, fingerprint);
	g_free (dir);

	stream = camel_stream_fs_new_with_name (path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (stream != NULL) {
		if (camel_stream_write (stream, (const char *) ccert->rawcert->data, ccert->rawcert->len) == -1) {
			g_warning ("Could not save cert: %s: %s", path, strerror (errno));
			g_unlink (path);
		}
		camel_stream_close (stream);
		camel_object_unref (stream);
	} else {
		g_warning ("Could not save cert: %s: %s", path, strerror (errno));
	}

	g_free (path);
}


#if 0
/* used by the mozilla-like code below */
static char *
get_nickname(CERTCertificate *cert)
{
	char *server, *nick = NULL;
	int i;
	PRBool status = PR_TRUE;

	server = CERT_GetCommonName(&cert->subject);
	if (server == NULL)
		return NULL;

	for (i=1;status == PR_TRUE;i++) {
		if (nick) {
			g_free(nick);
			nick = g_strdup_printf("%s #%d", server, i);
		} else {
			nick = g_strdup(server);
		}
		status = SEC_CertNicknameConflict(server, &cert->derSubject, cert->dbhandle);
	}

	return nick;
}
#endif

static SECStatus
ssl_bad_cert (void *data, PRFileDesc *sockfd)
{
	gboolean accept;
	CamelCertDB *certdb = NULL;
	CamelCert *ccert = NULL;
	char *prompt, *cert_str, *fingerprint;
	CamelTcpStreamSSL *ssl;
	CERTCertificate *cert;
	SECStatus status = SECFailure;
	struct _CamelTcpStreamSSLPrivate *priv;
	CamelCertTrust trust;

	g_return_val_if_fail (data != NULL, SECFailure);
	g_return_val_if_fail (CAMEL_IS_TCP_STREAM_SSL (data), SECFailure);

	ssl = data;
	priv = ssl->priv;

	cert = SSL_PeerCertificate (sockfd);
	if (cert == NULL)
		return SECFailure;

	certdb = camel_certdb_get_default();
	ccert = camel_certdb_nss_cert_get(certdb, cert, priv->session);
	if (ccert == NULL) {
		ccert = camel_certdb_nss_cert_add(certdb, cert, priv->session);
		camel_cert_set_hostname(certdb, ccert, ssl->priv->expected_host);
	}

	trust = camel_cert_get_trust (certdb, ccert);
	if (trust == CAMEL_CERT_TRUST_UNKNOWN) {
		status = CERT_VerifyCertNow(cert->dbhandle, cert, TRUE, certUsageSSLClient, NULL);
		fingerprint = cert_fingerprint(cert);
		cert_str = g_strdup_printf (_("Issuer:            %s\n"
					      "Subject:           %s\n"
					      "Fingerprint:       %s\n"
					      "Signature:         %s"),
					    ccert?camel_cert_get_issuer (certdb, ccert):CERT_NameToAscii (&cert->issuer),
					    ccert?camel_cert_get_subject (certdb, ccert):CERT_NameToAscii (&cert->subject),
					    fingerprint, status == SECSuccess?_("GOOD"):_("BAD"));
		g_free(fingerprint);

		/* construct our user prompt */
		prompt = g_strdup_printf (_("SSL Certificate check for %s:\n\n%s\n"),
					  ssl->priv->expected_host, cert_str);
		g_free (cert_str);

		/* query the user to find out if we want to accept this certificate */
		accept = camel_session_alert_user_with_id (ssl->priv->session, CAMEL_SESSION_ALERT_WARNING, CAMEL_EXCEPTION_SERVICE_CERTIFICATE, prompt, TRUE, priv->service);
		g_free(prompt);
		if (accept) {
			camel_certdb_nss_cert_set(certdb, ccert, cert, ssl->priv->session);
			camel_cert_set_trust(certdb, ccert, CAMEL_CERT_TRUST_FULLY);
			camel_certdb_touch(certdb);
		}
	} else {
		accept = trust != CAMEL_CERT_TRUST_NEVER;
	}

	camel_certdb_cert_unref(certdb, ccert);
	camel_object_unref(certdb);

	priv->accepted = accept;

	return accept ? SECSuccess : SECFailure;

#if 0
	int i, error;
	CERTCertTrust trust;
	SECItem *certs[1];
	int go = 1;
	char *host, *nick;

	error = PR_GetError();

	/* This code is basically what mozilla does - however it doesn't seem to work here
	   very reliably :-/ */
	while (go && status != SECSuccess) {
		char *prompt = NULL;

		printf("looping, error '%d'\n", error);

		switch(error) {
		case SEC_ERROR_UNKNOWN_ISSUER:
		case SEC_ERROR_CA_CERT_INVALID:
		case SEC_ERROR_UNTRUSTED_ISSUER:
		case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
			/* add certificate */
			printf("unknown issuer, adding ... \n");
			prompt = g_strdup_printf(_("Certificate problem: %s\nIssuer: %s"), cert->subjectName, cert->issuerName);

			CamelService *service = NULL; /* TODO: Is there a CamelService that we can use? */
			if (camel_session_alert_user_with_id(ssl->priv->session, CAMEL_SESSION_ALERT_WARNING, CAMEL_EXCEPTION_SERVICE_CERTIFICATE, prompt, TRUE, service)) {

				nick = get_nickname(cert);
				if (NULL == nick) {
					g_free(prompt);
					status = SECFailure;
					break;
				}

				printf("adding cert '%s'\n", nick);

				if (!cert->trust) {
					cert->trust = (CERTCertTrust*)PORT_ArenaZAlloc(cert->arena, sizeof(CERTCertTrust));
					CERT_DecodeTrustString(cert->trust, "P");
				}

				certs[0] = &cert->derCert;
				/*CERT_ImportCerts (cert->dbhandle, certUsageSSLServer, 1, certs, NULL, TRUE, FALSE, nick);*/
				CERT_ImportCerts(cert->dbhandle, certUsageUserCertImport, 1, certs, NULL, TRUE, FALSE, nick);
				g_free(nick);

				printf(" cert type %08x\n", cert->nsCertType);

				memset((void*)&trust, 0, sizeof(trust));
				if (CERT_GetCertTrust(cert, &trust) != SECSuccess) {
					CERT_DecodeTrustString(&trust, "P");
				}
				trust.sslFlags |= CERTDB_VALID_PEER | CERTDB_TRUSTED;
				if (CERT_ChangeCertTrust(cert->dbhandle, cert, &trust) != SECSuccess) {
					printf("couldn't change cert trust?\n");
				}

				/*status = SECSuccess;*/
#if 1
				/* re-verify? */
				status = CERT_VerifyCertNow(cert->dbhandle, cert, TRUE, certUsageSSLServer, NULL);
				error = PR_GetError();
				printf("re-verify status %d, error %d\n", status, error);
#endif

				printf(" cert type %08x\n", cert->nsCertType);
			} else {
				printf("failed/cancelled\n");
				go = 0;
			}

			break;
		case SSL_ERROR_BAD_CERT_DOMAIN:
			printf("bad domain\n");

			prompt = g_strdup_printf(_("Bad certificate domain: %s\nIssuer: %s"), cert->subjectName, cert->issuerName);

			CamelService *service = NULL; /* TODO: Is there a CamelService that we can use? */
			if (camel_session_alert_user_with_id (ssl->priv->session, CAMEL_SESSION_ALERT_WARNING, CAMEL_EXCEPTION_SERVICE_CERTIFICATE, prompt, TRUE, service)) {
				host = SSL_RevealURL(sockfd);
				status = CERT_AddOKDomainName(cert, host);
				printf("add ok domain name : %s\n", status == SECFailure?"fail":"ok");
				error = PR_GetError();
				if (status == SECFailure)
					go = 0;
			} else {
				go = 0;
			}

			break;

		case SEC_ERROR_EXPIRED_CERTIFICATE:
			printf("expired\n");

			prompt = g_strdup_printf(_("Certificate expired: %s\nIssuer: %s"), cert->subjectName, cert->issuerName);

			CamelService *service = NULL; /* TODO: Is there a CamelService that we can use? */
			if (camel_session_alert_user_with_id(ssl->priv->session, CAMEL_SESSION_ALERT_WARNING, CAMEL_EXCEPTION_SERVICE_CERTIFICATE, prompt, TRUE, service)) {
				cert->timeOK = PR_TRUE;
				status = CERT_VerifyCertNow(cert->dbhandle, cert, TRUE, certUsageSSLClient, NULL);
				error = PR_GetError();
				if (status == SECFailure)
					go = 0;
			} else {
				go = 0;
			}

			break;

		case SEC_ERROR_CRL_EXPIRED:
			printf("crl expired\n");

			prompt = g_strdup_printf(_("Certificate revocation list expired: %s\nIssuer: %s"), cert->subjectName, cert->issuerName);

			CamelService *service = NULL; /* TODO: Is there a CamelService that we can use? */
			if (camel_session_alert_user_with_id(ssl->priv->session, CAMEL_SESSION_ALERT_WARNING, CAMEL_EXCEPTION_SERVICE_CERTIFICATE, prompt, TRUE, service)) {
				host = SSL_RevealURL(sockfd);
				status = CERT_AddOKDomainName(cert, host);
			}

			go = 0;
			break;

		default:
			printf("generic error\n");
			go = 0;
			break;
		}

		g_free(prompt);
	}

	CERT_DestroyCertificate(cert);

	return status;
#endif
}

static PRFileDesc *
enable_ssl (CamelTcpStreamSSL *ssl, PRFileDesc *fd)
{
	PRFileDesc *ssl_fd;

	ssl_fd = SSL_ImportFD (NULL, fd ? fd : ssl->priv->sockfd);
	if (!ssl_fd)
		return NULL;

	SSL_OptionSet (ssl_fd, SSL_SECURITY, PR_TRUE);

	if (ssl->priv->flags & CAMEL_TCP_STREAM_SSL_ENABLE_SSL2) {
		SSL_OptionSet (ssl_fd, SSL_ENABLE_SSL2, PR_TRUE);
		SSL_OptionSet (ssl_fd, SSL_V2_COMPATIBLE_HELLO, PR_TRUE);
	} else {
		SSL_OptionSet (ssl_fd, SSL_ENABLE_SSL2, PR_FALSE);
		SSL_OptionSet (ssl_fd, SSL_V2_COMPATIBLE_HELLO, PR_FALSE);
	}

	if (ssl->priv->flags & CAMEL_TCP_STREAM_SSL_ENABLE_SSL3)
		SSL_OptionSet (ssl_fd, SSL_ENABLE_SSL3, PR_TRUE);
	else
		SSL_OptionSet (ssl_fd, SSL_ENABLE_SSL3, PR_FALSE);

	if (ssl->priv->flags & CAMEL_TCP_STREAM_SSL_ENABLE_TLS)
		SSL_OptionSet (ssl_fd, SSL_ENABLE_TLS, PR_TRUE);
	else
		SSL_OptionSet (ssl_fd, SSL_ENABLE_TLS, PR_FALSE);

	SSL_SetURL (ssl_fd, ssl->priv->expected_host);

	/*SSL_GetClientAuthDataHook (sslSocket, ssl_get_client_auth, (void *) certNickname);*/
	/*SSL_AuthCertificateHook (ssl_fd, ssl_auth_cert, (void *) CERT_GetDefaultCertDB ());*/
	SSL_BadCertHook (ssl_fd, ssl_bad_cert, ssl);

	ssl->priv->ssl_mode = TRUE;

	return ssl_fd;
}

static int
sockaddr_to_praddr(struct sockaddr *s, int len, PRNetAddr *addr)
{
	/* We assume the ip addresses are the same size - they have to be anyway.
	   We could probably just use memcpy *shrug* */

	memset(addr, 0, sizeof(*addr));

	if (s->sa_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)s;

		if (len < sizeof(*sin))
			return -1;

		addr->inet.family = PR_AF_INET;
		addr->inet.port = sin->sin_port;
		memcpy(&addr->inet.ip, &sin->sin_addr, sizeof(addr->inet.ip));

		return 0;
	}
#ifdef ENABLE_IPv6
	else if (s->sa_family == PR_AF_INET6) {
		struct sockaddr_in6 *sin = (struct sockaddr_in6 *)s;

		if (len < sizeof(*sin))
			return -1;

		addr->ipv6.family = PR_AF_INET6;
		addr->ipv6.port = sin->sin6_port;
		addr->ipv6.flowinfo = sin->sin6_flowinfo;
		memcpy(&addr->ipv6.ip, &sin->sin6_addr, sizeof(addr->ipv6.ip));
		addr->ipv6.scope_id = sin->sin6_scope_id;

		return 0;
	}
#endif

	return -1;
}

static int
socket_connect(CamelTcpStream *stream, struct addrinfo *host)
{
	CamelTcpStreamSSL *ssl = CAMEL_TCP_STREAM_SSL (stream);
	PRNetAddr netaddr;
	PRFileDesc *fd, *cancel_fd;

	if (sockaddr_to_praddr(host->ai_addr, host->ai_addrlen, &netaddr) != 0) {
		errno = EINVAL;
		return -1;
	}

	fd = PR_OpenTCPSocket(netaddr.raw.family);
	if (fd == NULL) {
		set_errno (PR_GetError ());
		return -1;
	}

	if (ssl->priv->ssl_mode) {
		PRFileDesc *ssl_fd;

		ssl_fd = enable_ssl (ssl, fd);
		if (ssl_fd == NULL) {
			int errnosave;

			set_errno (PR_GetError ());
			errnosave = errno;
			PR_Shutdown (fd, PR_SHUTDOWN_BOTH);
			PR_Close (fd);
			errno = errnosave;

			return -1;
		}

		fd = ssl_fd;
	}

	cancel_fd = camel_operation_cancel_prfd(NULL);

	if (PR_Connect (fd, &netaddr, cancel_fd?PR_INTERVAL_NO_WAIT:(CONNECT_TIMEOUT*1000)) == PR_FAILURE) {
		int errnosave;

		set_errno (PR_GetError ());
		if (PR_GetError () == PR_IN_PROGRESS_ERROR ||
		    (cancel_fd && (PR_GetError () == PR_CONNECT_TIMEOUT_ERROR ||
				   PR_GetError () == PR_IO_TIMEOUT_ERROR))) {
			gboolean connected = FALSE;
			PRPollDesc poll[2];

			poll[0].fd = fd;
			poll[0].in_flags = PR_POLL_WRITE | PR_POLL_EXCEPT;
			poll[1].fd = cancel_fd;
			poll[1].in_flags = PR_POLL_READ;

			do {
				poll[0].out_flags = 0;
				poll[1].out_flags = 0;

				if (PR_Poll (poll, cancel_fd?2:1, (CONNECT_TIMEOUT*1000)) == PR_FAILURE) {
					set_errno (PR_GetError ());
					goto exception;
				}

				if (poll[1].out_flags == PR_POLL_READ) {
					set_errno (PR_GetError ());
					goto exception;
				}

				if (PR_ConnectContinue(fd, poll[0].out_flags) == PR_FAILURE) {
					set_errno (PR_GetError ());
					if (PR_GetError () != PR_IN_PROGRESS_ERROR)
						goto exception;
				} else {
					connected = TRUE;
				}
			} while (!connected);
		} else {
		exception:
			errnosave = errno;
			PR_Shutdown (fd, PR_SHUTDOWN_BOTH);
			PR_Close (fd);
			errno = errnosave;

			return -1;
		}

		errno = 0;
	}

	g_mutex_lock (ssl->priv->reads_lock);
	ssl->priv->reads = 0;
	ssl->priv->scheduled_close = FALSE;
	ssl->priv->sockfd = fd;
	g_mutex_unlock (ssl->priv->reads_lock);

	return 0;
}

static int
stream_connect(CamelTcpStream *stream, struct addrinfo *host)
{
	while (host) {
		if (socket_connect(stream, host) == 0)
			return 0;
		host = host->ai_next;
	}

	return -1;
}

static int
stream_getsockopt (CamelTcpStream *stream, CamelSockOptData *data)
{
	PRSocketOptionData sodata;

	memset ((void *) &sodata, 0, sizeof (sodata));
	memcpy ((void *) &sodata, (void *) data, sizeof (CamelSockOptData));

	if (PR_GetSocketOption (((CamelTcpStreamSSL *)stream)->priv->sockfd, &sodata) == PR_FAILURE)
		return -1;

	memcpy ((void *) data, (void *) &sodata, sizeof (CamelSockOptData));

	return 0;
}

static int
stream_setsockopt (CamelTcpStream *stream, const CamelSockOptData *data)
{
	PRSocketOptionData sodata;

	memset ((void *) &sodata, 0, sizeof (sodata));
	memcpy ((void *) &sodata, (void *) data, sizeof (CamelSockOptData));

	if (PR_SetSocketOption (((CamelTcpStreamSSL *)stream)->priv->sockfd, &sodata) == PR_FAILURE)
		return -1;

	return 0;
}

static struct sockaddr *
sockaddr_from_praddr(PRNetAddr *addr, socklen_t *len)
{
	/* We assume the ip addresses are the same size - they have to be anyway */

	if (addr->raw.family == PR_AF_INET) {
		struct sockaddr_in *sin = g_malloc0(sizeof(*sin));

		sin->sin_family = AF_INET;
		sin->sin_port = addr->inet.port;
		memcpy(&sin->sin_addr, &addr->inet.ip, sizeof(sin->sin_addr));
		*len = sizeof(*sin);

		return (struct sockaddr *)sin;
	}
#ifdef ENABLE_IPv6
	else if (addr->raw.family == PR_AF_INET6) {
		struct sockaddr_in6 *sin = g_malloc0(sizeof(*sin));

		sin->sin6_family = AF_INET6;
		sin->sin6_port = addr->ipv6.port;
		sin->sin6_flowinfo = addr->ipv6.flowinfo;
		memcpy(&sin->sin6_addr, &addr->ipv6.ip, sizeof(sin->sin6_addr));
		sin->sin6_scope_id = addr->ipv6.scope_id;
		*len = sizeof(*sin);

		return (struct sockaddr *)sin;
	}
#endif

	return NULL;
}

static struct sockaddr *
stream_get_local_address(CamelTcpStream *stream, socklen_t *len)
{
	PRFileDesc *sockfd = CAMEL_TCP_STREAM_SSL (stream)->priv->sockfd;
	PRNetAddr addr;

	if (PR_GetSockName(sockfd, &addr) != PR_SUCCESS)
		return NULL;

	return sockaddr_from_praddr(&addr, len);
}

static struct sockaddr *
stream_get_remote_address (CamelTcpStream *stream, socklen_t *len)
{
	PRFileDesc *sockfd = CAMEL_TCP_STREAM_SSL (stream)->priv->sockfd;
	PRNetAddr addr;

	if (PR_GetPeerName(sockfd, &addr) != PR_SUCCESS)
		return NULL;

	return sockaddr_from_praddr(&addr, len);
}

#endif /* HAVE_NSS */
