#ifndef _SUMMARY_H
#define _SUMMARY_H

typedef struct _Summary Summary;
typedef struct _SummaryItem SummaryItem;


SummaryItem* summary_get_item_by_seq (Summary *s, int seq);
SummaryItem* summary_get_item_by_uid (Summary *s, const char *uid);
void summary_item_unref (SummaryItem *i);
SummaryItem* summary_item_ref (SummaryItem *i);

const char* summary_item_get_uid (SummaryItem *i);
const char* summary_item_get_subject (SummaryItem *i);
const char* summary_item_get_from (SummaryItem *i);
const char* summary_item_get_to (SummaryItem *i);
const char* summary_item_get_cc (SummaryItem *i);

int summary_item_get_seq (SummaryItem *i);
int summary_item_get_flags (SummaryItem *i);
void summary_item_set_flags (SummaryItem *i, int new_flags);

SummaryItem* summary_create_item (const char *uid, int seq, int flags, size_t size, const char *from, const char *subject, const char *to, const char *cc);
void summary_add_item (Summary *s, SummaryItem *i);

void summary_free (Summary *summary);
Summary* summary_open (const char *folder_path);

void summary_freeze (Summary *s);
void summary_thaw (Summary *s);

void summary_expunge_item_by_uid (Summary *s, const char *uid);
void summary_expunge_item_by_seq (Summary *s, int seq);


#endif
