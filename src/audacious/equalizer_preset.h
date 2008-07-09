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

#ifndef AUDACIOUS_EQUALIZER_PRESET_H
#define AUDACIOUS_EQUALIZER_PRESET_H

#include "audacious/rcfile.h"

struct _EqualizerPreset {
    gchar *name;
    gfloat preamp, bands[10];
};

typedef struct _EqualizerPreset EqualizerPreset;

EqualizerPreset * equalizer_preset_new(const gchar * name);
void    equalizer_preset_free(EqualizerPreset * preset);
GList * equalizer_read_presets(const gchar * basename);
void    equalizer_write_preset_file(GList * list, const gchar * basename);
GList * import_winamp_eqf(VFSFile * file);
void    save_preset_file(EqualizerPreset *preset, const gchar * filename);

EqualizerPreset * equalizer_read_aud_preset(const gchar * filename);
EqualizerPreset * load_preset_file(const gchar *filename);

#endif
