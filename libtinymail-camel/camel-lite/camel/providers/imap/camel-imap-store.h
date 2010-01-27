/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* camel-imap-store.h : class for an imap store */

/*
 * Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 * Copyright (C) 2000 Ximian, Inc.
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


#ifndef CAMEL_IMAP_STORE_H
#define CAMEL_IMAP_STORE_H 1

#ifdef DEBUG
#ifndef DISABLE_IDLE
#define IDLE_DEBUG 1
#endif
#define IMAP_DEBUG 1
#endif

#ifdef IDLE_DEBUG
#define idle_debug	g_print
#else
#define idle_debug(o, ...)
#endif

#ifdef IMAP_DEBUG
#define imap_debug	g_print
#else
#define imap_debug(o, ...)
#endif


#include "camel-imap-types.h"
#include <camel/camel-disco-store.h>
#include <sys/time.h>

#ifdef ENABLE_THREADS
#include <libedataserver/e-msgport.h>

G_BEGIN_DECLS

typedef struct _CamelImapMsg CamelImapMsg;

struct _CamelImapMsg {
	EMsg msg;

	void (*receive)(CamelImapStore *store, struct _CamelImapMsg *m);
	void (*free)(CamelImapStore *store, struct _CamelImapMsg *m);
};

CamelImapMsg *camel_imap_msg_new(void (*receive)(CamelImapStore *store, struct _CamelImapMsg *m),
				 void (*free)(CamelImapStore *store, struct _CamelImapMsg *m),
				 size_t size);
void camel_imap_msg_queue(CamelImapStore *store, CamelImapMsg *msg);

G_END_DECLS

#endif

#define CAMEL_IMAP_STORE_TYPE     (camel_imap_store_get_type ())
#define CAMEL_IMAP_STORE(obj)     (CAMEL_CHECK_CAST((obj), CAMEL_IMAP_STORE_TYPE, CamelImapStore))
#define CAMEL_IMAP_STORE_CLASS(k) (CAMEL_CHECK_CLASS_CAST ((k), CAMEL_IMAP_STORE_TYPE, CamelImapStoreClass))
#define CAMEL_IS_IMAP_STORE(o)    (CAMEL_CHECK_TYPE((o), CAMEL_IMAP_STORE_TYPE))

G_BEGIN_DECLS

enum {
	CAMEL_IMAP_STORE_ARG_FIRST  = CAMEL_DISCO_STORE_ARG_FIRST + 100,
	CAMEL_IMAP_STORE_ARG_NAMESPACE,
	CAMEL_IMAP_STORE_ARG_OVERRIDE_NAMESPACE,
	CAMEL_IMAP_STORE_ARG_CHECK_ALL,
	CAMEL_IMAP_STORE_ARG_FILTER_INBOX,
	CAMEL_IMAP_STORE_ARG_FILTER_JUNK,
	CAMEL_IMAP_STORE_ARG_FILTER_JUNK_INBOX
};

#define CAMEL_IMAP_STORE_NAMESPACE           (CAMEL_IMAP_STORE_ARG_NAMESPACE | CAMEL_ARG_STR)
#define CAMEL_IMAP_STORE_OVERRIDE_NAMESPACE  (CAMEL_IMAP_STORE_ARG_OVERRIDE_NAMESPACE | CAMEL_ARG_INT)
#define CAMEL_IMAP_STORE_CHECK_ALL           (CAMEL_IMAP_STORE_ARG_CHECK_ALL | CAMEL_ARG_INT)
#define CAMEL_IMAP_STORE_FILTER_INBOX        (CAMEL_IMAP_STORE_ARG_FILTER_INBOX | CAMEL_ARG_INT)
#define CAMEL_IMAP_STORE_FILTER_JUNK         (CAMEL_IMAP_STORE_ARG_FILTER_JUNK | CAMEL_ARG_BOO)
#define CAMEL_IMAP_STORE_FILTER_JUNK_INBOX   (CAMEL_IMAP_STORE_ARG_FILTER_JUNK_INBOX | CAMEL_ARG_BOO)

/* CamelFolderInfo flags */
#define CAMEL_IMAP_FOLDER_MARKED	     (1<<16)
#define CAMEL_IMAP_FOLDER_UNMARKED	     (1<<17)

typedef enum {
	IMAP_LEVEL_UNKNOWN,
	IMAP_LEVEL_IMAP4,
	IMAP_LEVEL_IMAP4REV1
} CamelImapServerLevel;

#define IMAP_CAPABILITY_IMAP4			(1 << 0)
#define IMAP_CAPABILITY_IMAP4REV1		(1 << 1)
#define IMAP_CAPABILITY_STATUS			(1 << 2)
#define IMAP_CAPABILITY_NAMESPACE		(1 << 3)
#define IMAP_CAPABILITY_UIDPLUS			(1 << 4)
#define IMAP_CAPABILITY_LITERALPLUS		(1 << 5)
#define IMAP_CAPABILITY_STARTTLS		(1 << 6)
#define IMAP_CAPABILITY_useful_lsub		(1 << 7)
#define IMAP_CAPABILITY_utf8_search		(1 << 8)
#define IMAP_CAPABILITY_XGWEXTENSIONS		(1 << 9)
#define IMAP_CAPABILITY_XGWMOVE			(1 << 10)
#define IMAP_CAPABILITY_LOGINDISABLED		(1 << 11)
#define IMAP_CAPABILITY_CONDSTORE		(1 << 12)
#define IMAP_CAPABILITY_IDLE			(1 << 13)
#define IMAP_CAPABILITY_BINARY			(1 << 14)
#define IMAP_CAPABILITY_QRESYNC			(1 << 15)
#define IMAP_CAPABILITY_ENABLE			(1 << 16)
#define IMAP_CAPABILITY_ESEARCH			(1 << 17)
#define IMAP_CAPABILITY_CONVERT			(1 << 18)
#define IMAP_CAPABILITY_LISTEXT			(1 << 19)
#define IMAP_CAPABILITY_COMPRESS		(1 << 20)
#define IMAP_CAPABILITY_XAOLNETMAIL             (1 << 21)

