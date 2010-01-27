#ifndef TNY_STATUS_H
#define TNY_STATUS_H

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

#include <glib.h>
#include <glib-object.h>
#include <tny-shared.h>

G_BEGIN_DECLS

#ifndef TNY_SHARED_H
typedef struct _TnyStatus TnyStatus;
#endif

#define TNY_TYPE_STATUS_DOMAIN (tny_status_domain_get_type())

typedef enum
{
	TNY_FOLDER_STATUS = 1,
	TNY_GET_MSG_QUEUE_STATUS = 2,
	TNY_GET_SUPPORTED_SECURE_AUTH_STATUS = 3
} TnyStatusDomain;

#define TNY_TYPE_STATUS_CODE (tny_status_code_get_type())

typedef enum
{
	TNY_FOLDER_STATUS_CODE_REFRESH = 1,
	TNY_FOLDER_STATUS_CODE_GET_MSG = 2,
	TNY_GET_MSG_QUEUE_STATUS_GET_MSG = 3,
	TNY_FOLDER_STATUS_CODE_XFER_MSGS = 4,
	TNY_FOLDER_STATUS_CODE_COPY_FOLDER = 5,
	TNY_GET_SUPPORTED_SECURE_AUTH_STATUS_GET_SECURE_AUTH = 6,
	TNY_FOLDER_STATUS_CODE_SYNC = 7
} TnyStatusCode;

struct _TnyStatus 
{
	GQuark domain;
	gint code;
	gchar *message;
	guint position;
	guint of_total;
};

GType tny_status_domain_get_type (void);
GType tny_status_code_get_type (void);

TnyStatus* tny_status_new (GQuark domain, gint code, guint position, guint of_total, const gchar *format, ...);
TnyStatus* tny_status_new_literal (GQuark domain, gint code, guint position, guint of_total, const gchar *message);
void tny_status_free (TnyStatus *status);
TnyStatus* tny_status_copy (const TnyStatus *status);
gboolean tny_status_matches (const TnyStatus *status, GQuark domain, gint code);
void tny_set_status (TnyStatus **status, GQuark domain, gint code, guint position, guint of_total, gchar *format, ...);
void tny_clear_status (TnyStatus **status);

void tny_status_set_fraction (TnyStatus *status, gdouble fraction);
gdouble tny_status_get_fraction (TnyStatus *status);

G_END_DECLS

#endif
