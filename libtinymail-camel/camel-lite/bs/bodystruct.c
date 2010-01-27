#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <glib/gstdio.h>

#include "bodystruct.h"

#define PRINT_NULL(o)	(o?o:"(null)")

#ifdef DEBUG
#define debug_printf	printf
#else
#define debug_printf(o, ...)	 
#endif

static void 
set_error (GError **err, unsigned char **in)
{
	if (in) {
		g_set_error (err, 0, 1,
			"Incorrect response from IMAP server in BODYSTRUCTURE (%s)", *in);
	} else {
		g_set_error (err, 0, 1,
			"Incorrect response from IMAP server in BODYSTRUCTURE");
	}
}


envelope_t *
envelope_new (void)
{
	return g_slice_new0 (struct _envelope);
}

void 
envelope_free (envelope_t *node)
{
	g_free (node->date);
	g_free (node->subject);
	g_free (node->from);
	g_free (node->sender);
	g_free (node->reply_to);
	g_free (node->to);
	g_free (node->cc);
	g_free (node->bcc);
	g_free (node->in_reply_to);
	g_free (node->message_id);

	g_slice_free (struct _envelope, node);
}

static struct _mimeparam*
mimeparam_new (void)
{
	return g_slice_new0 (struct _mimeparam);
}

static struct _bodystruct*
bodystruct_new (void)
{
	return g_slice_new0 (struct _bodystruct);
}

static void 
mimeparam_destroy (struct _mimeparam *param)
{
	struct _mimeparam *next;
	
	while (param) {
		next = param->next;
		g_free (param->name);
		g_free (param->value);
		g_slice_free (struct _mimeparam, param);
		param = next;
	}
}

static void
unescape_qstring (char *qstring)
{
	char *s, *d;
	
	d = s = qstring;
	while (*s != '\0') {
		if (*s != '\\')
			*d++ = *s++;
		else
			s++;
	}
	
	*d = '\0';
}

static char *
decode_qstring (unsigned char **in, unsigned char *inend, GError **err)
{
	unsigned char *inptr, *start;
	char *qstring = NULL;
	
	inptr = *in;
	
	while (inptr < inend && *inptr == ' ')
		inptr++;
	
	if (inptr == inend) {
		*in = inptr;
		set_error (err, in);
		return NULL;
	}

	if (strncmp ((const char *) inptr, "NIL", 3) != 0) {
		gboolean quoted = FALSE;

		if (*inptr == '"') {
			inptr++;
			quoted = TRUE;
		}
		start = inptr;
		while (inptr < inend) {
			if (quoted) {
				if (*inptr == '"' && inptr[-1] != '\\')
					break;
			} else {
				if (*inptr == ')' && inptr[-1] != '\\')
					break;
			}
			inptr++;
		}
		
		qstring = g_strndup ((const char *) start, inptr - start);
		unescape_qstring (qstring);

		if (quoted && *inptr != '"') {
			g_free (qstring);
			*in = inptr;
			set_error (err, in);
			return NULL;
		}
		if (quoted)
			inptr++;
	} else
		inptr += 3;

	*in = inptr;

	return qstring;
}



