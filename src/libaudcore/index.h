/*
 * index.h
 * Copyright 2009-2010 John Lindgren
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

struct index;

struct index * index_new (void);
void index_free (struct index * index);
int index_count (struct index * index);
void index_allocate (struct index * index, int size);
void index_set (struct index * index, int at, void * value);
void * index_get (struct index * index, int at);
void index_insert (struct index * index, int at, void * value);
void index_append (struct index * index, void * value);
void index_copy_set (struct index * source, int from, struct index * target,
 int to, int count);
void index_copy_insert (struct index * source, int from, struct index * target,
 int to, int count);
void index_copy_append (struct index * source, int from, struct index * target,
 int count);
void index_merge_insert (struct index * first, int at, struct index * second);
void index_merge_append (struct index * first, struct index * second);
void index_move (struct index * index, int from, int to, int count);
void index_delete (struct index * index, int at, int count);
void index_sort (struct index * index, int (* compare) (const void * a,
 const void * b));
void index_sort_with_data (struct index * index, int (* compare)
 (const void * a, const void * b, void * data), void * data);

#endif /* LIBAUDCORE_INDEX_H */
