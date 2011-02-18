/*
 * general.c
 * Copyright 2011 John Lindgren
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

#include <gtk/gtk.h>

#include "debug.h"
#include "general.h"
#include "interface.h"
#include "plugin.h"
#include "plugins.h"
#include "ui_preferences.h"

typedef struct {
    PluginHandle * plugin;
    GeneralPlugin * gp;
    GtkWidget * widget;
} LoadedGeneral;

static gint running = FALSE;
static GList * loaded_general_plugins = NULL;

static gint general_find_cb (LoadedGeneral * general, PluginHandle * plugin)
{
    return (general->plugin == plugin) ? 0 : -1;
}

static void general_load (PluginHandle * plugin)
{
    GList * node = g_list_find_custom (loaded_general_plugins, plugin,
     (GCompareFunc) general_find_cb);
    if (node != NULL)
        return;

    AUDDBG ("Loading %s.\n", plugin_get_name (plugin));
    GeneralPlugin * gp = plugin_get_header (plugin);
    g_return_if_fail (gp != NULL);

    LoadedGeneral * general = g_slice_new (LoadedGeneral);
    general->plugin = plugin;
    general->gp = gp;
    general->widget = NULL;

    if (gp->get_widget != NULL)
        general->widget = gp->get_widget ();

    if (general->widget != NULL)
    {
        AUDDBG ("Adding %s to interface.\n", plugin_get_name (plugin));
        g_signal_connect (general->widget, "destroy", (GCallback)
         gtk_widget_destroyed, & general->widget);
        interface_add_plugin_widget (plugin, general->widget);
    }

    loaded_general_plugins = g_list_prepend (loaded_general_plugins, general);
}

static void general_unload (PluginHandle * plugin)
{
    GList * node = g_list_find_custom (loaded_general_plugins, plugin,
     (GCompareFunc) general_find_cb);
    if (node == NULL)
        return;

    AUDDBG ("Unloading %s.\n", plugin_get_name (plugin));
    LoadedGeneral * general = node->data;
    loaded_general_plugins = g_list_delete_link (loaded_general_plugins, node);

    if (general->widget != NULL)
    {
        AUDDBG ("Removing %s from interface.\n", plugin_get_name (plugin));
        interface_remove_plugin_widget (plugin, general->widget);
        g_return_if_fail (general->widget == NULL); /* not destroyed? */
    }

    g_slice_free (LoadedGeneral, general);
}

static gboolean general_init_cb (PluginHandle * plugin)
{
    general_load (plugin);
    return TRUE;
}

void general_init (void)
{
    g_return_if_fail (! running);
    running = TRUE;

    plugin_for_enabled (PLUGIN_TYPE_GENERAL, (PluginForEachFunc)
     general_init_cb, NULL);
}

static void general_cleanup_cb (LoadedGeneral * general)
{
    general_unload (general->plugin);
}

void general_cleanup (void)
{
    g_return_if_fail (running);
    running = FALSE;

    g_list_foreach (loaded_general_plugins, (GFunc) general_cleanup_cb, NULL);
}

gboolean general_plugin_start (PluginHandle * plugin)
{
    GeneralPlugin * gp = plugin_get_header (plugin);
    g_return_val_if_fail (gp != NULL, FALSE);

    if (gp->init != NULL && ! gp->init ())
        return FALSE;

    if (running)
        general_load (plugin);

    return TRUE;
}

void general_plugin_stop (PluginHandle * plugin)
{
    GeneralPlugin * gp = plugin_get_header (plugin);
    g_return_if_fail (gp != NULL);

    if (running)
        general_unload (plugin);

    if (gp->settings != NULL)
        plugin_preferences_cleanup (gp->settings);
    if (gp->cleanup != NULL)
        gp->cleanup ();
}

PluginHandle * general_plugin_by_widget (/* GtkWidget * */ void * widget)
{
    g_return_val_if_fail (widget, NULL);

    for (GList * node = loaded_general_plugins; node; node = node->next)
    {
        LoadedGeneral * general = node->data;
        if (general->widget == widget)
            return general->plugin;
    }
    
    return NULL;
}
