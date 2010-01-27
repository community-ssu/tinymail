/* tinymail - Tiny Mail
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

#include <sys/mman.h>
#include <camel/camel-session.h>

#include <config.h>
#include <glib/gi18n-lib.h>

#include <string.h>
#include <glib.h>

#include <tny-platform-factory.h>
#include "platfact.h"

#include <tny-account-store.h>
#include <tny-account.h>
#include <tny-camel-account.h>

#include <tny-store-account.h>
#include <tny-transport-account.h>
#include <tny-camel-store-account.h>
#include <tny-camel-transport-account.h>
#include <tny-device.h>
#include <tny-session-camel.h>

#include "device.h"
#include "account-store.h"


static GObjectClass *parent_class = NULL;


static gchar* 
per_account_get_pass(TnyAccount *account, const gchar *prompt, gboolean *cancel)
{
	if (strstr (tny_account_get_name (account), "SMTP"))
		return g_strdup ("unittest");
	else
		return g_strdup ("tnytest");
}

static void
per_account_forget_pass(TnyAccount *account)
{
	g_print ("Invalid test account (password incorrect)\n");
	return;
}


static gboolean
tny_test_account_store_alert (TnyAccountStore *self, TnyAccount *account, TnyAlertType type, gboolean question, GError *error)
{
	return TRUE;
}


static const gchar*
tny_test_account_store_get_cache_dir (TnyAccountStore *self)
{
	TnyTestAccountStore *me = (TnyTestAccountStore*) self;

	if (me->cache_dir == NULL)
	{
		gint att=0;
		GDir *dir = NULL;
		do {
			gchar *attempt = g_strdup_printf ("tinymail.%d", att);
			gchar *str = g_build_filename (g_get_tmp_dir (), attempt, NULL);
			g_free (attempt);
			dir = g_dir_open (str, 0, NULL);
			if (dir)
			{
				g_dir_close (dir);
				g_free (str);
			} else 
				me->cache_dir = str;
			att++;
		} while (dir != NULL);
	}
    
	return me->cache_dir;
}

typedef struct {
	GCond *condition;
	GMutex *mutex;
	gboolean had_callback;
} SyncInfo;


static void went_online (TnyCamelAccount *account, gboolean canceled, GError *err, gpointer user_data)
{
	SyncInfo *info = (SyncInfo *) user_data;

	g_mutex_lock (info->mutex);
	g_cond_broadcast (info->condition);
	info->had_callback = TRUE;
	g_mutex_unlock (info->mutex);

	return;	
}

static void
tny_test_account_store_get_accounts (TnyAccountStore *self, TnyList *list, TnyGetAccountsRequestType types)
{
	TnyTestAccountStore *me = (TnyTestAccountStore *) self;
	TnyAccount *account;


	/* Dear visitor of the SVN-web. This is indeed a fully functional and
	   working IMAP account. This does not mean that you need to fuck it up */

	if (types == TNY_ACCOUNT_STORE_STORE_ACCOUNTS || types == TNY_ACCOUNT_STORE_BOTH)
	{
		account = TNY_ACCOUNT (tny_camel_store_account_new ());

		tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (account), me->session);
		camel_session_set_online ((CamelSession*)me->session, me->force_online); 

		tny_account_set_proto (account, "imap");
		tny_account_set_name (account, "imap1.tinymail.org");
		tny_account_set_user (account, "tnytest");
		tny_account_set_hostname (account, "imap1.tinymail.org");
		tny_account_set_id (account, "tnytest@imap1.tinymail.org");
		tny_account_set_forget_pass_func (account, per_account_forget_pass);
		tny_account_set_pass_func (account, per_account_get_pass);

		if (me->force_online) {
			SyncInfo *info = g_slice_new (SyncInfo);

			info->mutex = g_mutex_new ();
			info->condition = g_cond_new ();
			info->had_callback = FALSE;

			tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (account), me->force_online, went_online, info);

			g_mutex_lock (info->mutex);
			if (!info->had_callback)
				g_cond_wait (info->condition, info->mutex);
			g_mutex_unlock (info->mutex);

			g_mutex_free (info->mutex);
			g_cond_free (info->condition);
			g_slice_free (SyncInfo, info);
		}

		tny_list_prepend (list, (GObject*)account);
		g_object_unref (G_OBJECT (account));
	}

	if (types == TNY_ACCOUNT_STORE_TRANSPORT_ACCOUNTS || types == TNY_ACCOUNT_STORE_BOTH)
	{
		account = TNY_ACCOUNT (tny_camel_transport_account_new ());

		tny_camel_account_set_session (TNY_CAMEL_ACCOUNT (account), me->session);
		camel_session_set_online ((CamelSession*)me->session, me->force_online); 

		tny_account_set_proto (account, "something");
		tny_account_set_name (account, "SMTP unit test account");
		tny_account_set_id (account, "unique_smtp");
		tny_account_set_url_string (account, "smtp://tinymailunittest;auth=PLAIN@mail.tinymail.org:2222/;use_ssl=wrapped");

		tny_account_set_forget_pass_func (account, per_account_forget_pass);
		tny_account_set_pass_func (account, per_account_get_pass);

		if (me->force_online) {
			SyncInfo *info = g_slice_new (SyncInfo);

			info->mutex = g_mutex_new ();
			info->condition = g_cond_new ();
			info->had_callback = FALSE;

			tny_camel_account_set_online (TNY_CAMEL_ACCOUNT (account), me->force_online, went_online, info);

			g_mutex_lock (info->mutex);
			if (!info->had_callback)
				g_cond_wait (info->condition, info->mutex);
			g_mutex_unlock (info->mutex);

			g_mutex_free (info->mutex);
			g_cond_free (info->condition);
			g_slice_free (SyncInfo, info);
		}

		tny_list_prepend (list, (GObject*)account);
		g_object_unref (G_OBJECT (account));
	}

	if (me->force_online)
		tny_device_force_online (me->device);
	else
		tny_device_force_offline (me->device);

	return;
}



