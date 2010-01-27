#ifndef TNY_SESSION_CAMEL_PRIV_H
#define TNY_SESSION_CAMEL_PRIV_H

#include <tny-camel-send-queue.h>

struct _TnySessionCamelPriv
{
	gpointer device;
	gpointer account_store;
	gboolean interactive, prev_constat;
	guint connchanged_signal;

	GHashTable *current_accounts;
	GStaticRecMutex *current_accounts_lock;

	gchar *camel_dir;
	gboolean in_auth_function, is_connecting;
	TnyLockable *ui_lock;
	GMutex *queue_lock;

	gboolean stop_now, initialized;
	gboolean is_inuse;
	GList *regged_queues;
};

void _tny_session_camel_register_account (TnySessionCamel *self, TnyCamelAccount *account);
void _tny_session_camel_activate_account (TnySessionCamel *self, TnyCamelAccount *account);
void _tny_session_camel_unregister_account (TnySessionCamel *self, TnyCamelAccount *account);

void _tny_session_camel_reg_queue (TnySessionCamel *self, TnyCamelSendQueue *queue);
void _tny_session_camel_unreg_queue (TnySessionCamel *self, TnyCamelSendQueue *queue);

#endif
