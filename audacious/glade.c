/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include <stdlib.h>

#include "glade.h"


GladeXML *
glade_xml_new_or_die(const gchar * name,
                     const gchar * path,
                     const gchar * root,
                     const gchar * domain)
{
    const gchar *markup =
        N_("<b><big>Unable to create %s.</big></b>\n"
           "\n"
           "Could not open glade file (%s). Please check your "
           "installation.\n");

    GladeXML *xml = glade_xml_new(path, root, domain);

    if (!xml) {
        GtkWidget *dialog =
            gtk_message_dialog_new_with_markup(NULL,
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_CLOSE,
                                               _(markup),
                                               name, path);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        exit(EXIT_FAILURE);
    }

    return xml;
}

GtkWidget *
glade_xml_get_widget_warn(GladeXML * xml, const gchar * name)
{
    GtkWidget *widget = glade_xml_get_widget(xml, name);

    if (!widget) {
        g_warning("Widget not found (%s)", name);
        return NULL;
    }

    return widget;
}


static GCallback
self_symbol_lookup(const gchar * symbol_name)
{
    static GModule *module = NULL;
    gpointer symbol = NULL;

    if (!module)
        module = g_module_open(NULL, 0);

    g_module_symbol(module, symbol_name, &symbol);
    return (GCallback) symbol;
}

static GHashTable *
func_map_to_hash(FuncMap * map)
{
    GHashTable *hash;
    FuncMap *current;

    g_return_val_if_fail(map != NULL, NULL);

    hash = g_hash_table_new(g_str_hash, g_str_equal);

    for (current = map; current->name; current++)
        g_hash_table_insert(hash, current->name, (gpointer) current->function);

    return hash;
}

static void
map_connect_func(const gchar * handler_name,
                 GObject * object,
                 const gchar * signal_name,
                 const gchar * signal_data,
                 GObject * connect_object,
                 gboolean after,
                 gpointer data)
{
    GHashTable *hash = data;
    GCallback callback;

    g_return_if_fail(object != NULL);
    g_return_if_fail(handler_name != NULL);
    g_return_if_fail(signal_name != NULL);

    if (!(callback = self_symbol_lookup(handler_name)))
        callback = (GCallback) g_hash_table_lookup(hash, handler_name);

    if (!callback) {
        g_message("Signal handler (%s) not found", handler_name);
        return;
    }

    if (connect_object) {
        g_signal_connect_object(object, signal_name, callback,
                                connect_object,
                                (after ? G_CONNECT_AFTER : 0) |
                                G_CONNECT_SWAPPED);
    }
    else {
        if (after)
            g_signal_connect_after(object, signal_name, callback, NULL);
        else
            g_signal_connect(object, signal_name, callback, NULL);
    }
}

void
glade_xml_signal_autoconnect_map(GladeXML * xml,
                                 FuncMap * map)
{
    GHashTable *hash;

    g_return_if_fail(xml != NULL);
    g_return_if_fail(map != NULL);

    hash = func_map_to_hash(map);
    glade_xml_signal_autoconnect_full(xml, map_connect_func, hash);
    g_hash_table_destroy(hash);
}
