#ifndef TNY_MAEMO_PLATFORM_FACTORY_H
#define TNY_MAEMO_PLATFORM_FACTORY_H

/* libtinymail-maemo - The Tiny Mail base library for Maemo
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

#include <tny-platform-factory.h>

G_BEGIN_DECLS

#define TNY_TYPE_MAEMO_PLATFORM_FACTORY             (tny_maemo_platform_factory_get_type ())
#define TNY_MAEMO_PLATFORM_FACTORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TNY_TYPE_MAEMO_PLATFORM_FACTORY, TnyMaemoPlatformFactory))
#define TNY_MAEMO_PLATFORM_FACTORY_CLASS(vtable)    (G_TYPE_CHECK_CLASS_CAST ((vtable), TNY_TYPE_MAEMO_PLATFORM_FACTORY, TnyMaemoPlatformFactoryClass))
#define TNY_IS_MAEMO_PLATFORM_FACTORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TNY_TYPE_MAEMO_PLATFORM_FACTORY))
#define TNY_IS_MAEMO_PLATFORM_FACTORY_CLASS(vtable) (G_TYPE_CHECK_CLASS_TYPE ((vtable), TNY_TYPE_MAEMO_PLATFORM_FACTORY))
#define TNY_MAEMO_PLATFORM_FACTORY_GET_CLASS(inst)  (G_TYPE_INSTANCE_GET_CLASS ((inst), TNY_TYPE_MAEMO_PLATFORM_FACTORY, TnyMaemoPlatformFactoryClass))

typedef struct _TnyMaemoPlatformFactory TnyMaemoPlatformFactory;
typedef struct _TnyMaemoPlatformFactoryClass TnyMaemoPlatformFactoryClass;

struct _TnyMaemoPlatformFactory
{
	GObject parent;
};

struct _TnyMaemoPlatformFactoryClass 
{
	GObjectClass parent;
};

GType tny_maemo_platform_factory_get_type (void);

TnyPlatformFactory* tny_maemo_platform_factory_get_instance (void);

G_END_DECLS

#endif

