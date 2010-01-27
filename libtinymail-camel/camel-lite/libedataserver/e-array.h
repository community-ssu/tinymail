/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __E_ARRAY_H__
#define __E_ARRAY_H__

#include <glib.h>


G_BEGIN_DECLS

/* Resizable arrays. remove fills any cleared spot and shortens the
 * array, while preserving the order. remove_fast will distort the
 * order by moving the last element to the position of the removed.
 */

#define e_array_append_val(a,v)	  e_array_append_vals (a, &(v), 1)
#define e_array_prepend_val(a,v)  e_array_prepend_vals (a, &(v), 1)
#define e_array_insert_val(a,i,v) e_array_insert_vals (a, i, &(v), 1)
#define e_array_index(a,t,i)      (((t*) (void *) (a)->data) [(i)])

GArray* e_array_new               (gboolean          zero_terminated,
				   gboolean          clear_,
				   guint             element_size);
GArray* e_array_sized_new         (gboolean          zero_terminated,
				   gboolean          clear_,
				   guint             element_size,
				   guint             reserved_size);
gchar*  e_array_free              (GArray           *array,
				   gboolean          free_segment);
GArray* e_array_append_vals       (GArray           *array,
				   gconstpointer     data,
				   guint             len);
GArray* e_array_prepend_vals      (GArray           *array,
				   gconstpointer     data,
				   guint             len);
GArray* e_array_insert_vals       (GArray           *array,
				   guint             index_,
				   gconstpointer     data,
				   guint             len);
GArray* e_array_set_size          (GArray           *array,
				   guint             length);
GArray* e_array_remove_index      (GArray           *array,
				   guint             index_);
GArray* e_array_remove_index_fast (GArray           *array,
				   guint             index_);
GArray* e_array_remove_range      (GArray           *array,
				   guint             index_,
				   guint             length);
void    e_array_sort              (GArray           *array,
				   GCompareFunc      compare_func);
void    e_array_sort_with_data    (GArray           *array,
				   GCompareDataFunc  compare_func,
				   gpointer          user_data);

/* Resizable pointer array.  This interface is much less complicated
 * than the above.  Add appends a pointer.  Remove fills any cleared 
 * spot and shortens the array. remove_fast will again distort order.  
 */
#define    e_ptr_array_index(array,index_) ((array)->pdata)[index_]
GPtrArray* e_ptr_array_new                (void);
GPtrArray* e_ptr_array_sized_new          (guint             reserved_size);
gpointer*  e_ptr_array_free               (GPtrArray        *array,
					   gboolean          free_seg);
void       e_ptr_array_set_size           (GPtrArray        *array,
					   gint              length);
gpointer   e_ptr_array_remove_index       (GPtrArray        *array,
					   guint             index_);
gpointer   e_ptr_array_remove_index_fast  (GPtrArray        *array,
					   guint             index_);
gboolean   e_ptr_array_remove             (GPtrArray        *array,
					   gpointer          data);
gboolean   e_ptr_array_remove_fast        (GPtrArray        *array,
					   gpointer          data);
void       e_ptr_array_remove_range       (GPtrArray        *array,
					   guint             index_,
					   guint             length);
void       e_ptr_array_add                (GPtrArray        *array,
					   gpointer          data);
void       e_ptr_array_sort               (GPtrArray        *array,
					   GCompareFunc      compare_func);
void       e_ptr_array_sort_with_data     (GPtrArray        *array,
					   GCompareDataFunc  compare_func,
					   gpointer          user_data);
void       e_ptr_array_foreach            (GPtrArray        *array,
					   GFunc             func,
					   gpointer          user_data);


/* Byte arrays, an array of guint8.  Implemented as a GArray,
 * but type-safe.
 */

GByteArray* e_byte_array_new               (void);
GByteArray* e_byte_array_sized_new         (guint             reserved_size);
guint8*     e_byte_array_free              (GByteArray       *array,
					    gboolean          free_segment);
GByteArray* e_byte_array_append            (GByteArray       *array,
					    const guint8     *data,
					    guint             len);
GByteArray* e_byte_array_prepend           (GByteArray       *array,
					    const guint8     *data,
					    guint             len);
GByteArray* e_byte_array_set_size          (GByteArray       *array,
					    guint             length);
GByteArray* e_byte_array_remove_index      (GByteArray       *array,
					    guint             index_);
GByteArray* e_byte_array_remove_index_fast (GByteArray       *array,
					    guint             index_);
GByteArray* e_byte_array_remove_range      (GByteArray       *array,
					    guint             index_,
					    guint             length);
void        e_byte_array_sort              (GByteArray       *array,
					    GCompareFunc      compare_func);
void        e_byte_array_sort_with_data    (GByteArray       *array,
					    GCompareDataFunc  compare_func,
					    gpointer          user_data);


G_END_DECLS

#endif /* __e_ARRAY_H__ */

