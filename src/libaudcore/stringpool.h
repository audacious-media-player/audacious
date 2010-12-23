/*
 * Audacious
 * Copyright © 2009 William Pitcock <nenolod@atheme.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */
/**
 * @file stringpool.h
 * String pool API for "pooling" identical strings into references
 * instead of keeping multiple copies around.
 */

#ifndef AUDACIOUS_STRINGPOOL_H
#define AUDACIOUS_STRINGPOOL_H

/**
 * Fetches or allocates a given string from the stringpool.
 * If string already exists in the pool, reference to it is returned.
 * Otherwise, a new string is created in the pool with one reference.
 *
 * @param[in] str String to be poolified.
 * @param[in] take Nonzero if the caller no longer needs str; in this case, the
 * pool will eventually call g_free(str).
 * @return Reference to the pooled string, or NULL if the given
 * string was NULL or an error occured.
 */
gchar *stringpool_get(gchar *str, gboolean take);

/**
 * Unreference a pooled string. When there are no references left,
 * the string is unallocated and removed from the pool.
 *
 * @param[in] str Pointer to a string in the pool.
 */
void stringpool_unref(gchar *str);

#endif

