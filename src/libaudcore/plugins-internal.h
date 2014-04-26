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

#ifndef LIBAUDCORE_PLUGINS_INTERNAL_H
#define LIBAUDCORE_PLUGINS_INTERNAL_H

#include "plugins.h"

enum {
 INPUT_KEY_SCHEME,
 INPUT_KEY_EXTENSION,
 INPUT_KEY_MIME,
 INPUT_KEYS};

/* plugin-init.c */
void start_plugins_one (void);
void start_plugins_two (void);
void stop_plugins_two (void);
void stop_plugins_one (void);

/* plugin-load.c */
void plugin_system_init (void);
void plugin_system_cleanup (void);
Plugin * plugin_load (const char * path);

/* plugin-registry.c */
void plugin_registry_load (void);
void plugin_registry_prune (void);
void plugin_registry_save (void);

void plugin_register (const char * path, int timestamp);
void plugin_set_enabled (PluginHandle * plugin, bool_t enabled);

PluginHandle * transport_plugin_for_scheme (const char * scheme);
void playlist_plugin_for_ext (const char * ext, PluginForEachFunc func, void * data);
void input_plugin_for_key (int key, const char * value, PluginForEachFunc func, void * data);
bool_t input_plugin_has_images (PluginHandle * plugin);
bool_t input_plugin_has_subtunes (PluginHandle * plugin);
bool_t input_plugin_can_write_tuple (PluginHandle * plugin);
bool_t input_plugin_has_infowin (PluginHandle * plugin);

#endif
