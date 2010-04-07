/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

/*#define AUD_DEBUG*/

#include <glib.h>
#include <math.h>
#include "config.h"
#include "i18n.h"
#include "main.h"
#include "plugin.h"
#include "rcfile.h"
#include "equalizer.h"
#include "equalizer_preset.h"

EqualizerPreset *
equalizer_preset_new(const gchar * name)
{
    EqualizerPreset *preset = g_new0(EqualizerPreset, 1);
    preset->name = g_strdup(name);
    return preset;
}

void
equalizer_preset_free(EqualizerPreset * preset)
{
    if (!preset)
        return;

    g_free(preset->name);
    g_free(preset);
}

GList *
equalizer_read_presets(const gchar * basename)
{
    gchar *filename, *name;
    RcFile *rcfile;
    GList *list = NULL;
    gint i, p = 0;
    EqualizerPreset *preset;

    /* START mod: add check for the default presets locate in system path ({prefix}/share/audacious)
       by Massimo Cavalleri (submax) */

    filename = g_build_filename(aud_paths[BMP_PATH_USER_DIR], basename, NULL);

    if ((rcfile = aud_rcfile_open(filename)) == NULL) {
        g_free(filename);
        // DATA_DIR = "{prefix}/share/audacious" ; example is "/usr/share/audacious"
        filename = g_build_filename(DATA_DIR, basename, NULL);
        if ((rcfile = aud_rcfile_open(filename)) == NULL) {
           g_free(filename);
           return NULL;
        }
    }

    // END mod

    g_free(filename);

    for (;;) {
        gchar section[32];

        g_snprintf(section, sizeof(section), "Preset%d", p++);
        if (aud_rcfile_read_string(rcfile, "Presets", section, &name)) {
            preset = g_new0(EqualizerPreset, 1);
            preset->name = name;
            aud_rcfile_read_float(rcfile, name, "Preamp", &preset->preamp);
            for (i = 0; i < AUD_EQUALIZER_NBANDS; i++) {
                gchar band[16];
                g_snprintf(band, sizeof(band), "Band%d", i);
                aud_rcfile_read_float(rcfile, name, band, &preset->bands[i]);
            }
            list = g_list_prepend(list, preset);
        }
        else
            break;
    }
    list = g_list_reverse(list);
    aud_rcfile_free(rcfile);
    return list;
}

void
equalizer_write_preset_file(GList * list, const gchar * basename)
{
    gchar *filename, *tmp;
    gint i, p;
    EqualizerPreset *preset;
    RcFile *rcfile;
    GList *node;

    rcfile = aud_rcfile_new();
    p = 0;
    for (node = list; node; node = g_list_next(node)) {
        preset = node->data;
        tmp = g_strdup_printf("Preset%d", p++);
        aud_rcfile_write_string(rcfile, "Presets", tmp, preset->name);
        g_free(tmp);
        aud_rcfile_write_float(rcfile, preset->name, "Preamp",
                               preset->preamp);
        for (i = 0; i < 10; i++) {
            tmp = g_strdup_printf("Band%d\n", i);
            aud_rcfile_write_float(rcfile, preset->name, tmp,
                                   preset->bands[i]);
            g_free(tmp);
        }
    }

    filename = g_build_filename(aud_paths[BMP_PATH_USER_DIR], basename, NULL);
    aud_rcfile_write(rcfile, filename);
    aud_rcfile_free(rcfile);
    g_free(filename);
}

GList *
import_winamp_eqf(VFSFile * file)
{
    gchar header[31];
    gchar bands[11];
    gint i = 0;
    EqualizerPreset *preset = NULL;
    GList *list = NULL;
    gchar *markup;
    gchar *realfn;
    gchar preset_name[0xb4];

    vfs_fread(header, 1, 31, file);
    if (strncmp(header, "Winamp EQ library file v1.1", 27)) goto error;

    AUDDBG("The EQF header is OK\n");

    if (vfs_fseek(file, 0x1f, SEEK_SET) == -1) goto error;

    while (vfs_fread(preset_name, 1, 0xb4, file) == 0xb4) {
        AUDDBG("The preset name is '%s'\n", preset_name);
        vfs_fseek(file, 0x4d, SEEK_CUR); /* unknown crap --asphyx */
        if (vfs_fread(bands, 1, 11, file) != 11) break;

        preset = equalizer_preset_new(preset_name);
        /*this was divided by 63, but shouldn't it be 64? --majeru*/
        preset->preamp = EQUALIZER_MAX_GAIN - ((bands[10] * EQUALIZER_MAX_GAIN * 2) / 64.0);

        for (i = 0; i < 10; i++)
            preset->bands[i] = EQUALIZER_MAX_GAIN - ((bands[i] * EQUALIZER_MAX_GAIN * 2) / 64.0);

        list = g_list_prepend(list, preset);
    }

    list = g_list_reverse(list);
    if (list == NULL) goto error;

    return list;

error:
    realfn = g_filename_from_uri(file->uri, NULL, NULL);
    markup = g_strdup_printf(_("Error importing Winamp EQF file '%s'"),
                             realfn);

    interface_show_error_message(markup);

    g_free(markup);
    g_free(realfn);
    return NULL;
}

void
save_preset_file(EqualizerPreset *preset, const gchar * filename)
{
    RcFile *rcfile;
    gint i;

    rcfile = aud_rcfile_new();
    aud_rcfile_write_float(rcfile, "Equalizer preset", "Preamp",
                           preset->preamp);

    for (i = 0; i < 10; i++) {
        gchar tmp[7];
        g_snprintf(tmp, sizeof(tmp), "Band%d", i);
        aud_rcfile_write_float(rcfile, "Equalizer preset", tmp,
                               preset->bands[i]);
    }

    aud_rcfile_write(rcfile, filename);
    aud_rcfile_free(rcfile);
}

EqualizerPreset *
equalizer_read_aud_preset(const gchar * filename)
{
    gfloat val;
    gint i;
    EqualizerPreset *preset = g_new0(EqualizerPreset, 1);
    preset->name = g_strdup("");

    RcFile *rcfile = aud_rcfile_open(filename);
    if (rcfile == NULL)
        return NULL;

    if (aud_rcfile_read_float(rcfile, "Equalizer preset", "Preamp", &val))
        preset->preamp = val;
    for (i = 0; i < 10; i++) {
        gchar tmp[7];
        g_snprintf(tmp, sizeof(tmp), "Band%d", i);
        if (aud_rcfile_read_float(rcfile, "Equalizer preset", tmp, &val))
            preset->bands[i] = val;
    }
    aud_rcfile_free(rcfile);
    return preset;
}

EqualizerPreset *
load_preset_file(const gchar *filename)
{
    if (filename) {
        EqualizerPreset *preset = equalizer_read_aud_preset(filename);
        return preset;
    }
    return NULL;
}

