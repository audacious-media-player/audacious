/*
 * equalizer_preset.c
 * Copyright 2003-2013 Eugene Zagidullin, William Pitcock, John Lindgren, and
 *                     Thomas Lange
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

#include "equalizer.h"

#include <math.h>
#include <string.h>

#include <glib.h>

#include "audstrings.h"
#include "runtime.h"
#include "vfs.h"

EXPORT Index<EqualizerPreset> aud_eq_read_presets (const char * basename)
{
    Index<EqualizerPreset> list;

    GKeyFile * rcfile = g_key_file_new ();
    String filename = filename_build (aud_get_path (AUD_PATH_USER_DIR), basename);

    if (! g_key_file_load_from_file (rcfile, filename, G_KEY_FILE_NONE, NULL))
    {
        filename = filename_build (aud_get_path (AUD_PATH_DATA_DIR), basename);

        if (! g_key_file_load_from_file (rcfile, filename, G_KEY_FILE_NONE, NULL))
        {
            g_key_file_free (rcfile);
            return list;
        }
    }

    for (int p = 0;; p ++)
    {
        SPRINTF (section, "Preset%d", p);

        char * name = g_key_file_get_string (rcfile, "Presets", section, NULL);
        if (! name)
            break;

        EqualizerPreset & preset = list.append ();

        preset.name = String (name);
        preset.preamp = g_key_file_get_double (rcfile, name, "Preamp", NULL);

        for (int i = 0; i < AUD_EQ_NBANDS; i++)
        {
            SPRINTF (band, "Band%d", i);
            preset.bands[i] = g_key_file_get_double (rcfile, name, band, NULL);
        }

        g_free (name);
    }

    g_key_file_free (rcfile);

    return list;
}

EXPORT bool_t aud_eq_write_presets (const Index<EqualizerPreset> & list, const char * basename)
{
    GKeyFile * rcfile = g_key_file_new ();

    for (int p = 0; p < list.len (); p ++)
    {
        const EqualizerPreset & preset = list[p];

        SPRINTF (tmp, "Preset%d", p);
        g_key_file_set_string (rcfile, "Presets", tmp, preset.name);
        g_key_file_set_double (rcfile, preset.name, "Preamp", preset.preamp);

        for (int i = 0; i < AUD_EQ_NBANDS; i ++)
        {
            SPRINTF (tmp, "Band%d", i);
            g_key_file_set_double (rcfile, preset.name, tmp, preset.bands[i]);
        }
    }

    size_t len;
    char * data = g_key_file_to_data (rcfile, & len, NULL);

    String filename = filename_build (aud_get_path (AUD_PATH_USER_DIR), basename);
    bool_t success = g_file_set_contents (filename, data, len, NULL);

    g_key_file_free (rcfile);
    g_free (data);

    return success;
}

/* Note: Winamp 2.x had a +/- 20 dB range.
 *       Winamp 5.x had a +/- 12 dB range, which we use here. */
#define FROM_WINAMP_VAL(x)  ((31.5 - (x)) * (12.0 / 31.5))
#define TO_WINAMP_VAL(x)  (round (31.5 - (x) * (31.5 / 12.0)))

EXPORT Index<EqualizerPreset> aud_import_winamp_presets (VFSFile * file)
{
    char header[31];
    char bands[11];
    char preset_name[181];

    Index<EqualizerPreset> list;

    if (vfs_fread (header, 1, sizeof header, file) != sizeof header ||
     strncmp (header, "Winamp EQ library file v1.1", 27))
        return list;

    while (vfs_fread (preset_name, 1, 180, file) == 180)
    {
        preset_name[180] = 0; /* protect against buffer overflow */

        if (vfs_fseek (file, 77, SEEK_CUR)) /* unknown crap --asphyx */
            break;

        if (vfs_fread (bands, 1, 11, file) != 11)
            break;

        EqualizerPreset & preset = list.append ();

        preset.name = String (preset_name);
        preset.preamp = FROM_WINAMP_VAL (bands[10]);

        for (int i = 0; i < AUD_EQ_NBANDS; i ++)
            preset.bands[i] = FROM_WINAMP_VAL (bands[i]);
    }

    return list;
}

EXPORT bool_t aud_export_winamp_preset (const EqualizerPreset & preset, VFSFile * file)
{
    char name[257];
    char bands[11];

    if (vfs_fwrite ("Winamp EQ library file v1.1\x1a!--", 1, 31, file) != 31)
        return FALSE;

    strncpy (name, preset.name, 257);

    if (vfs_fwrite (name, 1, 257, file) != 257)
        return FALSE;

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
        bands[i] = TO_WINAMP_VAL (preset.bands[i]);

    bands[10] = TO_WINAMP_VAL (preset.preamp);

    if (vfs_fwrite (bands, 1, 11, file) != 11)
        return FALSE;

    return TRUE;
}

EXPORT bool_t aud_save_preset_file (const EqualizerPreset & preset, const char * filename)
{
    GKeyFile * rcfile = g_key_file_new ();

    g_key_file_set_double (rcfile, "Equalizer preset", "Preamp", preset.preamp);

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
    {
        SPRINTF (tmp, "Band%d", i);
        g_key_file_set_double (rcfile, "Equalizer preset", tmp, preset.bands[i]);
    }

    size_t len;
    char * data = g_key_file_to_data (rcfile, & len, NULL);

    VFSFile * file = vfs_fopen (filename, "w");
    bool_t success = FALSE;

    if (file)
    {
        success = (vfs_fwrite (data, 1, len, file) == (int64_t) len);
        vfs_fclose (file);
    }

    g_key_file_free (rcfile);
    g_free (data);

    return success;
}

EXPORT bool_t aud_load_preset_file (EqualizerPreset & preset, const char * filename)
{
    GKeyFile * rcfile = g_key_file_new ();

    if (! g_key_file_load_from_file (rcfile, filename, G_KEY_FILE_NONE, NULL))
    {
        g_key_file_free (rcfile);
        return FALSE;
    }

    preset.name = String ("");
    preset.preamp = g_key_file_get_double (rcfile, "Equalizer preset", "Preamp", NULL);

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
    {
        SPRINTF (tmp, "Band%d", i);
        preset.bands[i] = g_key_file_get_double (rcfile, "Equalizer preset", tmp, NULL);
    }

    g_key_file_free (rcfile);

    return TRUE;
}
