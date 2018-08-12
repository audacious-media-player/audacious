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

#define AUD_GLIB_INTEGRATION
#include "equalizer.h"

#include <math.h>
#include <string.h>

#include "audstrings.h"
#include "runtime.h"
#include "vfs.h"

EXPORT Index<EqualizerPreset> aud_eq_read_presets (const char * basename)
{
    Index<EqualizerPreset> list;

    GKeyFile * rcfile = g_key_file_new ();
    StringBuf filename = filename_build ({aud_get_path (AudPath::UserDir), basename});

    if (! g_key_file_load_from_file (rcfile, filename, G_KEY_FILE_NONE, nullptr))
    {
        StringBuf filename2 = filename_build ({aud_get_path (AudPath::DataDir), basename});

        if (! g_key_file_load_from_file (rcfile, filename2, G_KEY_FILE_NONE, nullptr))
        {
            g_key_file_free (rcfile);
            return list;
        }
    }

    for (int p = 0;; p ++)
    {
        CharPtr name (g_key_file_get_string (rcfile, "Presets", str_printf ("Preset%d", p), nullptr));
        if (! name)
            break;

        EqualizerPreset & preset = list.append (String (name));
        preset.preamp = g_key_file_get_double (rcfile, name, "Preamp", nullptr);

        for (int i = 0; i < AUD_EQ_NBANDS; i++)
            preset.bands[i] = g_key_file_get_double (rcfile, name, str_printf ("Band%d", i), nullptr);
    }

    g_key_file_free (rcfile);

    return list;
}

EXPORT bool aud_eq_write_presets (const Index<EqualizerPreset> & list, const char * basename)
{
    GKeyFile * rcfile = g_key_file_new ();

    for (int p = 0; p < list.len (); p ++)
    {
        const EqualizerPreset & preset = list[p];

        g_key_file_set_string (rcfile, "Presets", str_printf ("Preset%d", p), preset.name);
        g_key_file_set_double (rcfile, preset.name, "Preamp", preset.preamp);

        for (int i = 0; i < AUD_EQ_NBANDS; i ++)
            g_key_file_set_double (rcfile, preset.name, str_printf ("Band%d", i), preset.bands[i]);
    }

    size_t len;
    CharPtr data (g_key_file_to_data (rcfile, & len, nullptr));

    StringBuf filename = filename_build ({aud_get_path (AudPath::UserDir), basename});
    bool success = g_file_set_contents (filename, data, len, nullptr);

    g_key_file_free (rcfile);

    return success;
}

/* Note: Winamp 2.x had a +/- 20 dB range.
 *       Winamp 5.x had a +/- 12 dB range, which we use here. */
#define FROM_WINAMP_VAL(x)  ((31.5 - (x)) * (12.0 / 31.5))
#define TO_WINAMP_VAL(x)  (round (31.5 - (x) * (31.5 / 12.0)))

EXPORT Index<EqualizerPreset> aud_import_winamp_presets (VFSFile & file)
{
    char header[31];
    char bands[11];
    char preset_name[181];

    Index<EqualizerPreset> list;

    if (file.fread (header, 1, sizeof header) != sizeof header ||
     strncmp (header, "Winamp EQ library file v1.1", 27))
        return list;

    while (file.fread (preset_name, 1, 180) == 180)
    {
        preset_name[180] = 0; /* protect against buffer overflow */

        if (file.fseek (77, VFS_SEEK_CUR)) /* unknown crap --asphyx */
            break;

        if (file.fread (bands, 1, 11) != 11)
            break;

        EqualizerPreset & preset = list.append (String (preset_name));
        preset.preamp = FROM_WINAMP_VAL (bands[10]);

        for (int i = 0; i < AUD_EQ_NBANDS; i ++)
            preset.bands[i] = FROM_WINAMP_VAL (bands[i]);
    }

    return list;
}

EXPORT bool aud_export_winamp_preset (const EqualizerPreset & preset, VFSFile & file)
{
    char name[257];
    char bands[11];

    if (file.fwrite ("Winamp EQ library file v1.1\x1a!--", 1, 31) != 31)
        return false;

    strncpy (name, preset.name, 257);

    if (file.fwrite (name, 1, 257) != 257)
        return false;

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
        bands[i] = TO_WINAMP_VAL (preset.bands[i]);

    bands[10] = TO_WINAMP_VAL (preset.preamp);

    if (file.fwrite (bands, 1, 11) != 11)
        return false;

    return true;
}

EXPORT bool aud_save_preset_file (const EqualizerPreset & preset, VFSFile & file)
{
    GKeyFile * rcfile = g_key_file_new ();

    g_key_file_set_double (rcfile, "Equalizer preset", "Preamp", preset.preamp);

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
        g_key_file_set_double (rcfile, "Equalizer preset",
         str_printf ("Band%d", i), preset.bands[i]);

    size_t len;
    CharPtr data (g_key_file_to_data (rcfile, & len, nullptr));

    bool success = (file.fwrite (data, 1, len) == (int64_t) len);

    g_key_file_free (rcfile);

    return success;
}

EXPORT bool aud_load_preset_file (EqualizerPreset & preset, VFSFile & file)
{
    GKeyFile * rcfile = g_key_file_new ();

    Index<char> data = file.read_all ();

    if (! data.len () || ! g_key_file_load_from_data (rcfile, data.begin (),
     data.len (), G_KEY_FILE_NONE, nullptr))
    {
        g_key_file_free (rcfile);
        return false;
    }

    preset.name = String ("");
    preset.preamp = g_key_file_get_double (rcfile, "Equalizer preset", "Preamp", nullptr);

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
        preset.bands[i] = g_key_file_get_double (rcfile, "Equalizer preset",
         str_printf ("Band%d", i), nullptr);

    g_key_file_free (rcfile);

    return true;
}
