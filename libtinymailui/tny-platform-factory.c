/* libtinymailui - The Tiny Mail user interface library
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

/* The reason why this type is defined in libtinymailui rather than libtinymail
   is because the factory also returns types defined in libtinymailui */
   
/**
 * TnyPlatformFactory:
 * 
 * A factory that creates some instances
 *
 * free-function: g_object_unref
 **/

#include <config.h>

#include <tny-platform-factory.h>


/**
 * tny_platform_factory_new_msg:
 * @self: a TnyPlatformFactory
 *
 * Create a new #TnyMsg instance. The returned instance must be 
 * unreferenced after use.
 *
 * returns: (caller-owns): a #TnyMsg instance
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyMsg* 
tny_platform_factory_new_msg (TnyPlatformFactory *self)
{
#ifdef DEBUG
	if (!TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_msg)
		g_critical ("You must implement tny_platform_factory_new_msg\n");
#endif

	return TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_msg(self);

}

/**
 * tny_platform_factory_new_password_getter:
 * @self: a TnyPlatformFactory
 *
 * Create a new #TnyPasswordGetter instance. The returned instance must be 
 * unreferenced after use.
 *
 * returns: (caller-owns): a #TnyPasswordGetter instance
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyPasswordGetter* 
tny_platform_factory_new_password_getter (TnyPlatformFactory *self)
{
#ifdef DEBUG
	if (!TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_password_getter)
		g_critical ("You must implement tny_platform_factory_new_password_getter\n");
#endif

	return TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_password_getter(self);
}

/**
 * tny_platform_factory_new_mime_part:
 * @self: a TnyPlatformFactory
 *
 * Create a new #TnyMimePart instance. The returned instance must be 
 * unreferenced after use.
 *
 * returns: (caller-owns): a #TnyMimePart instance
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyMimePart* 
tny_platform_factory_new_mime_part (TnyPlatformFactory *self)
{
#ifdef DEBUG
	if (!TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_mime_part)
		g_critical ("You must implement tny_platform_factory_new_mime_part\n");
#endif

	return TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_mime_part(self);
}


/**
 * tny_platform_factory_new_account_store:
 * @self: a TnyPlatformFactory
 *
 * Create a new #TnyAccountStore instance. The returned instance must be 
 * unreferenced after use.
 *
 * When implementing a platform-specific library, return a new #TnyAccountStore
 * instance. It's allowed to reuse one instance, just make sure that you add a
 * reference.
 *
 * returns: (caller-owns): a #TnyAccountStore instance
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyAccountStore*
tny_platform_factory_new_account_store (TnyPlatformFactory *self)
{
#ifdef DEBUG
	if (!TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_account_store)
		g_critical ("You must implement tny_platform_factory_new_account_store\n");
#endif

	return TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_account_store(self);
}

/**
 * tny_platform_factory_new_device:
 * @self: a TnyPlatformFactory
 *
 * Create a new #TnyDevice instance. The returned instance must be 
 * unreferenced after use.
 *
 * When implementing a platform-specific library, return a new #TnyDevice instance.
 * It's allowed to reuse one instance, just make sure that you add a reference.
 *
 * returns: (caller-owns): a #TnyDevice instance
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyDevice*
tny_platform_factory_new_device (TnyPlatformFactory *self)
{
#ifdef DEBUG
	if (!TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_device)
		g_critical ("You must implement tny_platform_factory_new_device\n");
#endif

	return TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_device(self);
}

/**
 * tny_platform_factory_new_msg_view:
 * @self: a TnyPlatformFactory
 *
 * Create a new #TnyMsgView instance. The returned instance must be 
 * unreferenced after use.
 *
 * When implementing a platform-specific library, return a new #TnyMsgView instance.
 * It's allowed to reuse one instance, just make sure that you add a reference.
 *
 * returns: (caller-owns): a #TnyMsgView instance
 * since: 1.0
 * audience: application-developer, type-implementer
 **/
TnyMsgView*
tny_platform_factory_new_msg_view (TnyPlatformFactory *self)
{
#ifdef DEBUG
	if (!TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_msg_view)
		g_warning ("You must implement tny_platform_factory_new_msg_view\n");
#endif

	return TNY_PLATFORM_FACTORY_GET_IFACE (self)->new_msg_view(self);
}


static void
tny_platform_factory_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (!initialized) 
		initialized = TRUE;
}

static gpointer
tny_platform_factory_register_type (gpointer notused)
{
	GType type = 0;

	static const GTypeInfo info = 
		{
		  sizeof (TnyPlatformFactoryIface),
		  tny_platform_factory_base_init,   /* base_init */
		  NULL,   /* base_finalize */
		  NULL,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  0,
		  0,      /* n_preallocs */
		  NULL    /* instance_init */
		};
	type = g_type_register_static (G_TYPE_INTERFACE, 
				       "TnyPlatformFactory", &info, 0);

	g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);

	return GSIZE_TO_POINTER (type);
}

GType
tny_platform_factory_get_type (void)
{
	static GOnce once = G_ONCE_INIT;
	g_once (&once, tny_platform_factory_register_type, NULL);
	return GPOINTER_TO_SIZE (once.retval);
}
