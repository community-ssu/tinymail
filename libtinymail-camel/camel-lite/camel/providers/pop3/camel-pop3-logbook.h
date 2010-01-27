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

#ifndef CAMEL_POP3_LOGBOOK_H
#define CAMEL_POP3_LOGBOOK_H 1

#include <camel/camel-types.h>
#include <camel/camel-store.h>
#include "camel-pop3-store.h"

#define CAMEL_POP3_LOGBOOK_TYPE     (camel_pop3_logbook_get_type ())
#define CAMEL_POP3_LOGBOOK(obj)     (CAMEL_CHECK_CAST((obj), CAMEL_POP3_LOGBOOK_TYPE, CamelPOP3Logbook))
#define CAMEL_POP3_LOGBOOK_CLASS(k) (CAMEL_CHECK_CLASS_CAST ((k), CAMEL_POP3_LOGBOOK_TYPE, CamelPOP3LogbookClass))
#define CAMEL_IS_POP3_LOGBOOK(o)    (CAMEL_CHECK_TYPE((o), CAMEL_POP3_LOGBOOK_TYPE))

G_BEGIN_DECLS

typedef struct {
	CamelObject parent_object;
	gchar *path;
	GStaticRecMutex *lock;
	GList *registered;
} CamelPOP3Logbook;



typedef struct {
	CamelObjectClass parent_class;
} CamelPOP3LogbookClass;


/* Standard Camel function */
CamelType camel_pop3_logbook_get_type (void);
CamelPOP3Logbook* camel_pop3_logbook_new (gpointer store_in);

void camel_pop3_logbook_register (CamelPOP3Logbook *book, const gchar *uid);
gboolean camel_pop3_logbook_is_registered (CamelPOP3Logbook *book, const gchar *uid);

void camel_pop3_logbook_open (CamelPOP3Logbook *book);
gboolean camel_pop3_logbook_is_open (CamelPOP3Logbook *book, const gchar *uid);
void camel_pop3_logbook_close (CamelPOP3Logbook *book);

void camel_pop3_logbook_set_rootpath (CamelPOP3Logbook *book, const gchar *path);

G_END_DECLS

#endif /* CAMEL_POP3_LOGBOOK_H */
