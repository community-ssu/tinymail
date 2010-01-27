/* tinymail - Tiny Mail
 * Copyright (C) 2006-2008 Philip Van Hoof <pvanhoof@gnome.org>
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
#include <libintl.h>
#include <gtk/gtk.h>

#include <tny-account-store-view.h>
#include <tny-platform-factory.h>

#if PLATFORM==1
#include <tny-gnome-platform-factory.h>
#endif

#if PLATFORM==2
#include <tny-maemo-platform-factory.h>
#endif

#if PLATFORM==3
#include <tny-gpe-platform-factory.h>
#endif

#if PLATFORM==4
#include <tny-olpc-platform-factory.h>
#endif

#include <tny-demoui-summary-view.h>

#ifdef GNOME
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#endif

/**
 * main:
 * @argc: Amount of arguments
 * @argv: The arguments
 *
 * Main entry point of the application
 *
 * Return value: application return value (0 on success)
 **/
int 
main (int argc, char **argv)
{
	GtkWidget *view = NULL, *window = NULL;
	TnyPlatformFactory *platfact;
	GOptionContext *context;
	static gint plug = 0;
	TnyAccountStore *account_store;

	static GOptionEntry entries[] = {
		{ "plug", 'p', 0, G_OPTION_ARG_INT, &plug,
			"Socket ID of an XEmbed socket to plug into", NULL },
		{ NULL }
	};

	context = g_option_context_new (" - libtinymail demo application");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_parse (context, &argc, &argv, NULL);

	if (!g_thread_supported ()) 
		g_thread_init (NULL);
	gdk_threads_init ();

	gdk_threads_enter ();

	gtk_init (&argc, &argv);

#ifdef GNOME
	gnome_vfs_init ();
#endif

	bindtextdomain(GETTEXT_PACKAGE, TNY_LOCALE_DIR);

#if PLATFORM==1
	platfact = tny_gnome_platform_factory_get_instance ();
#endif
    
#if PLATFORM==2
	platfact = tny_maemo_platform_factory_get_instance ();    
#endif

#if PLATFORM==3
	platfact = tny_gpe_platform_factory_get_instance ();    
#endif

#if PLATFORM==4
	platfact = tny_olpc_platform_factory_get_instance ();    
#endif

	view = GTK_WIDGET (tny_demoui_summary_view_new ());

	if (plug > 0) {
		g_message ("Plugging into socket %d", plug);
		window = gtk_plug_new (plug);
	} else {
		window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title (GTK_WINDOW (window), "Demo ui");
	}

	gtk_container_add (GTK_CONTAINER (window), view);
	
	account_store = tny_platform_factory_new_account_store (platfact);
	tny_account_store_view_set_account_store (
		TNY_ACCOUNT_STORE_VIEW (view), account_store);
	g_object_unref (G_OBJECT (account_store));
	
	g_signal_connect (window, "destroy",
		G_CALLBACK (gtk_main_quit), 0);

	gtk_widget_show (view);
	gtk_widget_show (window);

	gtk_main();

	gdk_threads_leave ();

	return 0;
}
