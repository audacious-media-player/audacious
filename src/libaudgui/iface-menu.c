/*
 * iface-menu.c
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

#include <gtk/gtk.h>
#include <audacious/plugins.h>

#include "config.h"
#include "libaudgui-gtk.h"

static void switch_cb (GtkMenuItem * item, PluginHandle * plugin)
{
    if (gtk_check_menu_item_get_active ((GtkCheckMenuItem *) item))
        aud_iface_plugin_set_active (plugin);
}

typedef struct {
    GtkWidget * menu;
    GSList * group;
    PluginHandle * active_plugin;
} IfaceMenuAddState;

static gboolean add_item_cb (PluginHandle * plugin, IfaceMenuAddState * state)
{
    GtkWidget * item = gtk_radio_menu_item_new_with_label (state->group,
     aud_plugin_get_name (plugin));
    state->group = gtk_radio_menu_item_get_group ((GtkRadioMenuItem *) item);
    if (plugin == state->active_plugin)
        gtk_check_menu_item_set_active ((GtkCheckMenuItem *) item, TRUE);
    gtk_menu_shell_append ((GtkMenuShell *) state->menu, item);
    g_signal_connect (item, "activate", (GCallback) switch_cb, plugin);
    gtk_widget_show (item);
    return TRUE;
}

GtkWidget * audgui_create_iface_menu (void)
{
    IfaceMenuAddState state = {gtk_menu_new (), NULL,
     aud_iface_plugin_get_active ()};
    aud_plugin_for_each (PLUGIN_TYPE_IFACE, (PluginForEachFunc) add_item_cb,
     & state);
    return state.menu;
}
