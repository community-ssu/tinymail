#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "summary.h"

typedef struct _SummaryBlock SummaryBlock;

static inline void summary_block_kill (SummaryBlock *b);
static inline void summary_block_persist (SummaryBlock *b);

/* The size of a pointer in this documentation assumes a 32bit architecture. On
 * your fancy 64bit computer will each pointer be larger! size_t and off_t will
 * also be different! Recount to know the memory consumption on such archs. */

struct _SummaryItem {

	/* We don't use all 32 bits of the flags,perhaps we can make it 16 bits
	 * too. although right now I don't have another field to pad the data 
	 * with anyway. So I just keep it at 32 bits. */
	guint32 flags; /* 4 bytes */

	/* Sizes can be quite large, keep it 32 bits */
	size_t size; /* 4 bytes */

	/* A 16 bit sequence would mean maximum 65536 items: not enough, so 
	 * let's stick that to 32 bit in stead. */
	guint32 seq; /* 4 bytes */

	/* padding trick { */

		/* Blocks will have a high ref_count, but items wont, therefore are
		 * 16 bits fine for this field */
		gint16 ref_count;

		/* This is just a bool, a bitfield doesn't help, so I just made it 
		 * 16 bits too. Together with the ref_count, it forms one word, saving
		 * us 4 bytes per item. */
		gint16 mem;

	/* } together: 32 bits, 4 bytes */


	/* Let's now put all the pointers together, they'll each consume four 
	 * bytes (we can't do a lot of padding optimizations here anyway) */

	char *uid; /* 4 bytes */
	SummaryBlock *block; /* 4 bytes */

	union {
		struct {
			off_t from_offset;
			off_t subject_offset;
			off_t to_offset;
			off_t cc_offset;
		} offsets;
		struct {
			char *from;
			char *subject;
			char *to;
			char *cc;
		} pointers;
	} mmappable; /* 16 bytes */

}; /* Frequence of this type is significant to care about its size: 40 bytes per
    * item (plus heap admin, which is relatively small with gslice). Note that 
    * the VmRSS will also suffer from pages in the mapping that are needed! */

   /* Also important for the size are the hashtable entries in uids and seqs.
    * each such GHashNode is at least 16 bytes. Multiplied by two hashtables
    * that's 32 extra bytes. On top of that we store the uid twice in memory:
    * once as key and once in the item. ~12 bytes x 2= 24 bytes.*/

   /* So 40+32+24+mapping */
 
   /* Everything added I estimate that each item will consume between 100 and
    * 200 bytes in VmRSS (around 4 MB for 20,000 items) (TODO: accurately 
    * measure this) */

struct _SummaryBlock {
	int ref_count;
	char *data;
	FILE *data_file;
	int mmap_size;
	int block_id;
	GHashTable *uids;
	GHashTable *seqs;
	GStaticRecMutex *lock;
	gboolean frozen;
	gboolean has_expunges, 
		 has_flagchg, 
		 has_appends;

}; /* Frequence of this type is not significant enough to care about its size */

struct _Summary {
	GPtrArray *blocks;
	GStaticRecMutex *lock;
}; /* Frequence of this type is one per folder: don't care about its size*/


#if GLIB_CHECK_VERSION (2,14,0)
	/* Nothing */
#else
inline static void
list_values (gpointer key, gpointer value, GList **values)
{
	*values = g_list_append (*values, value);
}

inline static GList*
g_hash_table_get_values (GHashTable *hash)
{
	GList *values = NULL;
	g_hash_table_foreach (hash, (GHFunc)list_values, &values);
	return values;
}
#endif


static inline char*
summary_block_index_filename (SummaryBlock *b)
{
	return g_strdup_printf ("index_%d.idx", b->block_id);
}

static inline char*
summary_block_flags_filename (SummaryBlock *b)
{
	return g_strdup_printf ("flags_%d.idx", b->block_id);
}

static inline char*
summary_block_data_filename (SummaryBlock *b)
{
	return g_strdup_printf ("data_%d.mmap", b->block_id);
}

static inline void 
prepare_summary_block (SummaryBlock *b)
{
	b->uids = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, NULL);
	b->seqs = g_hash_table_new_full (g_int_hash, g_int_equal, NULL, NULL);
}