static char *
decode_estring (unsigned char **in, unsigned char *inend, GError **err)
{
	unsigned char *inptr, *start;
	char *qstring = NULL;

	inptr = *in;

	while (inptr < inend && *inptr == ' ')
		inptr++;

	if (inptr == inend) {
		*in = inptr;
		set_error (err, in);
		return NULL;
	}

	if (strncmp ((const char *) inptr, "NIL", 3) != 0) {
		if (*inptr == '(') {
			gboolean first = TRUE;
			GString *str = NULL;

			if (*inptr != '(') {
				*in = inptr;
				set_error (err, in);
				return NULL;
			}

			str = g_string_new ("");

			inptr++; /* My '(' */
			while (*inptr == '(') {
				char *separator = NULL;
				char *name = NULL;
				char *user = NULL;
				char *server = NULL;

				inptr++; /* My '(' */

				if (!first)
					g_string_append (str, ", ");
				first = FALSE;

				name = decode_qstring (&inptr, inend, err);
				separator = decode_qstring (&inptr, inend, err);
				user = decode_qstring (&inptr, inend, err);
				server = decode_qstring (&inptr, inend, err);

				if (name) {
					g_string_append (str, name);
					g_string_append (str, " <");
				}

				if (user && server) {
					g_string_append (str, user);
					g_string_append_c (str, '@');
				}

				if (server)
					g_string_append (str, server);

				if (name) 
					g_string_append_c (str, '>');

				if (separator)
					g_free (separator);
				if (name)
					g_free (name);
				if (user)
					g_free (user);
				if (server)
					g_free (server);


				if (*inptr != ')') {
					*in = inptr;
					set_error (err, in);
					g_string_free (str, TRUE);
					return NULL;
				}

				inptr++; /* My ')' */

				/* Read spaces */
				while (inptr < inend && *inptr == ' ')
					inptr++;

			}

			/* TODO: check for ')' */
			if (*inptr != ')')
				g_warning ("strange");
			inptr++; /* My ')' */

			qstring = str->str;
			g_string_free (str, FALSE);
		} else {
			if (strncmp ((const char *) inptr, "NIL", 3) != 0) {
				if (*inptr != '"') {
					*in = inptr;
					set_error (err, in);
					return NULL;
				}
				inptr++;
				start = inptr;
				while (inptr < inend) {
					if (*inptr == '"' && inptr[-1] != '\\')
						break;
					inptr++;
				}
				
				qstring = g_strndup ((const char *) start, inptr - start);
				unescape_qstring (qstring);

				if (*inptr != '"') {
					*in = inptr;
					g_free (qstring);
					set_error (err, in);
					return NULL;
				}
				inptr++;
			} else
				inptr += 3;
		}

	} else
		inptr += 3;

	*in = inptr;

	return qstring;
}

static unsigned int
decode_num (unsigned char **in, unsigned char *inend, GError **err)
{
	unsigned char *inptr;
	unsigned int ret = 0;

	inptr = *in;

	while (inptr < inend && *inptr == ' ')
		inptr++;

	if (inptr == inend)
		return 0;

	if (strncmp ((const char *) inptr, "NIL", 3) != 0)
		ret = strtoul ((const char *) inptr, (char **) &inptr, 10);
	else
		inptr += 3;

	*in = inptr;

	return ret;
}

#if DEBUG
static void 
print_params (struct _mimeparam *param)
{
	struct _mimeparam *next;
	
	while (param) {
		next = param->next;
		debug_printf ("%s = ", PRINT_NULL (param->name));
		debug_printf ("%s\n", PRINT_NULL (param->value));
		param = next;
	}
}
#else
#define print_params(o)		 
#endif

static struct _mimeparam *
decode_param (unsigned char **in, unsigned char *inend, GError **err)
{
	struct _mimeparam *param;
	char *name, *val;
	unsigned char *inptr;

	inptr = *in;
	if (!(name = decode_qstring (&inptr, inend, err)))
		return NULL; /* This is a NIL */

	val = decode_qstring (&inptr, inend, err);

	if (!val) {
		param = NULL;
		*in = inptr;
		g_free (name);
	} else {
		param = mimeparam_new ();
		param->name = name;
		param->value = val;
	}

	*in = inptr;

	return param;
}

static struct _mimeparam *
decode_params (unsigned char **in, unsigned char *inend, GError **err)
{
	/* "TEXT" "PLAIN" -> ("CHARSET" "UTF-8") <- */
	/* "TEXT" "PLAIN" -> NIL <- */

	struct _mimeparam *params, *tail, *n;
	unsigned char *inptr;

	inptr = *in;
	params = NULL;
	tail = (struct _mimeparam *) &params;

	while (inptr < inend && *inptr == ' ')
		inptr++;

	if (inptr == inend) {
		*in = inptr;
		set_error (err, in);
		return NULL;
	}

	if (strncmp ((const char *) inptr, "NIL", 3) != 0) {

		if (*inptr != '(') {
			*in = inptr;
			set_error (err, in);
			return NULL;
		}

		inptr++;

		while ((n = decode_param (&inptr, inend, err)) != NULL) {
			tail->next = n;
			tail = n;
				
			while (inptr < inend && *inptr == ' ')
				inptr++;
			
			if (*inptr == ')')
				break;
		}

		if (*inptr != ')') {
			*in = inptr;
			if (params)
				mimeparam_destroy (params);
			set_error (err, in);
			return NULL;
		}

		inptr++;
	} else
		inptr += 3;

	*in = inptr;

	return params;
}

