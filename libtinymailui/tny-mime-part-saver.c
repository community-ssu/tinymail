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
 * TnyMimePartSaver:
 * 
 * A type that can save a #TnyMimePart
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-mime-part-saver.h>


/**
 * tny_mime_part_saver_set_save_strategy:
 * @self: a #TnyMimePartSaver
 * @strategy: a #TnyMimePartSaveStrategy
 *
 * Set the strategy for saving mime-parts
 *
 * Devices can have a specific strategy for storing a #TnyMimePart. For example
 * a strategy that sends it to another computer or a strategy that saves it to
 * a flash disk. In the message view component, you don't care about that: You
 * only care about the API of the save-strategy interface.
 *
 * For more information take a look at tny_mime_part_save_strategy_perform_save()
 * of #TnyMimePartSaveStrategy.
 *
 *
 * Example:
 * <informalexample><programlisting>
 * static void 
 * tny_my_msg_view_set_save_strategy (TnyMimePartSaver *self_i, TnyMimePartSaveStrategy *strat)
 * {
 *      TnyMyMsgView *self = TNY_MY_MSG_VIEW (self_i);
 *      if (self->save_strategy)
 *            g_object_unref (self->save_strategy);
 *      self->save_strategy = g_object_ref (strat);
 * }
 * static void
 * tny_my_msg_view_finalize (TnyMyMsgView *self)
 * {
 *      if (self->save_strategy))
 *          g_object_unref (self->save_strategy);
 * }
 * </programlisting></informalexample>
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void 
tny_mime_part_saver_set_save_strategy (TnyMimePartSaver *self, TnyMimePartSaveStrategy *strategy)
{
#ifdef DEBUG
	if (!TNY_MIME_PART_SAVER_GET_IFACE (self)->set_save_strategy)
		g_critical ("You must implement tny_mime_part_saver_set_save_strategy\n");
#endif

	TNY_MIME_PART_SAVER_GET_IFACE (self)->set_save_strategy(self, strategy);
	return;
}



/**
 * tny_mime_part_saver_get_save_strategy:
 * @self: a #TnyMsgView
 *
 * Get the strategy for saving mime-parts. The returned value must be
 * unreferenced after use.
 *
 * For more information take a look at tny_mime_part_save_strategy_perform_save()
 * of #TnyMimePartSaveStrategy.
 *
 * Example:
 * <informalexample><programlisting>
 * static void 
 * tny_my_msg_view_on_save_clicked (TnyMimePartSaver *self, TnyMimePart *attachment)
 * {
 *     TnyMimePartSaveStrategy *strategy = tny_mime_part_saver_get_save_strategy (self);
 *     tny_save_strategy_save (strategy, attachment);
 *     g_object_unref (strategy);
 * }
 * </programlisting></informalexample>
 *
 * returns: (caller-owns): the #TnyMimePartSaveStrategy for @self
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyMimePartSaveStrategy*
tny_mime_part_saver_get_save_strategy (TnyMimePartSaver *self)
{
#ifdef DEBUG
	if (!TNY_MIME_PART_SAVER_GET_IFACE (self)->get_save_strategy)
		g_critical ("You must implement tny_mime_part_saver_get_save_strategy\n");
#endif

	return TNY_MIME_PART_SAVER_GET_IFACE (self)->get_save_strategy(self);
}


/**
 * tny_mime_part_saver_save:
 * @self: a #TnyMimePartSaver
 * @part: a #TnyMimePart
 *
 * Saves @mime_part using the save strategy of @self.
 *
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
void
tny_mime_part_saver_save (TnyMimePartSaver *self, TnyMimePart *part)
{
#ifdef DEBUG
	if (!TNY_MIME_PART_SAVER_GET_IFACE (self)->save)
		g_critical ("You must implement tny_mime_part_saver_save\n");
#endif

	TNY_MIME_PART_SAVER_GET_IFACE (self)->save(self, part);
	return;
}

static void
tny_mime_part_saver_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/* create interface signals here. */
		initialized = TRUE;
	}
}

static gpointer
tny_mime_part_saver_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyMimePartSaverIface),
		  tny_mime_part_saver_base_init,   /* base_init */
		  NULL,   /* base_finalize */
		  NULL,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  0,
		  0,      /* n_preallocs */
		  NULL    /* instance_init */
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyMimePartSaver", &info, 0);

	return GUINT_TO_POINTER (type);
}

GType
tny_mime_part_saver_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_mime_part_saver_register_type, NULL);
	return GPOINTER_TO_UINT (once.retval);
}
