/*
 * output.h
 * Copyright 2010-2013 John Lindgren
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

#ifndef AUDACIOUS_OUTPUT_H
#define AUDACIOUS_OUTPUT_H

#include <libaudcore/core.h>
#include "types.h"

bool_t output_open_audio (int format, int rate, int channels);
void output_set_replaygain_info (const ReplayGainInfo * info);
bool_t output_write_audio (void * data, int size, int stop_time);
void output_abort_write (void);
void output_pause (bool_t pause);
int output_written_time (void);
void output_set_time (int time);

bool_t output_is_open (void);
int output_get_time (void);
int output_get_raw_time (void);
void output_close_audio (void);
void output_drain (void);

void output_get_volume (int * left, int * right);
void output_set_volume (int left, int right);

PluginHandle * output_plugin_probe (void);
PluginHandle * output_plugin_get_current (void);
bool_t output_plugin_set_current (PluginHandle * plugin);

#endif /* AUDACIOUS_OUTPUT_H */