static void 
read_unknown_qstring (unsigned char **in, unsigned char *inend, GError **err)
{
	unsigned char *inptr;
	char *str;

	inptr = *in;
	str = decode_qstring (&inptr, inend, err);
	debug_printf ("unknown: %s\n", PRINT_NULL (str));
	g_free (str);
	*in = inptr;

	return;
}



static struct _envelope *
decode_envelope (unsigned char **in, unsigned char *inend, GError **err)
{
	struct _envelope *node;
	unsigned char *inptr;
	
	inptr = *in;

	while (inptr < inend && *inptr == ' ')
		inptr++;

	if (inptr == inend || *inptr != '(') {
		*in = inptr;
		set_error (err, in);
		return NULL;
	}

	inptr++;

	node = envelope_new ();

	node->date = decode_qstring (&inptr, inend, err);
	debug_printf ("env date: %s\n", PRINT_NULL (node->date));
	node->subject = decode_qstring (&inptr, inend, err);
	debug_printf ("env subject: %s\n", PRINT_NULL (node->subject));

	node->from = decode_estring (&inptr, inend, err);
	debug_printf ("env from: %s\n", PRINT_NULL (node->from));

	node->sender = decode_estring (&inptr, inend, err);
	debug_printf ("env sender: %s\n", PRINT_NULL (node->sender));

	node->reply_to = decode_estring (&inptr, inend, err);
	debug_printf ("env reply_to: %s\n", PRINT_NULL (node->reply_to));

	node->to = decode_estring (&inptr, inend, err);
	debug_printf ("env to: %s\n", PRINT_NULL (node->to));

	node->cc = decode_estring (&inptr, inend, err);
	debug_printf ("env cc: %s\n", PRINT_NULL (node->cc));

	node->bcc = decode_estring (&inptr, inend, err);
	debug_printf ("env bcc: %s\n", PRINT_NULL (node->bcc));

	node->in_reply_to = decode_estring (&inptr, inend, err);
	debug_printf ("env in_reply_to: %s\n", PRINT_NULL (node->in_reply_to));

	node->message_id = decode_qstring (&inptr, inend, err);
	debug_printf ("env message_id: %s\n", PRINT_NULL (node->message_id));

	while (inptr < inend && *inptr == ' ')
		inptr++;


	if (*inptr != ')') {
		envelope_free (node);
		*in = inptr;
		set_error (err, in);
		return NULL;
	}


	inptr++;

	*in = inptr;

	return node;
}



static void 
parse_lang (unsigned char **in, unsigned char *inend, struct _bodystruct *part, GError **err)
{
	unsigned char *inptr = *in;

	/* a) NIL 
	 * b) ("NL-BE") 
	 * c) "NL-BE" */

	while (inptr < inend && *inptr == ' ')
		inptr++;

	if (*inptr == '(') {

		inptr++; /* My '(' */

		/* case b */
		part->content.lang = decode_qstring (&inptr, inend, err);
		debug_printf ("lang: %s\n", PRINT_NULL (part->content.lang));

		while (inptr < inend && *inptr == ' ')
			inptr++;

		if (*inptr != ')') {
			g_free (part->content.lang);
			part->content.lang = NULL;
			*in = inptr;
			set_error (err, in);
			return;
		}

		inptr++; /* My ')' */

	} else {
		if (strncmp ((const char *) inptr, "NIL", 3) != 0) {
			/* case c */
			part->content.lang = decode_qstring (&inptr, inend, err);
			debug_printf ("lang: %s\n", PRINT_NULL (part->content.lang));
		} else /* case a */
			inptr += 3;
	}

	*in = inptr;

	return;
}

