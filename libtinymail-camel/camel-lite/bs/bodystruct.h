#ifndef BODYSTRUCT_H
#define BODYSTRUCT_H

#include <glib.h>

#include "envelope.h"

G_BEGIN_DECLS

typedef struct _bodystruct bodystruct_t;
typedef struct _mimeparam mimeparam_t;

struct _mimeparam {
	mimeparam_t *next;
	gchar *name;
	gchar *value;
};

typedef struct {
	gchar *type;
	mimeparam_t *params;
} disposition_t;

struct _bodystruct {
	bodystruct_t *next;

	struct {
		gchar *type;
		gchar *subtype;
		mimeparam_t *params;
		gchar *lang;
		gchar *loc;
		gchar *cid;
		gchar *md5;
	} content;

	disposition_t disposition;

	gchar *encoding;
	gchar *description;

	envelope_t *envelope;
	bodystruct_t *subparts;
	guint octets, lines;

	gchar *part_spec;
	bodystruct_t *parent;
	guint ref_count;
};


bodystruct_t* bodystruct_parse (guchar *inbuf, guint inlen, GError **err);
void bodystruct_free (bodystruct_t *node);

const gchar* mimeparam_get_value_for (mimeparam_t *mp, const gchar *key);

#ifdef DEBUG
void bodystruct_dump (bodystruct_t *part);
#endif

G_END_DECLS

#endif
