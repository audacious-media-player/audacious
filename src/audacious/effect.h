/*
 * effect.h
 * Copyright 2010-2012 John Lindgren
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

#ifndef AUDACIOUS_EFFECT_H
#define AUDACIOUS_EFFECT_H

#include <libaudcore/core.h>

#include "types.h"

void effect_start (int * channels, int * rate);
void effect_process (float * * data, int * samples);
void effect_flush (void);
void effect_finish (float * * data, int * samples);
int effect_adjust_delay (int delay);

bool_t effect_plugin_start (PluginHandle * plugin);
void effect_plugin_stop (PluginHandle * plugin);

#endif
