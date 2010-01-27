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


#include <config.h>

#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-common-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

/**
 * TnyProgressInfo:
 *
 * A progress info can be launched in for example the #GMainLoop. Such an event
 * will cause a TnyStatusCallback handler to be triggered and a TnyStatus to be
 * created.
 *
 * You can use tny_progress_info_destroy and tny_progress_info_idle_func for 
 * this.
 *
 * For example:
 * <informalexample><programlisting>
 * static void
 * refresh_async_status (struct _CamelOperation *op, const char *what, int sofar, int oftotal, void *thr_user_data) 
 * { 
 *   RefreshFolderInfo *oinfo = thr_user_data;
 *   TnyProgressInfo *info = NULL;
 *   TnyCamelFolderPriv *priv = TNY_CAMEL_FOLDER_GET_PRIVATE (oinfo->self);
 *   info = tny_progress_info_new (G_OBJECT (oinfo->self), oinfo->status_callback, 
 *      TNY_FOLDER_STATUS, TNY_FOLDER_STATUS_CODE_REFRESH, what, sofar, 
 *      oftotal, oinfo->stopper, oinfo->user_data);
 *   if (oinfo->depth > 0) {
 *      g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
 *         tny_progress_info_idle_func, info, 
 *         tny_progress_info_destroy);
 *      return;
 *   }
 *   tny_progress_info_idle(info);
 *   tny_progress_info_destroy (info);
 * }
 *
 * </programlisting></informalexample>
 *
 * This is INTERNAL API
 * 
 **/

struct _TnyProgressInfo 
{
	GObject *self;
	TnyStatusCallback status_callback;
	TnyStatusDomain domain;
	TnyStatusCode code;
	gchar *what;
	gint sofar;
	gint oftotal;
	TnyIdleStopper *stopper;
	TnyLockable *ui_lock;
	gpointer user_data;
};


/* TODO: What is the purpose of the status domain and code?
 */
/** 
 * tny_progress_info_new:
 * @self: the sender of the status event
 * @status_callback: a #TnyStatusCallback function
 * @domain: the #TnyStatusDomain
 * @code: the #TnyStatusCode
 * @what: a little text that describes what has happened
 * @sofar: How much of the task is done 
 * @oftotal: How much of the task is to be done
 * @stopper: an #TnyIdleStopper
 * @user_data: user data
 * 
 * Create a progress-info instance that can be used with tny_progress_info_destroy 
 * and tny_progress_info_idle_func.
 *
 * This is INTERNAL API
 * 
 * Return value: a #TnyProgressInfo instance that is ready to launch
 **/
TnyProgressInfo*
tny_progress_info_new (GObject *self, TnyStatusCallback status_callback, TnyStatusDomain domain, TnyStatusCode code, const gchar *what, gint sofar, gint oftotal, TnyIdleStopper* stopper, TnyLockable *ui_lock, gpointer user_data)
{
	TnyProgressInfo *info = g_slice_new (TnyProgressInfo);

	info->domain = domain;
	info->code = code;
	info->status_callback = status_callback;
	info->what = g_strdup (what);
	info->self = g_object_ref (self);
	info->ui_lock = g_object_ref (ui_lock);
	info->user_data = user_data;
	info->oftotal = oftotal;

	/* Share the TnyIdleStopper, so one callback can tell the other to stop,
	 * because they may be called in an unexpected sequence: 
	 * This is destroyed in the idle GDestroyNotify callback. */

	info->stopper = tny_idle_stopper_copy (stopper);

	if (oftotal < 1 || sofar < 1 ) {
		info->oftotal = 0;
		info->sofar = 0;
	} else {
		if (sofar > oftotal)
			info->sofar = oftotal;
		else
			info->sofar = sofar;
	}

	return info;
}


/** 
 * tny_progress_info_destroy:
 * @data: The #TnyProgressInfo instace to launch
 * 
 * Destroy a progress info.
 *
 * This is INTERNAL API
 * 
 **/
void 
tny_progress_info_destroy (gpointer data)
{
	TnyProgressInfo *info = data;

	g_object_unref (info->self);
	g_object_unref (info->ui_lock);
	g_free (info->what);
	tny_idle_stopper_destroy (info->stopper);
	info->stopper = NULL;

	g_slice_free (TnyProgressInfo, data);

	return;
}

/** 
 * tny_progress_info_idle_func:
 * @data: The #TnyProgressInfo instace to launch
 * 
 * Launch a progress info. This function is useful for letting @data's
 * status_callback happen once in the #GMainLoop using g_idle_add_full.
 *
 * You can use tny_progress_info_destroy as a destroyer for the #TnyProgressInfo
 * instance, @data.
 *
 * This is INTERNAL API
 * 
 * Return value: FALSE, always
 **/
gboolean 
tny_progress_info_idle_func (gpointer data)
{
	TnyProgressInfo *info = data;

	/* Do not call the status callback after the main callback 
	 * has already been called, because we should assume that 
	 * the user_data is invalid after that time: */

	if (info && tny_idle_stopper_is_stopped (info->stopper))
		return FALSE;

	if (info && info->status_callback)
	{
		TnyStatus *status = tny_status_new (info->domain, 
			info->code, info->sofar, info->oftotal, 
			info->what);

		tny_lockable_lock (info->ui_lock);
		info->status_callback (G_OBJECT (info->self), status, 
			info->user_data);
		tny_lockable_unlock (info->ui_lock);

		tny_status_free (status);
	}

	return FALSE;
} 
