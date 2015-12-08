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

#ifndef LIBAUDCORE_OUTPUT_H
#define LIBAUDCORE_OUTPUT_H

#include <libaudcore/audio.h>
#include <libaudcore/objects.h>

class PluginHandle;
class Tuple;

void output_init ();
void output_cleanup ();

bool output_open_audio (const String & filename, const Tuple & tuple,
 int format, int rate, int channels, int start_time);
void output_set_tuple (const Tuple & tuple);
void output_set_replay_gain (const ReplayGainInfo & info);
bool output_write_audio (const void * data, int size, int stop_time);
void output_flush (int time, bool force = false);
void output_resume ();
void output_pause (bool pause);

int output_get_time ();
int output_get_raw_time ();
void output_close_audio ();
void output_drain ();

PluginHandle * output_plugin_get_current ();
PluginHandle * output_plugin_get_secondary ();
bool output_plugin_set_current (PluginHandle * plugin);
bool output_plugin_set_secondary (PluginHandle * plugin);

#endif