static void 
parse_content_disposition (unsigned char **in, unsigned char *inend, disposition_t *disposition, GError **err)
{
	unsigned char *inptr = *in;

	/* a) NIL 
	 * b) ("INLINE" NIL) 
	 * c) ("ATTACHMENT" ("FILENAME", "myfile.ext")) 
	 * d) "ALTERNATIVE" ("BOUNDARY", "---")  */

	while (inptr < inend && *inptr == ' ')
		inptr++;

	if (*inptr == '(') {
		inptr++; /* My '(' */

		/* cases c & b */
		disposition->type = decode_qstring (&inptr, inend, err);
		debug_printf ("disposition.type: %s\n", PRINT_NULL (disposition->type));
		disposition->params = decode_params (&inptr, inend, err);
		print_params (disposition->params);

		while (inptr < inend && *inptr == ' ')
			inptr++;

		if (*inptr != ')') {
			g_free (disposition->type);
			disposition->type = NULL;
			if (disposition->params)
				mimeparam_destroy (disposition->params);
			*in = inptr;
			set_error (err, in);
			return;
		}

		inptr++; /* My ')' */
	} else {
		if (strncmp ((const char *) inptr, "NIL", 3) != 0) {
			/* case d */
			disposition->type = decode_qstring (&inptr, inend, err);
			debug_printf ("disposition.type: %s\n", PRINT_NULL (disposition->type));
			disposition->params = decode_params (&inptr, inend, err);
			print_params (disposition->params);
		} else /* case a */
			inptr += 3;
	}

	*in = inptr;

	return;
}



static void
end_this_piece (unsigned char **in, unsigned char *inend, GError **err)
{
	unsigned char *inptr = *in;
	gint open = 0;
	gboolean noerr = FALSE;

	while (inptr < inend)
	{
		if (*inptr == '(')
			open++;

		if (*inptr == ')') {
			if (open == 0) {
				noerr = TRUE;
				break;
			}
			open--;
		}
		inptr++;
	}

	if (!noerr) {
		*in = inptr;
		set_error (err, in);
	}

	*in = inptr;

	return;
}