static inline SummaryBlock*
create_new_summary_block (int block_id)
{
	SummaryBlock *b = g_slice_new0 (SummaryBlock);
	b->lock = g_new0 (GStaticRecMutex, 1);
	g_static_rec_mutex_init (b->lock);

	g_static_rec_mutex_lock (b->lock);
	b->block_id = block_id;
	b->has_expunges = FALSE;
	b->has_appends = FALSE;
	b->has_flagchg = FALSE;
	prepare_summary_block (b);
	g_static_rec_mutex_unlock (b->lock);

	return b;
}


static inline void 
summary_block_add_item_int (SummaryBlock *b, SummaryItem *i)
{
	g_static_rec_mutex_lock (b->lock);
	i->block = b;
	b->ref_count++;
	g_hash_table_insert (b->uids, g_strdup (i->uid), i);
	g_hash_table_insert (b->seqs, &i->seq, i);
	g_static_rec_mutex_unlock (b->lock);
}

static inline void 
summary_block_add_item (SummaryBlock *b, SummaryItem *i)
{
	g_static_rec_mutex_lock (b->lock);

	if (i->block && i->block != b) {
		g_static_rec_mutex_lock (i->block->lock);
		i->block->has_expunges = TRUE;
		g_hash_table_remove (i->block->uids, i->uid);
		g_hash_table_remove (i->block->seqs, &i->seq);
		i->block->ref_count--;
		if (i->block->ref_count <= 0)
			summary_block_kill (i->block);
		g_static_rec_mutex_unlock (i->block->lock);
	}

	i->block = b;
	b->ref_count++;
	b->has_appends = TRUE;
	/* TODO: insert extended and remove original if dup */
	g_hash_table_insert (b->uids, g_strdup (i->uid), i);
	g_hash_table_insert (b->seqs, &i->seq, i);
	g_static_rec_mutex_unlock (b->lock);
}

void
summary_add_item (Summary *s, SummaryItem *i)
{
	int block_id = 0; //i->seq % 1000;
	SummaryBlock *b;

	g_static_rec_mutex_lock (s->lock);

	if (s->blocks->len < block_id+1) {
		int t;
		for (t = s->blocks->len; t < block_id+1; t++) {
			SummaryBlock *mnew = create_new_summary_block (t);
			g_ptr_array_add (s->blocks, mnew);
		}
	}

	b = (SummaryBlock *) s->blocks->pdata[block_id];
	summary_block_add_item (b, i);

	g_static_rec_mutex_unlock (s->lock);

}

static inline SummaryItem*
summary_block_get_item_by_seq (SummaryBlock *b, int seq)
{
	return (SummaryItem *) g_hash_table_lookup (b->seqs, &seq);
}


SummaryItem*
summary_get_item_by_seq (Summary *s, int seq)
{
	int i;
	SummaryItem *item = NULL;

	g_static_rec_mutex_lock (s->lock);

	for (i = 0; i < s->blocks->len; i++) {
		SummaryBlock *b = (SummaryBlock *) s->blocks->pdata[i];
		item = summary_block_get_item_by_seq (b, seq);
		if (item) {
			item->ref_count++;
			item->block->ref_count++;
			break;
		}
	}

	g_static_rec_mutex_unlock (s->lock);

	return item;
}


static inline SummaryItem*
summary_block_get_item_by_uid (SummaryBlock *b, const char *uid)
{
	return (SummaryItem *) g_hash_table_lookup (b->uids, uid);
}

SummaryItem*
summary_get_item_by_uid (Summary *s, const char *uid)
{
	int i;
	SummaryItem *item = NULL;

	g_static_rec_mutex_lock (s->lock);

	for (i = 0; i < s->blocks->len; i++) {
		SummaryBlock *b = (SummaryBlock *) s->blocks->pdata[i];
		item = summary_block_get_item_by_uid (b, uid);
		if (item) {
			item->ref_count++;
			item->block->ref_count++;
			break;
		}
	}

	g_static_rec_mutex_unlock (s->lock);

	return item;
}

static inline void 
summary_expunge_item_shared (SummaryBlock *block, SummaryItem *i)
{
	GList *values; gint seq = i->seq;
	block->has_expunges = TRUE;
	g_hash_table_remove (block->uids, i->uid);
	g_hash_table_remove (block->seqs, &i->seq);
	values = g_hash_table_get_values (block->uids);
	while (values) {
		SummaryItem *item = values->data;
		if (item->seq > seq) {
			g_hash_table_remove (block->seqs, &item->seq);
			item->seq--;
			g_hash_table_insert (block->seqs, &item->seq, item);
		}
		values = g_list_next (values);
	}
	g_list_free (values);
	summary_block_persist (block);
}

