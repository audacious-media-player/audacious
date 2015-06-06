/*
 * equalizer.h
 * Copyright 2014-2015 John Lindgren
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

#ifndef LIBAUDCORE_EQUALIZER_H
#define LIBAUDCORE_EQUALIZER_H

#include <libaudcore/index.h>
#include <libaudcore/objects.h>

class VFSFile;

#define AUD_EQ_NBANDS 10
#define AUD_EQ_MAX_GAIN 12

struct EqualizerPreset {
    String name;
    float preamp;
    float bands[AUD_EQ_NBANDS];
};

void aud_eq_set_bands (const double values[AUD_EQ_NBANDS]);
void aud_eq_get_bands (double values[AUD_EQ_NBANDS]);
void aud_eq_set_band (int band, double value);
double aud_eq_get_band (int band);

void aud_eq_apply_preset (const EqualizerPreset & preset);
void aud_eq_update_preset (EqualizerPreset & preset);

Index<EqualizerPreset> aud_eq_read_presets (const char * basename);
bool aud_eq_write_presets (const Index<EqualizerPreset> & list, const char * basename);

bool aud_load_preset_file (EqualizerPreset & preset, VFSFile & file);
bool aud_save_preset_file (const EqualizerPreset & preset, VFSFile & file);

Index<EqualizerPreset> aud_import_winamp_presets (VFSFile & file);
bool aud_export_winamp_preset (const EqualizerPreset & preset, VFSFile & file);

#endif /* LIBAUDCORE_EQUALIZER_H */
