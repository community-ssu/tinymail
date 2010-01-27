/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Copyright (C) 2000 Ximian Inc.
 *
 *  Authors:
 *    Michael Zucchi <notzed@ximian.com>
 *    Dan Winship <danw@ximian.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "camel-file-utils.h"

#include "camel-imap-message-cache.h"
#include "camel-imap-summary.h"
#include "camel-imap-utils.h"
#include "camel-imap-folder.h"

#define CAMEL_IMAP_SUMMARY_VERSION (3)

static int summary_header_load (CamelFolderSummary *);
static int summary_header_save (CamelFolderSummary *, FILE *);

static CamelMessageInfo *message_info_load (CamelFolderSummary *s, gboolean *must_add);
static int message_info_save (CamelFolderSummary *s, FILE *out,
			      CamelMessageInfo *info);
static gboolean info_set_user_tag(CamelMessageInfo *info, const char *name, const char  *value);
static CamelMessageContentInfo *content_info_load (CamelFolderSummary *s);
static int content_info_save (CamelFolderSummary *s, FILE *out,
			      CamelMessageContentInfo *info);

static void camel_imap_summary_class_init (CamelImapSummaryClass *klass);
static void camel_imap_summary_init       (CamelImapSummary *obj);

static CamelFolderSummaryClass *camel_imap_summary_parent;

CamelType
camel_imap_summary_get_type (void)
{
	static CamelType type = CAMEL_INVALID_TYPE;

	if (type == CAMEL_INVALID_TYPE) {
		type = camel_type_register(
			camel_folder_summary_get_type(), "CamelImapSummary",
			sizeof (CamelImapSummary),
			sizeof (CamelImapSummaryClass),
			(CamelObjectClassInitFunc) camel_imap_summary_class_init,
			NULL,
			(CamelObjectInitFunc) camel_imap_summary_init,
			NULL);
	}

	return type;
}

static CamelMessageInfo *
imap_message_info_clone(CamelFolderSummary *s, const CamelMessageInfo *mi)
{
	CamelImapMessageInfo *to;
	const CamelImapMessageInfo *from = (const CamelImapMessageInfo *)mi;

	to = (CamelImapMessageInfo *)camel_imap_summary_parent->message_info_clone(s, mi);
	to->server_flags = from->server_flags;

	/* FIXME: parent clone should do this */
	to->info.content = camel_folder_summary_content_info_new(s);

	return (CamelMessageInfo *)to;
}

static void
camel_imap_summary_class_init (CamelImapSummaryClass *klass)
{
	CamelFolderSummaryClass *cfs_class = (CamelFolderSummaryClass *) klass;

	camel_imap_summary_parent = CAMEL_FOLDER_SUMMARY_CLASS (camel_type_get_global_classfuncs (camel_folder_summary_get_type()));

	cfs_class->message_info_clone = imap_message_info_clone;

	cfs_class->summary_header_load = summary_header_load;
	cfs_class->summary_header_save = summary_header_save;
	cfs_class->message_info_load = message_info_load;
	cfs_class->message_info_save = message_info_save;
	cfs_class->content_info_load = content_info_load;
	cfs_class->content_info_save = content_info_save;

	cfs_class->info_set_user_tag = info_set_user_tag;
}

static void
camel_imap_summary_set_extra_flags (CamelFolder *folder, CamelMessageInfoBase *mi)
{
	if (folder && CAMEL_IS_OBJECT (folder) && CAMEL_IS_IMAP_FOLDER (folder))
	{
		CamelImapFolder *imap_folder = CAMEL_IMAP_FOLDER (folder);
		camel_imap_message_cache_set_flags (imap_folder->folder_dir, mi);
	}
}

static void
camel_imap_summary_init (CamelImapSummary *obj)
{
	CamelFolderSummary *s = (CamelFolderSummary *)obj;

	s->set_extra_flags_func = camel_imap_summary_set_extra_flags;
	/* subclasses need to set the right instance data sizes */
	s->message_info_size = sizeof(CamelImapMessageInfo);
	s->content_info_size = sizeof(CamelImapMessageContentInfo);
}

/**
 * camel_imap_summary_new:
 * @folder: Parent folder.
 * @filename: the file to store the summary in.
 *
 * This will create a new CamelImapSummary object and read in the
 * summary data from disk, if it exists.
 *
 * Return value: A new CamelImapSummary object.
 **/
CamelFolderSummary *
camel_imap_summary_new (struct _CamelFolder *folder, const char *filename)
{
	CamelFolderSummary *summary = CAMEL_FOLDER_SUMMARY (camel_object_new (camel_imap_summary_get_type ()));

	summary->folder = folder;

	camel_folder_summary_set_build_content (summary, TRUE);
	camel_folder_summary_set_filename (summary, filename);

	if (camel_folder_summary_load (summary) == -1) {
		camel_folder_summary_clear (summary);
		camel_folder_summary_touch (summary);
	}

	return summary;
}

