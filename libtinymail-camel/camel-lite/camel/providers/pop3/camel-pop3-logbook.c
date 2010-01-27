/*
 * Authors:
 *   Philip Van Hoof <pvanhoof@gnome.org>
 *
 * Copyright (C) 2007-2008 Philip Van Hoof
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

#include "camel-pop3-logbook.h"

#include "camel-file-utils.h"
#include "camel-data-cache.h"
#include "camel-pop3-engine.h"
#include "camel-pop3-folder.h"
#include "camel-pop3-store.h"
#include "camel-session.h"


void
camel_pop3_logbook_set_rootpath (CamelPOP3Logbook *book, const gchar *path)
{
	g_static_rec_mutex_lock (book->lock);
	if (book->path)
		g_free (book->path);
	book->path = g_strdup_printf ("%s/logbook", path);
	if (book->registered)
	{
		camel_pop3_logbook_close (book);
		camel_pop3_logbook_open (book);
	}
	g_static_rec_mutex_unlock (book->lock);
	return;
}

static void
stripit (char *buffer)
{
	if (!buffer)
		return;

	if (buffer[strlen (buffer)-1] == '\n')
		buffer[strlen (buffer)-1] = '\0';

	return;
}

void
camel_pop3_logbook_register (CamelPOP3Logbook *book, const gchar *uid)
{
	FILE *f = NULL;

	g_static_rec_mutex_lock (book->lock);

	if (book->registered) {
		book->registered = g_list_prepend (book->registered,
			g_strdup (uid));
	}

	f = fopen (book->path, "a");
	if (f) {
		gchar *nbuf = g_strdup_printf ("%s\n", uid);
		fputs (nbuf, f);
		g_free (nbuf);
		fclose (f);
	}

	g_static_rec_mutex_unlock (book->lock);

	return;
}

gboolean
camel_pop3_logbook_is_registered (CamelPOP3Logbook *book, const gchar *uid)
{
	gboolean truth = FALSE;

	if (!uid)
		return FALSE;

	g_static_rec_mutex_lock (book->lock);

	if (!book->registered)
	{
		FILE *f = fopen (book->path, "r");
		if (f) {
			char *buffer = (char *) malloc (1024);
			while (!feof (f) && !truth)
			{
				gchar *tmp_buffer = fgets (buffer, 1024, f);
				stripit (tmp_buffer);
				if (tmp_buffer) {
					if (!strcmp (tmp_buffer, uid))
						truth = TRUE;
					memset (buffer, 0, 1024);
				}
			}
			fclose (f);
			free (buffer);
		}
	} else {
		GList *copy = book->registered;
		while (copy && !truth) {
			if (copy->data && uid && (!strcmp ((const char *) copy->data, uid)))
				truth = TRUE;
			copy = g_list_next (copy);
		}
	}

	g_static_rec_mutex_unlock (book->lock);

	return truth;
}

void
camel_pop3_logbook_open (CamelPOP3Logbook *book)
{
	g_static_rec_mutex_lock (book->lock);
	if (!book->registered)
	{
		FILE *f = fopen (book->path, "rw");
		if (f) {
			char *buffer = (char *) malloc (1024);
			while (!feof (f))
			{
				gchar *tmp_buffer = fgets (buffer, 1024, f);
				stripit (tmp_buffer);
				if (tmp_buffer) {
					book->registered = g_list_prepend (book->registered,
									   g_strdup (tmp_buffer));
					memset (buffer, 0, 1024);
				}
			}
			g_free (buffer);
			fclose (f);
		}
	}
	g_static_rec_mutex_unlock (book->lock);
}

gboolean
camel_pop3_logbook_is_open (CamelPOP3Logbook *book, const gchar *uid)
{
	gboolean truth = FALSE;

	g_static_rec_mutex_lock (book->lock);
	truth = (gboolean) book->registered;
	g_static_rec_mutex_unlock (book->lock);

	return truth;
}

static void
foreach_free (gpointer data, gpointer user_data)
{
	g_free (data);
}

void
camel_pop3_logbook_close (CamelPOP3Logbook *book)
{
	g_static_rec_mutex_lock (book->lock);

	if (book->registered) {
		g_list_foreach (book->registered, foreach_free, NULL);
		g_list_free (book->registered);
	}
	book->registered = NULL;

	g_static_rec_mutex_unlock (book->lock);

	return;
}


static void
finalize (CamelObject *object)
{
	CamelPOP3Logbook *book = (CamelPOP3Logbook *) object;

	camel_pop3_logbook_close (book);

	if (book->path)
		g_free (book->path);
	book->path = NULL;

	/* g_static_rec_mutex_free (book->lock); */
	g_free (book->lock);
	book->lock = NULL;

	return;
}


static void
camel_pop3_logbook_init (gpointer object, gpointer klass)
{
	CamelPOP3Logbook *book = (CamelPOP3Logbook *) object;

	book->path = NULL;
	book->lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (book->lock);

	return;
}


CamelPOP3Logbook*
camel_pop3_logbook_new (gpointer store_in)
{
	CamelPOP3Logbook *book = NULL;

	book = CAMEL_POP3_LOGBOOK (camel_object_new (CAMEL_POP3_LOGBOOK_TYPE));

	return book;
}

static void
camel_pop3_logbook_class_init (CamelPOP3LogbookClass *camel_pop3_logbook_class)
{
	return;
}



CamelType
camel_pop3_logbook_get_type (void)
{
	static CamelType camel_pop3_logbook_type = CAMEL_INVALID_TYPE;

	if (!camel_pop3_logbook_type) {
		camel_pop3_logbook_type = camel_type_register (CAMEL_OBJECT_TYPE,
				"CamelPOP3Logbook",
				sizeof (CamelPOP3Logbook),
				sizeof (CamelPOP3LogbookClass),
				(CamelObjectClassInitFunc) camel_pop3_logbook_class_init,
				NULL,
				(CamelObjectInitFunc) camel_pop3_logbook_init,
				finalize);
	}

	return camel_pop3_logbook_type;
}
