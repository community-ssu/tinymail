#ifndef TNY_SIMPLE_LIST_PRIV_H
#define TNY_SIMPLE_LIST_PRIV_H

typedef struct _TnySimpleListPriv TnySimpleListPriv;

struct _TnySimpleListPriv
{
	GMutex *iterator_lock;
	GList *first;
};

#define TNY_SIMPLE_LIST_GET_PRIVATE(o)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), TNY_TYPE_SIMPLE_LIST, TnySimpleListPriv))

#endif
