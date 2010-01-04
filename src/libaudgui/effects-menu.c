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

#include "libaudgui-gtk.h"

static GtkWidget * create_title (void)
{
    GtkWidget * title = gtk_menu_item_new_with_label (_("Effects"));

    gtk_widget_set_sensitive (title, FALSE);
    gtk_widget_show (title);
    return title;
}

static void effect_item_toggled (GtkCheckMenuItem * item, void * data)
{
    aud_enable_effect (data, gtk_check_menu_item_get_active (item));
}

static GtkWidget * create_effect_item (EffectPlugin * effect)
{
    GtkWidget * item = gtk_check_menu_item_new_with_label (effect->description);

    gtk_check_menu_item_set_active ((GtkCheckMenuItem *) item, effect->enabled);
    gtk_widget_show (item);
    g_signal_connect (item, "toggled", (GCallback) effect_item_toggled, effect);
    return item;
}

static void settings_item_activate (GtkCheckMenuItem * item, void * data)
{
    ((EffectPlugin *) data)->configure ();
}

static GtkWidget * create_settings_item (EffectPlugin * effect)
{
    GtkWidget * item = gtk_menu_item_new_with_label (_("settings ..."));

    gtk_widget_show (item);
    g_signal_connect (item, "activate", (GCallback) settings_item_activate,
     effect);
    return item;
}

GtkWidget * audgui_create_effects_menu (void)
{
    GtkWidget * menu = gtk_menu_new ();
    GList * list = aud_get_effect_list ();
    GList * node;

    gtk_menu_shell_append ((GtkMenuShell *) menu, create_title ());
    gtk_menu_shell_append ((GtkMenuShell *) menu, gtk_separator_menu_item_new ());

    for (node = list; node != NULL; node = node->next)
    {
        EffectPlugin * effect = node->data;

        gtk_menu_shell_append ((GtkMenuShell *) menu, create_effect_item (effect));

        if (effect->configure != NULL)
            gtk_menu_shell_append ((GtkMenuShell *) menu, create_settings_item
             (effect));
    }

    return menu;
}
