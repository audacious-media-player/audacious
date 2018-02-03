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
#include <sys/types.h>

#include "index.h"
#include "objects.h"

class InputPlugin;
class Plugin;
class PluginHandle;
class VFSFile;
class Tuple;

typedef bool (* DirForeachFunc) (const char * path, const char * basename, void * user);

/* adder.cc */
void adder_cleanup ();

/* art.cc */
void art_cache_current (const String & filename, Index<char> && data, String && art_file);
void art_clear_current ();
void art_cleanup ();

/* art-search.cc */
String art_search (const char * filename);

/* charset.cc */
void chardet_init ();
void chardet_cleanup ();

/* config.cc */
void config_load ();
void config_save ();
void config_cleanup ();

/* drct.cc */
void record_init ();
void record_cleanup ();

/* effect.cc */
void effect_start (int & channels, int & rate);
Index<float> & effect_process (Index<float> & data);
bool effect_flush (bool force);
Index<float> & effect_finish (Index<float> & data, bool end_of_playlist);
int effect_adjust_delay (int delay);

bool effect_plugin_start (PluginHandle * plugin);
void effect_plugin_stop (PluginHandle * plugin);

/* equalizer.cc */
void eq_init ();
void eq_cleanup ();
void eq_set_format (int new_channels, int new_rate);
void eq_filter (float * data, int samples);

/* eventqueue.cc */
void event_queue_cancel_all ();

/* fft.cc */
void calc_freq (const float data[512], float freq[256]);

/* hook.cc */
void hook_cleanup ();

/* interface.cc */
PluginHandle * iface_plugin_probe ();
PluginHandle * iface_plugin_get_current ();
bool iface_plugin_set_current (PluginHandle * plugin);

void interface_run ();

/* playback.cc */
/* do not call these; use aud_drct_play/stop() instead */
void playback_play (int seek_time, bool pause);
void playback_stop (bool exiting = false);

bool playback_check_serial (int serial);
void playback_set_info (int entry, Tuple && tuple);

/* probe.cc */
bool open_input_file (const char * filename, const char * mode,
 InputPlugin * ip, VFSFile & file, String * error = nullptr);
InputPlugin * load_input_plugin (PluginHandle * decoder, String * error = nullptr);

#define PROBE_FLAG_HAS_DECODER         (1 << 0)
#define PROBE_FLAG_MIGHT_HAVE_SUBTUNES (1 << 1)
int probe_by_filename (const char * filename);

/* runtime.cc */
extern size_t misc_bytes_allocated;

/* strpool.cc */
void string_leak_check ();

/* timer.cc */
void timer_cleanup ();

/* util.cc */
const char * get_home_utf8 ();
bool dir_foreach (const char * path, DirForeachFunc func, void * user_data);
String write_temp_file (const void * data, int64_t len);

bool same_basename (const char * a, const char * b);
const char * last_path_element (const char * path);
void cut_path_element (char * path, int pos);

bool is_cuesheet_entry (const char * filename);
bool is_subtune (const char * filename);
StringBuf strip_subtune (const char * filename);

unsigned int32_hash (unsigned val);
unsigned ptr_hash (const void * ptr);

struct IntHashKey
{
    int val;

    constexpr IntHashKey (int val) :
        val (val) {}
    operator int () const
        { return val; }
    unsigned hash () const
        { return int32_hash (val); }
};

/* vis-runner.cc */
void vis_runner_start_stop (bool playing, bool paused);
void vis_runner_pass_audio (int time, const Index<float> & data, int channels, int rate);
void vis_runner_flush ();
void vis_runner_enable (bool enable);

/* visualization.cc */
void vis_activate (bool activate);
void vis_send_clear ();
void vis_send_audio (const float * data, int channels);

bool vis_plugin_start (PluginHandle * plugin);
void vis_plugin_stop (PluginHandle * plugin);

#endif
