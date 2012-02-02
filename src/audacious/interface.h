/*
 * interface.h
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

#ifndef __AUDACIOUS2_INTERFACE_H__
#define __AUDACIOUS2_INTERFACE_H__

#include <gtk/gtk.h>
#include <audacious/plugins.h>

bool_t interface_load (PluginHandle * plugin);
void interface_unload (void);

void interface_add_plugin_widget (PluginHandle * plugin, GtkWidget * widget);
void interface_remove_plugin_widget (PluginHandle * plugin, GtkWidget * widget);

PluginHandle * iface_plugin_probe (void);
PluginHandle * iface_plugin_get_current (void);
bool_t iface_plugin_set_current (PluginHandle * plugin);

#endif
