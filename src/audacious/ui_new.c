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
#include "playback.h"
#include "playlist.h"
#include "ui_fileopener.h"
#include "ui_new.h"

static GtkWidget *label_prev, *label_current, *label_next;

static gboolean
window_delete()
{
    return FALSE;
}

static void
window_destroy(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

static void
button_open_pressed()
{
    run_filebrowser(TRUE);
}

static void
button_add_pressed()
{
    run_filebrowser(FALSE);
}

static void
button_play_pressed()
{
    if (playlist_get_length(playlist_get_active()))
        playback_initiate();
    else
        button_open_pressed();
}

static void
button_previous_pressed()
{
    playlist_prev(playlist_get_active());
}

static void
button_next_pressed()
{
    playlist_next(playlist_get_active());
}

static void
set_song_title(gpointer hook_data, gpointer user_data)
{
    gchar *title =
        g_strdup_printf("<big>%s</big>",
                        playlist_get_info_text(playlist_get_active()));
    gtk_label_set_text(GTK_LABEL(label_current), title);
    g_object_set(G_OBJECT(label_current), "use-markup", TRUE, NULL);
    g_free(title);
}


static GtkToolItem *
gtk_toolbar_button_add(GtkWidget *toolbar, void(*callback)(), const gchar *stock_id)
{
    GtkToolItem *button = gtk_tool_button_new_from_stock(stock_id);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button, -1);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(callback), NULL);
    return button;
}

static GtkWidget *
gtk_markup_label_new(const gchar *str)
{
    GtkWidget *label = gtk_label_new(str);
    g_object_set(G_OBJECT(label), "use-markup", TRUE, NULL);
    return label;
}

static gboolean
_ui_initialize(void)
{
    GtkWidget *window;		/* the main window */
    GtkWidget *vbox;		/* the main vertical box */
    GtkWidget *buttonbox;	/* box containing buttons like "open", "next" */
    GtkWidget *pcnbox;		/* box containing information about previous,
                               current and next track */

    GtkWidget *chbox;	/* box containing album art and information
                           about current track */
    GtkWidget *cvbox;	/* box containing information about current track
                           and some control elements like position bar */

    GtkToolItem *button_open, *button_add,
                *button_play, *button_previous, *button_next;


    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(window), "delete_event",
                     G_CALLBACK(window_delete), NULL);
    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(window_destroy), NULL);


    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);


    buttonbox = gtk_toolbar_new();
    button_open = gtk_toolbar_button_add(buttonbox, button_open_pressed,
                                     GTK_STOCK_OPEN);
    button_add = gtk_toolbar_button_add(buttonbox, button_add_pressed,
                                    GTK_STOCK_ADD);
    button_play = gtk_toolbar_button_add(buttonbox, button_play_pressed,
                                         GTK_STOCK_MEDIA_PLAY);
    button_previous = gtk_toolbar_button_add(buttonbox, button_previous_pressed,
                                         GTK_STOCK_MEDIA_PREVIOUS);
    button_next = gtk_toolbar_button_add(buttonbox, button_next_pressed,
                                     GTK_STOCK_MEDIA_NEXT);
    gtk_box_pack_start(GTK_BOX(vbox), buttonbox, TRUE, TRUE, 0);


    pcnbox = gtk_vbox_new(FALSE, 0);

    chbox = gtk_hbox_new(FALSE, 0);
    cvbox = gtk_vbox_new(FALSE, 0);
    label_current = gtk_markup_label_new("<big>Current: ?</big>");
    gtk_box_pack_start(GTK_BOX(cvbox), label_current, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(chbox), cvbox, TRUE, TRUE, 0);

    label_prev = gtk_markup_label_new("<small>Previous: ?</small>");
    label_next = gtk_markup_label_new("<small>Next: ?</small>");
    gtk_box_pack_start(GTK_BOX(pcnbox), label_prev, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pcnbox), chbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pcnbox), label_next, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), pcnbox, TRUE, TRUE, 0);

    hook_associate("title change", set_song_title, NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return TRUE;
}

static Interface default_interface = {
    .id = "default",
    .desc = N_("Default Interface"),
    .init = _ui_initialize,
};

Interface *
ui_populate_default_interface(void)
{
    interface_register(&default_interface);

    return &default_interface;
}
