#ifndef CHECK_LIBTINYMAIL_H
#define CHECK_LIBTINYMAIL_H

#include <string.h>		/* Used by most unit test suites */
#include <glib.h>
#include <check.h>

Suite *create_tny_account_store_suite (void);
Suite *create_tny_account_suite (void);
Suite *create_tny_device_suite (void);
Suite *create_tny_folder_store_query_suite (void);
Suite *create_tny_folder_store_suite (void);
Suite *create_tny_folder_suite (void);
Suite *create_tny_header_suite (void);
Suite *create_tny_list_suite (void);
Suite *create_tny_mime_part_suite (void);
Suite *create_tny_msg_suite (void);
Suite *create_tny_stream_suite (void);

#endif /* CHECK_LIBTINYMAIL_H */
