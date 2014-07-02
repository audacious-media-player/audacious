/*
 * plugins.h
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

#ifndef LIBAUDCORE_PLUGINS_H
#define LIBAUDCORE_PLUGINS_H

enum {
    PLUGIN_TYPE_TRANSPORT,
    PLUGIN_TYPE_PLAYLIST,
    PLUGIN_TYPE_INPUT,
    PLUGIN_TYPE_EFFECT,
    PLUGIN_TYPE_OUTPUT,
    PLUGIN_TYPE_VIS,
    PLUGIN_TYPE_GENERAL,
    PLUGIN_TYPE_IFACE,
    PLUGIN_TYPES
};

struct PluginHandle;

/* CAUTION: These functions are not thread safe. */

/* returns true to call again for the next plugin, false to stop */
typedef bool (* PluginForEachFunc) (PluginHandle * plugin, void * data);

PluginHandle * aud_plugin_get_current (int type);
bool aud_plugin_enable (PluginHandle * plugin, bool enable);
int aud_plugin_send_message (PluginHandle * plugin, const char * code, const void * data, int size);
void /*GtkWidget*/ * aud_plugin_get_widget (PluginHandle * plugin);

int aud_plugin_get_type (PluginHandle * plugin);
const char * aud_plugin_get_filename (PluginHandle * plugin);
PluginHandle * aud_plugin_lookup (const char * filename);
PluginHandle * aud_plugin_lookup_basename (const char * basename);

const void * aud_plugin_get_header (PluginHandle * plugin);
PluginHandle * aud_plugin_by_header (const void * header);

int aud_plugin_count (int type);
int aud_plugin_get_index (PluginHandle * plugin);
PluginHandle * aud_plugin_by_index (int type, int index);

int aud_plugin_compare (PluginHandle * a, PluginHandle * b);
void aud_plugin_for_each (int type, PluginForEachFunc func, void * data);

bool aud_plugin_get_enabled (PluginHandle * plugin);
void aud_plugin_for_enabled (int type, PluginForEachFunc func, void * data);

const char * aud_plugin_get_name (PluginHandle * plugin);
bool aud_plugin_has_about (PluginHandle * plugin);
bool aud_plugin_has_configure (PluginHandle * plugin);

void aud_plugin_add_watch (PluginHandle * plugin, PluginForEachFunc func, void * data);
void aud_plugin_remove_watch (PluginHandle * plugin, PluginForEachFunc func, void * data);

#endif
