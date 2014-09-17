/*
 * probe.c
 * Copyright 2009-2013 John Lindgren
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

#include "probe.h"

#include <string.h>

#include "audstrings.h"
#include "internal.h"
#include "playlist.h"
#include "plugin.h"
#include "plugins-internal.h"
#include "runtime.h"

struct ProbeState {
    const char * filename;
    VFSFile handle;
    bool failed;
    PluginHandle * plugin;
};

static bool check_opened (ProbeState * state)
{
    if (state->handle)
        return true;
    if (state->failed)
        return false;

    AUDDBG ("Opening %s.\n", state->filename);
    state->handle = probe_buffer_new (state->filename);

    if (state->handle)
        return true;

    AUDWARN ("Failed to open %s.\n", state->filename);
    state->failed = true;
    return false;
}

static bool probe_func (PluginHandle * plugin, ProbeState * state)
{
    AUDINFO ("Trying input plugin %s.\n", aud_plugin_get_name (plugin));

    InputPlugin * decoder = (InputPlugin *) aud_plugin_get_header (plugin);
    if (decoder == nullptr)
        return true;

    if (decoder->is_our_file_from_vfs)
    {
        if (! check_opened (state))
            return false;

        if (decoder->is_our_file_from_vfs (state->filename, state->handle))
        {
            state->plugin = plugin;
            return false;
        }

        if (state->handle.fseek (0, VFS_SEEK_SET) < 0)
            return false;
    }

    return true;
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

static bool probe_func_fast (PluginHandle * plugin, ProbeState * state)
{
    if (state->plugin)
    {
        PluginHandle * prev = state->plugin;
        state->plugin = nullptr;

        if (! probe_func (prev, state))
            return false;
    }

    AUDDBG ("Guessing input plugin %s.\n", aud_plugin_get_name (plugin));
    state->plugin = plugin;
    return true;
}

static void probe_by_scheme (ProbeState * state)
{
    const char * s = strstr (state->filename, "://");
    if (s == nullptr)
        return;

    AUDDBG ("Probing by scheme.\n");
    StringBuf buf = str_copy (state->filename, s - state->filename);
    input_plugin_for_key (INPUT_KEY_SCHEME, buf, (PluginForEachFunc) probe_func_fast, state);
}

static void probe_by_extension (ProbeState * state)
{
    StringBuf buf = uri_get_extension (state->filename);
    if (! buf)
        return;

    AUDDBG ("Probing by extension.\n");
    input_plugin_for_key (INPUT_KEY_EXTENSION, buf, (PluginForEachFunc) probe_func_fast, state);
}

static void probe_by_mime (ProbeState * state)
{
    if (! check_opened (state))
        return;

    String mime = state->handle.get_metadata ("content-type");
    if (! mime)
        return;

    AUDDBG ("Probing by MIME type.\n");
    input_plugin_for_key (INPUT_KEY_MIME, mime, (PluginForEachFunc)
     probe_func_fast, state);
}

static void probe_by_content (ProbeState * state)
{
    AUDDBG ("Probing by content.\n");
    aud_plugin_for_enabled (PLUGIN_TYPE_INPUT, (PluginForEachFunc) probe_func, state);
}

EXPORT PluginHandle * aud_file_find_decoder (const char * filename, bool fast)
{
    ProbeState state = {filename};

    AUDINFO ("Probing %s.\n", filename);

    probe_by_scheme (& state);

    if (state.plugin)
        goto DONE;

    probe_by_extension (& state);

    if (state.plugin || fast)
        goto DONE;

    probe_by_mime (& state);

    if (state.plugin)
        goto DONE;

    probe_by_content (& state);

DONE:
    if (state.plugin)
        AUDINFO ("Probe succeeded: %s\n", aud_plugin_get_name (state.plugin));
    else
        AUDINFO ("Probe failed.\n");

    return state.plugin;
}

static bool open_file (const char * filename, InputPlugin * ip,
 const char * mode, VFSFile & handle)
{
    /* no need to open a handle for custom URI schemes */
    if (ip->schemes && ip->schemes[0])
        return true;

    handle = VFSFile (filename, mode);
    return (bool) handle;
}

EXPORT Tuple aud_file_read_tuple (const char * filename, PluginHandle * decoder)
{
    InputPlugin * ip = (InputPlugin *) aud_plugin_get_header (decoder);
    if (! ip || ! ip->probe_for_tuple)
        return Tuple ();

    VFSFile handle;
    if (! open_file (filename, ip, "r", handle))
        return Tuple ();

    return ip->probe_for_tuple (filename, handle);
}

EXPORT Index<char> aud_file_read_image (const char * filename, PluginHandle * decoder)
{
    if (! input_plugin_has_images (decoder))
        return Index<char> ();

    InputPlugin * ip = (InputPlugin *) aud_plugin_get_header (decoder);
    if (! ip || ! ip->get_song_image)
        return Index<char> ();

    VFSFile handle;
    if (! open_file (filename, ip, "r", handle))
        return Index<char> ();

    return ip->get_song_image (filename, handle);
}

EXPORT bool aud_file_can_write_tuple (const char * filename, PluginHandle * decoder)
{
    return input_plugin_can_write_tuple (decoder);
}

EXPORT bool aud_file_write_tuple (const char * filename,
 PluginHandle * decoder, const Tuple & tuple)
{
    InputPlugin * ip = (InputPlugin *) aud_plugin_get_header (decoder);
    if (! ip || ! ip->update_song_tuple)
        return false;

    VFSFile handle;
    if (! open_file (filename, ip, "r+", handle))
        return false;

    bool success = ip->update_song_tuple (filename, handle, tuple) &&
     (! handle || handle.fflush () == 0);

    if (success)
        aud_playlist_rescan_file (filename);

    return success;
}

EXPORT bool aud_custom_infowin (const char * filename, PluginHandle * decoder)
{
    if (! input_plugin_has_infowin (decoder))
        return false;

    InputPlugin * ip = (InputPlugin *) aud_plugin_get_header (decoder);
    if (! ip || ! ip->file_info_box)
        return false;

    ip->file_info_box (filename);
    return true;
}
