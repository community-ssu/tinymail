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

#include <tny-error.h>


/**
 * tny_error_get_message:
 * @err: a #GError
 *
 * Get the error message
 *
 * returns: a error message
 **/
const gchar* 
tny_error_get_message (GError *err)
{
	return err->message;
}

/**
 * tny_error_get_code:
 * @err: a #GError
 *
 * Get the error's code
 *
 * returns: a error code
 **/
gint
tny_error_get_code (GError *err)
{
	return err->code;
}

/**
 * tny_g_error_quark:
 *
 * The implementation of #TNY_ERROR_DOMAIN error domain. See documentation
 * for #GError in GLib reference manual.
 *
 * Returns: the error domain quark for use with #GError
 */
GQuark
tny_get_error_quark (void)
{
  static GQuark quark = 0;
  if (quark == 0)
    quark = g_quark_from_static_string ("tinymail-error-quark");
  return quark;
}

