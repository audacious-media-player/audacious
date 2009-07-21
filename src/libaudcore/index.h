/*
 * index.h
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef AUDACIOUS_INDEX_H
#define AUDACIOUS_INDEX_H

struct index;

struct index * index_new (void);
void index_free (struct index * index);
gint index_count (struct index * index);
void index_set (struct index * index, gint at, void * value);
void * index_get (struct index * index, gint at);
void index_insert (struct index * index, gint at, void * value);
void index_append (struct index * index, void * value);
void index_merge_insert (struct index * first, gint at, struct index * second);
void index_merge_append (struct index * first, struct index * second);
void index_delete (struct index * index, gint at, gint length);
void index_sort (struct index * index, gint (* compare) (const void * *, const
 void * *));

#endif
