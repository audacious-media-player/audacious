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

#include <assert.h>
#include <glib.h>
#include <mowgli.h>

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

    if (stringpool_tree == NULL)
        stringpool_tree = mowgli_patricia_create(NULL);

    mowgli_patricia_elem_t *elem = mowgli_patricia_elem_find(stringpool_tree, str);
    if (elem != NULL)
    {
        gint refcount = GPOINTER_TO_INT(mowgli_patricia_elem_get_data(elem));
        mowgli_patricia_elem_set_data(elem, GINT_TO_POINTER(refcount + 1));
    }
    else
    {
        elem = mowgli_patricia_elem_add(stringpool_tree, str, GINT_TO_POINTER(1));
        assert(elem != NULL);
    }

    if (take)
        g_free(str);
    str = (gchar *)mowgli_patricia_elem_get_key(elem);

    g_static_mutex_unlock(&stringpool_mutex);

    return str;
}

void
stringpool_unref(gchar *str)
{
    if (str == NULL)
        return;

    if (strlen(str) > MAXLEN)
        return g_free(str);

    g_return_if_fail(stringpool_tree != NULL);

    g_static_mutex_lock(&stringpool_mutex);

    mowgli_patricia_elem_t *elem = mowgli_patricia_elem_find(stringpool_tree, str);
    assert(elem != NULL);

    gint refcount = GPOINTER_TO_INT(mowgli_patricia_elem_get_data(elem));
    if (refcount == 1)
        mowgli_patricia_elem_delete(stringpool_tree, elem);
    else
        mowgli_patricia_elem_set_data(elem, GINT_TO_POINTER(refcount - 1));

    g_static_mutex_unlock(&stringpool_mutex);
}
