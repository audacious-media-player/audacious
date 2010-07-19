/*
 * general.c
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

#include <glib.h>

#include "debug.h"
#include "general.h"
#include "plugin.h"
#include "plugins.h"

static GList * loaded_general_plugins = NULL;

static void general_load (PluginHandle * plugin)
{
    GList * node = g_list_find (loaded_general_plugins, plugin);
    if (node != NULL)
        return;

    AUDDBG ("Loading %s.\n", plugin_get_name (plugin));
    GeneralPlugin * header = plugin_get_header (plugin);
    g_return_if_fail (header != NULL);

    if (header->init != NULL)
        header->init ();

    loaded_general_plugins = g_list_prepend (loaded_general_plugins, plugin);
}

static void general_unload (PluginHandle * plugin)
{
    GList * node = g_list_find (loaded_general_plugins, plugin);
    if (node == NULL)
        return;

    AUDDBG ("Unloading %s.\n", plugin_get_name (plugin));
    GeneralPlugin * header = plugin_get_header (plugin);
    g_return_if_fail (header != NULL);

    loaded_general_plugins = g_list_delete_link (loaded_general_plugins, node);

    if (header->cleanup != NULL)
        header->cleanup ();
}

static gboolean general_init_cb (PluginHandle * plugin)
{
    general_load (plugin);
    return TRUE;
}

void general_init (void)
{
    plugin_for_enabled (PLUGIN_TYPE_GENERAL, (PluginForEachFunc)
     general_init_cb, NULL);
}

void general_cleanup (void)
{
    g_list_foreach (loaded_general_plugins, (GFunc) general_unload, NULL);
}

void general_plugin_enable (PluginHandle * plugin, gboolean enable)
{
    plugin_set_enabled (plugin, enable);

    if (enable)
        general_load (plugin);
    else
        general_unload (plugin);
}
