/* libtinymail-camel - The Tiny Mail base library for Camel
 * Copyright (C) 2008 Rob Taylor <rob.taylor@codethink.co.uk>
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

#include <config.h>

#include <tny-lockable.h>
#include <tny-folder-store-observer.h>

#include "observer-mixin-priv.h"


void
_observer_mixin_init (ObserverMixin *mixin)
{
	mixin->lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (mixin->lock);
	mixin->list = NULL;
}


void
_observer_mixin_destroy (gpointer owner, ObserverMixin *mixin)
{
	_observer_mixin_remove_all_observers (owner, mixin);
	if (mixin->lock)
		g_free (mixin->lock);
	mixin->lock = NULL;
}


static void
notify_observer_del (gpointer user_data, GObject *observer)
{
	ObserverMixin *mixin = user_data;
	g_static_rec_mutex_lock (mixin->lock);
	mixin->list = g_list_remove (mixin->list, observer);
	g_static_rec_mutex_unlock (mixin->lock);
}

void
_observer_mixin_add_observer (gpointer owner, ObserverMixin *mixin, gpointer observer)
{
	g_assert (TNY_IS_FOLDER_STORE_OBSERVER (observer));

	g_static_rec_mutex_lock (mixin->lock);
	if (!g_list_find (mixin->list, observer)) {
		mixin->list = g_list_prepend (mixin->list, observer);
		g_object_weak_ref (G_OBJECT (observer), notify_observer_del, (GObject*)owner);
	}
	g_static_rec_mutex_unlock (mixin->lock);

	return;
}


void
_observer_mixin_remove_observer (gpointer owner, ObserverMixin *mixin, gpointer observer)
{
	GList *found = NULL;

	g_assert (TNY_IS_FOLDER_STORE_OBSERVER (observer));

	g_static_rec_mutex_lock (mixin->lock);

	if (!mixin->list) {
		g_static_rec_mutex_unlock (mixin->lock);
		return;
	}

	found = g_list_find (mixin->list, observer);
	if (found) {
		mixin->list = g_list_remove_link (mixin->list, found);
		g_object_weak_unref (found->data, notify_observer_del, (GObject*) owner);
		g_list_free (found);
	}

	g_static_rec_mutex_unlock (mixin->lock);
}

void
_observer_mixin_remove_all_observers (gpointer owner, ObserverMixin *mixin)
{
	g_static_rec_mutex_lock (mixin->lock);
	if (mixin->list) {
		GList *copy = mixin->list;
		while (copy) {
			g_object_weak_unref ((GObject *) copy->data, notify_observer_del, (GObject *) owner);
			copy = g_list_next (copy);
		}
		g_list_free (mixin->list);
		mixin->list = NULL;
	}
	g_static_rec_mutex_unlock (mixin->lock);
}

typedef struct {
	GObject *mixin_owner;
	ObserverMixin *mixin;
	GObject *change;
	TnyLockable *ui_lock;
	ObserverUpdateMethod method;
	GDestroyNotify notify;
} NotObInIdleInfo;

static void
do_notify_in_idle_destroy (gpointer user_data)
{
	NotObInIdleInfo *info = (NotObInIdleInfo *) user_data;

	g_object_unref (info->change);
	if (info->notify)
		(info->notify)(info->mixin_owner);
	g_object_unref (info->mixin_owner);
	if (info->ui_lock)
		g_object_unref (info->ui_lock);

	g_slice_free (NotObInIdleInfo, info);
}

static void
notify_observers_about (ObserverMixin *mixin, ObserverUpdateMethod method, gpointer change, TnyLockable *ui_lock)
{
	GList *list, *list_iter;

	g_static_rec_mutex_lock (mixin->lock);
	if (!mixin->list) {
		g_static_rec_mutex_unlock (mixin->lock);
		return;
	}
	list = g_list_copy (mixin->list);
	list_iter = list;
	g_static_rec_mutex_unlock (mixin->lock);

	while (list_iter)
	{
		if (ui_lock)
			tny_lockable_lock (ui_lock);
		method (list_iter->data, change);
		if (ui_lock)
			tny_lockable_unlock (ui_lock);
		list_iter = g_list_next (list_iter);
	}

	g_list_free (list);
}

static gboolean
notify_observers_about_idle (gpointer user_data)
{
	NotObInIdleInfo *info = (NotObInIdleInfo *) user_data;
	notify_observers_about (info->mixin, info->method, info->change, info->ui_lock);
	return FALSE;
}


void
_observer_mixin_notify_observers_about_in_idle (gpointer mixin_owner, ObserverMixin *mixin, ObserverUpdateMethod method, gpointer change, TnyLockable *ui_lock, GDestroyNotify done_notify)
{
	NotObInIdleInfo *info = g_slice_new0 (NotObInIdleInfo);

	info->mixin_owner = g_object_ref (mixin_owner);
	info->mixin = mixin;
	info->change = g_object_ref (change);
	if (ui_lock)
		info->ui_lock = g_object_ref (ui_lock);
	info->method = method;
	info->notify = done_notify;

	g_idle_add_full (G_PRIORITY_HIGH, notify_observers_about_idle,
		info, do_notify_in_idle_destroy);
}

void
_observer_mixin_notify_observers_about (ObserverMixin *mixin, ObserverUpdateMethod method, gpointer change, TnyLockable *ui_lock)
{
	if (ui_lock)
		tny_lockable_lock (ui_lock);
	notify_observers_about (mixin, method, change, ui_lock);
	if (ui_lock)
		tny_lockable_unlock (ui_lock);
}

