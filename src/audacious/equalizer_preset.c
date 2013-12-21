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

#include <glib.h>
#include <string.h>

#include <libaudcore/audstrings.h>

#include "debug.h"
#include "i18n.h"
#include "interface.h"
#include "misc.h"

static EqualizerPreset * equalizer_preset_new (const char * name)
{
    EqualizerPreset * preset = g_new0 (EqualizerPreset, 1);
    preset->name = g_strdup (name);
    return preset;
}

Index * equalizer_read_presets (const char * basename)
{
    GKeyFile * rcfile = g_key_file_new ();

    char * filename = filename_build (get_path (AUD_PATH_USER_DIR), basename);

    if (! g_key_file_load_from_file (rcfile, filename, G_KEY_FILE_NONE, NULL))
    {
        str_unref (filename);
        filename = filename_build (get_path (AUD_PATH_DATA_DIR), basename);

        if (! g_key_file_load_from_file (rcfile, filename, G_KEY_FILE_NONE, NULL))
        {
            str_unref (filename);
            g_key_file_free (rcfile);
            return NULL;
        }
    }

    str_unref (filename);

    Index * list = index_new ();

    for (int p = 0;; p ++)
    {
        SPRINTF (section, "Preset%d", p);

        char * name = g_key_file_get_string (rcfile, "Presets", section, NULL);
        if (! name)
            break;

        EqualizerPreset * preset = g_new0 (EqualizerPreset, 1);
        preset->name = name;
        preset->preamp = g_key_file_get_double (rcfile, name, "Preamp", NULL);

        for (int i = 0; i < AUD_EQUALIZER_NBANDS; i++)
        {
            SPRINTF (band, "Band%d", i);
            preset->bands[i] = g_key_file_get_double (rcfile, name, band, NULL);
        }

        index_insert (list, -1, preset);
    }

    g_key_file_free (rcfile);

    return list;
}

bool_t equalizer_write_preset_file (Index * list, const char * basename)
{
    GKeyFile * rcfile = g_key_file_new ();

    for (int p = 0; p < index_count (list); p ++)
    {
        EqualizerPreset * preset = index_get (list, p);

        SPRINTF (tmp, "Preset%d", p);
        g_key_file_set_string (rcfile, "Presets", tmp, preset->name);
        g_key_file_set_double (rcfile, preset->name, "Preamp", preset->preamp);

        for (int i = 0; i < AUD_EQUALIZER_NBANDS; i ++)
        {
            SPRINTF (tmp, "Band%d", i);
            g_key_file_set_double (rcfile, preset->name, tmp, preset->bands[i]);
        }
    }

    size_t len;
    char * data = g_key_file_to_data (rcfile, & len, NULL);

    char * filename = filename_build (get_path (AUD_PATH_USER_DIR), basename);
    bool_t success = g_file_set_contents (filename, data, len, NULL);
    str_unref (filename);

    g_key_file_free (rcfile);
    g_free (data);

    return success;
}

Index * import_winamp_eqf (VFSFile * file)
{
    char header[31];
    char bands[11];
    char preset_name[0xb4];

    if (vfs_fread (header, 1, sizeof header, file) != sizeof header ||
     strncmp (header, "Winamp EQ library file v1.1", 27))
        goto error;

    AUDDBG ("The EQF header is OK\n");

    if (vfs_fseek (file, 0x1f, SEEK_SET) < 0)
        goto error;

    Index * list = index_new ();

    while (vfs_fread (preset_name, 1, 0xb4, file) == 0xb4)
    {
        AUDDBG ("The preset name is '%s'\n", preset_name);

        if (vfs_fseek (file, 0x4d, SEEK_CUR)) /* unknown crap --asphyx */
            break;

        if (vfs_fread (bands, 1, 11, file) != 11)
            break;

        EqualizerPreset * preset = equalizer_preset_new (preset_name);

        /* this was divided by 63, but shouldn't it be 64? --majeru */
        preset->preamp = EQUALIZER_MAX_GAIN - ((bands[10] * EQUALIZER_MAX_GAIN * 2) / 64.0);

        for (int i = 0; i < AUD_EQUALIZER_NBANDS; i ++)
            preset->bands[i] = EQUALIZER_MAX_GAIN - ((bands[i] * EQUALIZER_MAX_GAIN * 2) / 64.0);

        index_insert (list, -1, preset);
    }

    return list;

error:;
    SPRINTF (markup, _("Error importing Winamp EQF file '%s'"), vfs_get_filename (file));
    interface_show_error (markup);

    return NULL;
}

bool_t save_preset_file (EqualizerPreset * preset, const char * filename)
{
    GKeyFile * rcfile = g_key_file_new ();

    g_key_file_set_double (rcfile, "Equalizer preset", "Preamp", preset->preamp);

    for (int i = 0; i < AUD_EQUALIZER_NBANDS; i ++)
    {
        SPRINTF (tmp, "Band%d", i);
        g_key_file_set_double (rcfile, "Equalizer preset", tmp, preset->bands[i]);
    }

    size_t len;
    char * data = g_key_file_to_data (rcfile, & len, NULL);

    VFSFile * file = vfs_fopen (filename, "w");
    bool_t success = FALSE;

    if (file)
    {
        success = (vfs_fwrite (data, 1, len, file) == len);
        vfs_fclose (file);
    }

    g_key_file_free (rcfile);
    g_free (data);

    return success;
}

EqualizerPreset * load_preset_file (const char * filename)
{
    GKeyFile * rcfile = g_key_file_new ();

    if (! g_key_file_load_from_file (rcfile, filename, G_KEY_FILE_NONE, NULL))
    {
        g_key_file_free (rcfile);
        return NULL;
    }

    EqualizerPreset * preset = g_new0 (EqualizerPreset, 1);
    preset->name = g_strdup ("");
    preset->preamp = g_key_file_get_double (rcfile, "Equalizer preset", "Preamp", NULL);

    for (int i = 0; i < AUD_EQUALIZER_NBANDS; i ++)
    {
        SPRINTF (tmp, "Band%d", i);
        preset->bands[i] = g_key_file_get_double (rcfile, "Equalizer preset", tmp, NULL);
    }

    g_key_file_free (rcfile);

    return preset;
}
