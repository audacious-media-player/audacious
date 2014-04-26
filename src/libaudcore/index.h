/*
 * index.h
 * Copyright 2009-2013 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#ifndef LIBAUDCORE_INDEX_H
#define LIBAUDCORE_INDEX_H

/* An "index" is an opaque structure representing a list of pointers.  It is
 * used primarily to store Audacious playlists, but can be useful for other
 * purposes as well. */

struct _Index;
typedef struct _Index Index;

typedef void (* IndexFreeFunc) (void * value);

/* Returns a new, empty index. */
Index * index_new (void);

/* Destroys <index>. */
void index_free (Index * index);

/* Destroys <index>, first calling <func> on each pointer stored in it. */
void index_free_full (Index * index, IndexFreeFunc func);

/* Returns the number of pointers stored in <index>. */
int index_count (Index * index);

/* Preallocates space to store <size> pointers in <index>.  This can be used to
 * avoid repeated memory allocations when adding pointers to <index> one by one.
 * The value returned by index_count() does not changed until the pointers are
 * actually added. */
void index_allocate (Index * index, int size);

/* Returns the value stored in <index> at position <at>. */
void * index_get (Index * index, int at);

/* Stores <value> in <index> at position <at>, overwriting the previous value. */
void index_set (Index * index, int at, void * value);

/* Stores <value> in <index> at position <at>, shifting the previous value (if
 * any) and those following it forward to make room.  If <at> is -1, <value> is
 * added to the end of <index>. */
void index_insert (Index * index, int at, void * value);

/* Copies <count> pointers from <source>, starting at position <from>, to
 * <target>, starting at position <to>.  Existing pointers in <target> are
 * overwritten.  Overlapping regions are handled correctly if <source> and
 * <target> are the same index. */
void index_copy_set (Index * source, int from, Index * target, int to, int count);

/* Copies <count> pointers from <source>, starting at position <from>, to
 * <target>, starting at position <to>.  Existing pointers in <target> are
 * shifted forward to make room.  All cases are handled correctly if <source>
 * and <target> are the same index, including the case where <to> is between
 * <from> and <from + count>.  If <to> is -1, the pointers are added to the end
 * of <target>.  If <count> is -1, copying continues until the end of <source>
 * is reached. */
void index_copy_insert (Index * source, int from, Index * target, int to, int count);

/* Removes <count> pointers from <index>, start at position <at>.  Any following
 * pointers are shifted backward to close the gap.  If <count> is -1, all
 * pointers from <at> to the end of <index> are removed. */
void index_delete (Index * index, int at, int count);

/* Like index_delete(), but first calls <func> on any pointer that is being
 * removed. */
void index_delete_full (Index * index, int at, int count, IndexFreeFunc func);

/* Sort the entries in <index> using the quick-sort algorithm with the given
 * comparison function.  The return value of <compare> should follow the
 * convention used by strcmp(): negative if (a < b), zero if (a = b), positive
 * if (a > b). */
void index_sort (Index * index, int (* compare) (const void * a, const void * b));

/* Exactly like index_sort() except that <data> is forwarded to the comparison
 * function.  This allows comparison functions whose behavior changes depending
 * on context. */
void index_sort_with_data (Index * index, int (* compare) (const void * a,
 const void * b, void * data), void * data);

#endif /* LIBAUDCORE_INDEX_H */