static struct _bodystruct *
bodystruct_part_decode (unsigned char **in, unsigned char *inend, bodystruct_t *parent, gint num, GError **err)
{
	struct _bodystruct *part, *list, *tail, *n;
	unsigned char *inptr;

	inptr = *in;

	while (inptr < inend && *inptr == ' ')
		inptr++;


	if (inptr == inend || *inptr != '(') {
		*in = inptr;
		return NULL;
	}

	inptr++; /* My '(' */

	part = bodystruct_new ();

	part->part_spec = NULL;
	part->parent = parent;

	if (parent) {
		if (parent->part_spec && *parent->part_spec) {
			if (!strcasecmp (parent->content.type, "message") && !strcasecmp (parent->content.subtype, "rfc822")) {
				part->part_spec = g_strdup (parent->part_spec);
			} else {
				part->part_spec = g_strdup_printf ("%s.%d", parent->part_spec, num);
			}
		} else {
			part->part_spec = g_strdup_printf ("%d", num);
		}
	} else {
		part->part_spec = g_strdup ("");
	}

	if (*inptr == '(') {
		gint cnt = 1;

		part->content.type = g_strdup ("MULTIPART");

		list = NULL;
		tail = (struct _bodystruct *) &list;

		while ((n = bodystruct_part_decode (&inptr, inend, part, cnt, err)) != NULL) 
		{
			tail->next = n;
			tail = n;
			cnt++;

			while (inptr < inend && *inptr == ' ')
				inptr++;

			if (*inptr == ')')
				break;
		}

		part->subparts = list;

		if (*inptr != ')') {
			part->content.subtype = decode_qstring (&inptr, inend, err);
			debug_printf ("contensubtype: %s\n", PRINT_NULL (part->content.subtype));
		}

		if (*inptr != ')') {
			part->content.params = decode_params (&inptr, inend, err);
			print_params (part->content.params);
		}

		/* if (*inptr != ')') {
		 *	parse_something_unknown (&inptr, inend, err);
		 * } */

		if (*inptr != ')') {
			parse_content_disposition (&inptr, inend, &part->disposition, err);
		}

		if (*inptr != ')') {
			parse_lang (&inptr, inend, part, err);
		}

		if (*inptr != ')') {
			end_this_piece (&inptr, inend, err);
		}

	} else {
		part->next = NULL;
		part->content.type = decode_qstring (&inptr, inend, err);
		if (!part->content.type)
			part->content.type = g_strdup ("TEXT");
		debug_printf ("contentype: %s\n", PRINT_NULL (part->content.type));

		part->content.subtype = decode_qstring (&inptr, inend, err);
		if (!part->content.subtype)
			part->content.subtype = g_strdup ("PLAIN");
		debug_printf ("contensubtype: %s\n", PRINT_NULL (part->content.subtype));

		part->disposition.type = NULL;
		part->disposition.params = NULL;
		part->encoding = NULL;
		part->envelope = NULL;
		part->subparts = NULL;


		if (!strcasecmp (part->content.type, "message") && !strcasecmp (part->content.subtype, "rfc822")) {

			if (*inptr != ')') {
				part->content.params = decode_params (&inptr, inend, err);
				print_params (part->content.params);
			}

			if (*inptr != ')') {
				part->content.cid = decode_qstring (&inptr, inend, err);
				debug_printf ("content.cid: %s\n", PRINT_NULL (part->content.cid));
			}

			if (*inptr != ')') {
				part->description = decode_qstring (&inptr, inend, err);
				debug_printf ("description: %s\n", PRINT_NULL (part->description));
			}

			if (*inptr != ')') {
				part->encoding = decode_qstring (&inptr, inend, err);
				if (!part->encoding)
					part->encoding = g_strdup ("7BIT");
				debug_printf ("encoding: %s\n", PRINT_NULL (part->encoding));
			}

			if (*inptr != ')') {
				part->octets = decode_num (&inptr, inend, err);
				debug_printf ("octets: %d\n", part->octets);
			}

			if (*inptr != ')') {
				part->envelope = decode_envelope (&inptr, inend, err);
			}

			if (*inptr != ')') {
				part->subparts = bodystruct_part_decode (&inptr, inend, part, 1, err);
			}

			if (*inptr != ')') {
				part->lines = decode_num (&inptr, inend, err);
				debug_printf ("lines: %d\n", part->lines);
			}

			if (*inptr != ')') {
				read_unknown_qstring (&inptr, inend, err);
			}

			if (*inptr != ')') {
				parse_content_disposition (&inptr, inend, &part->disposition, err);
			}

			if (*inptr != ')') {
				parse_lang (&inptr, inend, part, err);
			}

			if (*inptr != ')') {
				end_this_piece (&inptr, inend, err);
			}

		} else if (!strcasecmp (part->content.type, "text")) {

			if (*inptr != ')') {
				part->content.params = decode_params (&inptr, inend, err);
				print_params (part->content.params);
			}

			if (*inptr != ')') {
				part->content.cid = decode_qstring (&inptr, inend, err);
				debug_printf ("content.cid: %s\n", PRINT_NULL (part->content.cid));
			}

			if (*inptr != ')') {
				part->description = decode_qstring (&inptr, inend, err);
				debug_printf ("description: %s\n", PRINT_NULL (part->description));
			}

			if (*inptr != ')') {
				part->encoding = decode_qstring (&inptr, inend, err);
				debug_printf ("encoding: %s\n", PRINT_NULL (part->encoding));
			}

			if (*inptr != ')') {
				part->octets = decode_num (&inptr, inend, err);
				debug_printf ("octets: %d\n", part->octets);
			}

			if (*inptr != ')') {
				part->lines = decode_num (&inptr, inend, err);
				debug_printf ("lines: %d\n", part->lines);
			}

			if (*inptr != ')') {
				read_unknown_qstring (&inptr, inend, err);
			}

			if (*inptr != ')') {
				parse_content_disposition (&inptr, inend, &part->disposition, err);
			}

			if (*inptr != ')') {
				parse_lang (&inptr, inend, part, err);
			}

			if (*inptr != ')') {
				end_this_piece (&inptr, inend, err);
			}

		} else if (!strcasecmp (part->content.type, "APPLICATION")||
				!strcasecmp (part->content.type, "IMAGE") ||
				!strcasecmp (part->content.type, "VIDEO") ||
				!strcasecmp (part->content.type, "AUDIO"))
		{

			if (*inptr != ')') {
				part->content.params = decode_params (&inptr, inend, err);
				print_params (part->content.params);
			}

			if (*inptr != ')') {
				part->content.cid = decode_qstring (&inptr, inend, err);
				debug_printf ("content.cid: %s\n", PRINT_NULL (part->content.cid));
			}

			if (*inptr != ')') {
				part->description = decode_qstring (&inptr, inend, err);
				debug_printf ("description: %s\n", PRINT_NULL (part->description));
			}

			if (*inptr != ')') {
				part->encoding = decode_qstring (&inptr, inend, err);
				debug_printf ("encoding: %s\n", PRINT_NULL (part->encoding));
			}

			if (*inptr != ')') {
				part->octets = decode_num (&inptr, inend, err);
				debug_printf ("octets: %d\n", part->octets);
			}

			if (*inptr != ')') {
				read_unknown_qstring (&inptr, inend, err);
			}

			if (*inptr != ')') {
				parse_content_disposition (&inptr, inend, &part->disposition, err);
			}

			if (*inptr != ')') {
				parse_lang (&inptr, inend, part, err);
			}

			if (*inptr != ')') {
				end_this_piece (&inptr, inend, err);
			}

		} else {
			/* I don't know how it looks, so I just read it away */
			end_this_piece (&inptr, inend, err);
		}


	}

	if (*inptr != ')') {
		*in = inptr;
		set_error (err, in);
		bodystruct_free (part);
		return NULL;
	}

	inptr++; /* My ')' */

	*in = inptr;

	return part;
}