void 
summary_expunge_item_by_uid (Summary *s, const char *uid)
{
	int i;
	SummaryItem *item = NULL;

	g_static_rec_mutex_lock (s->lock);

	for (i = 0; i < s->blocks->len; i++) {
		SummaryBlock *b = (SummaryBlock *) s->blocks->pdata[i];
		item = summary_block_get_item_by_uid (b, uid);
		if (item) {
			summary_expunge_item_shared (b, item);
			break;
		}
	}
	g_static_rec_mutex_unlock (s->lock);
	return;
}

void 
summary_expunge_item_by_seq (Summary *s, int seq)
{
	int i;
	SummaryItem *item = NULL;

	g_static_rec_mutex_lock (s->lock);

	for (i = 0; i < s->blocks->len; i++) {
		SummaryBlock *b = (SummaryBlock *) s->blocks->pdata[i];
		item = summary_block_get_item_by_seq (b, seq);
		if (item) {
			summary_expunge_item_shared (b, item);
			break;
		}
	}

	g_static_rec_mutex_unlock (s->lock);

	return;
}

static inline SummaryBlock *
read_summary_block (int block_id, SummaryBlock *b_in)
{
	FILE *idx, *flags;
	SummaryBlock *b;
	struct stat buf;
	char *index_filename;
	char *data_filename;
	char *flags_filename;

	if (b_in)
		b = b_in;
	else {
		b = g_slice_new0 (SummaryBlock);
		b->lock = g_new0 (GStaticRecMutex, 1);
		prepare_summary_block (b);
		g_static_rec_mutex_init (b->lock);
	}

	g_static_rec_mutex_lock (b->lock);

	b->block_id = block_id;

	index_filename = summary_block_index_filename (b);

	idx = fopen (index_filename, "r");

	if (!idx) {
		g_static_rec_mutex_unlock (b->lock);
		return b;
	}

	data_filename = summary_block_data_filename (b);

	if (b->data_file)
		fclose (b->data_file);
	if (b->data)
		munmap (b->data, b->mmap_size);

	b->data_file = fopen (data_filename, "r");

	if (!b->data_file) {
		fclose (idx);
		g_free (index_filename);
		g_free (data_filename);
		g_static_rec_mutex_unlock (b->lock);
		return b;
	}

	flags_filename = summary_block_flags_filename (b);
	flags = fopen (flags_filename, "r");
	if (!flags) {
		fclose (idx);
		fclose (b->data_file);
		b->data_file = NULL;
		g_free (index_filename);
		g_free (data_filename);
		g_free (flags_filename);
		g_static_rec_mutex_unlock (b->lock);
		return b;
	}

	g_free (index_filename);
	g_free (data_filename);
	g_free (flags_filename);

	fstat(fileno (b->data_file), &buf);
	b->mmap_size = buf.st_size;
	b->data = mmap (NULL, b->mmap_size, PROT_READ, MAP_PRIVATE, fileno (b->data_file), 0);

	while (!feof (idx)) {
		int len;
		if (fscanf (idx, "%d", &len) != EOF) 
		{
			SummaryItem *cur;
			char *uid = (char *) malloc(len + 1);
			gboolean do_add = FALSE;

			memset (uid, 0, len);
			fgetc (idx);
			fread (uid, 1, len, idx);
			uid[len] = '\0';

			cur = summary_block_get_item_by_uid (b, uid);
			if (!cur) {
				do_add = TRUE;
				cur = g_slice_new0 (SummaryItem);
				cur->ref_count = 1;
			}
			cur->uid = uid;
			cur->block = b;

			if (cur->mem == 1) {
				g_free (cur->mmappable.pointers.from);
				g_free (cur->mmappable.pointers.subject);
				g_free (cur->mmappable.pointers.to);
				g_free (cur->mmappable.pointers.cc);
			}
			cur->mem = 0;

			fscanf (idx, "%d%d%d%d%d%d",
				&cur->seq, &cur->size,
				(int *) &cur->mmappable.offsets.subject_offset, 
				(int *) &cur->mmappable.offsets.from_offset,
				(int *) &cur->mmappable.offsets.to_offset,
				(int *) &cur->mmappable.offsets.cc_offset);

			if (do_add)
				summary_block_add_item_int (b, cur);
		}
	}
	fclose (idx);

	/* Perhaps scan this in a table and loop the table in above loop in stead */
	while (!feof (flags)) {
		int seq, flag;
		if (fscanf (flags, "%d%d", &seq, &flag) != EOF) {
			SummaryItem *c = summary_block_get_item_by_seq (b, seq);
			if (c)
				c->flags = flag;
		}
	}
	fclose(flags);

	b->has_flagchg = FALSE;
	b->has_expunges = FALSE;
	b->has_appends = FALSE;

	g_static_rec_mutex_unlock (b->lock);

	return b;
}