TnyAccountStore*
tny_test_account_store_new (gboolean force_online, const gchar *cachedir)
{
	TnyTestAccountStore *self = g_object_new (TNY_TYPE_TEST_ACCOUNT_STORE, NULL);

	if (cachedir)
	{
		if (self->cache_dir)
			g_free (self->cache_dir);
		self->cache_dir = g_strdup (cachedir);
	}

	self->session = tny_session_camel_new (TNY_ACCOUNT_STORE (self));

	self->force_online = force_online;


	return TNY_ACCOUNT_STORE (self);
}


static void
tny_test_account_store_instance_init (GTypeInstance *instance, gpointer g_class)
{
	TnyTestAccountStore *self = (TnyTestAccountStore *)instance;
	TnyPlatformFactory *platfact = tny_test_platform_factory_get_instance ();

	self->device = tny_platform_factory_new_device (platfact);
	g_object_unref (G_OBJECT (platfact));

	return;
}


static void
tny_test_account_store_finalize (GObject *object)
{
	TnyTestAccountStore *me = (TnyTestAccountStore*) object;
    
	if (me->cache_dir)
		g_free (me->cache_dir);

	g_object_unref (me->device);
    
	(*parent_class->finalize) (object);

	return;
}


static void 
tny_test_account_store_class_init (TnyTestAccountStoreClass *class)
{
	GObjectClass *object_class;
    
	parent_class = g_type_class_peek_parent (class);
	object_class = (GObjectClass*) class;

	object_class->finalize = tny_test_account_store_finalize;

	return;
}


static TnyDevice*
tny_test_account_store_get_device (TnyAccountStore *self)
{
	TnyTestAccountStore *me =  (TnyTestAccountStore*) self;

	return g_object_ref (G_OBJECT (me->device));
}


static void
tny_account_store_init (gpointer g, gpointer iface_data)
{
	TnyAccountStoreIface *klass = (TnyAccountStoreIface *)g;

	klass->get_accounts= tny_test_account_store_get_accounts;
	klass->get_cache_dir= tny_test_account_store_get_cache_dir;
	klass->alert= tny_test_account_store_alert;
	klass->get_device= tny_test_account_store_get_device;

	return;
}


GType 
tny_test_account_store_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
		  sizeof (TnyTestAccountStoreClass),
		  NULL,   /* base_init */
		  NULL,   /* base_finalize */
		  (GClassInitFunc) tny_test_account_store_class_init,   /* class_init */
		  NULL,   /* class_finalize */
		  NULL,   /* class_data */
		  sizeof (TnyTestAccountStore),
		  0,      /* n_preallocs */
		  tny_test_account_store_instance_init    /* instance_init */
		};

		static const GInterfaceInfo tny_account_store_info = 
		{
		  (GInterfaceInitFunc) tny_account_store_init, /* interface_init */
		  NULL,         /* interface_finalize */
		  NULL          /* interface_data */
		};

		type = g_type_register_static (G_TYPE_OBJECT,
			"TnyTestAccountStore",
			&info, 0);

		g_type_add_interface_static (type, TNY_TYPE_ACCOUNT_STORE, 
			&tny_account_store_info);
	}

	return type;
}
