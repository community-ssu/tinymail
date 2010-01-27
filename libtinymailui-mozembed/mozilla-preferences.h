#ifndef MOZILLA_PREFERENCES_H
#define MOZILLA_PREFERENCES_H

/* libtinymailui-mozembed - The Tiny Mail UI library for Gtk+
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

G_BEGIN_DECLS

gboolean _mozilla_preference_set (const char *preference_name, const char *new_value);
gboolean _mozilla_preference_set_boolean (const char *preference_name, gboolean new_boolean_value);
gboolean _mozilla_preference_set_int (const char *preference_name, gint new_int_value);
void _mozilla_preference_init ();

G_END_DECLS

#endif