const gchar * 
mimeparam_get_value_for (mimeparam_t *mp, const gchar *key)
{
	struct _mimeparam *param;
	param = mp;
	gboolean found = FALSE;

	while (param) {
		if (!strcasecmp (param->name, key)) {
			found = TRUE;
			break;
		}
		param = param->next;
	}

	if (found)
		return param->value;

	return NULL;
}

#ifdef DEBUG
static void
bodystruct_dump_r (bodystruct_t *part, gint depth)
{
	struct _mimeparam *param;
	int i;

	for (i = 0; i < depth; i++)
		printf ("  ");

	printf ("IMAP part specification: %s\n", PRINT_NULL (part->part_spec));

	for (i = 0; i < depth; i++)
		printf ("  ");

	printf ("Content-Type: %s/%s", PRINT_NULL (part->content.type),
		 part->content.subtype);

	if (part->content.params) {
		param = part->content.params;
		while (param) {
			printf ("; %s=%s", PRINT_NULL (param->name), 
				PRINT_NULL (param->value));
			param = param->next;
		}
	}

	printf ("\n");

	if (part->content.type && !strcasecmp (part->content.type, "multipart")) {
		part = part->subparts;
		while (part != NULL) {
			bodystruct_dump_r (part, depth + 1);
			part = part->next;
		}
	} else if (part->content.type && !strcasecmp (part->content.type, "message") && part->content.subtype && !strcasecmp (part->content.subtype, "rfc822")) {
		depth++;
		
		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ( "Date: %s\n", PRINT_NULL (part->envelope->date));
		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ("Subject: %s\n", PRINT_NULL (part->envelope->subject));
		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ("From: %s\n", PRINT_NULL (part->envelope->from));
		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ("Sender: %s\n", PRINT_NULL (part->envelope->sender));
		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ("Reply-To: %s\n", PRINT_NULL (part->envelope->reply_to));
		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ("To: %s\n", PRINT_NULL (part->envelope->to));
		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ("Cc: %s\n", PRINT_NULL (part->envelope->cc));
		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ("Bcc: %s\n", PRINT_NULL (part->envelope->bcc));
		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ("In-Reply-To: %s\n", PRINT_NULL (part->envelope->in_reply_to));
		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ("Message-Id: %s\n", PRINT_NULL (part->envelope->message_id));
		bodystruct_dump_r (part->subparts, depth);
		depth--;
	} else {
		if (part->disposition.type) {
			for (i = 0; i < depth; i++)
				printf ("  ");
			printf ("Content-Disposition: %s", PRINT_NULL (part->disposition.type));
			if (part->disposition.params) {
				param = part->disposition.params;
				while (param) {
					printf ("; %s=%s", PRINT_NULL (param->name), 
						PRINT_NULL (param->value));
					param = param->next;
				}
			}
			
			printf ("\n");
		}
		
		if (part->encoding) {
			for (i = 0; i < depth; i++)
				printf ("  ");
			printf ("Content-Transfer-Encoding: %s\n", PRINT_NULL (part->encoding));
		}

		if (part->description) {
			for (i = 0; i < depth; i++)
				printf ("  ");
			printf ("Description: %s\n", PRINT_NULL (part->description));
		}

		for (i = 0; i < depth; i++)
			printf ("  ");
		printf ("Octets: %d, Lines: %d\n", part->octets, part->lines);

		if (part->content.lang) {
			for (i = 0; i < depth; i++)
				printf ("  ");
			printf ("Language: %s\n", PRINT_NULL (part->content.lang));
		}

		if (part->content.loc) {
			for (i = 0; i < depth; i++)
				printf ("  ");
			printf ("Location: %s\n", PRINT_NULL (part->content.loc));
		}

		if (part->content.cid) {
			for (i = 0; i < depth; i++)
				printf ("  ");
			printf ("Cid: %s\n", PRINT_NULL (part->content.cid));
		}

		if (part->content.md5) {
			for (i = 0; i < depth; i++)
				printf ("  ");
			printf ("MD5: %s\n", PRINT_NULL (part->content.md5));
		}
	}


	printf ("\n");
}

