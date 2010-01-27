/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* camel-pop3-store.c : class for a pop3 store */

/*
 * Authors:
 *   Dan Winship <danw@ximian.com>
 *   Michael Zucchi <notzed@ximian.com>
 *   Philip Van Hoof <pvanhoof@gnome.org>
 *
 * This is CamelPop3Store for camel-lite that implements CamelDiscoStore. Its
 * implementation is significantly different from Camel's upstream version
 * (being used by Evolution): this version supports offline and online modes.
 *
 * Copyright (C) 2000-2002 Ximian, Inc. (www.ximian.com)
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

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <sys/types.h>

#include "camel-file-utils.h"

#include "camel-data-cache.h"
#include "camel-exception.h"
#include "camel-net-utils.h"
#include "camel-operation.h"
#include "camel-pop3-engine.h"
#include "camel-pop3-folder.h"
#include "camel-pop3-store.h"
#include "camel-sasl.h"
#include "camel-session.h"
#include "camel-stream-buffer.h"
#include "camel-tcp-stream-raw.h"
#include "camel-tcp-stream.h"
#include "camel-url.h"
#include "camel/camel-string-utils.h"

#ifdef HAVE_SSL
#include "camel-tcp-stream-ssl.h"
#endif
#include "camel-disco-diary.h"

#include <libedataserver/md5-utils.h>

/* Specified in RFC 1939 */
#define POP3_PORT "110"
#define POP3S_PORT "995"

#define _(o) o

static CamelStoreClass *parent_class = NULL;

static void finalize (CamelObject *object);

static gboolean pop3_connect (CamelService *service, CamelException *ex);
static gboolean pop3_disconnect (CamelService *service, gboolean clean, CamelException *ex);
static GList *query_auth_types (CamelService *service, CamelException *ex);

static CamelFolder *get_folder (CamelStore *store, const char *folder_name,
				guint32 flags, CamelException *ex);

static CamelFolder *get_trash  (CamelStore *store, CamelException *ex);

static void pop3_get_folder_status (CamelStore *store, const char *folder_name, int *unseen, int *messages, int *uidnext);


static int
pop3store_get_local_size (CamelStore *store, const gchar *folder_name)
{
	CamelPOP3Store *p3store = (CamelPOP3Store *) store;
	int msize = 0;
	camel_du (p3store->storage_path , &msize);
	return msize;
}

static char*
pop3_delete_cache  (CamelStore *store)
{
	CamelPOP3Store *pop3_store = (CamelPOP3Store *) store;
	gchar *folder_dir = pop3_store->storage_path;
	camel_rm (folder_dir);
	return g_strdup (folder_dir);
}


static gboolean
pop3_can_work_offline (CamelDiscoStore *disco_store)
{
	return TRUE;
}

static gboolean
pop3_connect_offline (CamelService *service, CamelException *ex)
{
	CamelPOP3Store *store = CAMEL_POP3_STORE (service);
	store->connected = !camel_exception_is_set (ex);
	return store->connected ;
}

static gboolean
unref_it (gpointer user_data)
{
	if (user_data)
		camel_object_unref (user_data);
	return FALSE;
}

static gboolean
pop3_connect_online (CamelService *service, CamelException *ex)
{
	return pop3_connect (service, ex);
}


static gboolean
pop3_disconnect_offline (CamelService *service, gboolean clean, CamelException *ex)
{
	return TRUE;
}

static gboolean
pop3_disconnect_online (CamelService *service, gboolean clean, CamelException *ex)
{

	CamelPOP3Store *store = CAMEL_POP3_STORE (service);

	g_static_rec_mutex_lock (store->eng_lock);

	if (store->engine == NULL) {
		g_static_rec_mutex_unlock (store->eng_lock);
		return TRUE;
	}

	if (clean) {
		CamelPOP3Command *pc;

		pc = camel_pop3_engine_command_new(store->engine, 0, NULL, NULL, "QUIT\r\n");
		while (camel_pop3_engine_iterate(store->engine, NULL) > 0)
			;
		camel_pop3_engine_command_free(store->engine, pc);
	}

	g_static_rec_mutex_unlock (store->eng_lock);

	g_timeout_add (20000, unref_it, store->engine);
	store->engine = NULL;

	/* camel_object_unref((CamelObject *)store->engine); */

	return TRUE;
}

static CamelFolder *
pop3_get_folder_online (CamelStore *store, const char *folder_name, guint32 flags, CamelException *ex)
{
	return get_folder (store, folder_name, flags, ex);
}


