/*
 * plugins.h
 * Copyright 2010 John Lindgren
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

#ifndef AUDACIOUS_PLUGINS_H
#define AUDACIOUS_PLUGINS_H

#include <glib.h>
#include <audacious/api.h>

enum {
 PLUGIN_TYPE_BASIC,
 PLUGIN_TYPE_INPUT,
 PLUGIN_TYPE_OUTPUT,
 PLUGIN_TYPE_EFFECT,
 PLUGIN_TYPE_VIS,
 PLUGIN_TYPE_IFACE,
 PLUGIN_TYPE_GENERAL,
 PLUGIN_TYPES};

typedef struct PluginHandle PluginHandle;
typedef gboolean (* PluginForEachFunc) (PluginHandle * plugin, void * data);

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

void plugin_registry_load (void);
void plugin_registry_prune (void);
void plugin_registry_save (void);

void module_register (const gchar * path);
void plugin_register (const gchar * path, gint type, gint number, void * header);

void input_plugin_for_key (gint key, const gchar * value, PluginForEachFunc
 func, void * data);

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