const char*
summary_item_get_uid (SummaryItem *i)
{
	return i->uid;
}

int
summary_item_get_seq (SummaryItem *i)
{
	return (const int) i->seq;
}

int
summary_item_get_flags (SummaryItem *i)
{
	return (const int) i->flags;
}

void
summary_item_set_flags (SummaryItem *i, int new_flags)
{
	i->flags = new_flags;

	if (G_LIKELY (i->block)) {
		i->block->has_flagchg = TRUE;
		summary_block_persist (i->block);
	}

	return;
}


const char*
summary_item_get_subject (SummaryItem *i)
{
	const char* retval = NULL;

	if (i->block)
		g_static_rec_mutex_lock (i->block->lock);
	if (i->mem == 1)
		retval = i->mmappable.pointers.subject;
	else if (i->block)
		retval = i->block->data + i->mmappable.offsets.subject_offset;
	if (i->block)
		g_static_rec_mutex_unlock (i->block->lock);

	return retval;
}

const char*
summary_item_get_from (SummaryItem *i)
{
	const char *retval = NULL;

	if (i->block)
		g_static_rec_mutex_lock (i->block->lock);
	if (i->mem == 1)
		retval = i->mmappable.pointers.from;
	else if (i->block)
		retval = i->block->data + i->mmappable.offsets.from_offset;
	if (i->block)
		g_static_rec_mutex_unlock (i->block->lock);

	return retval;
}


const char*
summary_item_get_to (SummaryItem *i)
{
	const char *retval = NULL;

	if (i->block)
		g_static_rec_mutex_lock (i->block->lock);
	if (i->mem == 1)
		retval = i->mmappable.pointers.to;
	else if (i->block)
		retval = i->block->data + i->mmappable.offsets.to_offset;
	if (i->block)
		g_static_rec_mutex_unlock (i->block->lock);

	return retval;
}

const char*
summary_item_get_cc (SummaryItem *i)
{
	const char *retval = NULL;

	if (i->block)
		g_static_rec_mutex_lock (i->block->lock);
	if (i->mem == 1)
		retval = i->mmappable.pointers.cc;
 	else if (i->block)
		retval = i->block->data + i->mmappable.offsets.cc_offset;
	if (i->block)
		g_static_rec_mutex_unlock (i->block->lock);

	return retval;
}

static inline void
summary_block_kill (SummaryBlock *b)
{
	summary_block_persist (b);

	if (b->data)
		munmap (b->data, b->mmap_size);
	if (b->data_file)
		fclose (b->data_file);

	g_hash_table_destroy (b->seqs);
	g_hash_table_destroy (b->uids);
	b->seqs = NULL;
	b->uids = NULL;

	g_free (b->lock);
	g_slice_free (SummaryBlock, b);

}

SummaryItem* 
summary_item_ref (SummaryItem *i)
{
	i->ref_count++;

	if (i->block) {
		g_static_rec_mutex_lock (i->block->lock);
		i->block->ref_count++;
		g_static_rec_mutex_unlock (i->block->lock);
	}

	return i;
}

void
summary_item_unref (SummaryItem *i)
{
	i->ref_count--;

	if (i->block) {
		g_static_rec_mutex_lock (i->block->lock);
		i->block->ref_count--;
		if (i->block->ref_count <= 0)
			summary_block_kill (i->block);
		g_static_rec_mutex_unlock (i->block->lock);
	}

	if (i->ref_count <= 0) 
	{
		g_free (i->uid);
		if (i->mem == 1) {
			g_free (i->mmappable.pointers.from);
			g_free (i->mmappable.pointers.subject);
			g_free (i->mmappable.pointers.to);
			g_free (i->mmappable.pointers.cc);
			i->mem = 0;
		}
		g_slice_free (SummaryItem, i);
	}
}


typedef struct {
	char *str;
	int cnt, cpy;
	GList *affected;
} StoreString;


