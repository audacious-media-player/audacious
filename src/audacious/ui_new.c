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

static GtkWidget *label_prev, *label_current, *label_next, *label_time;
static GtkWidget *slider;

static gulong slider_change_handler_id;
static gboolean slider_is_moving = FALSE;
static gint update_song_timeout_source = 0;

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
button_pause_pressed()
{
    playback_pause();
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
ui_set_current_song_title(gchar *text, gpointer user_data)
{
    gchar *title = g_strdup_printf("<big>%s</big>", text);
    gtk_label_set_text(GTK_LABEL(label_current), title);
    g_object_set(G_OBJECT(label_current), "use-markup", TRUE, NULL);
    g_free(title);
}

static void
ui_playlist_update(Playlist *playlist, gpointer user_data)
{
    gchar *text = playlist_get_info_text(playlist);
    ui_set_current_song_title(text, NULL);
    g_free(text);
}

static gboolean
ui_update_song_info(gpointer hook_data, gpointer user_data)
{
    gchar text[128];

    if (!playback_get_playing())
    {
        gtk_range_set_value(GTK_RANGE(slider), (gdouble)0);
        return FALSE;
    }

    if (slider_is_moving)
        return TRUE;

    gint time = playback_get_time();
    gint length = playback_get_length();

    g_signal_handler_block(slider, slider_change_handler_id);
    gtk_range_set_range(GTK_RANGE(slider), 0.0, (gdouble)length);
    gtk_range_set_value(GTK_RANGE(slider), (gdouble)time);
    g_signal_handler_unblock(slider, slider_change_handler_id);

    time /= 1000;
    length /= 1000;

    g_snprintf(text, 128, "<tt><b>%d:%.2d/%d:%.2d</b></tt>", time / 60, time % 60,
        length / 60, length % 60);
    gtk_label_set_markup(GTK_LABEL(label_time), text);

    return TRUE;
}

static gboolean
ui_slider_value_changed_cb(GtkRange *range, gpointer user_data)
{
    /* we are not allowed to do a playback_seek() with values < 1, therefore
     * we add 1 to be on the safe side --mf0102 */
    playback_seek(gtk_range_get_value(range)/1000 + 1);

    return TRUE;
}

static gboolean
ui_slider_button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    slider_is_moving = TRUE;
    return FALSE;
}

static gboolean
ui_slider_button_release_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    slider_is_moving = FALSE;
    return FALSE;
}

static void
ui_playback_begin(gpointer hook_data, gpointer user_data)
{
    ui_update_song_info(NULL, NULL);
    update_song_timeout_source =
        g_timeout_add_seconds(1, (GSourceFunc) ui_update_song_info, NULL);
}

static void
ui_playback_stop(gpointer hook_data, gpointer user_data)
{
    if (update_song_timeout_source) {
        g_source_remove(update_song_timeout_source);
        update_song_timeout_source = 0;
    }
}