static CamelFolderInfo *
pop3_build_folder_info (CamelPOP3Store *store, const char *folder_name)
{
	const char *name;
	CamelFolderInfo *fi;
	gint msize = 0;
	gchar *folder_dir = store->storage_path;
	gchar *spath;
	FILE *f;

	fi = camel_folder_info_new ();

	fi->flags |= CAMEL_FOLDER_SYSTEM|CAMEL_FOLDER_TYPE_INBOX|CAMEL_FOLDER_SUBSCRIBED;

	fi->full_name = g_strdup(folder_name);
	fi->unread = 0;
	fi->total = 0;

	camel_du (folder_dir, &msize);

	spath = g_strdup_printf ("%s/summary.mmap", folder_dir);
	f = fopen (spath, "r");
	g_free (spath);
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
			fi->total = g_ntohl(get_unaligned_u32(ptr));
			ptr += 4;
			if (version < 0x100 && version >= 13)
				fi->unread = g_ntohl(get_unaligned_u32(ptr));
		}
		g_free (buffer);
		fclose (f);
	}

	fi->local_size = (guint) msize;

	fi->uri = g_strdup ("");
	name = strrchr (fi->full_name, '/');
	if (name == NULL)
		name = fi->full_name;
	else
		name++;
	if (!g_ascii_strcasecmp (fi->full_name, "INBOX"))
		fi->name = g_strdup ((const gchar *) _("Inbox"));
	else
		fi->name = g_strdup (name);

	return fi;
}

static CamelFolder *
pop3_get_folder_offline (CamelStore *store, const char *folder_name,
		    guint32 flags, CamelException *ex)
{
	return get_folder (store, folder_name, flags, ex);
}


static CamelFolderInfo *
pop3_get_folder_info_offline (CamelStore *store, const char *top, guint32 flags, CamelException *ex)
{
	return pop3_build_folder_info (CAMEL_POP3_STORE (store), "INBOX");
}

static CamelFolderInfo *
pop3_get_folder_info_online (CamelStore *store, const char *top, guint32 flags, CamelException *ex)
{
	CamelFolderInfo *info =  pop3_get_folder_info_offline (store, top, flags, ex);
	gint unseen = -1, messages = -1, uidnext = -1;

	pop3_get_folder_status (store, "INBOX", &unseen, &messages, &uidnext);

	if (messages != -1)
		info->total = messages;

	return info;
}


enum {
	MODE_CLEAR,
	MODE_SSL,
	MODE_TLS
};

#ifdef HAVE_SSL
#define SSL_PORT_FLAGS (CAMEL_TCP_STREAM_SSL_ENABLE_SSL2 | CAMEL_TCP_STREAM_SSL_ENABLE_SSL3)
#define STARTTLS_FLAGS (CAMEL_TCP_STREAM_SSL_ENABLE_TLS)
#endif

static void 
kill_lists (CamelPOP3Store *pop3_store)
{
	if (pop3_store->uids)
		g_ptr_array_free(pop3_store->uids, TRUE);
	pop3_store->uids = NULL;
	if (pop3_store->uids_uid)
		g_hash_table_destroy(pop3_store->uids_uid);
	if (pop3_store->uids_id)
		g_hash_table_destroy(pop3_store->uids_id);
}

void
camel_pop3_store_destroy_lists (CamelPOP3Store *pop3_store)
{
	g_static_rec_mutex_lock (pop3_store->uidl_lock);

	g_static_rec_mutex_lock (pop3_store->eng_lock);

	if (pop3_store->uids != NULL)
	{
		CamelPOP3FolderInfo **fi = (CamelPOP3FolderInfo **) pop3_store->uids->pdata;
		int i;

		for (i=0;i<pop3_store->uids->len;i++,fi++) {
			if (fi[0]->cmd) {

				if (pop3_store->engine == NULL) {
					g_ptr_array_free(pop3_store->uids, TRUE);
					g_hash_table_destroy(pop3_store->uids_uid);
					g_free(fi[0]->uid);
					g_free(fi[0]);
					g_static_rec_mutex_unlock (pop3_store->eng_lock);
					g_static_rec_mutex_unlock (pop3_store->uidl_lock);
					return;
				}

				while (camel_pop3_engine_iterate(pop3_store->engine, fi[0]->cmd) > 0)
					;
				camel_pop3_engine_command_free(pop3_store->engine, fi[0]->cmd);
				fi[0]->cmd = NULL;
			}

			g_free(fi[0]->uid);
			g_free(fi[0]);
		}

		kill_lists (pop3_store);

		pop3_store->uids = g_ptr_array_new ();
		pop3_store->uids_uid = g_hash_table_new(g_str_hash, g_str_equal);
		pop3_store->uids_id = g_hash_table_new(NULL, NULL);

	}

	g_static_rec_mutex_unlock (pop3_store->eng_lock);
	g_static_rec_mutex_unlock (pop3_store->uidl_lock);

}



