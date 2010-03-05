/* libtinymailui - The Tiny Mail UI library
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

/**
 * TnyMsgWindow:
 * 
 * A type that can view a #TnyMsg in a window
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-msg-window.h>


static void
tny_msg_window_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_msg_window_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyMsgWindowIface),
		  tny_msg_window_base_init,   /* base_init */
		  NULL,   /* base_finalize */
		  NULL,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  0,
		  0,      /* n_preallocs */
		  NULL    /* instance_init */
		};

	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyMsgWindow", &info, 0);

	g_type_interface_add_prerequisite (type, TNY_TYPE_MSG_VIEW);

	return GSIZE_TO_POINTER (type);
}

GType
tny_msg_window_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_msg_window_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
