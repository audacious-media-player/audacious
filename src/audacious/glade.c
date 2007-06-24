/*
 * audacious: Cross-platform multimedia player.
 * glade.c: libglade loading and function mapping support
 *
 * Copyright (c) 2005-2007 Audacious development team.
 * Copyright (c) 2003-2005 BMP development team.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
