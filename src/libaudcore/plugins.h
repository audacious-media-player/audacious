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

#include <libaudcore/index.h>

enum class PluginType {
    Transport,
    Playlist,
    Input,
    Effect,
    Output,
    Vis,
    General,
    Iface,
    count
};

class PluginHandle;

/* CAUTION: These functions are not thread safe. */

PluginHandle * aud_plugin_get_current (PluginType type);
bool aud_plugin_get_enabled (PluginHandle * plugin);
bool aud_plugin_enable (PluginHandle * plugin, bool enable);

int aud_plugin_send_message (PluginHandle * plugin, const char * code, const void * data, int size);
void * aud_plugin_get_gtk_widget (PluginHandle * plugin);  // returns (GtkWidget *)
void * aud_plugin_get_qt_widget (PluginHandle * plugin);  // return (QWidget *)

PluginType aud_plugin_get_type (PluginHandle * plugin);
const char * aud_plugin_get_basename (PluginHandle * plugin);
PluginHandle * aud_plugin_lookup_basename (const char * basename);

const void * aud_plugin_get_header (PluginHandle * plugin);
PluginHandle * aud_plugin_by_header (const void * header);

const Index<PluginHandle *> & aud_plugin_list (PluginType type);

const char * aud_plugin_get_name (PluginHandle * plugin);
bool aud_plugin_has_about (PluginHandle * plugin);
bool aud_plugin_has_configure (PluginHandle * plugin);

/* returns true to continue watching, false to stop */
typedef bool (* PluginWatchFunc) (PluginHandle * plugin, void * data);

void aud_plugin_add_watch (PluginHandle * plugin, PluginWatchFunc func, void * data);
void aud_plugin_remove_watch (PluginHandle * plugin, PluginWatchFunc func, void * data);

#endif
