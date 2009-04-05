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
#include "ui_manager.h"
#include "ui_playlist_widget.h"

static GtkWidget *label_time;
static GtkWidget *slider;
static GtkWidget *playlist_notebook;

static gulong slider_change_handler_id;
static gboolean slider_is_moving = FALSE;
static gint update_song_timeout_source = 0;

typedef struct {
    gint tab_id;
    GtkWidget *treeview;
} UIPlaylistTab;

static void
ui_playlist_create_tab(Playlist *playlist)
{
    GtkWidget *scrollwin;   /* widget to hold playlist widget */
    GtkWidget *label;
    UIPlaylistTab *tab = g_slice_new(UIPlaylistTab);

    g_return_if_fail(playlist != NULL);

    tab->treeview = ui_playlist_widget_new(playlist);
    scrollwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrollwin), tab->treeview);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin),
                                        GTK_SHADOW_IN);
    gtk_widget_show_all(scrollwin);

    label = gtk_label_new(playlist->title != NULL && *(playlist->title) != '\0' ? playlist->title : playlist->filename);
    tab->tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(playlist_notebook), GTK_WIDGET(scrollwin), GTK_WIDGET(label));

    playlist->ui_data = tab;
}

static void
ui_playlist_destroy_tab(Playlist *playlist)
{
    UIPlaylistTab *tab;

    g_return_if_fail(playlist != NULL);

    tab = playlist->ui_data;
    if (tab != NULL) {
        GList *playlist_iter;

        /* update any tab indexes after this tab. */
        MOWGLI_ITER_FOREACH(playlist_iter, playlist_get_playlists()) {
            Playlist *playlist2 = playlist_iter->data;
            UIPlaylistTab *tab2 = playlist2->ui_data;

            if (tab2->tab_id > tab->tab_id)
                tab2->tab_id--;
        }

        gtk_notebook_remove_page(GTK_NOTEBOOK(playlist_notebook), tab->tab_id);
        g_slice_free(UIPlaylistTab, tab);
    }

    playlist->ui_data = NULL;
}

static void
ui_playlist_change_tab(GtkNotebook *notebook, GtkNotebookPage *page, gint tab_id, gpointer user_data)
{
    GList *playlist_iter;

    MOWGLI_ITER_FOREACH(playlist_iter, playlist_get_playlists()) {
        Playlist *playlist = playlist_iter->data;
        UIPlaylistTab *tab = playlist->ui_data;

        if (tab->tab_id == tab_id) {
            playlist_select_playlist(playlist);
        }
    }
}

static void
ui_populate_playlist_notebook(void)
{
    GList *playlists = playlist_get_playlists(), *playlists_iter;

    MOWGLI_ITER_FOREACH(playlists_iter, playlists) {
        Playlist *playlist = playlists_iter->data;

        ui_playlist_create_tab(playlist);
    }
}

