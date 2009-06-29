/*
 * scanner.c
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

#include <glib.h>

#include "playlist.h"

/*
 * Note: There may be a more efficient way to do this. We keep the playlist
 * locked almost constantly and iterate through it each cycle. If you change the
 * code, though, be careful not to create any race conditions. -jlindgren
 */

static GMutex * mutex;
static GCond * wake;
static GThread * thread;
static Playlist * active;
static char enabled, reset, quit;

void * scanner (void * unused)
{
    char done = 0;
    GList * node;
    PlaylistEntry * entry;

    while (! quit)
    {
        g_mutex_lock (mutex);

        if (reset)
        {
            done = 0;
            reset = 0;
        }

        if (! active)
            done = 1;

        if (! enabled || done)
        {
            g_cond_wait (wake, mutex);
            g_mutex_unlock (mutex);
            continue;
        }

        PLAYLIST_LOCK (active);

        for (node = active->entries; node; node = node->next)
        {
            entry = node->data;

            if (! entry->tuple && ! entry->failed)
                goto FOUND;
        }

        PLAYLIST_UNLOCK (active);
        g_mutex_unlock (mutex);

        done = 1;
        continue;

      FOUND:
        playlist_entry_get_info (entry);

        PLAYLIST_UNLOCK (active);
        g_mutex_unlock (mutex);

        event_queue ("playlist update", 0);
    }

    return 0;
}

void scanner_init (void)
{
    mutex = g_mutex_new ();
    wake = g_cond_new ();

    active = 0;
    enabled = 0;
    reset = 0;
    quit = 0;

    thread = g_thread_create (scanner, 0, 1, 0);
}

void scanner_enable (char enable)
{
    enabled = enable;
    g_cond_signal (wake);
}

void scanner_reset (void)
{
    g_mutex_lock (mutex);
    reset = 1;
    g_cond_signal (wake);
    g_mutex_unlock (mutex);
}

void scanner_end (void)
{
    quit = 1;
    g_cond_signal (wake);

    g_thread_join (thread);

    g_mutex_free (mutex);
    g_cond_free (wake);
}

Playlist * get_active_playlist (void)
{
    return active;
}

void set_active_playlist (Playlist * playlist)
{
    if (playlist == active)
        return;

    g_mutex_lock (mutex);
    active = playlist;
    reset = 1;
    g_cond_signal (wake);
    g_mutex_unlock (mutex);
}
