/* libtinymailui-mozembed - The Tiny Mail UI library for Gtk+
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

#include "mozilla-preferences.h"

#include <stdlib.h>

#ifdef XPCOM_GLUE
#include <gtkmozembed_glue.cpp>
#endif

#ifdef HAVE_MOZILLA_1_9
#include <gtkmozembed_common.h>
#else
// #include <nsEmbedString.h>
#include <nsComponentManagerUtils.h>
// #include <nsIPrefService.h>
#include <nsIServiceManager.h>
#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <pref/nsIPref.h>
#endif
#include <gtkmozembed.h>


extern "C" gboolean
_mozilla_preference_set (const char *preference_name, const char *new_value)
{
    g_return_val_if_fail (preference_name != NULL, FALSE);
    g_return_val_if_fail (new_value != NULL, FALSE);

#ifdef HAVE_MOZILLA_1_9    
    return gtk_moz_embed_common_set_pref (GTK_TYPE_STRING, (gchar *) preference_name, (void *) new_value);
#else
    nsCOMPtr<nsIPref> pref = do_CreateInstance (NS_PREF_CONTRACTID);
    
    if (pref)
    {
	nsresult rv = pref->SetCharPref (preference_name, new_value);
	
	return NS_SUCCEEDED (rv) ? TRUE : FALSE;
    }
    return FALSE;
#endif    
}

extern "C" gboolean
_mozilla_preference_set_boolean (const char *preference_name, gboolean new_boolean_value)
{
    g_return_val_if_fail (preference_name != NULL, FALSE);

#ifdef HAVE_MOZILLA_1_9
    gboolean bool_value = new_boolean_value;
    gtk_moz_embed_common_set_pref (GTK_TYPE_BOOL, "security.checkloaduri", &bool_value);
#else
    nsCOMPtr<nsIPref> pref = do_CreateInstance (NS_PREF_CONTRACTID);
    
    if (pref)
    {
	nsresult rv = pref->SetBoolPref (preference_name,
					 new_boolean_value ? PR_TRUE : PR_FALSE);
	
	return NS_SUCCEEDED (rv) ? TRUE : FALSE;
    }
    
    return FALSE;
#endif
}

extern "C" gboolean
_mozilla_preference_set_int (const char *preference_name, gint new_int_value)
{
    g_return_val_if_fail (preference_name != NULL, FALSE);
    
#ifdef HAVE_MOZILLA_1_9
    gint int_value = new_int_value;
    gtk_moz_embed_common_set_pref (GTK_TYPE_INT, "permissions.default.image", &int_value);
#else
    nsCOMPtr<nsIPref> pref = do_CreateInstance (NS_PREF_CONTRACTID);
    
    if (pref)
    {
	nsresult rv = pref->SetIntPref (preference_name, new_int_value);
	
	return NS_SUCCEEDED (rv) ? TRUE : FALSE;
    }
    
    return FALSE;
#endif
}

extern "C" void
_mozilla_preference_init ()
{
    const gchar *home_path;
    const gchar *full_path;
    const gchar *prgname;
    gchar *profile_path;
    gchar *useragent;

#ifdef XPCOM_GLUE
    nsresult rv;
    static const GREVersionRange greVersion = {
      "1.9a", PR_TRUE,
      "2", PR_TRUE
    };
    char xpcomLocation[4096];
    rv = GRE_GetGREPathWithProperties(&greVersion, 1, nsnull, 0, xpcomLocation, 4096);
    if (NS_FAILED (rv))
    {
      g_warning ("Could not determine locale!\n");
      return;
    }

    // Startup the XPCOM Glue that links us up with XPCOM.
    rv = XPCOMGlueStartup(xpcomLocation);
    if (NS_FAILED (rv))
    {
      g_warning ("Could not determine locale!\n");
      return;
    }

    rv = GTKEmbedGlueStartup();
    if (NS_FAILED (rv))
    {
      g_warning ("Could not determine locale!\n");
      return;
    }

    char *lastSlash = strrchr(xpcomLocation, '/');
    if (lastSlash)
      *lastSlash = '\0';

    gtk_moz_embed_set_path(xpcomLocation);

#else // XPCOM_GLUE
#ifdef HAVE_MOZILLA_1_9
    gtk_moz_embed_set_path (MOZILLA_HOME);
#else
    gtk_moz_embed_set_comp_path (MOZILLA_HOME);
#endif // HAVE_MOZILLA_1_9
#endif // XPCOM_GLUE

    home_path = getenv ("HOME");
    if (!home_path)
	home_path = g_get_home_dir ();
    prgname = g_get_prgname ();
    if (!prgname)
	prgname = "tinymail";
    full_path = g_strdup_printf("%s/%s", home_path, ".tinymail");
    profile_path = g_strconcat ("mozembed-", prgname, NULL);
    gtk_moz_embed_set_profile_path (full_path, profile_path);
    g_free (profile_path);
    
    gtk_moz_embed_push_startup ();
    
    _mozilla_preference_set_int ("permissions.default.image", 2);
    _mozilla_preference_set_int ("permissions.default.script", 2);
    
    _mozilla_preference_set_boolean ("security.checkloaduri", FALSE);
    
    useragent = g_strconcat (prgname, "/", VERSION, NULL);
    _mozilla_preference_set ("general.useragent.misc", useragent);
    g_free (useragent);
    _mozilla_preference_set ("network.proxy.no_proxies_on", "localhost");
}
