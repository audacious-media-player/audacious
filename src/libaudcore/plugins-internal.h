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
#include "objects.h"

enum class InputKey;
class Plugin;

enum class PluginEnabled {
    Disabled = 0,
    Primary = 1,
    Secondary = 2
};

/* plugin-init.c */
void start_plugins_one ();
void start_plugins_two ();
void stop_plugins_two ();
void stop_plugins_one ();

bool plugin_enable_secondary (PluginHandle * plugin, bool enable);

/* plugin-load.c */
void plugin_system_init ();
void plugin_system_cleanup ();
bool plugin_check_flags (int flags);
Plugin * plugin_load (const char * path);

/* plugin-registry.c */
void plugin_registry_load ();
void plugin_registry_prune ();
void plugin_registry_save ();
void plugin_registry_cleanup ();

void plugin_register (const char * path, int timestamp);
PluginEnabled plugin_get_enabled (PluginHandle * plugin);
void plugin_set_enabled (PluginHandle * plugin, PluginEnabled enabled);
void plugin_set_failed (PluginHandle * plugin);

bool transport_plugin_has_scheme (PluginHandle * plugin, const char * scheme);
bool playlist_plugin_can_save (PluginHandle * plugin);
const Index<String> & playlist_plugin_get_exts (PluginHandle * plugin);
bool playlist_plugin_has_ext (PluginHandle * plugin, const char * ext);
bool input_plugin_has_key (PluginHandle * plugin, InputKey key, const char * value);
bool input_plugin_has_subtunes (PluginHandle * plugin);
bool input_plugin_can_write_tuple (PluginHandle * plugin);

#endif