void
bodystruct_dump (bodystruct_t *part)
{
	bodystruct_dump_r (part, 0);
}
#endif

void
bodystruct_free (bodystruct_t *node)
{
	struct _bodystruct *next;

	while (node != NULL) {
		g_free (node->content.type);
		g_free (node->content.subtype);
		g_free (node->content.lang);
		g_free (node->content.loc);
		g_free (node->content.cid);
		g_free (node->content.md5);

		if (node->content.params)
			mimeparam_destroy (node->content.params);
		
		g_free (node->disposition.type);
		if (node->disposition.params)
			mimeparam_destroy (node->disposition.params);
		
		g_free (node->encoding);
		g_free (node->description);

		if (node->envelope)
			envelope_free (node->envelope);

		if (node->subparts)
			bodystruct_free (node->subparts);

		g_free (node->part_spec);

		/* leave node->parent, recursiveness will take care of it */

		next = node->next;
		g_slice_free (struct _bodystruct, node);
		node = next;
	}
}


bodystruct_t *
bodystruct_parse (guchar *inbuf, guint inlen, GError **err)
{
	unsigned char *start = (unsigned char  *) strstr ((const char *) inbuf, "BODYSTRUCTURE");
	int lendif;
	bodystruct_t *r = NULL;

	if (!start) {
		r = bodystruct_part_decode (&inbuf, inbuf + inlen, NULL, 1, err);
	} else {

		start += 13;
		lendif = (int) start - (int) inbuf;
		
		r = bodystruct_part_decode (&start, (unsigned char *) ( start + (inlen - lendif) ), NULL, 1, err);
	}
	if (!r->part_spec)
		r->part_spec = g_strdup ("");
	return r;
}

envelope_t*
envelope_parse (guchar *inbuf, guint inlen, GError **err)
{
	unsigned char *start = (unsigned char  *) strstr ((const char *) inbuf, "ENVELOPE");
	int lendif;

	if (!start)
		return decode_envelope (&inbuf, inbuf + inlen, err);

	start += 8;
	lendif = (int) start - (int) inbuf;

	return decode_envelope (&start, (unsigned char *) ( start + (inlen - lendif) ), err);
}



