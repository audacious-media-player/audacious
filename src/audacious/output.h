/*
 * output.h
 * Copyright 2010-2011 John Lindgren
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

#include "plugin.h"

extern const struct OutputAPI output_api;

void output_get_volume(int * l, int * r);
void output_set_volume(int l, int r);

int get_output_time (void);
int get_raw_output_time (void);
void output_drain (void);

PluginHandle * output_plugin_probe (void);
PluginHandle * output_plugin_get_current (void);
bool_t output_plugin_set_current (PluginHandle * plugin);

#endif /* AUDACIOUS_OUTPUT_H */
