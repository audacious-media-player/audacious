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

/*
 * Note: This code used to do some normalization of strings: conversion to
 * UTF-8, conversion of the empty string to NULL, and (optionally) conversion
 * to uppercase.  However, because such conversions can change the length of the
 * string, they can lead to a double-free.
 *
 * Consider:
 *
 * stringpool_get is called twice with the same 99-character ISO-8859-1 string.
 * The string is short enough to be cached, so stringpool_get returns a cached,
 * 101-character UTF-8 string.  stringpool_unref is then called twice
 * with the cached string.  Now that it has been converted, it is too long to be
 * cached, so stringpool_unref simply frees it, twice.
 *
 * Therefore, it is essential for stringpool_get to return a string that is
 * exactly the same as the one passed it.
 *
 * --jlindgren
 */

#include <glib.h>
#include <mowgli.h>

#include "audstrings.h"

#define MAXLEN 100

static void
noopcanon(gchar *str)
{
    return;
}

/** Structure to handle string refcounting. */
typedef struct {
    gint refcount;
    gchar *str;
} PooledString;

static mowgli_patricia_t *stringpool_tree = NULL;
static GStaticMutex stringpool_mutex = G_STATIC_MUTEX_INIT;

static inline gboolean stringpool_should_cache(const gchar *string)
{
    const gchar *end = memchr(string, '\0', MAXLEN + 1);
    return end != NULL ? TRUE : FALSE;
}

gchar *
stringpool_get(const gchar *str)
{
    PooledString *ps;

    g_return_val_if_fail(str != NULL, NULL);

    if (!stringpool_should_cache(str))
        return g_strdup(str);

    g_static_mutex_lock(&stringpool_mutex);

    if (stringpool_tree == NULL)
        stringpool_tree = mowgli_patricia_create(noopcanon);

    if ((ps = mowgli_patricia_retrieve(stringpool_tree, str)) != NULL)
    {
        ps->refcount++;

        g_static_mutex_unlock(&stringpool_mutex);
        return ps->str;
    }

    ps = g_slice_new0(PooledString);
    ps->refcount++;
    ps->str = g_strdup(str);
    mowgli_patricia_add(stringpool_tree, str, ps);

    g_static_mutex_unlock(&stringpool_mutex);
    return ps->str;
}

void
stringpool_unref(gchar *str)
{
    PooledString *ps;

    g_return_if_fail(str != NULL);

    if (!stringpool_should_cache(str))
    {
        g_free(str);
        return;
    }

    g_return_if_fail(stringpool_tree != NULL);

    g_static_mutex_lock(&stringpool_mutex);

    ps = mowgli_patricia_retrieve(stringpool_tree, str);
    if (ps != NULL && --ps->refcount <= 0)
    {
        mowgli_patricia_delete(stringpool_tree, str);
        g_free(ps->str);
        g_slice_free(PooledString, ps);
    }

    g_static_mutex_unlock(&stringpool_mutex);
}
