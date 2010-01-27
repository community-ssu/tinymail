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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#define TINYMAIL_ENABLE_PRIVATE_API
#include "tny-idle-stopper-priv.h"
#undef TINYMAIL_ENABLE_PRIVATE_API

/** 
 * TnyIdleStopper:
 * 
 * This API can be used to allow 2 idle callbacks to cooperate, 
 * so that one idle callback (for instance the main callback)
 * can tell the other one (for instance a repeated status/progress callback)
 * to stop.
 * 
 * This is necessary when the two idle callbacks have different priorities, 
 * meaning that the status/progress callback might be called after the main 
 * callback. The main callback would normally be expected to called last, and 
 * may therefore be the point at which other shared objects are finally 
 * released, meaning that their use in a further status/referesh callback might 
 * access invalid or released memory.
 * 
 * Instantiate an TnyIdleStopper with tny_idle_stopper_new() and create a second shared 
 * instance with tny_idle_stopper_copy(). Calling tny_idle_stopper_stopped() on one 
 * instance will then cause tny_idle_stopper_is_stopped() to return TRUE for either 
 * instance.
 * Call tny_idle_stopper_destroy() on each instance when you are finished with it.
 */
struct _TnyIdleStopper
{
	/* This is a pointer to a gboolean so we can share the value with both 
	 * idle callbacks, so that each callback can stop the the other from calling 
	 * the callbacks. */
	gboolean* stopped;
	
	/* This is a pointer an int so we can share a refcount,
	 * so we can release the shared stopped and ref_count only 
	 * when nobody else needs to check stopped. */
	gint* refcount;
};

/** 
 * tny_idle_stopper_new:
 * 
 * Creates a new ref count object, with an initial ref count.
 * Use tny_idle_stopper_copy() to create a second instance sharing 
 * the same underlying stopped status.
 * You must call tny_idle_stopper_stop() on one of these instances,
 * and call tny_idle_stopper_destroy() on all instances.
 *
 * Returns: A new #TnyIdleStopper instance. Release this with tny_idle_stopper_destroy().
 */
TnyIdleStopper* tny_idle_stopper_new()
{
	TnyIdleStopper *result = g_new0(TnyIdleStopper, 1);
	
	result->stopped = g_malloc0(sizeof(gboolean));
	*(result->stopped) = FALSE;
	
	result->refcount = g_malloc0(sizeof(gint));
	*(result->refcount) = 1;
	
	return result;
}

/** 
 * tny_idle_stopper_copy:
 * @stopper: The #TnyIdleStopper instance to be shared.
 * 
 * Create a shared copy of the stopper, 
 * so that you can call tny_idle_stopper_stop() on one instance, 
 * so that tny_idle_stopper_is_stopped() returns %TRUE for ther other instance.
 */
TnyIdleStopper* tny_idle_stopper_copy (TnyIdleStopper *stopper)
{
	g_return_val_if_fail (stopper, NULL);
	g_return_val_if_fail (stopper->refcount, NULL);
	
	TnyIdleStopper *result = g_new0(TnyIdleStopper, 1);
	
	/* Share the gboolean: */
	result->stopped = stopper->stopped;
	
	result->refcount = stopper->refcount;
	++(*(result->refcount));
	
	return result;
}

/** 
 * tny_idle_stopper_stop:
 * @stopper: The #TnyIdleStopper instance.
 * 
 * Call this to make tny_idle_stopper_is_stopped() return %TRUE for all 
 * instances of the stopper.
 */
void tny_idle_stopper_stop (TnyIdleStopper *stopper)
{
	g_return_if_fail (stopper);
	g_return_if_fail(stopper->stopped);
	g_return_if_fail (stopper->refcount);
	
	*(stopper->stopped) = TRUE;
}

/** 
 * tny_idle_stopper_destroy:
 * @stopper: The #TnyIdleStopper instance.
 * 
 * Call this when you are sure that the callback will never be called again.
 * For instance, in your #GDestroyNotify callback provided to g_idle_add_full().
 * Do not attempt to use @stopper after calling this.
 */
void tny_idle_stopper_destroy(TnyIdleStopper *stopper)
{
	g_return_if_fail (stopper);
		
	--(*(stopper->refcount));
	
	/* Free the shared variables if this is the last destroy: */
	if(*(stopper->refcount) == 0) {
		g_free (stopper->refcount);
		stopper->refcount = 0;
		
		g_free (stopper->stopped);
		stopper->stopped = 0;
	}
	
	g_free (stopper);
}

/* tny_idle_stopper_is_stopped:
 * @stopper: The TnyIdleStopper instance.
 * @returns: Whether tny_idle_stopper_stop() has been called on one of the shared instances.
 * 
 * This returns TRUE is tny_idle_stopper_stop() was called 
 * one one of the shared instances.
 */
gboolean tny_idle_stopper_is_stopped(TnyIdleStopper* stopper) 
{
	g_return_val_if_fail(stopper, FALSE);
	g_return_val_if_fail(stopper->stopped, FALSE);
	
	return *(stopper->stopped);
}
