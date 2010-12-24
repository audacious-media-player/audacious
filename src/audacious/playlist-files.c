/*
 * playlist-files.c
 * Copyright 2010 John Lindgren
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

#include "debug.h"
#include "playlist.h"
#include "plugin.h"
#include "plugins.h"

static const gchar * get_extension (const gchar * filename, gboolean quiet)
{
    const gchar * s = strrchr (filename, '/');
    if (! s)
        goto FAIL;

    const gchar * p = strrchr (s + 1, '.');
    if (! p)
        goto FAIL;

    return p + 1;

FAIL:
    if (! quiet)
        fprintf (stderr, "Failed to parse playlist filename %s.\n", filename);
    return NULL;
}

gboolean filename_is_playlist (const gchar * filename)
{
    const gchar * ext = get_extension (filename, TRUE);
    if (! ext)
        return FALSE;

    return playlist_plugin_for_extension (ext) ? TRUE : FALSE;
}

static PlaylistPlugin * get_plugin (const gchar * filename)
{
    const gchar * ext = get_extension (filename, FALSE);
    if (! ext)
        return NULL;

    PluginHandle * plugin = playlist_plugin_for_extension (ext);
    if (! plugin)
    {
        fprintf (stderr, "Unrecognized playlist file type \"%s\".\n", ext);
        return NULL;
    }

    return plugin_get_header (plugin);
}

gboolean playlist_insert_playlist (gint list, gint at, const gchar * filename)
{
    AUDDBG ("Loading playlist %s.\n", filename);
    PlaylistPlugin * pp = get_plugin (filename);
    g_return_val_if_fail (pp && pp->load, FALSE);
    return pp->load (filename, list, at);
}

gboolean playlist_save (gint list, const gchar * filename)
{
    AUDDBG ("Saving playlist %s.\n", filename);
    PlaylistPlugin * pp = get_plugin (filename);
    g_return_val_if_fail (pp && pp->save, FALSE);
    return pp->save (filename, list);
}
