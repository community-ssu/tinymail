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

#include <glib.h>

#include <tny-status.h>

/**
 * TnyStatus:
 *
 * A progress status
 *
 * free-function: tny_status_free
 **/

/**
 * tny_status_set_fraction:
 * @status: a #TnyStatus
 * @fraction: the fraction to set
 * 
 * Set the fraction of @status. The of_total member of @status will be set to 100
 * by this method.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_status_set_fraction (TnyStatus *status, gdouble fraction)
{
	status->of_total = 100;
	status->position = fraction * 100;
}

/**
 * tny_status_get_fraction:
 * @status: a #TnyStatus
 * 
 * Get the fraction of @status
 *
 * returns: the fraction of @status
 * since: 1.0
 * audience: application-developer
 **/
gdouble 
tny_status_get_fraction (TnyStatus *status)
{
	if (status->of_total == 0)
		return 1;

	return (gdouble) ( ((gdouble) status->position) / ((gdouble)status->of_total) );
}


static TnyStatus* 
tny_status_new_valist (GQuark domain, int code, guint position, guint of_total, const gchar *format, va_list args)
{
	TnyStatus *status;

	status = g_slice_new (TnyStatus);

	status->position = position;
	status->of_total = of_total;
	status->domain = domain;
	status->code = code;
	status->message = g_strdup_vprintf (format, args);

	return status;
}

/**
 * tny_status_new:
 * @domain: status domain 
 * @code: status code
 * @position: the position
 * @of_total: the total amount of events to happen
 * @format: printf()-style format for status message
 * @Varargs: parameters for message format
 * 
 * Creates a new #TnyStatus with the given @domain and @code,
 * and a message formatted with @format.
 * 
 * returns: (caller-owns): a new #TnyStatus
 **/
TnyStatus* 
tny_status_new (GQuark domain, gint code, guint position, guint of_total, const gchar *format, ...)
{
	TnyStatus* status;
	va_list args;

	g_return_val_if_fail (format != NULL, NULL);
	g_return_val_if_fail (domain != 0, NULL);

	va_start (args, format);
	status = tny_status_new_valist (domain, code, position, of_total, format, args);
	va_end (args);

	return status;
}

/**
 * tny_status_new_literal:
 * @domain: status domain
 * @code: status code
 * @position: the position
 * @of_total: the total amount of events to happen
 * @message: status message
 * 
 * Creates a new #TnyStatus; unlike tny_status_new(), @message is not
 * a printf()-style format string. Use this 
 * function if @message contains text you don't have control over, 
 * that could include printf() escape sequences.
 * 
 * returns: (caller-owns): a new #TnyStatus
 **/
TnyStatus* 
tny_status_new_literal (GQuark domain, gint code, guint position, guint of_total, const gchar *message)
{
	TnyStatus* status;

	g_return_val_if_fail (message != NULL, NULL);
	g_return_val_if_fail (domain != 0, NULL);

	status = g_slice_new (TnyStatus);

	status->position = position;
	status->of_total = of_total;
	status->domain = domain;
	status->code = code;
	status->message = g_strdup (message);

	return status;
}

/**
 * tny_status_free:
 * @status: a #TnyStatus
 *
 * Destroys a #TnyStatus and associated resources.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_status_free (TnyStatus *status)
{
	g_return_if_fail (status != NULL);  

	g_free (status->message);

	g_slice_free (TnyStatus, status);
}

/**
 * tny_status_copy:
 * @status: a #TnyStatus
 * 
 * Makes a full copy of @status (not just a shallow copy).
 * 
 * returns: (caller-owns): a new #TnyStatus
 * since: 1.0
 * audience: application-developer
 **/
TnyStatus* 
tny_status_copy (const TnyStatus *status)
{
	TnyStatus *copy;

	g_return_val_if_fail (status != NULL, NULL);

	copy = g_slice_new (TnyStatus);

	*copy = *status;

	copy->message = g_strdup (status->message);

	return copy;
}

/**
 * tny_status_matches:
 * @status: a #TnyStatus
 * @domain: a status domain
 * @code: a status code
 * 
 * Returns %TRUE if @status matches @domain and @code, %FALSE
 * otherwise.
 * 
 * returns: TRUE if @status has @domain and @code, else FALSE
 * since: 1.0
 * audience: application-developer
 **/
gboolean 
tny_status_matches (const TnyStatus *status, GQuark domain, gint code)
{
	return status &&
		status->domain == domain &&
		status->code == code;
}