static void
camel_pop3_store_destroy_lists_nl (CamelPOP3Store *pop3_store)
{

	if (pop3_store->uids != NULL)
	{
		CamelPOP3FolderInfo **fi = (CamelPOP3FolderInfo **) pop3_store->uids->pdata;
		int i;

		for (i=0;i<pop3_store->uids->len;i++,fi++) {
			if (fi[0]->cmd) {

				if (pop3_store->engine == NULL) {
					g_ptr_array_free(pop3_store->uids, TRUE);
					g_hash_table_destroy(pop3_store->uids_uid);
					g_free(fi[0]->uid);
					g_free(fi[0]);
					return;
				}

				while (camel_pop3_engine_iterate(pop3_store->engine, fi[0]->cmd) > 0)
					;
				camel_pop3_engine_command_free(pop3_store->engine, fi[0]->cmd);
				fi[0]->cmd = NULL;
			}

			g_free(fi[0]->uid);
			g_free(fi[0]);
		}

		kill_lists (pop3_store);

		pop3_store->uids = g_ptr_array_new ();
		pop3_store->uids_uid = g_hash_table_new(g_str_hash, g_str_equal);
		pop3_store->uids_id = g_hash_table_new(NULL, NULL);

	}
}

static gboolean
connect_to_server (CamelService *service, struct addrinfo *ai, int ssl_mode, int must_tls, CamelException *ex)
{
	CamelPOP3Store *store = CAMEL_POP3_STORE (service);
	CamelStream *tcp_stream;
	CamelPOP3Command *pc;
	guint32 flags = 0;
	int clean_quit = TRUE;
	int ret;
	gchar *delete_days;

	store->connected = FALSE;

	if (ssl_mode != MODE_CLEAR)
	{

#ifdef HAVE_SSL
		if (ssl_mode == MODE_TLS)
			tcp_stream = camel_tcp_stream_ssl_new_raw (service, service->url->host, STARTTLS_FLAGS);
		else
			tcp_stream = camel_tcp_stream_ssl_new (service, service->url->host, SSL_PORT_FLAGS);
#else

		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CONNECT,
				_("Could not connect to %s: %s"),
				service->url->host, _("SSL unavailable"));

		return FALSE;

#endif /* HAVE_SSL */
	} else
		tcp_stream = camel_tcp_stream_raw_new ();

	if (!tcp_stream) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CONNECT,
				_("Could not connect to %s: %s"),
				service->url->host, _("Couldn't create socket"));
		return FALSE;
	}

	if ((ret = camel_tcp_stream_connect ((CamelTcpStream *) tcp_stream, ai)) == -1) {
		if (errno == EINTR)
			camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL,
				_("Connection canceled"));
		else
			camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CONNECT,
				_("Could not connect to %s: %s"),
				service->url->host,
				g_strerror (errno));

		camel_object_unref (tcp_stream);

		return FALSE;
	}

	if (camel_url_get_param (service->url, "disable_extensions"))
		flags |= CAMEL_POP3_ENGINE_DISABLE_EXTENSIONS;

	if ((delete_days = (gchar *) camel_url_get_param(service->url,"delete_after")))
		store->delete_after =  atoi(delete_days);

	if (!tcp_stream) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CONNECT,
				_("Could not connect to %s: %s"),
				service->url->host, _("Couldn't create socket"));
		return FALSE;
	}

	if (!(store->engine = camel_pop3_engine_new (tcp_stream, flags))) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CONNECT,
			_("Failed to read a valid greeting from POP server %s"),
			service->url->host);
		camel_object_unref (tcp_stream);

		return FALSE;
	}
	store->engine->store = store;
	store->engine->partial_happening = FALSE;

#ifndef HAVE_SSL /* ! */
	g_static_rec_mutex_unlock (store->eng_lock);
#endif

	if (!must_tls && (ssl_mode != MODE_TLS)) {
		camel_object_unref (tcp_stream);
		store->connected = TRUE;
		return TRUE;
	}

#ifdef HAVE_SSL

	if (!(store->engine->capa & CAMEL_POP3_CAP_STLS)) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
			_("Failed to connect to POP server %s in secure mode: %s"),
			service->url->host, _("STLS not supported by server"));
		goto stls_exception;
	}

	/* as soon as we send a STLS command, all hope is lost of a clean QUIT if problems arise */
	clean_quit = FALSE;

	pc = camel_pop3_engine_command_new (store->engine, 0, NULL, NULL, "STLS\r\n");
	while (camel_pop3_engine_iterate (store->engine, NULL) > 0)
		;

	ret = pc->state == CAMEL_POP3_COMMAND_OK;
	camel_pop3_engine_command_free (store->engine, pc);

	if (ret == FALSE) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
			_("Failed to connect to POP server %s in secure mode: %s"),
			service->url->host, store->engine->line);
		goto stls_exception;
	}

	/* Okay, now toggle SSL/TLS mode */
	ret = camel_tcp_stream_ssl_enable_ssl (CAMEL_TCP_STREAM_SSL (tcp_stream));

	if (ret == -1) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
				      _("Failed to connect to POP server %s in secure mode: %s"),
				      service->url->host, _("TLS negotiations failed"));
		goto stls_exception;
	}

#else
	camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
		_("Failed to connect to POP server %s in secure mode: %s"),
		service->url->host, _("TLS is not available in this build"));

	goto stls_exception;