#define IMAP_PARAM_OVERRIDE_NAMESPACE		(1 << 0)
#define IMAP_PARAM_CHECK_ALL			(1 << 1)
#define IMAP_PARAM_FILTER_INBOX			(1 << 2)
#define IMAP_PARAM_FILTER_JUNK			(1 << 3)
#define IMAP_PARAM_FILTER_JUNK_INBOX		(1 << 4)
#define IMAP_PARAM_SUBSCRIPTIONS		(1 << 5)
#define IMAP_PARAM_DONT_TOUCH_SUMMARY		(1 << 6)

struct _CamelImapStore {
	CamelDiscoStore parent_object;

	CamelStream *istream;
	CamelStream *ostream;

	struct _CamelImapStoreSummary *summary;

	/* Information about the command channel / connection status */
	guint connected:1;
	guint preauthed:1;

	/* broken server - don't use BODY, dont use partial fetches for message retrival */
	guint braindamaged:1;
	guint renaming:1;
	/* broken server - wont let us append with custom flags even if the folder allows them */
	guint nocustomappend:1;

	char tag_prefix;
	guint32 command;
	CamelFolder *current_folder, *last_folder, *old_folder;

	/* Information about the server */
	CamelImapServerLevel server_level;
	guint32 capabilities, parameters;
	/* NB: namespace should be handled by summary->namespace */
	char *namespace, dir_sep, *base_url, *storage_path;
	GHashTable *authtypes;
	struct _namespaces *namespaces;

	time_t refresh_stamp;
	gchar *idle_prefix;
	gboolean dontdistridlehack, has_login;

	GStaticRecMutex *idle_prefix_lock, *idle_lock, *sum_lock, *idle_t_lock;
	GThread *idle_thread;
	gboolean idle_cont, in_idle, idle_kill, idle_send_done_happened;
	guint idle_sleep, getsrv_sleep;
	gboolean courier_crap, idle_sleep_set;
	gboolean going_online, got_online, clean_exit;
	gboolean not_recon, needs_lsub;

	GStaticRecMutex *idle_wait_reasons_lock;
	guint idle_wait_reasons;
	gboolean already_in_stop_idle;

	gboolean idle_blocked;

	struct addrinfo *addrinfo;
};

typedef struct {
	CamelDiscoStoreClass parent_class;

} CamelImapStoreClass;



/* Standard Camel function */
CamelType camel_imap_store_get_type (void);


gboolean camel_imap_store_connected (CamelImapStore *store, CamelException *ex);

ssize_t camel_imap_store_readline_nl (CamelImapStore *store, char **dest, CamelException *ex);
ssize_t camel_imap_store_readline_nb (CamelImapStore *store, char **dest, CamelException *ex);
ssize_t camel_imap_store_readline (CamelImapStore *store, char **dest, CamelException *ex);

gboolean camel_imap_store_restore_stream_buffer (CamelImapStore *store);

void _camel_imap_store_stop_idle (CamelImapStore *store);
void _camel_imap_store_stop_idle_connect_lock (CamelImapStore *store);
void _camel_imap_store_start_idle (CamelImapStore *store);
void _camel_imap_store_connect_unlock_start_idle (CamelImapStore *store);
void _camel_imap_store_start_idle_if_unlocked (CamelImapStore *store);
void _camel_imap_store_connect_unlock_no_start_idle (CamelImapStore *store);
void camel_imap_recon (CamelImapStore *store, CamelException *mex, gboolean was_cancel);

#define camel_imap_store_stop_idle_connect_lock(store) {idle_debug ("Thread %d StopIdle-ConnectLock(%x) %s:%d\n", (gint) g_thread_self (), (gint) store, __FUNCTION__, (gint) __LINE__);  _camel_imap_store_stop_idle_connect_lock((store));}
#define camel_imap_store_connect_unlock_start_idle(store) {idle_debug ("Thread %d ConnectUnlock-StartIdle(%x) %s:%d\n", (gint) g_thread_self (), (gint) store, __FUNCTION__, (gint) __LINE__);  _camel_imap_store_connect_unlock_start_idle((store));}
#define camel_imap_store_start_idle_if_unlocked(store) {idle_debug ("Thread %d StartIdle-IfUnlocked(%x) %s:%d\n", (gint) g_thread_self (), (gint) store, __FUNCTION__, (gint) __LINE__);  _camel_imap_store_start_idle_if_unlocked((store));}
#define camel_imap_store_connect_unlock_no_start_idle(store) {idle_debug ("Thread %d ConnectUnlock-NO-StartIdle(%x) %s:%d\n", (gint) g_thread_self (), (gint) store, __FUNCTION__, (gint) __LINE__);  _camel_imap_store_connect_unlock_no_start_idle((store));}
#define camel_imap_store_stop_idle(store) {idle_debug ("Thread %d StopIdle(%x) %s:%d\n", (gint) g_thread_self (), (gint) store, __FUNCTION__, (gint) __LINE__);  _camel_imap_store_stop_idle((store));}
#define camel_imap_store_start_idle(store) {idle_debug ("Thread %d StartIdle(%x) %s:%d\n", (gint) g_thread_self (), (gint) store, __FUNCTION__, (gint) __LINE__);  _camel_imap_store_start_idle((store));}

G_END_DECLS

#endif /* CAMEL_IMAP_STORE_H */
