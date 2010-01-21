/*
 * plugin-registry.h
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef AUDACIOUS_PLUGIN_REGISTRY_H
#define AUDACIOUS_PLUGIN_REGISTRY_H

#include <glib.h>

#include "plugin.h"

enum {PLUGIN_TYPE_BASIC, PLUGIN_TYPE_INPUT, PLUGIN_TYPE_OUTPUT,
 PLUGIN_TYPE_EFFECT, PLUGIN_TYPE_VIS, PLUGIN_TYPE_IFACE, PLUGIN_TYPE_GENERAL,
 PLUGIN_TYPES};
enum {INPUT_KEY_SCHEME, INPUT_KEY_EXTENSION, INPUT_KEY_MIME, INPUT_KEYS};

void plugin_registry_load (void);
void plugin_registry_prune (void);
void plugin_registry_save (void);

void module_register (const gchar * path);
void plugin_register (const gchar * path, gint type, gint number, void * plugin);
void plugin_get_path (void * plugin, const gchar * * path, gint * type, gint *
 number);
void * plugin_by_path (const gchar * path, gint type, gint number);
void plugin_for_each (gint type, gboolean (* func) (void * plugin, void * data),
 void * data);

void input_plugin_set_enabled (InputPlugin * plugin, gboolean enabled);
gboolean input_plugin_get_enabled (InputPlugin * plugin);
void input_plugin_set_priority (InputPlugin * plugin, gint priority);
void input_plugin_by_priority (gboolean (* func) (InputPlugin * plugin, void *
 data), void * data);
void input_plugin_add_keys (InputPlugin * plugin, gint key, GList * values);
void input_plugin_for_key (gint key, const gchar * value, gboolean (* func)
 (InputPlugin * plugin, void * data), void * data);

void input_plugin_add_scheme_compat (const gchar * scheme, InputPlugin * plugin);
void input_plugin_add_mime_compat (const gchar * mime, InputPlugin * plugin);

void output_plugin_set_priority (OutputPlugin * plugin, gint priority);
void output_plugin_by_priority (gboolean (* func) (OutputPlugin * plugin, void *
 data), void * data);

#endif