#endif /* HAVE_SSL */

	camel_object_unref (tcp_stream);

	/* rfc2595, section 4 states that after a successful STLS
	   command, the client MUST discard prior CAPA responses */

	camel_pop3_engine_reget_capabilities (store->engine);
	store->connected = TRUE;

	return TRUE;

 stls_exception:


	if (clean_quit) {
		/* try to disconnect cleanly */
		pc = camel_pop3_engine_command_new (store->engine, 0, NULL, NULL, "QUIT\r\n");
		while (camel_pop3_engine_iterate (store->engine, NULL) > 0)
			;
		camel_pop3_engine_command_free (store->engine, pc);
	}

	/* camel_pop3_engine_iterate could issue a disconnect that
	   could set engine to NULL */
	if (store->engine)
		camel_object_unref (CAMEL_OBJECT (store->engine));
	camel_object_unref (CAMEL_OBJECT (tcp_stream));
	g_static_rec_mutex_lock (store->eng_lock);
	store->engine = NULL;
	g_static_rec_mutex_unlock (store->eng_lock);
	store->connected = FALSE;

	return FALSE;
}

static struct {
	char *value;
	char *serv;
	char *port;
	int mode;
	int must_tls;
} ssl_options[] = {
	{ "",              "pop3s", POP3S_PORT, MODE_SSL, 0   },  /* really old (1.x) */
	{ "wrapped",       "pop3s", POP3S_PORT, MODE_SSL, 0   },
	{ "tls",           "pop3",  POP3_PORT,  MODE_TLS, 1   },
	{ "when-possible", "pop3",  POP3_PORT,  MODE_TLS, 0   },
	{ "never",         "pop3",  POP3_PORT,  MODE_CLEAR, 0 },
	{ NULL,            "pop3",  POP3_PORT,  MODE_CLEAR, 0 },
};

static gboolean
connect_to_server_wrapper (CamelService *service, CamelException *ex)
{
	struct addrinfo hints, *ai;
	const char *ssl_mode;
	int mode, ret, i, must_tls=0;
	char *serv;
	const char *port;

	if (!service->url)
		return FALSE;

	if ((ssl_mode = camel_url_get_param (service->url, "use_ssl"))) {
		for (i = 0; ssl_options[i].value; i++)
			if (!strcmp (ssl_options[i].value, ssl_mode))
				break;
		mode = ssl_options[i].mode;
		serv = ssl_options[i].serv;
		port = ssl_options[i].port;
		must_tls = ssl_options[i].must_tls;
	} else {
		mode = MODE_CLEAR;
		serv = "pop3";
		port = POP3S_PORT;
		must_tls = 0;
	}

	if (service->url->port) {
		serv = g_alloca (16);
		sprintf (serv, "%d", service->url->port);
		port = NULL;
	}

	memset (&hints, 0, sizeof (hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = PF_UNSPEC;
	if (!service->url)
		return FALSE;
	ai = camel_getaddrinfo(service->url->host, serv, &hints, ex);
	if (ai == NULL && port != NULL && camel_exception_get_id(ex) != CAMEL_EXCEPTION_USER_CANCEL) {
		camel_exception_clear (ex);
		ai = camel_getaddrinfo(service->url->host, port, &hints, ex);
	}

	if (ai == NULL)
		return FALSE;

	ret = connect_to_server (service, ai, mode, must_tls, ex);

	camel_freeaddrinfo (ai);

	return ret;
}

extern CamelServiceAuthType camel_pop3_password_authtype;
extern CamelServiceAuthType camel_pop3_apop_authtype;

static GList *
query_auth_types (CamelService *service, CamelException *ex)
{
	CamelPOP3Store *store = CAMEL_POP3_STORE (service);
	GList *types = NULL;

	types = CAMEL_SERVICE_CLASS (parent_class)->query_auth_types (service, ex);
	if (camel_exception_is_set (ex))
		return NULL;

	if (connect_to_server_wrapper (service, NULL)) {

		g_static_rec_mutex_lock (store->eng_lock);

		if (store->engine == NULL) {
			g_static_rec_mutex_unlock (store->eng_lock);
			return NULL;
		}

		types = g_list_concat(types, g_list_copy(store->engine->auth));

		pop3_disconnect (service, TRUE, NULL);

		g_static_rec_mutex_unlock (store->eng_lock);

	} else {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CONNECT,
				      _("Could not connect to POP server %s"),
				      service->url->host);
	}

	return types;
}

/**
 * camel_pop3_store_expunge:
 * @store: the store
 * @ex: a CamelException
 *
 * Expunge messages from the store. This will result in the connection
 * being closed, which may cause later commands to fail if they can't
 * reconnect.
 **/
void
camel_pop3_store_expunge (CamelPOP3Store *store, CamelException *ex)
{
	CamelPOP3Command *pc;

	if (!store->engine)
		return;

	pc = camel_pop3_engine_command_new(store->engine, 0, NULL, NULL, "QUIT\r\n");
	while (camel_pop3_engine_iterate(store->engine, NULL) > 0)
		;
	camel_pop3_engine_command_free(store->engine, pc);

	camel_service_disconnect (CAMEL_SERVICE (store), FALSE, ex);
}

static int
try_sasl(CamelPOP3Store *store, const char *mech, CamelException *ex)
{
	CamelPOP3Stream *stream = store->engine->stream;
	unsigned char *line, *resp;
	CamelSasl *sasl;
	unsigned int len;
	int ret;

	sasl = camel_sasl_new("pop3", mech, (CamelService *)store);
	if (sasl == NULL) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
				      _("Unable to connect to POP server %s: "
					"No support for requested authentication mechanism."),
				      CAMEL_SERVICE (store)->url->host);
		return -1;
	}

	if (camel_stream_printf((CamelStream *)stream, "AUTH %s\r\n", mech) == -1)
		goto ioerror;

	while (1) {
		if (camel_pop3_stream_line(stream, &line, &len) == -1)
			goto ioerror;
		if (strncmp((char *) line, "+OK", 3) == 0)
			break;
		if (strncmp((char *) line, "-ERR", 4) == 0) {
			camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
					      _("SASL `%s' Login failed for POP server %s: %s"),
					      mech, CAMEL_SERVICE (store)->url->host, line);
			goto done;
		}
		/* If we dont get continuation, or the sasl object's run out of work, or we dont get a challenge,
		   its a protocol error, so fail, and try reset the server */
		if (strncmp((char *) line, "+ ", 2) != 0
		    || camel_sasl_authenticated(sasl)
		    || (resp = (unsigned char *) camel_sasl_challenge_base64(sasl, (const char *) line+2, ex)) == NULL) {
			camel_stream_printf((CamelStream *)stream, "*\r\n");
			camel_pop3_stream_line(stream, &line, &len);
			camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
					      _("Cannot login to POP server %s: SASL Protocol error"),
					      CAMEL_SERVICE (store)->url->host);
			goto done;
		}

		ret = camel_stream_printf((CamelStream *)stream, "%s\r\n", resp);
		g_free(resp);
		if (ret == -1)
			goto ioerror;

	}
	camel_object_unref((CamelObject *)sasl);
	return 0;

 ioerror:
	if (errno == EINTR) {
		camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL, _("Canceled"));
	} else {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
				      _("Failed to authenticate on POP server %s: %s"),
				      CAMEL_SERVICE (store)->url->host, g_strerror (errno));
	}
 done:
	camel_object_unref((CamelObject *)sasl);
	return -1;
}