static gint 
cmp_storestring (gconstpointer a, gconstpointer b)
{
	StoreString *sa = (StoreString *) a;
	StoreString *sb = (StoreString *) b;
	return sb->cnt - sa->cnt;
}

static void 
summary_block_write_flags (SummaryBlock *b)
{
	GList *items = NULL;
	char *flags_filename;
	char *flags_filename_n;
	FILE *flags;

	g_static_rec_mutex_lock (b->lock);

	items = g_hash_table_get_values (b->seqs);
	flags_filename = summary_block_flags_filename (b);
	flags_filename_n = g_strdup_printf ("%s~", flags_filename);

	flags = fopen (flags_filename_n, "w");

	if (!flags) {
		g_free (flags_filename);
		g_free (flags_filename_n);
		g_list_free (items);
		/* TODO: Error reporting */
		g_static_rec_mutex_unlock (b->lock);
		return;
	}

	while (items) {
		SummaryItem *item = (SummaryItem *) items->data;
		fprintf (flags, "%d %d\n", item->seq, item->flags); 
		items = g_list_next (items);
	}

	g_list_free (items);

	fclose (flags);

	rename (flags_filename_n, flags_filename);

	b->has_flagchg = FALSE;

	g_free (flags_filename);
	g_free (flags_filename_n);

	g_static_rec_mutex_unlock (b->lock);

}