static gboolean
window_configured_cb(gpointer data)
{
    GtkWindow *window = GTK_WINDOW(data);
    gtk_window_get_position(window, &cfg.player_x, &cfg.player_y);
    gtk_window_get_size(window, &cfg.player_width, &cfg.player_height);

    return FALSE;
}

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
button_stop_pressed()
{
    playback_stop();
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
ui_set_song_info(gchar *text, gpointer user_data)
{
    gint length = playback_get_length();
    UIPlaylistTab *tab;
    Playlist *playlist = playlist_get_active();

    /* block "value-changed" signal handler to prevent skipping track
       if the next track is shorter than the previous one --desowin */
    g_signal_handler_block(slider, slider_change_handler_id);
    gtk_range_set_range(GTK_RANGE(slider), 0.0, (gdouble)length);
    g_signal_handler_unblock(slider, slider_change_handler_id);

    gtk_widget_show(label_time);

    tab = playlist->ui_data;
    if (tab != NULL) {
        ui_playlist_widget_set_current(tab->treeview, playlist_get_position(playlist));
        ui_playlist_widget_update(tab->treeview);
    }
}

static void
ui_playlist_created(Playlist *playlist, gpointer user_data)
{
    ui_playlist_create_tab(playlist);
}

static void
ui_playlist_destroyed(Playlist *playlist, gpointer user_data)
{
    ui_playlist_destroy_tab(playlist);    
}

static void
ui_playlist_update(Playlist *playlist, gpointer user_data)
{
    UIPlaylistTab *tab;

    gchar *text = playlist_get_info_text(playlist);
    ui_set_song_info(text, NULL);
    g_free(text);

    if (playlist == NULL)
        return;

    tab = playlist->ui_data;
    if (tab != NULL) {
        ui_playlist_widget_update(tab->treeview);
    }
}

static void
ui_update_time_info(gint time)
{
    gchar text[128];
    gint length = playback_get_length();

    time /= 1000;
    length /= 1000;

    g_snprintf(text, sizeof(text)/sizeof(gchar),
               "<tt><b>%d:%.2d/%d:%.2d</b></tt>",
               time / 60, time % 60,
               length / 60, length % 60);
    gtk_label_set_markup(GTK_LABEL(label_time), text);
}

static gboolean
ui_update_song_info(gpointer hook_data, gpointer user_data)
{
    if (!playback_get_playing())
    {
        gtk_range_set_value(GTK_RANGE(slider), 0.0);
        return FALSE;
    }

    if (slider_is_moving)
        return TRUE;

    gint time = playback_get_time();

    g_signal_handler_block(slider, slider_change_handler_id);
    gtk_range_set_value(GTK_RANGE(slider), (gdouble)time);
    g_signal_handler_unblock(slider, slider_change_handler_id);

    ui_update_time_info(time);

    return TRUE;
}

static void
ui_clear_song_info()
{
    gtk_widget_hide(label_time);
    gtk_range_set_value(GTK_RANGE(slider), 0);
}

static gboolean
ui_slider_value_changed_cb(GtkRange *range, gpointer user_data)
{
    gint seek_;

    seek_ = gtk_range_get_value(range) / 1000;

    /* XXX: work around a horrible bug in playback_seek(), also
       we should do mseek here. --nenolod */
    playback_seek(seek_ != 0 ? seek_ : 1);

    return TRUE;
}

static gboolean
ui_slider_change_value_cb(GtkRange *range, GtkScrollType scroll)
{
    ui_update_time_info(gtk_range_get_value(range));
    return FALSE;
}

static gboolean
ui_slider_button_press_cb(GtkWidget *widget, GdkEventButton *event,
                          gpointer user_data)
{
    slider_is_moving = TRUE;
    return FALSE;
}

static gboolean
ui_slider_button_release_cb(GtkWidget *widget, GdkEventButton *event,
                            gpointer user_data)
{
    slider_is_moving = FALSE;
    return FALSE;
}

static void
ui_playback_begin(gpointer hook_data, gpointer user_data)
{
    ui_update_song_info(NULL, NULL);

    /* update song info 4 times a second */
    update_song_timeout_source =
        g_timeout_add(250, (GSourceFunc) ui_update_song_info, NULL);
}

static void
ui_playback_stop(gpointer hook_data, gpointer user_data)
{
    Playlist *playlist;
    UIPlaylistTab *tab;

    if (update_song_timeout_source) {
        g_source_remove(update_song_timeout_source);
        update_song_timeout_source = 0;
    }

    ui_clear_song_info();

    playlist = playlist_get_active();
    tab = playlist->ui_data;
    if (tab != NULL) {
        ui_playlist_widget_set_current(tab->treeview, -1);
    }
}

static void
ui_playback_end(gpointer hook_data, gpointer user_data)
{
    ui_update_song_info(NULL, NULL);
}

static GtkWidget *
gtk_toolbar_button_add(GtkWidget *toolbar, void(*callback)(),
                       const gchar *stock_id)
{
    GtkWidget *button = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(button), stock_id);
    gtk_button_set_use_stock(GTK_BUTTON(button), TRUE);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

    /* remove label */
    GtkBox *box = GTK_BOX(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(button)))));
    GList *iter;
    for (iter = box->children; iter; iter = g_list_next(iter)) {
        GtkBoxChild *child = (GtkBoxChild *) iter->data;
        if (GTK_IS_LABEL(child->widget)) {
            gtk_label_set_text(GTK_LABEL(child->widget), NULL);
            break;
        }
    }

    gtk_box_pack_start(GTK_BOX(toolbar), button, TRUE, TRUE, 0);
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

static void
ui_hooks_associate(void)
{
    hook_associate("title change", (HookFunction) ui_set_song_info, NULL);
    hook_associate("playback seek", (HookFunction) ui_update_song_info, NULL);
    hook_associate("playback begin", (HookFunction) ui_playback_begin, NULL);
    hook_associate("playback stop", (HookFunction) ui_playback_stop, NULL);
    hook_associate("playback end", (HookFunction) ui_playback_end, NULL);
    hook_associate("playlist create", (HookFunction) ui_playlist_created, NULL);
    hook_associate("playlist destroy", (HookFunction) ui_playlist_destroyed, NULL);
    hook_associate("playlist update", (HookFunction) ui_playlist_update, NULL);
}

static void
ui_hooks_disassociate(void)
{
    hook_dissociate("title change", (HookFunction) ui_set_song_info);
    hook_dissociate("playback seek", (HookFunction) ui_update_song_info);
    hook_dissociate("playback begin", (HookFunction) ui_playback_begin);
    hook_dissociate("playback stop", (HookFunction) ui_playback_stop);
    hook_dissociate("playback end", (HookFunction) ui_playback_end);
    hook_dissociate("playlist create", (HookFunction) ui_playlist_created);
    hook_dissociate("playlist destroy", (HookFunction) ui_playlist_destroyed);
    hook_dissociate("playlist update", (HookFunction) ui_playlist_update);
}

