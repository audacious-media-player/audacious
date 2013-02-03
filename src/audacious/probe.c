/*
 * probe.c
 * Copyright 2009-2010 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <glib.h>
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
    const char * filename;
    VFSFile * handle;
    bool_t failed;
    PluginHandle * plugin;
}
ProbeState;

static bool_t check_opened (ProbeState * state)
{
    if (state->handle != NULL)
        return TRUE;
    if (state->failed)
        return FALSE;

    AUDDBG ("Opening %s.\n", state->filename);
    if (vfs_is_remote (state->filename))
        state->handle = probe_buffer_new (state->filename);
    else
        state->handle = vfs_fopen (state->filename, "r");

    if (state->handle != NULL)
        return TRUE;

    AUDDBG ("FAILED.\n");
    state->failed = TRUE;
    return FALSE;
}

static bool_t probe_func (PluginHandle * plugin, ProbeState * state)
{
    AUDDBG ("Trying %s.\n", plugin_get_name (plugin));
    InputPlugin * decoder = plugin_get_header (plugin);
    if (decoder == NULL)
        return TRUE;

    if (decoder->is_our_file_from_vfs != NULL)
    {
        if (! check_opened (state))
            return FALSE;

        if (decoder->is_our_file_from_vfs (state->filename, state->handle))
        {
            state->plugin = plugin;
            return FALSE;
        }

        if (vfs_fseek (state->handle, 0, SEEK_SET) < 0)
            return FALSE;
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

static bool_t probe_func_fast (PluginHandle * plugin, ProbeState * state)
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
    const char * s = strstr (state->filename, "://");

    if (s == NULL)
        return;

    AUDDBG ("Probing by scheme.\n");
    char buf[s - state->filename + 1];
    memcpy (buf, state->filename, s - state->filename);
    buf[s - state->filename] = 0;

    input_plugin_for_key (INPUT_KEY_SCHEME, buf, (PluginForEachFunc) probe_func_fast, state);
}

static void probe_by_extension (ProbeState * state)
{
    char buf[32];
    if (! uri_get_extension (state->filename, buf, sizeof buf))
        return;

    AUDDBG ("Probing by extension.\n");
    input_plugin_for_key (INPUT_KEY_EXTENSION, buf, (PluginForEachFunc) probe_func_fast, state);
}

static void probe_by_mime (ProbeState * state)
{
    char * mime;

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
    plugin_for_enabled (PLUGIN_TYPE_INPUT, (PluginForEachFunc) probe_func, state);
}

PluginHandle * file_find_decoder (const char * filename, bool_t fast)
{
    ProbeState state;

    AUDDBG ("Probing %s.\n", filename);
    state.plugin = NULL;
    state.filename = filename;
    state.handle = NULL;
    state.failed = FALSE;

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
    if (state.handle != NULL)
        vfs_fclose (state.handle);

    return state.plugin;
}

Tuple * file_read_tuple (const char * filename, PluginHandle * decoder)
{
    InputPlugin * ip = plugin_get_header (decoder);
    g_return_val_if_fail (ip, NULL);
    g_return_val_if_fail (ip->probe_for_tuple, NULL);

    VFSFile * handle = vfs_fopen (filename, "r");
    Tuple * tuple = ip->probe_for_tuple (filename, handle);

    if (handle)
        vfs_fclose (handle);

    return tuple;
}

bool_t file_read_image (const char * filename, PluginHandle * decoder,
 void * * data, int64_t * size)
{
    if (! input_plugin_has_images (decoder))
        return FALSE;

    InputPlugin * ip = plugin_get_header (decoder);
    g_return_val_if_fail (ip, FALSE);
    g_return_val_if_fail (ip->get_song_image, FALSE);

    VFSFile * handle = vfs_fopen (filename, "r");
    bool_t success = ip->get_song_image (filename, handle, data, size);

    if (handle)
        vfs_fclose (handle);

    if (! success)
    {
        * data = NULL;
        * size = 0;
    }

    return success;
}

bool_t file_can_write_tuple (const char * filename, PluginHandle * decoder)
{
    return input_plugin_can_write_tuple (decoder);
}

bool_t file_write_tuple (const char * filename, PluginHandle * decoder,
 const Tuple * tuple)
{
    InputPlugin * ip = plugin_get_header (decoder);
    g_return_val_if_fail (ip, FALSE);
    g_return_val_if_fail (ip->update_song_tuple, FALSE);

    VFSFile * handle = vfs_fopen (filename, "r+");

    if (! handle)
        return FALSE;

    bool_t success = ip->update_song_tuple (tuple, handle);

    if (handle)
        vfs_fclose (handle);

    if (success)
        playlist_rescan_file (filename);

    return success;
}

bool_t custom_infowin (const char * filename, PluginHandle * decoder)
{
    if (! input_plugin_has_infowin (decoder))
        return FALSE;

    InputPlugin * ip = plugin_get_header (decoder);
    g_return_val_if_fail (ip, FALSE);
    g_return_val_if_fail (ip->file_info_box, FALSE);

    ip->file_info_box (filename);
    return TRUE;
}