static int
summary_header_load (CamelFolderSummary *s)
{
	CamelImapSummary *ims = CAMEL_IMAP_SUMMARY (s);

	if (camel_imap_summary_parent->summary_header_load (s) == -1)
		return -1;

	/* Legacy version */
	if (s->version == 0x30c) {
		unsigned char* ptrchr = s->filepos;
		ptrchr = camel_file_util_mmap_decode_uint32 (ptrchr, &ims->validity, FALSE);
		s->filepos = ptrchr;
		return 0;
	}

	/* Version 1 */
	ims->version = g_ntohl(get_unaligned_u32(s->filepos)); s->filepos += 4;

	if (ims->version == 2) {
		/* Version 2: for compat with version 2 of the imap4 summary files */
		int have_mlist;
		have_mlist = g_ntohl(get_unaligned_u32(s->filepos)); s->filepos += 4;
	}

	ims->validity = g_ntohl(get_unaligned_u32(s->filepos)); s->filepos += 4;

	if (ims->version > CAMEL_IMAP_SUMMARY_VERSION) {
		g_warning("Unkown summary version\n");
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int
summary_header_save (CamelFolderSummary *s, FILE *out)
{
	CamelImapSummary *ims = CAMEL_IMAP_SUMMARY(s);

	if (camel_imap_summary_parent->summary_header_save (s, out) == -1)
		return -1;

	if (camel_file_util_encode_fixed_int32(out, CAMEL_IMAP_SUMMARY_VERSION) == -1) return -1;

	return camel_file_util_encode_fixed_int32(out, ims->validity);
}

/* We snoop the setting of the 'label' tag, so we can store it on
   the server, also in a mozilla compatiable way as a bonus */
static void
label_to_flags(CamelImapMessageInfo *info)
{
	guint32 flags;

	flags = imap_label_to_flags((CamelMessageInfo *)info);
	if ((info->info.flags & CAMEL_IMAP_MESSAGE_LABEL_MASK) != flags)
		info->info.flags = (info->info.flags & ~CAMEL_IMAP_MESSAGE_LABEL_MASK) | flags | CAMEL_MESSAGE_FOLDER_FLAGGED;
}

static CamelMessageInfo *
message_info_load (CamelFolderSummary *s, gboolean *must_add)
{
	CamelMessageInfo *info;
	CamelImapMessageInfo *iinfo;

	info = camel_imap_summary_parent->message_info_load (s, must_add);
	iinfo = (CamelImapMessageInfo*)info;

	if (info) {
		unsigned char* ptrchr = s->filepos;
		ptrchr = camel_file_util_mmap_decode_uint32 (ptrchr, &iinfo->server_flags, FALSE);
		s->filepos = ptrchr;
		label_to_flags(iinfo);
	}

	return info;
}

static int
message_info_save (CamelFolderSummary *s, FILE *out, CamelMessageInfo *info)
{
	CamelImapMessageInfo *iinfo = (CamelImapMessageInfo *)info;

	if (camel_imap_summary_parent->message_info_save (s, out, info) == -1)
		return -1;

	return camel_file_util_encode_uint32 (out, iinfo->server_flags);
}

static gboolean
info_set_user_tag(CamelMessageInfo *info, const char *name, const char  *value)
{
	int res;

	res = camel_imap_summary_parent->info_set_user_tag(info, name, value);

	if (!strcmp(name, "label"))
		label_to_flags((CamelImapMessageInfo *)info);

	return res;
}

static CamelMessageContentInfo *
content_info_load (CamelFolderSummary *s)
{
	guint32 doit;
	unsigned char* ptrchr = s->filepos;

	ptrchr = camel_file_util_mmap_decode_uint32 (ptrchr, &doit, FALSE);
	s->filepos = ptrchr;

	if (doit == 1)
		return camel_imap_summary_parent->content_info_load (s);
	else
		return camel_folder_summary_content_info_new (s);
}

static int
content_info_save (CamelFolderSummary *s, FILE *out,
		   CamelMessageContentInfo *info)
{
	if (info && info->type) {
		if (camel_file_util_encode_uint32 (out, 1)== -1) return -1;
		return camel_imap_summary_parent->content_info_save (s, out, info);
	} else
		return camel_file_util_encode_uint32 (out, 0);
}

void
camel_imap_summary_add_offline (CamelFolderSummary *summary, const char *uid,
				CamelMimeMessage *message,
				const CamelMessageInfo *info)
{
	CamelImapMessageInfo *mi;
#ifdef NON_TINYMAIL_FEATURES
	const CamelFlag *flag;
	const CamelTag *tag;
#endif

	/* Create summary entry */
	mi = (CamelImapMessageInfo *)camel_folder_summary_info_new_from_message (summary, message);

	/* Copy flags 'n' tags */
	mi->info.flags = camel_message_info_flags(info);

#ifdef NON_TINYMAIL_FEATURES
	flag = camel_message_info_user_flags(info);
	while (flag) {
		camel_message_info_set_user_flag((CamelMessageInfo *)mi, flag->name, TRUE);
		flag = flag->next;
	}
	tag = camel_message_info_user_tags(info);
	while (tag) {
		camel_message_info_set_user_tag((CamelMessageInfo *)mi, tag->name, tag->value);
		tag = tag->next;
	}
#endif

	mi->info.size = ((CamelMessageInfoBase *)info)->size;
	g_free (mi->info.uid);
	mi->info.uid = g_strdup (uid);

	label_to_flags(mi);

	camel_folder_summary_add (summary, (CamelMessageInfo *)mi);
}

void
camel_imap_summary_add_offline_uncached (CamelFolderSummary *summary, const char *uid,
					 const CamelMessageInfo *info)
{
	CamelImapMessageInfo *mi;

	mi = camel_message_info_clone(info);
	g_free (mi->info.uid);
	mi->info.uid = g_strdup(uid);

	label_to_flags(mi);

	camel_folder_summary_add (summary, (CamelMessageInfo *)mi);
}
