/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious development team
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "interface.h"
#include "input.h"
#include "general.h"
#include "visualization.h"
#include "effect.h"
#include "preferences.h"

static void
print_indent(gint indent)
{
    gint i;
    for (i = 0; i < indent; i++)
        printf(" ");
}

static void
print_value(PreferencesWidget *widget)
{
    switch (widget->cfg_type) {
        case VALUE_INT:
        case VALUE_BOOLEAN:
            printf(" = %d", *((gint*)widget->cfg));
            break;
        case VALUE_STRING:
            printf(" = %s", *((gchar**)widget->cfg));
            break;
        default:
            break;
    }
}

static void
show_preferences(PreferencesWidget *widgets, gint amt, gint indent)
{
    gint x;

    for (x = 0; x < amt; x++) {
        switch (widgets[x].type) {
            case WIDGET_TABLE:
            {
                show_preferences(widgets[x].data.table.elem,
                                 widgets[x].data.table.rows,
                                 indent+2);
                break;
            }
            case WIDGET_BOX:
            {
                if (widgets[x].data.box.frame) {
                    print_indent(indent);
                    printf("%s\n", _(widgets[x].label));
                }
                show_preferences(widgets[x].data.box.elem,
                                 widgets[x].data.box.n_elem,
                                 indent+2);
                break;
            }
            case WIDGET_NOTEBOOK:
            {
                gint i;

                for (i = 0; i < widgets[x].data.notebook.n_tabs; i++) {
                    print_indent(indent);
                    printf("%s\n",
                           widgets[x].data.notebook.tabs[i].name);
                    show_preferences(widgets[x].data.notebook.tabs[i].settings,
                                     widgets[x].data.notebook.tabs[i].n_settings,
                                     indent+2);
                }
            break;
            }
            case WIDGET_CUSTOM:
            {
                print_indent(indent);
                printf("WIDGET_CUSTOM are available only for Gtk+ interfaces\n");
                break;
            }
            default:
            {
                print_indent(indent);
                printf("%s", _(widgets[x].label));
                print_value(&widgets[x]);
                printf("\n");
            }
        }
    }
}

static void
show_plugin_prefs(GList *list)
{
    GList *iter;

    MOWGLI_ITER_FOREACH(iter, list)
    {
        Plugin *plugin = PLUGIN(iter->data);
        if (plugin->settings) {
            printf("  *** %s ***\n", _(plugin->settings->title));
            if (plugin->settings->init)
                plugin->settings->init();
            show_preferences(plugin->settings->prefs, plugin->settings->n_prefs, 4);
            if (plugin->settings->cleanup)
                plugin->settings->cleanup();
        }
    }
}

static void
show_preferenes(gboolean show)
{
    printf(_("Available Plugin Preferences:\n"));
    show_plugin_prefs(get_input_list());
    show_plugin_prefs(get_general_enabled_list());
    show_plugin_prefs(get_vis_enabled_list());
    show_plugin_prefs(get_effect_enabled_list());
}

static gboolean
_ui_initialize(InterfaceCbs *cbs)
{
    cbs->show_prefs_window = show_preferenes;
    g_main_loop_run(g_main_loop_new(NULL, TRUE));

    return TRUE;
}

static Interface headless_interface = {
    .id = "headless",
    .desc = N_("Headless Interface"),
    .init = _ui_initialize,
};

void
ui_populate_headless_interface(void)
{
    interface_register(&headless_interface);
}
