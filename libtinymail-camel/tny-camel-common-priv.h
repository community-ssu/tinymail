#ifndef TNY_CAMEL_COMMON_PRIV_H
#define TNY_CAMEL_COMMON_PRIV_H

/* libtinymail-camel - The Tiny Mail base library for Camel
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
#include <camel/camel-folder.h>
#include <camel/camel.h>
#include <camel/camel-folder-summary.h>
#include <camel/camel-store.h>
#include <tny-camel-folder.h>
#include <tny-session-camel.h>
#include <tny-error.h>

void _string_to_camel_inet_addr (gchar *tok, CamelInternetAddress *target);
void _foreach_email_add_to_inet_addr (const gchar *emails, CamelInternetAddress *target);
gboolean _tny_folder_store_query_passes (TnyFolderStoreQuery *query, CamelFolderInfo *finfo);
gboolean _tny_session_check_operation (TnySessionCamel *session, TnyAccount *account, GError **err, GQuark domain, gint code);
void _tny_session_stop_operation (TnySessionCamel *session);
void _tny_camel_exception_to_tny_error (CamelException *ex, GError **err);
char *_tny_camel_decode_raw_header (CamelMimePart *part, const char *str, gboolean is_addr);



#endif
