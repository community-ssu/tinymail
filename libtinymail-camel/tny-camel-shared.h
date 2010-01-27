#ifndef TNY_CAMEL_SHARED_H
#define TNY_CAMEL_SHARED_H

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

#include <tny-shared.h>

G_BEGIN_DECLS

extern gboolean _camel_type_init_done;

typedef struct _TnySessionCamel TnySessionCamel;
typedef struct _TnySessionCamelClass TnySessionCamelClass;

TnySessionCamel*  tny_session_camel_new  (TnyAccountStore *account_store);

G_END_DECLS

#endif
