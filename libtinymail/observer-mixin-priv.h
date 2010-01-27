#ifndef OBSERVER_MIXIN_H
#define OBSERVER_MIXIN_H

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

typedef struct _ObserverMixin {
	GList *list;
	GStaticRecMutex *lock;
} ObserverMixin;


typedef void (*ObserverUpdateMethod) (gpointer, gpointer);

void _observer_mixin_init (ObserverMixin *mixin);
void _observer_mixin_destroy (gpointer owner, ObserverMixin *mixin);
void _observer_mixin_add_observer (gpointer owner, ObserverMixin *mixin, gpointer observer);
void _observer_mixin_remove_observer (gpointer owner, ObserverMixin *mixin, gpointer observer);
void _observer_mixin_remove_all_observers (gpointer owner, ObserverMixin *mixin);
void _observer_mixin_notify_observers_about_in_idle (gpointer mixin_owner, ObserverMixin *mixin, ObserverUpdateMethod method, gpointer change, TnyLockable *ui_lock, GDestroyNotify done_notify);
void _observer_mixin_notify_observers_about (ObserverMixin *mixin, ObserverUpdateMethod method, gpointer change, TnyLockable *ui_lock);

#endif