static void
summary_block_write_all (SummaryBlock *b)
{
	FILE *data, *idx, *flags;
	GList *orig = NULL, *items = NULL;
	GHashTable *strings = NULL;
	GList *sstrings;

	char *index_filename;
	char *data_filename;
	char *flags_filename;
	char *index_filename_n;
	char *data_filename_n;
	char *flags_filename_n;

	g_static_rec_mutex_lock (b->lock);

	orig = g_hash_table_get_values (b->seqs);
	items = orig;
	strings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

	index_filename = summary_block_index_filename (b);
	data_filename = summary_block_data_filename (b);
	flags_filename = summary_block_flags_filename (b);

	index_filename_n = g_strdup_printf ("%s~", index_filename);
	data_filename_n = g_strdup_printf ("%s~", data_filename);
	flags_filename_n = g_strdup_printf ("%s~", flags_filename);

	idx = fopen (index_filename_n, "w");
	data = fopen (data_filename_n, "w");
	flags = fopen (flags_filename_n, "w");

	if (!idx || !data || !flags) {
		/* TODO: Error reporting */
		if (idx) fclose (idx);
		if (flags) fclose (flags);
		if (data) fclose (data);
		g_free (index_filename);
		g_free (data_filename);
		g_free (flags_filename);
		g_free (index_filename_n);
		g_free (data_filename_n);
		g_free (flags_filename_n);
		g_hash_table_destroy (strings);
		g_static_rec_mutex_unlock (b->lock);
		return;
	}

	while (items) {
		SummaryItem *item = (SummaryItem *) items->data;

		StoreString *sb;
		char *str;

		/* subject */
		if (!item->mem)
			str = item->block->data + item->mmappable.offsets.subject_offset;
		else 
			str = item->mmappable.pointers.subject;

		sb =  g_hash_table_lookup (strings, str);
		if (sb) {
			sb->cnt++;
			sb->cpy = FALSE;
		} else {
			sb = g_slice_new0 (StoreString);
			sb->cnt = 1;
			sb->str = item->mem ? g_strdup (item->mmappable.pointers.subject) : str;
			sb->cpy = item->mem;
			g_hash_table_insert (strings, g_strdup (str), sb);
		}
		sb->affected = g_list_prepend (sb->affected, &item->mmappable.offsets.subject_offset);

		/* from */
		if (!item->mem)
			str = item->block->data + item->mmappable.offsets.from_offset;
		else 
			str = item->mmappable.pointers.from;

		sb = g_hash_table_lookup (strings, str);
		if (sb) {
			sb->cnt++;
			sb->cpy = FALSE;
		} else {
			sb = g_slice_new0 (StoreString);
			sb->cnt = 1;
			sb->str = item->mem ? g_strdup (item->mmappable.pointers.from) : str;
			sb->cpy = item->mem;
			g_hash_table_insert (strings, g_strdup (str), sb);
		}
		sb->affected = g_list_prepend (sb->affected, &item->mmappable.offsets.from_offset);


		/* to */
		if (!item->mem)
			str = item->block->data + item->mmappable.offsets.to_offset;
		else
			str = item->mmappable.pointers.to;

		sb = g_hash_table_lookup (strings, str);
		if (sb) {
			sb->cnt++;
			sb->cpy = FALSE;
		} else {
			sb = g_slice_new0 (StoreString);
			sb->cnt = 1;
			sb->str = item->mem ? g_strdup (item->mmappable.pointers.to) : str;
			sb->cpy = item->mem;
			g_hash_table_insert (strings, g_strdup (str), sb);
		}
		sb->affected = g_list_prepend (sb->affected, &item->mmappable.offsets.to_offset);


		/* cc */
		if (!item->mem)
			str = item->block->data + item->mmappable.offsets.cc_offset;
		else
			str = item->mmappable.pointers.cc;

		sb = g_hash_table_lookup (strings, str);
		if (sb) {
			sb->cnt++;
			sb->cpy = FALSE;
		} else {
			sb = g_slice_new0 (StoreString);
			sb->cnt = 1;
			sb->str = item->mem ? g_strdup (item->mmappable.pointers.cc) : str;
			sb->cpy = item->mem;
			g_hash_table_insert (strings, g_strdup (str), sb);
		}
		sb->affected = g_list_prepend (sb->affected, &item->mmappable.offsets.cc_offset);

		if (item->mem == 1) {
			g_free (item->mmappable.pointers.from);
			g_free (item->mmappable.pointers.subject);
			g_free (item->mmappable.pointers.to);
			g_free (item->mmappable.pointers.cc);
			item->mem = 0;
		}

		items = g_list_next (items);
	}

	sstrings = g_hash_table_get_values (strings);
	sstrings = g_list_sort (sstrings, cmp_storestring);

	while (sstrings) {
		StoreString *ss = (StoreString *) sstrings->data;
		int location = ftell (data);

		fputs (ss->str, data);
		if (ss->cpy) {
			g_free (ss->str);
			ss->cpy = FALSE;
		}
		fputc ('\0', data);

		while (ss->affected) {
			int *data = ss->affected->data;
			*data = location;
			ss->affected = g_list_next (ss->affected);
		}
		g_list_free (ss->affected);
		g_slice_free (StoreString, ss);
		sstrings = g_list_next (sstrings);
	}

	g_list_free (sstrings);
	g_hash_table_destroy (strings);

	fclose (data);

	items = orig;
	while (items) {
		SummaryItem *item = (SummaryItem *) items->data;

		fprintf (flags, "%d %d\n", item->seq, item->flags); 
		fprintf (idx, "%d %s %d %d %d %d %d %d\n",
				strlen (item->uid), item->uid,
				item->seq, item->size,
				(int) item->mmappable.offsets.subject_offset, 
				(int) item->mmappable.offsets.from_offset,
				(int) item->mmappable.offsets.to_offset,
				(int) item->mmappable.offsets.cc_offset);

		items = g_list_next (items);
	}


	g_list_free (orig);

	fclose (idx);
	fclose (flags);

	if (b->data)
		munmap (b->data, b->mmap_size);
	if (b->data_file)
		fclose (b->data_file);

	b->data = NULL;
	b->data_file = NULL;

	rename (index_filename_n, index_filename);
	rename (data_filename_n, data_filename);
	rename (flags_filename_n, flags_filename);

	/* Reread it (this copes with existing items too) */
	read_summary_block (b->block_id, b);

	g_free (index_filename);
	g_free (data_filename);
	g_free (flags_filename);
	g_free (index_filename_n);
	g_free (data_filename_n);
	g_free (flags_filename_n);

	g_static_rec_mutex_unlock (b->lock);

	return;
}


static inline void 
summary_block_persist (SummaryBlock *b)
{
	g_static_rec_mutex_lock (b->lock);
	if (G_UNLIKELY (!b->frozen)) {
		if (b->has_flagchg && !b->has_appends && !b->has_expunges)
			summary_block_write_flags (b);
		else if (b->has_appends || b->has_expunges)
			summary_block_write_all (b);
	}
	g_static_rec_mutex_unlock (b->lock);
}

void
summary_freeze (Summary *s)
{
	gint i;

	g_static_rec_mutex_lock (s->lock);
	for (i = 0; i < s->blocks->len; i++) {
		SummaryBlock *b = (SummaryBlock *) s->blocks->pdata[i];
		g_static_rec_mutex_lock (b->lock);
		b->frozen = TRUE;
		g_static_rec_mutex_unlock (b->lock);
	}
	g_static_rec_mutex_unlock (s->lock);
}

