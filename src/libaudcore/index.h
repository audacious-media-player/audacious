/*
 * index.h
 * Copyright 2009-2011 John Lindgren
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

struct _Index;
typedef struct _Index Index;

Index * index_new (void);
void index_free (Index * index);
int index_count (Index * index);
void index_allocate (Index * index, int size);
void index_set (Index * index, int at, void * value);
void * index_get (Index * index, int at);
void index_insert (Index * index, int at, void * value);
void index_append (Index * index, void * value);
void index_copy_set (Index * source, int from, Index * target, int to, int count);
void index_copy_insert (Index * source, int from, Index * target, int to, int count);
void index_copy_append (Index * source, int from, Index * target, int count);
void index_merge_insert (Index * first, int at, Index * second);
void index_merge_append (Index * first, Index * second);
void index_move (Index * index, int from, int to, int count);
void index_delete (Index * index, int at, int count);
void index_sort (Index * index, int (* compare) (const void * a, const void * b));
void index_sort_with_data (Index * index, int (* compare) (const void * a,
 const void * b, void * data), void * data);

#endif /* LIBAUDCORE_INDEX_H */
