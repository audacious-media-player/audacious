#ifndef AUDACIOUS_PLUGIN_REGISTRY_H
#define AUDACIOUS_PLUGIN_REGISTRY_H

#include <glib.h>

#include "plugin.h"

enum {PLUGIN_TYPE_BASIC, PLUGIN_TYPE_INPUT, PLUGIN_TYPE_OUTPUT,
 PLUGIN_TYPE_EFFECT, PLUGIN_TYPE_VIS, PLUGIN_TYPE_IFACE, PLUGIN_TYPE_GENERAL,
 PLUGIN_TYPES};
enum {INPUT_KEY_SCHEME, INPUT_KEY_EXTENSION, INPUT_KEY_MIME, INPUT_KEYS};

void plugin_registry_load (void);
void plugin_registry_save (void);

void module_register (const gchar * path);
void plugin_register (const gchar * path, gint type, gint number, void * plugin);
void plugin_for_each (gint type, gboolean (* func) (void * plugin, void * data),
 void * data);

void input_plugin_set_enabled (InputPlugin * plugin, gboolean enabled);
gboolean input_plugin_get_enabled (InputPlugin * plugin);
void input_plugin_set_priority (InputPlugin * plugin, gint priority);
void input_plugin_by_priority (gboolean (* func) (InputPlugin * plugin, void *
 data), void * data);
void input_plugin_add_keys (InputPlugin * plugin, gint key, GList * values);
InputPlugin * input_plugin_for_key (gint key, const gchar * value);

void input_plugin_add_scheme_compat (const gchar * scheme, InputPlugin * plugin);
void input_plugin_add_mime_compat (const gchar * mime, InputPlugin * plugin);

#endif
