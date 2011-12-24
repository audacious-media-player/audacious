/*
 * history.c
 * Copyright 2011 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
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

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include <libaudcore/hook.h>

#include "main.h"
#include "misc.h"

#define MAX_ENTRIES 30

static GQueue history = G_QUEUE_INIT;
static bool_t loaded, modified;

static void history_save (void)
{
    if (! modified)
        return;

    config_clear_section ("history");

    GList * node = history.head;
    for (int i = 0; i < MAX_ENTRIES; i ++)
    {
        if (! node)
            break;

        char name[32];
        snprintf (name, sizeof name, "entry%d", i);
        set_string ("history", name, node->data);

        node = node->next;
    }

    modified = FALSE;
}

static void history_load (void)
{
    if (loaded)
        return;

    for (int i = 0; ; i ++)
    {
        char name[32];
        snprintf (name, sizeof name, "entry%d", i);
        char * path = get_string ("history", name);

        if (! path[0])
        {
            g_free (path);
            break;
        }

        g_queue_push_tail (& history, path);
    }

    loaded = TRUE;
    hook_associate ("config save", (HookFunction) history_save, NULL);
}

void history_cleanup (void)
{
    if (! loaded)
        return;

    hook_dissociate ("config save", (HookFunction) history_save);

    g_queue_foreach (& history, (GFunc) g_free, NULL);
    g_queue_clear (& history);

    loaded = FALSE;
    modified = FALSE;
}

const char * history_get (int entry)
{
    history_load ();
    return g_queue_peek_nth (& history, entry);
}

void history_add (const char * path)
{
    history_load ();

    GList * next;
    for (GList * node = history.head; node; node = next)
    {
        next = node->next;
        if (! strcmp (node->data, path))
        {
            g_free (node->data);
            g_queue_delete_link (& history, node);
        }
    }

    g_queue_push_head (& history, g_strdup (path));
    modified = TRUE;
}
