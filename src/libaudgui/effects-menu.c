/*
 * effects-menu.c
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

#include "config.h"

#include <audacious/i18n.h>
#include <audacious/plugin.h>
#include <audacious/plugins.h>

#include "libaudgui-gtk.h"

static void enable_cb (GtkCheckMenuItem * item, PluginHandle * plugin)
{
    aud_effect_plugin_enable (plugin, gtk_check_menu_item_get_active (item));

    GtkWidget * settings = g_object_get_data ((GObject *) item, "settings");
    if (settings != NULL)
        gtk_widget_set_sensitive (settings, gtk_check_menu_item_get_active (item));
}

static void settings_cb (GtkMenuItem * settings, PluginHandle * plugin)
{
    if (! aud_plugin_get_enabled (plugin))
        return;

    EffectPlugin * header = aud_plugin_get_header (plugin);
    g_return_if_fail (header != NULL);
    header->configure ();
}

static gboolean add_item_cb (PluginHandle * plugin, GtkWidget * menu)
{
    GtkWidget * item = gtk_check_menu_item_new_with_label (aud_plugin_get_name
     (plugin));
    gtk_check_menu_item_set_active ((GtkCheckMenuItem *) item,
     aud_plugin_get_enabled (plugin));
    gtk_menu_shell_append ((GtkMenuShell *) menu, item);
    g_signal_connect (item, "toggled", (GCallback) enable_cb, plugin);
    gtk_widget_show (item);

    if (aud_plugin_has_configure (plugin))
    {
        GtkWidget * settings = gtk_menu_item_new_with_label (_("settings ..."));
        gtk_widget_set_sensitive (settings, aud_plugin_get_enabled (plugin));
        g_object_set_data ((GObject *) item, "settings", settings);
        gtk_menu_shell_append ((GtkMenuShell *) menu, settings);
        g_signal_connect (settings, "activate", (GCallback) settings_cb, plugin);
        gtk_widget_show (settings);
    }

    return TRUE;
}

GtkWidget * audgui_create_effects_menu (void)
{
    GtkWidget * menu = gtk_menu_new ();
    aud_plugin_for_each (PLUGIN_TYPE_EFFECT, (PluginForEachFunc) add_item_cb,
     menu);
    return menu;
}