/**
 * tny_set_status:
 * @status: a return location for a #TnyStatus, or %NULL
 * @domain: status domain
 * @code: status code 
 * @position: the position
 * @of_total: the total amount of events to happen
 * @format: printf()-style format
 * @Varargs: args for @format 
 * 
 * Does nothing if @status is %NULL; if @status is non-%NULL, then *@status must
 * be %NULL. A new #TnyStatus is created and assigned to *@status.
 *
 * since: 1.0
 * audience: application-developer
 **/
void 
tny_set_status (TnyStatus **status, GQuark domain, gint code, guint position, guint of_total, gchar *format, ...)
{
	TnyStatus *new;

	va_list args;

	if (status == NULL)
		return;

	va_start (args, format);
	new = tny_status_new_valist (domain, code, position, of_total, format, args);
	va_end (args);

	if (*status == NULL)
		*status = new;
}


/**
 * tny_clear_status:
 * @status: a #TnyStatus return location
 * 
 * If @status is %NULL, does nothing. If @status is non-%NULL,
 * calls tny_status_free() on *@status and sets *@status to %NULL.
 *
 * since: 1.0
 * audience: application-developer
 **/
void
tny_clear_status (TnyStatus **status)
{
	if (status && *status)
	{
		tny_status_free (*status);
		*status = NULL;
	}
}

/**
 * tny_status_get_message:
 * @status: a #TnyStatus 
 * 
 * returns: (null-ok): the message of @status (as a const gchar*)
 **/

/**
 * tny_status_get_domain:
 * @status: a #TnyStatus 
 * 
 * returns: the domain of @status (as a TnyStatusDomain)
 **/

/**
 * tny_status_get_code:
 * @status: a #TnyStatus 
 * 
 * returns: the code of @status (as a TnyStatusCode)
 **/


static gpointer
tny_status_domain_register_type (gpointer notused)
{
  GType etype = 0;
  static const GEnumValue values[] = {
    { TNY_FOLDER_STATUS, "TNY_FOLDER_STATUS", "folder_status" },
    { TNY_GET_MSG_QUEUE_STATUS, "TNY_GET_MSG_QUEUE_STATUS", "get_msg_queue_status" },
    { TNY_GET_SUPPORTED_SECURE_AUTH_STATUS, "TNY_GET_SUPPORTED_SECURE_AUTH_STATUS", "secured-auth-status" },
    { 0, NULL, NULL }
  };
  etype = g_enum_register_static ("TnyStatusDomain", values);
  return GUINT_TO_POINTER (etype);
}

/**
 * tny_status_domain_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_status_domain_get_type (void)
{
  static GOnce once = G_ONCE_INIT;
  g_once (&once, tny_status_domain_register_type, NULL);
  return GPOINTER_TO_UINT (once.retval);
}

static gpointer
tny_status_code_register_type (gpointer notused)
{
  GType etype = 0;
  static const GEnumValue values[] = {
    { TNY_FOLDER_STATUS_CODE_REFRESH, "TNY_FOLDER_STATUS_CODE_REFRESH", "folder_status_code_refresh" },
    { TNY_FOLDER_STATUS_CODE_GET_MSG, "TNY_FOLDER_STATUS_CODE_GET_MSG", "folder_status_code_get_msg" },
    { TNY_GET_MSG_QUEUE_STATUS_GET_MSG, "TNY_GET_MSG_QUEUE_STATUS_GET_MSG", "get_msg_queue_status_get_msg" },
    { TNY_FOLDER_STATUS_CODE_XFER_MSGS, "TNY_FOLDER_STATUS_CODE_XFER_MSGS", "xfer-msgs" },
    { TNY_FOLDER_STATUS_CODE_COPY_FOLDER, "TNY_FOLDER_STATUS_CODE_COPY_FOLDER", "copy-folder" },
    { TNY_GET_SUPPORTED_SECURE_AUTH_STATUS_GET_SECURE_AUTH, "TNY_GET_SUPPORTED_SECURE_AUTH_STATUS_GET_SECURE_AUTH", "get-secure-auth" },
    { TNY_FOLDER_STATUS_CODE_SYNC, "TNY_FOLDER_STATUS_CODE_SYNC", "code-sync" },
    { 0, NULL, NULL }
  };
  etype = g_enum_register_static ("TnyStatusCode", values);
  return GUINT_TO_POINTER (etype);
}

/**
 * tny_status_code_get_type:
 *
 * GType system helper function
 *
 * returns: a #GType
 **/
GType
tny_status_code_get_type (void)
{
  static GOnce once = G_ONCE_INIT;
  g_once (&once, tny_status_code_register_type, NULL);
  return GPOINTER_TO_UINT (once.retval);
}