static void
ui_playback_end(gpointer hook_data, gpointer user_data)
{
    ui_update_song_info(NULL, NULL);
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
    GtkWidget *shbox;   /* box for slider + time combo --nenolod */

    GtkToolItem *button_open, *button_add,
                *button_play, *button_pause,
                *button_previous, *button_next;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 450, 150);
    g_signal_connect(G_OBJECT(window), "delete_event",
                     G_CALLBACK(window_delete), NULL);
    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(window_destroy), NULL);


    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);


    buttonbox = gtk_toolbar_new();
    gtk_toolbar_set_style(buttonbox, GTK_TOOLBAR_ICONS);
    button_open = gtk_toolbar_button_add(buttonbox, button_open_pressed,
                                         GTK_STOCK_OPEN);
    button_add = gtk_toolbar_button_add(buttonbox, button_add_pressed,
                                        GTK_STOCK_ADD);
    button_play = gtk_toolbar_button_add(buttonbox, button_play_pressed,
                                         GTK_STOCK_MEDIA_PLAY);
    button_pause = gtk_toolbar_button_add(buttonbox, button_pause_pressed,
                                          GTK_STOCK_MEDIA_PAUSE);
    button_previous = gtk_toolbar_button_add(buttonbox, button_previous_pressed,
                                             GTK_STOCK_MEDIA_PREVIOUS);
    button_next = gtk_toolbar_button_add(buttonbox, button_next_pressed,
                                         GTK_STOCK_MEDIA_NEXT);
    gtk_box_pack_start(GTK_BOX(vbox), buttonbox, FALSE, TRUE, 0);


    pcnbox = gtk_vbox_new(FALSE, 0);

    chbox = gtk_hbox_new(FALSE, 0);
    cvbox = gtk_vbox_new(FALSE, 0);
    label_current = gtk_markup_label_new("<big>Current: ?</big>");
    gtk_misc_set_alignment(GTK_MISC(label_current), 0.0, 0.5);
    gtk_box_pack_start(GTK_BOX(cvbox), label_current, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(chbox), cvbox, TRUE, TRUE, 0);

    label_prev = gtk_markup_label_new("<small>Previous: ?</small>");
    label_next = gtk_markup_label_new("<small>Next: ?</small>");
    gtk_misc_set_alignment(GTK_MISC(label_prev), 0.0, 0.5);
    gtk_misc_set_alignment(GTK_MISC(label_next), 1.0, 0.5);
    gtk_box_pack_start(GTK_BOX(pcnbox), label_prev, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pcnbox), chbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pcnbox), label_next, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), pcnbox, TRUE, TRUE, 0);

    shbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_end(GTK_BOX(cvbox), shbox, TRUE, TRUE, 0);

    slider = gtk_hscale_new(NULL);
    gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
    /* TODO: make this configureable */
    gtk_range_set_update_policy(GTK_RANGE(slider), GTK_UPDATE_DELAYED);
    gtk_box_pack_start(GTK_BOX(shbox), slider, TRUE, TRUE, 0);

    label_time = gtk_markup_label_new("<tt><b>0:00/0:00</b></tt>");
    gtk_box_pack_start(GTK_BOX(shbox), label_time, FALSE, FALSE, 0);

    hook_associate("title change", (HookFunction) ui_set_current_song_title, NULL);
    hook_associate("playback seek", (HookFunction) ui_update_song_info, NULL);
    hook_associate("playback begin", (HookFunction) ui_playback_begin, NULL);
    hook_associate("playback stop", (HookFunction) ui_playback_stop, NULL);
    hook_associate("playback end", (HookFunction) ui_playback_end, NULL);
    hook_associate("playlist update", (HookFunction) ui_playlist_update, NULL);

    slider_change_handler_id =
        g_signal_connect(slider, "value-changed",
                         G_CALLBACK(ui_slider_value_changed_cb), NULL);

    g_signal_connect(slider, "button-press-event",
                     G_CALLBACK(ui_slider_button_press_cb), NULL);
    g_signal_connect(slider, "button-release-event",
                     G_CALLBACK(ui_slider_button_release_cb), NULL);

    ui_playlist_update(playlist_get_active(), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return TRUE;
}

static gboolean
_ui_finalize(void)
{
    hook_dissociate("title change", (HookFunction) ui_set_current_song_title);
    hook_dissociate("playback seek", (HookFunction) ui_update_song_info);
    hook_dissociate("playback begin", (HookFunction) ui_playback_begin);
    hook_dissociate("playback stop", (HookFunction) ui_playback_stop);
    hook_dissociate("playback end", (HookFunction) ui_playback_end);
    hook_dissociate("playlist update", (HookFunction) ui_playlist_update);

    return TRUE;
}

static Interface default_interface = {
    .id = "default",
    .desc = N_("Default Interface"),
    .init = _ui_initialize,
    .fini = _ui_finalize,
};

Interface *
ui_populate_default_interface(void)
{
    interface_register(&default_interface);

    return &default_interface;
}
