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

#include <audacious/misc.h>
#include <libaudcore/hook.h>

#define MAX_ENTRIES 30

static GQueue history = G_QUEUE_INIT;
static gboolean loaded, modified;

static void history_save (void)
{
    if (! modified)
        return;

    config_clear_section ("history");

    GList * node = history.head;
    for (gint i = 0; i < MAX_ENTRIES; i ++)
    {
        if (! node)
            break;

        gchar name[32];
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

    for (gint i = 0; ; i ++)
    {
        gchar name[32];
        snprintf (name, sizeof name, "entry%d", i);
        gchar * path = get_string ("history", name);

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

const gchar * history_get (gint entry)
{
    history_load ();
    return g_queue_peek_nth (& history, entry);
}

void history_add (const gchar * path)
{
    history_load ();
    g_queue_push_head (& history, g_strdup (path));
    modified = TRUE;
}