static int
pop3_try_authenticate (CamelService *service, gboolean reprompt, const char *errmsg, CamelException *ex)
{
	CamelPOP3Store *store = (CamelPOP3Store *)service;
	CamelPOP3Command *pcu = NULL, *pcp = NULL;
	int status;

	/* override, testing only */
	/*printf("Forcing authmech to 'login'\n");
	service->url->authmech = g_strdup("LOGIN");*/

	if (!service->url->passwd) {
		char *base_prompt;
		char *full_prompt;
		guint32 flags = CAMEL_SESSION_PASSWORD_SECRET;

		if (reprompt)
			flags |= CAMEL_SESSION_PASSWORD_REPROMPT;

		base_prompt = camel_session_build_password_prompt (
			"POP", service->url->user, service->url->host);

		if (errmsg != NULL)
			full_prompt = g_strconcat (errmsg, base_prompt, NULL);
		else
			full_prompt = g_strdup (base_prompt);

		service->url->passwd = camel_session_get_password (
			camel_service_get_session (service), service,
			NULL, full_prompt, "password", flags, ex);

		g_free (base_prompt);
		g_free (full_prompt);
		if (!service->url->passwd)
			return FALSE;
	}

	if (!service->url->authmech) {

		g_static_rec_mutex_lock (store->eng_lock);

		if (store->engine == NULL) {
			g_static_rec_mutex_unlock (store->eng_lock);
			return FALSE;
		}

		/* pop engine will take care of pipelining ability */
		pcu = camel_pop3_engine_command_new(store->engine, 0, NULL, NULL, "USER %s\r\n", service->url->user);
		pcp = camel_pop3_engine_command_new(store->engine, 0, NULL, NULL, "PASS %s\r\n", service->url->passwd);

		g_static_rec_mutex_unlock (store->eng_lock);

	} else if (strcmp(service->url->authmech, "+APOP") == 0 && store->engine && store->engine->apop) {
		char *secret, md5asc[33], *d;
		unsigned char md5sum[16], *s;

		d = store->engine->apop;

		while (*d != '\0') {
			if (!isascii((int)*d)) {

				/* README for Translators: The string APOP should not be translated */
				camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
						_("Unable to connect to POP server %s:	Invalid APOP ID received. Impersonation attack suspected. Please contact your admin."),
						CAMEL_SERVICE (store)->url->host);

				return FALSE;
			}
			d++;
		}

		if (store->engine == NULL) {
			return FALSE;
		}

		secret = g_alloca(strlen(store->engine->apop)+strlen(service->url->passwd)+1);
		sprintf(secret, "%s%s",  store->engine->apop, service->url->passwd);
		md5_get_digest(secret, strlen (secret), md5sum);

		for (s = md5sum, d = md5asc; d < md5asc + 32; s++, d += 2)
			sprintf (d, "%.2x", *s);

		pcp = camel_pop3_engine_command_new(store->engine, 0, NULL, NULL, "APOP %s %s\r\n",
						    service->url->user, md5asc);

	} else {
		CamelServiceAuthType *auth;
		GList *l;

		if (store->engine == NULL) {
			return FALSE;
		}

		l = store->engine->auth;
		while (l) {
			auth = l->data;
			if (strcmp(auth->authproto, service->url->authmech) == 0) {
				return try_sasl(store, service->url->authmech, ex) == -1;
			}
			l = l->next;
		}

		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
				      _("Unable to connect to POP server %s: "
					"No support for requested authentication mechanism."),
				      CAMEL_SERVICE (store)->url->host);
		return FALSE;
	}

	if (store->engine == NULL) {
		return FALSE;
	}

	while ((status = camel_pop3_engine_iterate(store->engine, pcp)) > 0)
		;

	if (status == -1) {
		if (errno == EINTR) {
			camel_exception_set (ex, CAMEL_EXCEPTION_USER_CANCEL, _("Canceled"));
		} else {
			camel_exception_setv (ex, CAMEL_EXCEPTION_SYSTEM,
					      _("Unable to connect to POP server %s.\n"
						"Error sending password: %s"),
					      CAMEL_SERVICE (store)->url->host,
					      errno ? g_strerror (errno) : _("Unknown error"));
		}
	} else if (pcu && pcu->state != CAMEL_POP3_COMMAND_OK) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
				      _("Unable to connect to POP server %s.\n"
					"Error sending username: %s"),
				      CAMEL_SERVICE (store)->url->host,
				      store->engine->line ? (char *)store->engine->line : _("Unknown error"));
	} else if (pcp->state != CAMEL_POP3_COMMAND_OK)
		camel_exception_setv (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
				      _("Unable to connect to POP server %s.\n"
					"Error sending password: %s"),
				      CAMEL_SERVICE (store)->url->host,
				      store->engine->line ? (char *)store->engine->line : _("Unknown error"));

	camel_pop3_engine_command_free (store->engine, pcp);

	if (pcu)
		camel_pop3_engine_command_free(store->engine, pcu);

	/* camel_pop3_engine_reget_capabilities (store->engine); */

	return status;
}

