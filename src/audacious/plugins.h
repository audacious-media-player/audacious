/*
 * plugins.h
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

#ifndef AUDACIOUS_PLUGINS_H
#define AUDACIOUS_PLUGINS_H

#include <audacious/api.h>
#include <audacious/types.h>
#include <libaudcore/core.h>

/* returns TRUE to call again for the next plugin, FALSE to stop */
typedef bool_t (* PluginForEachFunc) (PluginHandle * plugin, void * data);

#define AUD_API_NAME PluginsAPI
#define AUD_API_SYMBOL plugins_api

#ifdef _AUDACIOUS_CORE

#include "api-local-begin.h"
#include "plugins-api.h"
#include "api-local-end.h"

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

/* plugin-registry.c */
void plugin_registry_load (void);
void plugin_registry_prune (void);
void plugin_registry_save (void);

void plugin_register (const char * path, int timestamp);

void plugin_set_enabled (PluginHandle * plugin, bool_t enabled);
void * plugin_get_misc_data (PluginHandle * plugin, int size);

PluginHandle * transport_plugin_for_scheme (const char * scheme);
void playlist_plugin_for_ext (const char * ext, PluginForEachFunc func, void * data);
void input_plugin_for_key (int key, const char * value, PluginForEachFunc func, void * data);
bool_t input_plugin_has_images (PluginHandle * plugin);
bool_t input_plugin_has_subtunes (PluginHandle * plugin);
bool_t input_plugin_can_write_tuple (PluginHandle * plugin);
bool_t input_plugin_has_infowin (PluginHandle * plugin);

/* pluginenum.c */
void plugin_system_init (void);
void plugin_system_cleanup (void);
Plugin * plugin_load (const char * path);

#else

#include <audacious/api-define-begin.h>
#include <audacious/plugins-api.h>
#include <audacious/api-define-end.h>

#include <audacious/api-alias-begin.h>
#include <audacious/plugins-api.h>
#include <audacious/api-alias-end.h>

#endif

#undef AUD_API_NAME
#undef AUD_API_SYMBOL

#endif

#ifdef AUD_API_DECLARE

#define AUD_API_NAME PluginsAPI
#define AUD_API_SYMBOL plugins_api

#include "api-define-begin.h"
#include "plugins-api.h"
#include "api-define-end.h"

#include "api-declare-begin.h"
#include "plugins-api.h"
#include "api-declare-end.h"

#undef AUD_API_NAME
#undef AUD_API_SYMBOL

#endif
