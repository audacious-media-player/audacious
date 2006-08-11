/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <glib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include "playlist.h"
#include "playlist_container.h"

static void
playlist_load_pls(const gchar * filename, gint pos)
{
    guint i, count, added_count = 0;
    gchar key[10];
    gchar *line;

    g_return_val_if_fail(filename != NULL, 0);

    if (!str_has_suffix_nocase(filename, ".pls"))
        return;

    if (!(line = read_ini_string(filename, "playlist", "NumberOfEntries")))
        return;

    count = atoi(line);
    g_free(line);

    for (i = 1; i <= count; i++) {
        g_snprintf(key, sizeof(key), "File%d", i);
        if ((line = read_ini_string(filename, "playlist", key))) {
            playlist_load_ins_file(line, filename, pos, NULL, -1);
            added_count++;

            if (pos >= 0)
                pos++;

            g_free(line);
        }
    }
}

static void
playlist_save_pls(gchar *filename, gint pos)
{
    GList *node;
    FILE *file = fopen(filename, "wb");

    g_return_if_fail(file != NULL);

    g_fprintf(file, "[playlist]\n");
    g_fprintf(file, "NumberOfEntries=%d\n", playlist_get_length());

    PLAYLIST_LOCK();

    for (node = playlist_get(); node; node = g_list_next(node)) {
        PlaylistEntry *entry = PLAYLIST_ENTRY(node->data);

        g_fprintf(file, "File%d=%s\n", g_list_position(playlist_get(), node) + 1,
                  entry->filename);
    }

    PLAYLIST_UNLOCK();

    fclose(file);
}

PlaylistContainer plc_pls = {
	.name = "Winamp .pls Playlist Format",
	.ext = "pls",
	.plc_read = playlist_load_pls,
	.plc_write = playlist_save_pls,
};