static gboolean
pop3_connect (CamelService *service, CamelException *ex)
{
	CamelPOP3Store *store = (CamelPOP3Store *)service;
	gboolean reprompt = FALSE;
	CamelSession *session;
	char *errbuf = NULL;
	int status;
	int mytry;
	gboolean auth = FALSE;

	session = camel_service_get_session (service);

	if (!connect_to_server_wrapper (service, ex))
		return FALSE;

	mytry = 0;
	while (mytry < 3) {
		status = pop3_try_authenticate (service, reprompt, errbuf, ex);
		g_free (errbuf);
		errbuf = NULL;

		/* we only re-prompt if we failed to authenticate, any other error and we just abort */
		if (status == 0 && camel_exception_get_id (ex) == CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE) {
			gchar *tmp = camel_utf8_make_valid (camel_exception_get_description (ex));
			errbuf = g_markup_printf_escaped ("%s\n\n", tmp);
			g_free (tmp);
			camel_exception_clear (ex);

			camel_session_forget_password (session, service, NULL, "password", ex);
			camel_exception_clear (ex);

			g_free (service->url->passwd);
			service->url->passwd = NULL;
			reprompt = TRUE;

			sleep (5); /* For example Cyrus-POPd dislikes hammering */
		} else {
			auth = TRUE;
			break;
		}

		mytry++;
	}

	g_free (errbuf);

	if (!auth) {
		camel_exception_set (ex, CAMEL_EXCEPTION_SERVICE_CANT_AUTHENTICATE,
			_("Authentication failure"));
		camel_service_disconnect(service, TRUE, ex);
		return FALSE;
	}

	if (status == -1 || camel_exception_is_set(ex)) {
		camel_service_disconnect(service, TRUE, ex);
		return FALSE;
	}

	/* Now that we are in the TRANSACTION state, try regetting the capabilities */

	if (store->engine == NULL) {
		return FALSE;
	}

	store->engine->state = CAMEL_POP3_ENGINE_TRANSACTION;
	camel_pop3_engine_reget_capabilities (store->engine);

	return TRUE;
}


