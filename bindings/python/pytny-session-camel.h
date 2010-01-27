#ifndef PYTNY_SESSION_CAMEL_H
#define PYTNY_SESSION_CAMEL_H

/* pytinymail - tinymail python bindings
 * Copyright (C) 2006-2007 - Philip Van Hoof
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

#ifdef TNY_TYPE_BOXED_SESSION_CAMEL
#define PYTNY_TYPE_STATUS TNY_TYPE_BOXED_SESSION_CAMEL
#else
GType pytny_session_camel_get_type(void) G_GNUC_CONST;

#define PYTNY_TYPE_SESSION_CAMEL (pytny_session_camel_get_type())
#endif

#endif
