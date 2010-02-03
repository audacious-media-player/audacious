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

#include <libaudcore/audstrings.h>
#include <libaudcore/vfs.h>
#include <stdio.h>
#include <string.h>

#include "plugin-registry.h"
#include "probe.h"

typedef struct
{
    gchar * filename;
    VFSFile * handle;
    InputPlugin * decoder;
}
ProbeState;

static gboolean check_opened (ProbeState * state)
{
    if (state->handle != NULL)
        return TRUE;

    AUDDBG ("Opening %s.\n", state->filename);

    if ((state->handle = vfs_fopen (state->filename, "r")) != NULL)
        return TRUE;

    AUDDBG ("FAILED.\n");
    return FALSE;
}

static gboolean probe_func (InputPlugin * decoder, void * data)
{
    ProbeState * state = data;

    AUDDBG ("Trying %s.\n", decoder->description);

    if (decoder->is_our_file_from_vfs != NULL)
    {
        if (! check_opened (state))
            return FALSE;

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

static gboolean probe_func_fast (InputPlugin * decoder, void * data)
{
    ProbeState * state = data;

    /* For the sake of speed, we do not call is_our_file[_from_vfs] nor do we
       open the file until the second call, when we know we have to decide
       between at least two decoders. */
    if (state->decoder != NULL)
    {
        AUDDBG ("Checking %s.\n", state->decoder->description);

        if (state->decoder->is_our_file_from_vfs != NULL)
        {
            if (! check_opened (state))
            {
                state->decoder = NULL;
                return FALSE;
            }

            if (state->decoder->is_our_file_from_vfs (state->filename,
             state->handle))
                return FALSE;

            vfs_fseek (state->handle, 0, SEEK_SET);
        }
        else if (state->decoder->is_our_file != NULL)
        {
            if (state->decoder->is_our_file (state->filename))
                return FALSE;
        }
    }

    /* The last decoder didn't work; we'll try this one next time.  If we don't
       get called again, we'll assume that this is the right one. */
    AUDDBG ("Guessing %s.\n", decoder->description);
    state->decoder = decoder;
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
    input_plugin_for_key (INPUT_KEY_SCHEME, state->filename, probe_func_fast,
     state);
    s[3] = c;
}

static void probe_by_extension (ProbeState * state)
{
    gchar * s = strrchr (state->filename, '.');

    if (s == NULL)
        return;

    AUDDBG ("Probing by extension.\n");
    s = g_ascii_strdown (s + 1, -1);
    input_plugin_for_key (INPUT_KEY_EXTENSION, s, probe_func_fast, state);
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
    input_plugin_for_key (INPUT_KEY_MIME, mime, probe_func_fast, state);
    g_free (mime);
}

static void probe_by_content (ProbeState * state)
{
    AUDDBG ("Probing by content.\n");
    input_plugin_by_priority (probe_func, state);
}

InputPlugin * file_probe (const gchar * filename, gboolean fast)
{
    ProbeState state;

    AUDDBG ("Probing %s.\n", filename);
    state.decoder = NULL;
    state.filename = filename_split_subtune (filename, NULL);
    state.handle = NULL;

    probe_by_scheme (& state);

    if (state.decoder != NULL)
        goto DONE;

    probe_by_extension (& state);

    if (state.decoder != NULL || fast)
        goto DONE;

    probe_by_mime (& state);

    if (state.decoder != NULL)
        goto DONE;

    probe_by_content (& state);

DONE:
    g_free (state.filename);

    if (state.handle != NULL)
        vfs_fclose (state.handle);

    return state.decoder;
}