// This is called locked by query_auth_types
static gboolean
pop3_disconnect (CamelService *service, gboolean clean, CamelException *ex)
{
	CamelPOP3Store *store = CAMEL_POP3_STORE (service);

	//g_static_rec_mutex_lock (store->eng_lock);

	if (store->engine == NULL) {
		//g_static_rec_mutex_unlock (store->eng_lock);
		return TRUE;
	}

	if (clean) {
		CamelPOP3Command *pc;

		pc = camel_pop3_engine_command_new(store->engine, 0, NULL, NULL, "QUIT\r\n");
		while (camel_pop3_engine_iterate(store->engine, NULL) > 0)
			;
		camel_pop3_engine_command_free(store->engine, pc);
	}

	g_timeout_add (20000, unref_it, store->engine);
	store->engine = NULL;
	//g_static_rec_mutex_unlock (store->eng_lock);

	return TRUE;
}

static CamelFolder *
get_folder (CamelStore *store, const char *folder_name, guint32 flags, CamelException *ex)
{
	if (g_ascii_strcasecmp (folder_name, "inbox") != 0) {
		camel_exception_setv (ex, CAMEL_EXCEPTION_FOLDER_INVALID,
				      _("No such folder `%s'."), folder_name);
		return NULL;
	}
	return camel_pop3_folder_new (store, ex);
}

static CamelFolder *
get_trash (CamelStore *store, CamelException *ex)
{
	/* no-op */
	return NULL;
}


static void
finalize (CamelObject *object)
{
	CamelPOP3Store *pop3_store = CAMEL_POP3_STORE (object);

	camel_service_disconnect (CAMEL_SERVICE (pop3_store), TRUE, NULL);

	g_static_rec_mutex_lock (pop3_store->eng_lock);
	if (pop3_store->engine)
		camel_object_unref((CamelObject *)pop3_store->engine);
	pop3_store->engine = NULL;
	g_static_rec_mutex_unlock (pop3_store->eng_lock);

	if (pop3_store->cache)
		camel_object_unref((CamelObject *)pop3_store->cache);
	pop3_store->cache = NULL;
	if (pop3_store->storage_path)
		g_free (pop3_store->storage_path);
	pop3_store->storage_path = NULL;

	/* g_static_rec_mutex_free (pop3_store->eng_lock); */
	g_free (pop3_store->eng_lock);
	pop3_store->eng_lock = NULL;

	g_free (pop3_store->uidl_lock);
	pop3_store->uidl_lock = NULL;

	kill_lists (pop3_store);

	camel_object_unref (pop3_store->book);
	pop3_store->book = NULL;
	return;
}

typedef struct {
	guint items, bytes;
} StatInfo;

static int
cmd_stat (CamelPOP3Engine *pe, CamelPOP3Stream *stream, void *data)
{
	unsigned char *line = (unsigned char *) stream; /* moeha, ugly! */
	StatInfo *info = data;

	if (!line)
		return 0;

	sscanf((char *) line, "+OK %d %d", (int *) &info->items, (int *) &info->bytes);

	return 1;
}

static void
pop3_get_folder_status (CamelStore *store, const char *folder_name, int *unseen, int *messages, int *uidnext)
{
	CamelPOP3Store *pop3_store = (CamelPOP3Store *) store;
	CamelPOP3Command *cmd = NULL;
	StatInfo *info = NULL;
	int i = -1;

	gchar *spath = g_strdup_printf ("%s/summary.mmap", pop3_store->storage_path);
	guint32 mversion=-1; guint32 mflags=-1;
	guint32 mnextuid=-1; time_t mtime=-1; guint32 msaved_count=-1;
	guint32 munread_count=-1; guint32 mdeleted_count=-1;
	guint32 mjunk_count=-1;

	camel_file_util_read_counts_2 (spath, &mversion, &mflags, &mnextuid,
		&mtime, &msaved_count, &munread_count, &mdeleted_count,
		&mjunk_count);

	if (munread_count != -1)
		*unseen = munread_count;
	if (msaved_count != -1)
		*messages = msaved_count;
	if (mnextuid != -1)
		*uidnext = mnextuid;

	g_free (spath);

	if (camel_disco_store_status (CAMEL_DISCO_STORE (pop3_store)) == CAMEL_DISCO_STORE_OFFLINE)
		return;

	g_static_rec_mutex_lock (pop3_store->eng_lock);

	if (pop3_store->engine == NULL) {
		g_static_rec_mutex_unlock (pop3_store->eng_lock);
		return;
	}

	camel_operation_start(NULL, _("Getting POP3 status"));

	info = g_slice_new (StatInfo);

	info->items = -1;
	info->bytes = -1;

	cmd = camel_pop3_engine_command_new(pop3_store->engine, 0,
			cmd_stat, info, "STAT\r\n");
	while ((i = camel_pop3_engine_iterate(pop3_store->engine, NULL)) > 0);

	if (info->items != -1) {
		*messages = info->items;
		if (munread_count == -1) {
			CamelFolderInfo *inf = pop3_build_folder_info (
				CAMEL_POP3_STORE (store), "INBOX");
			*unseen = inf->unread;
			camel_folder_info_free (inf);
		}
	}

	g_slice_free (StatInfo, info);
	camel_pop3_engine_command_free (pop3_store->engine, cmd);

	camel_operation_end(NULL);
	g_static_rec_mutex_unlock (pop3_store->eng_lock);
}


