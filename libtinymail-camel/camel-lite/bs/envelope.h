#ifndef ENVELOPE_H
#define ENVELOPE_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct _envelope envelope_t;

struct _envelope {
	gchar *date;
	gchar *subject;
	gchar *from;
	gchar *sender;
	gchar *reply_to;
	gchar *to;
	gchar *cc;
	gchar *bcc;
	gchar *in_reply_to;
	gchar *message_id;
};

envelope_t* envelope_new (void);
envelope_t* envelope_parse (guchar *inbuf, guint inlen, GError **err);
void envelope_free (envelope_t *node);

G_END_DECLS

#endif
