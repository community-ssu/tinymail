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

/**
 * TnySeekable:
 *
 * A seekable type
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-seekable.h>


/**
 * tny_seekable_seek:
 * @self: a #TnySeekable
 * @offset: offset value
 * @policy: what to do with the offset
 *
 * Seek to the specified position in @self.
 *
 * If @policy is #SEEK_SET, seeks to @offset.
 *
 * If @policy is #SEEK_CUR, seeks to the current position plus @offset.
 *
 * If @policy is #SEEK_END, seeks to the end of the stream plus @offset.
 *
 * Regardless of @policy, the stream's final position will be clamped
 * to the range specified by its lower and upper bounds, and the
 * stream's eos state will be updated.
 *
 * returns: new position, %-1 if operation failed.
 * since: 1.0
 * audience: application-developer
 **/
off_t 
tny_seekable_seek (TnySeekable *self, off_t offset, int policy)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_SEEKABLE (self));
	g_assert (TNY_SEEKABLE_GET_IFACE (self)->seek!= NULL);
#endif
	return TNY_SEEKABLE_GET_IFACE (self)->seek (self, offset, policy);
}

/**
 * tny_seekable_tell:
 * @self: a #TnySeekable
 *
 * Get the current position of a seekable stream.
 *
 * returns: the current position of the stream.
 * since: 1.0
 * audience: application-developer
 **/
off_t 
tny_seekable_tell (TnySeekable *self)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_SEEKABLE (self));
	g_assert (TNY_SEEKABLE_GET_IFACE (self)->tell!= NULL);
#endif
	return TNY_SEEKABLE_GET_IFACE (self)->tell (self);
}

/**
 * tny_seekable_set_bounds:
 * @self: a #TnySeekable
 * @start: the first valid position
 * @end: the first invalid position, or ~0 for unbound
 *
 * Set the range of valid data this stream is allowed to cover.  If
 * there is to be no @end value, then @end should be set to ~0.
 *
 * returns: %-1 on error
 * since: 1.0
 * audience: application-developer
 **/
gint 
tny_seekable_set_bounds (TnySeekable *self, off_t start, off_t end)
{
#ifdef DBC /* require */
	g_assert (TNY_IS_SEEKABLE (self));
	g_assert (TNY_SEEKABLE_GET_IFACE (self)->set_bounds!= NULL);
#endif
	return TNY_SEEKABLE_GET_IFACE (self)->set_bounds (self, start, end);
}

static void
tny_seekable_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_seekable_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
			sizeof (TnySeekableIface),
			tny_seekable_base_init,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL,   /* instance_init */
			NULL
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnySeekable", &info, 0);

	return GUINT_TO_POINTER (type);
}

GType
tny_seekable_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_seekable_register_type, NULL);
	return  GPOINTER_TO_UINT (once.retval);
}