static void
pop3_construct (CamelService *service, CamelSession *session,
	   CamelProvider *provider, CamelURL *url,
	   CamelException *ex)
{
	CamelPOP3Store *pop3_store = CAMEL_POP3_STORE (service);
	CamelDiscoStore *disco_store = CAMEL_DISCO_STORE (service);
	char *path;

	CAMEL_SERVICE_CLASS (parent_class)->construct (service, session, provider, url, ex);
	if (camel_exception_is_set (ex))
		return;

	pop3_store->storage_path = camel_session_get_storage_path (session, service, ex);
	camel_pop3_logbook_set_rootpath (pop3_store->book, pop3_store->storage_path);

	if (!pop3_store->storage_path)
		return;

	pop3_store->cache = camel_data_cache_new(pop3_store->storage_path, 0, ex);
	if (pop3_store->cache) {
		/* Default cache expiry - 1 week or not visited in a day */
		camel_data_cache_set_expire_age (pop3_store->cache, 60*60*24*7);
		camel_data_cache_set_expire_access (pop3_store->cache, 60*60*24);
	}

	pop3_store->base_url = camel_url_to_string (service->url, (CAMEL_URL_HIDE_PASSWORD |
								   CAMEL_URL_HIDE_PARAMS |
								   CAMEL_URL_HIDE_AUTH));

	/* setup journal*/
	path = g_strdup_printf ("%s/journal", pop3_store->storage_path);
	disco_store->diary = camel_disco_diary_new (disco_store, path, ex);
	g_free (path);

}

static void
camel_pop3_store_class_init (CamelPOP3StoreClass *camel_pop3_store_class)
{
	CamelServiceClass *camel_service_class =
		CAMEL_SERVICE_CLASS (camel_pop3_store_class);
	CamelStoreClass *camel_store_class =
		CAMEL_STORE_CLASS (camel_pop3_store_class);
	CamelDiscoStoreClass *camel_disco_store_class =
		CAMEL_DISCO_STORE_CLASS (camel_pop3_store_class);

	parent_class = CAMEL_STORE_CLASS (camel_type_get_global_classfuncs (camel_disco_store_get_type ()));

	/* virtual method overload */
	camel_service_class->construct = pop3_construct;
	camel_service_class->query_auth_types = query_auth_types;

	/* camel_service_class->connect = pop3_connect; */
	/* camel_service_class->disconnect = pop3_disconnect; */

	camel_store_class->get_local_size = pop3store_get_local_size;
	camel_store_class->get_folder = get_folder;
	camel_store_class->get_trash = get_trash;
	camel_store_class->get_folder_status = pop3_get_folder_status;
	camel_store_class->delete_cache = pop3_delete_cache;

	camel_disco_store_class->can_work_offline = pop3_can_work_offline;
	camel_disco_store_class->connect_online = pop3_connect_online;
	camel_disco_store_class->connect_offline = pop3_connect_offline;
	camel_disco_store_class->disconnect_online = pop3_disconnect_online;
	camel_disco_store_class->disconnect_offline = pop3_disconnect_offline;
	camel_disco_store_class->get_folder_online = pop3_get_folder_online;
	camel_disco_store_class->get_folder_offline = pop3_get_folder_offline;
	camel_disco_store_class->get_folder_resyncing = pop3_get_folder_online;
	camel_disco_store_class->get_folder_info_online = pop3_get_folder_info_online;
	camel_disco_store_class->get_folder_info_offline = pop3_get_folder_info_offline;
	camel_disco_store_class->get_folder_info_resyncing = pop3_get_folder_info_online;
}



static void
camel_pop3_store_init (gpointer object, gpointer klass)
{
	CamelPOP3Store *store = (CamelPOP3Store *) object;

	store->uids = g_ptr_array_new ();
	store->uids_uid = g_hash_table_new(g_str_hash, g_str_equal);
	store->uids_id = g_hash_table_new(NULL, NULL);

	store->is_refreshing = FALSE;
	store->immediate_delete_after = FALSE;
	store->book = camel_pop3_logbook_new (store);
	store->eng_lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (store->eng_lock);
	store->uidl_lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (store->uidl_lock);

	return;
}

CamelType
camel_pop3_store_get_type (void)
{
	static CamelType camel_pop3_store_type = CAMEL_INVALID_TYPE;

	if (!camel_pop3_store_type) {
		camel_pop3_store_type = camel_type_register (CAMEL_DISCO_STORE_TYPE,
							     "CamelPOP3Store",
							     sizeof (CamelPOP3Store),
							     sizeof (CamelPOP3StoreClass),
							     (CamelObjectClassInitFunc) camel_pop3_store_class_init,
							     NULL,
							     (CamelObjectInitFunc) camel_pop3_store_init,
							     finalize);
	}

	return camel_pop3_store_type;
}
