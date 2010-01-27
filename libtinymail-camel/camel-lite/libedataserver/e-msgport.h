
#ifndef _E_MSGPORT_H
#define _E_MSGPORT_H

#include <time.h>
#include <glib.h>

/* double-linked list yeah another one, deal */
typedef struct _EDListNode {
	struct _EDListNode *next;
	struct _EDListNode *prev;
} EDListNode;

typedef struct _EDList {
	struct _EDListNode *head;
	struct _EDListNode *tail;
	struct _EDListNode *tailpred;
} EDList;

#define E_DLIST_INITIALISER(l) { (EDListNode *)&l.tail, 0, (EDListNode *)&l.head }

void e_dlist_init(EDList *v);
EDListNode *e_dlist_addhead(EDList *l, EDListNode *n);
EDListNode *e_dlist_addtail(EDList *l, EDListNode *n);
EDListNode *e_dlist_remove(EDListNode *n);
EDListNode *e_dlist_remhead(EDList *l);
EDListNode *e_dlist_remtail(EDList *l);
int e_dlist_empty(EDList *l);
int e_dlist_length(EDList *l);

/* a time-based cache */
typedef struct _EMCache EMCache;
typedef struct _EMCacheNode EMCacheNode;

/* subclass this for your data nodes, EMCache is opaque */
struct _EMCacheNode {
	struct _EMCacheNode *next, *prev;
	char *key;
	int ref_count;
	time_t stamp;
};

EMCache *em_cache_new(time_t timeout, size_t nodesize, GFreeFunc nodefree);
void em_cache_destroy(EMCache *emc);
EMCacheNode *em_cache_lookup(EMCache *emc, const char *key);
EMCacheNode *em_cache_node_new(EMCache *emc, const char *key);
void em_cache_node_unref(EMCache *emc, EMCacheNode *n);
void em_cache_add(EMCache *emc, EMCacheNode *n);
void em_cache_clear(EMCache *emc);

/* message ports - a simple inter-thread 'ipc' primitive */
/* opaque handle */
typedef struct _EMsgPort EMsgPort;

/* header for any message */
typedef struct _EMsg {
	EDListNode ln;
	EMsgPort *reply_port;
	gint flags;
} EMsg;

EMsgPort *e_msgport_new(void);
void e_msgport_destroy(EMsgPort *mp);
/* get a fd that can be used to wait on the port asynchronously */
int e_msgport_fd(EMsgPort *mp);
void e_msgport_put(EMsgPort *mp, EMsg *msg);
EMsg *e_msgport_wait(EMsgPort *mp);
EMsg *e_msgport_get(EMsgPort *mp);
void e_msgport_reply(EMsg *msg);
#ifdef HAVE_NSS
struct PRFileDesc *e_msgport_prfd(EMsgPort *mp);
#endif


#endif
