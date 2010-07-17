/*
 * probe.c
 * Copyright 2009-2010 John Lindgren
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

#include <stdio.h>
#include <string.h>

#include <libaudcore/audstrings.h>

#include "debug.h"
#include "misc.h"
#include "playlist.h"
#include "plugin.h"
#include "plugins.h"
#include "probe-buffer.h"

typedef struct
{
    gchar * filename;
    VFSFile * handle;
    gboolean buffered;
    PluginHandle * plugin;
}
ProbeState;

static gboolean check_opened (ProbeState * state)
{
    if (state->handle != NULL)
        return TRUE;

    AUDDBG ("Opening %s.\n", state->filename);
    if ((state->buffered = vfs_is_remote (state->filename)))
        state->handle = probe_buffer_new (state->filename);
    else
        state->handle = vfs_fopen (state->filename, "r");

    if (state->handle != NULL)
        return TRUE;

    AUDDBG ("FAILED.\n");
    return FALSE;
}

static gboolean probe_func (PluginHandle * plugin, ProbeState * state)
{
    AUDDBG ("Trying %s.\n", plugin_get_name (plugin));
    InputPlugin * decoder = plugin_get_header (plugin);
    if (decoder == NULL)
        return TRUE;

    if (decoder->is_our_file_from_vfs != NULL)
    {
        if (! check_opened (state))
            return FALSE;

        if (state->buffered)
            probe_buffer_set_decoder (state->handle, plugin_get_name (plugin));

        if (decoder->is_our_file_from_vfs (state->filename, state->handle))
        {
            state->plugin = plugin;
            return FALSE;
        }

        if (vfs_fseek (state->handle, 0, SEEK_SET) < 0)
            return FALSE;
    }
    else if (decoder->is_our_file != NULL)
    {
        if (decoder->is_our_file (state->filename))
        {
            state->plugin = plugin;
            return FALSE;
        }
    }

    return TRUE;
}

/* Optimization: If we have found plugins with a key match, assume that at least
 * one of them will succeed.  This means that we need not check the very last
 * plugin.  (If there is only one, we do not need to check it at all.)  This is
 * implemented as follows:
 *
 * 1. On the first call, assume until further notice the plugin passed is the
 *    last one and will therefore succeed.
 * 2. On a subsequent call, think twice and probe the plugin we assumed would
 *    succeed.  If it does in fact succeed, then we are done.  If not, assume
 *    similarly that the plugin passed in this call is the last one.
 */

static gboolean probe_func_fast (PluginHandle * plugin, ProbeState * state)
{
    if (state->plugin != NULL)
    {
        PluginHandle * prev = state->plugin;
        state->plugin = NULL;

        if (prev != NULL && ! probe_func (prev, state))
            return FALSE;
    }

    AUDDBG ("Guessing %s.\n", plugin_get_name (plugin));
    state->plugin = plugin;
    return TRUE;
}

static void probe_by_scheme (ProbeState * state)
{
    gchar * s = strstr (state->filename, "://");
    gchar c;

    if (s == NULL)
        return;

    AUDDBG ("Probing by scheme.\n");
    c = s[3];
    s[3] = 0;
    input_plugin_for_key (INPUT_KEY_SCHEME, state->filename, (PluginForEachFunc)
     probe_func_fast, state);
    s[3] = c;
}

static void probe_by_extension (ProbeState * state)
{
    gchar * s = strrchr (state->filename, '.');

    if (s == NULL)
        return;

    AUDDBG ("Probing by extension.\n");
    s = g_ascii_strdown (s + 1, -1);
    input_plugin_for_key (INPUT_KEY_EXTENSION, s, (PluginForEachFunc)
     probe_func_fast, state);
    g_free (s);
}

static void probe_by_mime (ProbeState * state)
{
    gchar * mime;

    if (! check_opened (state))
        return;

    if ((mime = vfs_get_metadata (state->handle, "content-type")) == NULL)
        return;

    AUDDBG ("Probing by MIME type.\n");
    input_plugin_for_key (INPUT_KEY_MIME, mime, (PluginForEachFunc)
     probe_func_fast, state);
    g_free (mime);
}

static void probe_by_content (ProbeState * state)
{
    AUDDBG ("Probing by content.\n");
    plugin_for_each (PLUGIN_TYPE_INPUT, (PluginForEachFunc) probe_func, state);
}

InputPlugin * file_find_decoder (const gchar * filename, gboolean fast)
{
    ProbeState state;

    AUDDBG ("Probing %s.\n", filename);
    state.plugin = NULL;
    state.filename = filename_split_subtune (filename, NULL);
    state.handle = NULL;

    probe_by_scheme (& state);

    if (state.plugin != NULL)
        goto DONE;

    probe_by_extension (& state);

    if (state.plugin != NULL || fast)
        goto DONE;

    probe_by_mime (& state);

    if (state.plugin != NULL)
        goto DONE;

    probe_by_content (& state);

DONE:
    g_free (state.filename);

    if (state.handle != NULL)
        vfs_fclose (state.handle);

    return (state.plugin != NULL) ? plugin_get_header (state.plugin) : NULL;
}

Tuple * file_read_tuple (const gchar * filename, InputPlugin * decoder)
{
    if (decoder->get_song_tuple != NULL)
        return decoder->get_song_tuple (filename);

    if (decoder->probe_for_tuple != NULL)
    {
        VFSFile * handle = vfs_fopen (filename, "r");
        Tuple * tuple;

        if (handle == NULL)
            return NULL;

        tuple = decoder->probe_for_tuple (filename, handle);
        vfs_fclose (handle);
        return tuple;
    }

    return NULL;
}

gboolean file_read_image (const gchar * filename, InputPlugin * decoder,
 void * * data, gint * size)
{
    VFSFile * handle;
    gboolean success;

    if (decoder->get_song_image == NULL)
        return FALSE;

    handle = vfs_fopen (filename, "r");
    success = decoder->get_song_image (filename, handle, data, size);

    if (handle != NULL)
        vfs_fclose (handle);

    return success;
}

gboolean file_can_write_tuple (const gchar * filename, InputPlugin * decoder)
{
    return (decoder->update_song_tuple != NULL);
}

gboolean file_write_tuple (const gchar * filename, InputPlugin * decoder,
 const Tuple * tuple)
{
    VFSFile * handle;
    gboolean success;

    if (decoder->update_song_tuple == NULL)
        return FALSE;

    handle = vfs_fopen (filename, "r+");

    if (handle == NULL)
        return FALSE;

    success = decoder->update_song_tuple (tuple, handle);
    vfs_fclose (handle);

    if (success)
        playlist_rescan_file (filename);

    return success;
}

gboolean custom_infowin (const gchar * filename, InputPlugin * decoder)
{
    if (decoder->file_info_box == NULL)
        return FALSE;

    decoder->file_info_box (filename);
    return TRUE;
}