void
summary_thaw (Summary *s)
{
	gint i;

	g_static_rec_mutex_lock (s->lock);
	for (i = 0; i < s->blocks->len; i++) {
		SummaryBlock *b = (SummaryBlock *) s->blocks->pdata[i];
		g_static_rec_mutex_lock (b->lock);
		b->frozen = FALSE;
		summary_block_persist (b);
		g_static_rec_mutex_unlock (b->lock);
	}
	g_static_rec_mutex_unlock (s->lock);
}

#ifdef DONT
static void
summary_dump (Summary *summary)
{
	int i;

	g_static_rec_mutex_lock (summary->lock);

	for (i = 0; i < summary->blocks->len; i++) {
		SummaryBlock *b = (SummaryBlock *) summary->blocks->pdata[i];
		summary_block_persist (b);
	}

	g_static_rec_mutex_unlock (summary->lock);

	return;
}
#endif

static inline void
foreach_item_free (gpointer key, gpointer value, gpointer user_data)
{
	SummaryItem *i = (SummaryItem *) value;
	summary_item_unref (i);
}


static inline void
summary_block_free (SummaryBlock *b)
{
	g_hash_table_foreach (b->seqs, foreach_item_free, NULL);
}

void
summary_free (Summary *summary)
{
	int i;

	g_static_rec_mutex_lock (summary->lock);

	for (i = 0; i < summary->blocks->len; i++) {
		SummaryBlock *b = (SummaryBlock *) summary->blocks->pdata[i];

		g_static_rec_mutex_lock (b->lock);
		summary_block_free (b);
		b->ref_count--;
		if (b->ref_count <= 0)
			summary_block_kill (b);
		g_static_rec_mutex_unlock (b->lock);
	}
	g_ptr_array_free (summary->blocks, TRUE);

	g_static_rec_mutex_unlock (summary->lock);

	g_free (summary->lock);
	g_slice_free (Summary, summary);

	return;
}


Summary*
summary_open (const char *folder_path)
{
	Summary *summary = g_slice_new0 (Summary);
	summary->lock = g_new0 (GStaticRecMutex, 1);
	int i, items;
	char *filename = g_strdup_printf ("%s/summary.idx", folder_path);
	FILE *f = fopen (filename, "r");
	g_free (filename);

	g_static_rec_mutex_init (summary->lock);

	if (f) {
		fscanf (f, "%d", &items);
		fclose (f);
	} else 
		items = 0;

	g_static_rec_mutex_lock (summary->lock);

	summary->blocks = g_ptr_array_sized_new (items);

	for (i=0; i < items; i++) {
		SummaryBlock *b = read_summary_block (i, NULL);
		g_ptr_array_add (summary->blocks, b);
		b->ref_count++;
	}

	g_static_rec_mutex_unlock (summary->lock);

	return summary;
}

static inline int
cmpstringp(const void *p1, const void *p2)
{
        return strcmp(* (char * const *) p1, * (char * const *) p2);
}


static GPtrArray*
split_recipients (gchar *buffer)
{
        gchar *tmp, *start;
        gboolean is_quoted = FALSE;
        GPtrArray *array = g_ptr_array_new ();

        start = tmp = buffer;

        while (*tmp != '\0') {
                if (*tmp == '\"')
                        is_quoted = !is_quoted;
                if (*tmp == '\\')
                        tmp++;
                if ((!is_quoted) && ((*tmp == ',') || (*tmp == ';'))) {
                        gchar *part;
                        part = g_strndup (start, tmp - start);
                        part = g_strstrip (part);
                        g_ptr_array_add (array, part);
                        start = tmp+1;
                }

                tmp++;
        }

        if (start != tmp)
                g_ptr_array_add (array, g_strstrip (g_strdup (start)));

        return array;
}


/* In reality will this be done by the application in stead (probably by the ENVELOPE parser!) */

static gchar *
sort_address_list (const gchar *unsorted)
{
	GPtrArray *a = split_recipients ((gchar*)unsorted);
	gint i; GString *result = g_string_new ("");
	gchar *item;

	g_ptr_array_sort (a, (GCompareFunc) cmpstringp);

	for (i=0; i < a->len; i++) {
		if (i > 0) {
			g_string_append_c (result, ',');
			g_string_append_c (result, ' ');
		}
		g_string_append (result, a->pdata[i]);
		g_free (a->pdata[i]);
	}

	g_ptr_array_free (a, TRUE);
	item = result->str;
	g_string_free (result, FALSE);

	return item;
}