static gboolean
_ui_initialize(void)
{
    Playlist *playlist;
    GtkWidget *window;      /* the main window */
    GtkWidget *vbox;        /* the main vertical box */
    GtkWidget *tophbox;     /* box to contain toolbar and shbox */
    GtkWidget *buttonbox;   /* contains buttons like "open", "next" */
    GtkWidget *shbox;       /* box for slider + time combo --nenolod */
    GtkWidget *button_open, *button_add,
              *button_play, *button_pause, *button_stop,
              *button_previous, *button_next;
    GtkWidget *menu;
    GtkAccelGroup *accel;
    gint x = cfg.player_x;
    gint y = cfg.player_y;

    playlist = playlist_get_active();
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 450, 150);

    if(cfg.save_window_position && cfg.player_width && cfg.player_height)
        gtk_window_resize(GTK_WINDOW(window),
                            cfg.player_width, cfg.player_height);
    if(cfg.save_window_position && cfg.player_x != -1)
        gtk_window_move(GTK_WINDOW(window), x, y);

    g_signal_connect(G_OBJECT(window), "configure-event",
                     G_CALLBACK(window_configured_cb), window);
    g_signal_connect(G_OBJECT(window), "delete-event",
                     G_CALLBACK(window_delete), NULL);
    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(window_destroy), NULL);


    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    menu = ui_manager_get_menus();
    gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, TRUE, 0);

    accel = ui_manager_get_accel_group();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel);

    tophbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), tophbox, FALSE, TRUE, 0);

    buttonbox = gtk_hbox_new(FALSE, 0);
    button_open = gtk_toolbar_button_add(buttonbox, button_open_pressed,
                                         GTK_STOCK_OPEN);
    button_add = gtk_toolbar_button_add(buttonbox, button_add_pressed,
                                        GTK_STOCK_ADD);
    button_play = gtk_toolbar_button_add(buttonbox, button_play_pressed,
                                         GTK_STOCK_MEDIA_PLAY);
    button_pause = gtk_toolbar_button_add(buttonbox, button_pause_pressed,
                                          GTK_STOCK_MEDIA_PAUSE);
    button_stop = gtk_toolbar_button_add(buttonbox, button_stop_pressed,
                                         GTK_STOCK_MEDIA_STOP);
    button_previous = gtk_toolbar_button_add(buttonbox, button_previous_pressed,
                                             GTK_STOCK_MEDIA_PREVIOUS);
    button_next = gtk_toolbar_button_add(buttonbox, button_next_pressed,
                                         GTK_STOCK_MEDIA_NEXT);

    gtk_box_pack_start(GTK_BOX(tophbox), buttonbox, FALSE, FALSE, 0);

    shbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tophbox), shbox, TRUE, TRUE, 0);

    slider = gtk_hscale_new(NULL);
    gtk_scale_set_draw_value(GTK_SCALE(slider), FALSE);
    /* TODO: make this configureable */
    gtk_range_set_update_policy(GTK_RANGE(slider), GTK_UPDATE_DISCONTINUOUS);
    gtk_box_pack_start(GTK_BOX(shbox), slider, TRUE, TRUE, 0);

    label_time = gtk_markup_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(shbox), label_time, FALSE, FALSE, 0);

    playlist_notebook = gtk_notebook_new();
    gtk_box_pack_end(GTK_BOX(vbox), playlist_notebook, TRUE, TRUE, 0);

    ui_hooks_associate();
    ui_populate_playlist_notebook();

    g_signal_connect(playlist_notebook, "switch-page",
                     G_CALLBACK(ui_playlist_change_tab), NULL);

    slider_change_handler_id =
        g_signal_connect(slider, "value-changed",
                         G_CALLBACK(ui_slider_value_changed_cb), NULL);

    g_signal_connect(slider, "change-value",
                     G_CALLBACK(ui_slider_change_value_cb), NULL);
    g_signal_connect(slider, "button-press-event",
                     G_CALLBACK(ui_slider_button_press_cb), NULL);
    g_signal_connect(slider, "button-release-event",
                     G_CALLBACK(ui_slider_button_release_cb), NULL);

    ui_playlist_update(playlist, NULL);

    gtk_widget_show_all(window);

    ui_clear_song_info();

    gtk_main();

    return TRUE;
}

static gboolean
_ui_finalize(void)
{
    ui_hooks_disassociate();
    return TRUE;
}

static Interface default_interface = {
    .id = "newui",
    .desc = N_("New Interface"),
    .init = _ui_initialize,
    .fini = _ui_finalize,
};

Interface *
ui_populate_default_interface(void)
{
    interface_register(&default_interface);

    return &default_interface;
}
