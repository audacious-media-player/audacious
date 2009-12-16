/*
 * probe.c
 * Copyright 2009 John Lindgren
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

#include "plugin-registry.h"
#include "probe.h"

static InputPlugin * probe_by_scheme (gchar * filename)
{
    InputPlugin * decoder;
    gchar * s = strstr (filename, "://");
    gchar c;

    if (s == NULL)
        return NULL;

    c = s[3];
    s[3] = 0;
    decoder = input_plugin_for_key (INPUT_KEY_SCHEME, s);
    s[3] = c;
    return decoder;
}

static void cut_subtune (gchar * filename)
{
    gchar * s = strrchr (filename, '?');

    if (s != NULL)
        * s = 0;
}

static InputPlugin * probe_by_extension (gchar * filename)
{
    InputPlugin * decoder;
    gchar * s = strrchr (filename, '.');

    if (s == NULL)
        return NULL;

    s = g_ascii_strdown (s + 1, -1);
    decoder = input_plugin_for_key (INPUT_KEY_EXTENSION, s);
    g_free (s);
    return decoder;
}

static InputPlugin * probe_by_mime (VFSFile * handle)
{
    InputPlugin * decoder;
    gchar * mime = vfs_get_metadata (handle, "content-type");

    if (mime == NULL)
        return NULL;

    decoder = input_plugin_for_key (INPUT_KEY_MIME, mime);
    g_free (mime);
    return decoder;
}

typedef struct
{
    const gchar * filename;
    VFSFile * handle;
    InputPlugin * decoder;
}
ProbeState;

static gboolean probe_func (InputPlugin * decoder, void * data)
{
    ProbeState * state = data;

    if (decoder->is_our_file_from_vfs != NULL)
    {
        if (decoder->is_our_file_from_vfs (state->filename, state->handle))
            state->decoder = decoder;

        vfs_fseek (state->handle, 0, SEEK_SET);
    }
    else if (decoder->is_our_file != NULL)
    {
        if (decoder->is_our_file (state->filename))
            state->decoder = decoder;
    }

    return (state->decoder == NULL);
}

static InputPlugin * probe_by_content (gchar * filename, VFSFile * handle)
{
    ProbeState state = {filename, handle, NULL};

    input_plugin_by_priority (probe_func, & state);
    return state.decoder;
}

InputPlugin * file_probe (const gchar * filename, gboolean fast)
{
    InputPlugin * decoder = NULL;
    gchar * temp = g_strdup (filename);
    VFSFile * handle;

    decoder = probe_by_scheme (temp);

    if (decoder != NULL)
        goto DONE;

    cut_subtune (temp);

    decoder = probe_by_extension (temp);

    if (decoder != NULL || fast)
        goto DONE;

    handle = vfs_fopen (temp, "r");

    if (handle == NULL)
        goto DONE;

    decoder = probe_by_mime (handle);

    if (decoder != NULL)
        goto CLOSE;

    decoder = probe_by_content (temp, handle);

CLOSE:
    vfs_fclose (handle);

DONE:
    g_free (temp);
    return decoder;
}
