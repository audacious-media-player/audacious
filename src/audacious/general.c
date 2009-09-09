/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#include <glib.h>
#include <string.h>
#include "plugin.h"
#include "pluginenum.h"
#include "general.h"

GeneralPluginData gp_data = {
    NULL,
    NULL
};

GList *
get_general_list(void)
{
    return gp_data.general_list;
}

GList *
get_general_enabled_list(void)
{
    return gp_data.enabled_list;
}

void
general_enable_plugin(GeneralPlugin *plugin, gboolean enable)
{
    g_return_if_fail(plugin != NULL);

    if (enable && !plugin->enabled) {
        gp_data.enabled_list = g_list_append(gp_data.enabled_list, plugin);
        if (plugin->init)
	{
	    plugin_set_current((Plugin *)plugin);
            plugin->init();
	}
    }
    else if (!enable && plugin->enabled) {
        gp_data.enabled_list = g_list_remove(gp_data.enabled_list, plugin);
        if (plugin->cleanup)
	{
	    plugin_set_current((Plugin *)plugin);
            plugin->cleanup();
	}
    }

    plugin->enabled = enable;
}

gchar *
general_stringify_enabled_list(void)
{
    GString *enable_str;
    gchar *name;
    GList *node = get_general_enabled_list();

    if (!node)
        return NULL;

    name = g_path_get_basename(GENERAL_PLUGIN(node->data)->filename);
    enable_str = g_string_new(name);
    g_free(name);

    for (node = g_list_next(node); node; node = g_list_next(node)) {
        name = g_path_get_basename(GENERAL_PLUGIN(node->data)->filename);
        g_string_append_c(enable_str, ',');
        g_string_append(enable_str, name);
        g_free(name);
    }

    return g_string_free(enable_str, FALSE);
}

void
general_enable_from_stringified_list(const gchar * list_str)
{
    gchar **list, **str;
    GeneralPlugin *plugin;

    if (!list_str || !strcmp(list_str, ""))
        return;

    list = g_strsplit(list_str, ",", 0);

    for (str = list; *str; str++) {
        GList *node;

        for (node = get_general_list(); node; node = g_list_next(node)) {
            gchar *base;

            base = g_path_get_basename(GENERAL_PLUGIN(node->data)->filename);

            if (!strcmp(*str, base)) {
                plugin = GENERAL_PLUGIN(node->data);
                plugin->enabled = TRUE;

                gp_data.enabled_list = g_list_append(gp_data.enabled_list,
                                                      plugin);
                if (plugin->init)
		{
	            plugin_set_current((Plugin *)plugin);
                    plugin->init();
		}
            }

            g_free(base);
        }
    }

    g_strfreev(list);
}
