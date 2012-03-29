/*
 * ui_urlopener.c
 * Copyright 1998-2003 XMMS development team
 * Copyright 2003-2004 BMP development team
 * Copyright 2008-2011 Tomasz Moń and John Lindgren
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include <glib.h>
#include <gtk/gtk.h>

#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>
#include <audacious/drct.h>
#include <audacious/misc.h>

#include "config.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static void urlopener_add_url_callback (GtkWidget * widget, GtkEntry * entry)
{
    aud_history_add (gtk_entry_get_text (entry));
}

static GtkWidget * urlopener_add_url_dialog_new (GCallback func, bool_t open)
{
    GtkWidget * win, * vbox, * bbox, * cancel, * ok, * combo, * entry;

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) win, open ? _("Open URL") : _("Add URL"));
    gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(win), 400, -1);
    gtk_container_set_border_width(GTK_CONTAINER(win), 12);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    combo = gtk_combo_box_text_new_with_entry ();
    gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);

    entry = gtk_bin_get_child(GTK_BIN(combo));
    gtk_window_set_focus(GTK_WINDOW(win), entry);
    gtk_entry_set_text(GTK_ENTRY(entry), "");

    const char * path;
    for (int i = 0; (path = aud_history_get (i)); i ++)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) combo, path);

    g_signal_connect(entry, "activate",
                     G_CALLBACK(urlopener_add_url_callback),
                     entry);
    g_signal_connect (entry, "activate", func, entry);
    g_signal_connect_swapped(entry, "activate",
                             G_CALLBACK(gtk_widget_destroy),
                             win);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    cancel = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_box_pack_start(GTK_BOX(bbox), cancel, FALSE, FALSE, 0);
    gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(bbox), cancel, TRUE);

    g_signal_connect_swapped(cancel, "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             win);

    ok = gtk_button_new_from_stock (open ? GTK_STOCK_OPEN : GTK_STOCK_ADD);
    g_signal_connect(ok, "clicked",
                     G_CALLBACK(urlopener_add_url_callback), entry);
    g_signal_connect (ok, "clicked", func, entry);
    g_signal_connect_swapped(ok, "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             win);
    gtk_box_pack_start(GTK_BOX(bbox), ok, FALSE, FALSE, 0);

    gtk_widget_show_all(vbox);

    return win;
}

static void
on_add_url_add_clicked(GtkWidget * widget,
                       GtkWidget * entry)
{
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));

    if (text != NULL && * text)
        aud_drct_pl_add (text, -1);
}

static void
on_add_url_ok_clicked(GtkWidget * widget,
                      GtkWidget * entry)
{
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));

    if (text != NULL && * text)
        aud_drct_pl_open (text);
}

EXPORT void audgui_show_add_url_window (bool_t open)
{
    static GtkWidget *url_window = NULL;

    if (!url_window) {
        url_window = urlopener_add_url_dialog_new (open ? (GCallback)
         on_add_url_ok_clicked : (GCallback) on_add_url_add_clicked, open);

        audgui_destroy_on_escape (url_window);
        g_signal_connect(url_window, "destroy",
                         G_CALLBACK(gtk_widget_destroyed),
                         &url_window);
    }

    gtk_window_present(GTK_WINDOW(url_window));
}
