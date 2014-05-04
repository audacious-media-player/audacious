/*
 * internal.h
 * Copyright 2014 John Lindgren
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

#ifndef LIBAUDCORE_INTERNAL_H
#define LIBAUDCORE_INTERNAL_H

#include <stdint.h>

#include "index.h"
#include "objects.h"

typedef bool_t (* DirForeachFunc) (const char * path, const char * basename, void * user);

/* adder.c */
void adder_cleanup (void);

/* art.c */
void art_init (void);
void art_cleanup (void);

/* art-search.c */
String art_search (const char * filename);

/* charset.c */
void chardet_init (void);
void chardet_cleanup (void);

/* config.c */
void config_load (void);
void config_save (void);
void config_cleanup (void);

/* effect.c */
void effect_start (int * channels, int * rate);
void effect_process (float * * data, int * samples);
void effect_flush (void);
void effect_finish (float * * data, int * samples);
int effect_adjust_delay (int delay);

bool_t effect_plugin_start (PluginHandle * plugin);
void effect_plugin_stop (PluginHandle * plugin);

/* equalizer.c */
void eq_init (void);
void eq_cleanup (void);
void eq_set_format (int new_channels, int new_rate);
void eq_filter (float * data, int samples);

/* fft.c */
void calc_freq (const float data[512], float freq[256]);

/* interface.c */
PluginHandle * iface_plugin_probe (void);
PluginHandle * iface_plugin_get_current (void);
bool_t iface_plugin_set_current (PluginHandle * plugin);

void interface_run (void);

/* playback.c */
/* do not call these; use aud_drct_play/stop() instead */
void playback_play (int seek_time, bool_t pause);
void playback_stop (void);

/* probe-buffer.c */
VFSFile * probe_buffer_new (const char * filename);

/* util.c */
bool_t dir_foreach (const char * path, DirForeachFunc func, void * user_data);
String write_temp_file (void * data, int64_t len);

void describe_song (const char * filename, const Tuple * tuple, String & title,
 String & artist, String & album);
const char * last_path_element (const char * path);

unsigned int32_hash (unsigned val);

/* vis-runner.c */
void vis_runner_start_stop (bool_t playing, bool_t paused);
void vis_runner_pass_audio (int time, float * data, int samples, int channels, int rate);
void vis_runner_flush (void);
void vis_runner_enable (bool_t enable);

/* visualization.c */
void vis_activate (bool_t activate);
void vis_send_clear (void);
void vis_send_audio (const float * data, int channels);

bool_t vis_plugin_start (PluginHandle * plugin);
void vis_plugin_stop (PluginHandle * plugin);

#endif
