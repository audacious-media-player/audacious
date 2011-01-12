/*
 * Audacious
 * Copyright © 2009 William Pitcock <nenolod@atheme.org>
 * Copyright © 2010 John Lindgren <john.lindgren@tds.net>
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

#include <glib.h>
#include <mowgli.h>

#ifndef MOWGLI_PATRICIA_ALLOWS_NULL_CANONIZE
static void
noopcanon(gchar *str)
{
    return;
}
#endif

/** Structure to handle string refcounting. */
typedef struct {
    gint refcount;
    gchar *str;
} PooledString;

static mowgli_heap_t *stringpool_heap = NULL;
static mowgli_patricia_t *stringpool_tree = NULL;
static GStaticMutex stringpool_mutex = G_STATIC_MUTEX_INIT;

#define MAXLEN 100

gchar *
stringpool_get(gchar *str, gboolean take)
{
    if (str == NULL)
        return NULL;

    if (strlen(str) > MAXLEN)
        return take ? str : g_strdup(str);

    g_static_mutex_lock(&stringpool_mutex);

    if (stringpool_heap == NULL)
        stringpool_heap = mowgli_heap_create(sizeof(PooledString), 1024, 0);
    if (stringpool_tree == NULL)
#ifdef MOWGLI_PATRICIA_ALLOWS_NULL_CANONIZE
        stringpool_tree = mowgli_patricia_create(NULL);
#else
        stringpool_tree = mowgli_patricia_create(noopcanon);
#endif

    PooledString *ps;

    if ((ps = mowgli_patricia_retrieve(stringpool_tree, str)) != NULL)
    {
        ps->refcount++;

        if (take)
            g_free(str);
    }
    else
    {
        ps = mowgli_heap_alloc(stringpool_heap);
        ps->refcount = 1;
        ps->str = take ? str : g_strdup(str);
        mowgli_patricia_add(stringpool_tree, str, ps);
    }

    g_static_mutex_unlock(&stringpool_mutex);
    return ps->str;
}

void
stringpool_unref(gchar *str)
{
    if (str == NULL)
        return;

    if (strlen(str) > MAXLEN)
        return g_free(str);

    g_return_if_fail(stringpool_heap != NULL);
    g_return_if_fail(stringpool_tree != NULL);

    g_static_mutex_lock(&stringpool_mutex);

    PooledString *ps;
    ps = mowgli_patricia_retrieve(stringpool_tree, str);
    if (ps != NULL && --ps->refcount <= 0)
    {
        mowgli_patricia_delete(stringpool_tree, str);
        g_free(ps->str);
        mowgli_heap_free(stringpool_heap, ps);
    }

    g_static_mutex_unlock(&stringpool_mutex);
}