SummaryItem*
summary_create_item (const char *uid, int seq, int flags, size_t size, const char *from, const char *subject, const char *to, const char *cc)
{
	SummaryItem *item = g_slice_new0 (SummaryItem);

	item->ref_count = 1;
	item->size = size;
	item->seq = seq;
	item->flags = flags;
	item->uid = g_strdup (uid);
	item->mmappable.pointers.from = g_strdup (from);
	item->mmappable.pointers.to = sort_address_list (to);
	item->mmappable.pointers.cc = sort_address_list (cc);
	item->mmappable.pointers.subject = g_strdup (subject);
	item->block = NULL;
	item->mem = 1; /* memory item */

	return item;
}



#ifdef DONT
static void 
foreach_item_print_it (gpointer key, gpointer value, gpointer user_data)
{
	SummaryItem *mc = (SummaryItem *) value;

	printf ("UID %s is at sequence %d with flags %d\n", 
		summary_item_get_uid (mc),
		summary_item_get_seq (mc),
		summary_item_get_flags (mc));

	printf ("\tFrom=%s\n\tSubj=%s\n\tCc=%s\n\tTo=%s\n",
		summary_item_get_from (mc),
		summary_item_get_subject (mc),
		summary_item_get_cc (mc),
		summary_item_get_to (mc));
}

static void 
print_summary (Summary *summary)
{
	int i;

	for (i = 0; i < summary->blocks->len; i++) {
		SummaryBlock *b = (SummaryBlock *) summary->blocks->pdata[i];
		g_hash_table_foreach (b->seqs, foreach_item_print_it, NULL);
	}
}


int 
main (int argc, char **argv)
{
	Summary *summary = summary_open (".");
	int i, y = 0;
	GList *vals = NULL;

	/* printf ("sizeof (SummaryItem): %d\n", sizeof (SummaryItem)); */

	/* print_summary (summary); */

	for (i = 0; i < summary->blocks->len; i++) {
		gint z = 0;
		SummaryBlock *b = (SummaryBlock *) summary->blocks->pdata[i];
		GList *l = g_hash_table_get_values (b->seqs);

		while (l) {
			if (z % 2) {
				SummaryItem *i = summary_item_ref (l->data);
				vals = g_list_prepend (vals, i);
			}
			l = g_list_next (l); 
			z++;
		}
		g_list_free (l);
	}

	if (argc > 1) {


		for (i = 0; i < summary->blocks->len; i++) {
			SummaryBlock *b = (SummaryBlock *) summary->blocks->pdata[i];
			y += g_hash_table_size (b->seqs);
		}

		for (i = y; i < y + 50000; i++) {
			const char *uid = g_strdup_printf ("%08d", i);
			int seq = 10+i;
			int flags = i+1;
			size_t size = i+2;
			char *from = g_strdup_printf ("Jan%dtje <jantje@gmail.com>",i);
			const char *subject = g_strdup_printf ("Ik ga naar %dde bakker he Pietje!",i);
			const char *to = g_strdup_printf ("Gr%dietje <grietje@gmail.com>, Ha%dnsje <hansje@gmail.com>, Pietje <pi%detje@gmail.com>",i,i,i);
			const char *cc = g_strdup_printf ("Pietje <p%dietje@gmail.com>, Hansje <hansj%de@gmail.com>, Grietje <gr%dietje@gmail.com>", i,i,i);

			SummaryItem *item = summary_create_item (uid, seq, flags, size, from, subject, to, cc);
			summary_add_item (summary, item);
		}

		summary_dump (summary);

		printf ("\n\n\n\n");

		print_summary (summary);

	}

if (FALSE) {
	printf ("\n\n---\n\n");
	SummaryItem *item1 = summary_get_item_by_seq (summary, 22);
	printf ("%s\n", summary_item_get_from (item1));
	summary_expunge_item_by_seq (summary, 22);
	item1 = summary_get_item_by_seq (summary, 22);
	if (item1) {
		printf ("%s\n", summary_item_get_from (item1));
	}
	getchar();
}

	summary_free (summary);

getchar();

	/* Print the odd ones */
	while (vals) {
		SummaryItem *i = vals->data;

		foreach_item_print_it (&i->seq, i, NULL);

		summary_item_unref (i);
		vals = g_list_next (vals);
	}
	g_list_free (vals);

	printf ("The blocks should be freed now\n");

	return 0;
}

#endif

