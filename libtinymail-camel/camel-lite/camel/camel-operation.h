/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Authors: Michael Zucchi <NotZed@Ximian.com>
 *
 * Copyright 2001 Ximian, Inc. (www.ximian.com/)
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

#ifndef CAMEL_OPERATION_H
#define CAMEL_OPERATION_H 1

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>


#include <glib.h>

#include <libedataserver/e-msgport.h>

#ifdef HAVE_NSS
#include <nspr.h>
#endif

G_BEGIN_DECLS

/* cancellation helper stuff, not yet finalised */

typedef struct _CamelOperation CamelOperation;

typedef void (*CamelOperationStatusFunc)(struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *data);

#ifndef CAMEL_OPERATION_CANCELLED
#define CAMEL_OPERATION_CANCELLED (1<<0)
#endif


struct _CamelOperation {
	struct _CamelOperation *next;
	struct _CamelOperation *prev;

	pthread_t id;		/* id of running thread */
	guint32 flags;		/* cancelled ? */
	int blocked;		/* cancellation blocked depth */
	int refcount;

	CamelOperationStatusFunc status;
	void *status_data;
	unsigned int status_update;

	/* stack of status messages (struct _status_stack *) */
	GSList *status_stack;
	struct _status_stack *lastreport;

	EMsgPort *cancel_port;
	int cancel_fd;
#ifdef HAVE_NSS
	PRFileDesc *cancel_prfd;
#endif
};

typedef enum _camel_operation_status_t {
	CAMEL_OPERATION_START = -1,
	CAMEL_OPERATION_END = -2
} camel_operation_status_t;

/* main thread functions */
CamelOperation *camel_operation_new(CamelOperationStatusFunc status, void *status_data);
void camel_operation_mute(CamelOperation *cc);
void camel_operation_ref(CamelOperation *cc);
void camel_operation_unref(CamelOperation *cc);
void camel_operation_cancel(CamelOperation *cc);
void camel_operation_uncancel(CamelOperation *cc);
/* subthread functions */
CamelOperation *camel_operation_register(CamelOperation *cc);
void camel_operation_unregister (CamelOperation *cc);

/* called internally by camel, for the current thread */
void camel_operation_cancel_block(CamelOperation *cc);
void camel_operation_cancel_unblock(CamelOperation *cc);
int camel_operation_cancel_check(CamelOperation *cc);
int camel_operation_cancel_fd(CamelOperation *cc);
#ifdef HAVE_NSS
struct PRFileDesc *camel_operation_cancel_prfd(CamelOperation *cc);
#endif
/* return the registered operation for this thread, if there is one */
CamelOperation *camel_operation_registered(void);

void camel_operation_start(CamelOperation *cc, char *what, ...);
void camel_operation_start_transient(CamelOperation *cc, char *what, ...);
void camel_operation_progress(CamelOperation *cc, int sofar, int oftotal);
void camel_operation_progress_count(CamelOperation *cc, int sofar);
void camel_operation_end(CamelOperation *cc);

G_END_DECLS

#endif /* CAMEL_OPERATION_H */
