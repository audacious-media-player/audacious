/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* GDK including */
#include "platform/smartinclude.h"

#if defined(USE_REGEX_ONIGURUMA)
#include <onigposix.h>
#elif defined(USE_REGEX_PCRE)
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#include "configdb.h"
#include "input.h"
#include "main.h"
#include "playback.h"
#include "playlist.h"
#include "ui_credits.h"
#include "ui_fileinfo.h"
#include "ui_fileopener.h"
#include "ui_urlopener.h"
#include "ui_jumptotrack.h"
#include "ui_preferences.h"
#include "ui_playlist_manager.h"
#include "equalizer_flow.h"
#include "util.h"
#include "audstrings.h"

static GtkWidget *mainwin_jtt = NULL;

static int ab_position_a = -1;
static int ab_position_b = -1;

/* toggleactionentries actions */

void
action_anamode_peaks( GtkToggleAction * action )
{
    cfg.analyzer_peaks = gtk_toggle_action_get_active( action );
}

void
action_playback_noplaylistadvance( GtkToggleAction * action )
{
    cfg.no_playlist_advance = gtk_toggle_action_get_active( action );
}

void
action_playback_repeat( GtkToggleAction * action )
{
    cfg.repeat = gtk_toggle_action_get_active( action );
}

void
action_playback_shuffle( GtkToggleAction * action )
{
    cfg.shuffle = gtk_toggle_action_get_active( action );
    playlist_set_shuffle(cfg.shuffle);
}

void
action_stop_after_current_song( GtkToggleAction * action )
{
    cfg.stopaftersong = gtk_toggle_action_get_active( action );
}

/* actionentries actions */

void
action_about_audacious( void )
{
    show_about_window();
}

void
action_play_file( void )
{
    run_filebrowser(TRUE);
}

void
action_play_location( void )
{
    show_add_url_window();
}

void
action_ab_set( void )
{
    Playlist *playlist = playlist_get_active();
    if (playlist_get_current_length(playlist) != -1)
    {
        if (ab_position_a == -1)
        {
            ab_position_a = playback_get_time();
            ab_position_b = -1;
            /* info-text: Loop-Point A position set */
        }
        else if (ab_position_b == -1)
        {
            int time = playback_get_time();
            if (time > ab_position_a)
                ab_position_b = time;
            /* release info text */
        }
        else
        {
            ab_position_a = playback_get_time();
            ab_position_b = -1;
            /* info-text: Loop-Point A position reset */
        }
    }
}

void
action_ab_clear( void )
{
    Playlist *playlist = playlist_get_active();
    if (playlist_get_current_length(playlist) != -1)
    {
        ab_position_a = ab_position_b = -1;
        /* release info text */
    }
}

void
action_current_track_info( void )
{
    ui_fileinfo_show_current(playlist_get_active());
}

void
action_jump_to_file( void )
{
    ui_jump_to_track();
}

void
action_jump_to_playlist_start( void )
{
    Playlist *playlist = playlist_get_active();
    playlist_set_position(playlist, 0);
}

static void
mainwin_jump_to_time_cb(GtkWidget * widget,
                        GtkWidget * entry)
{
    guint min = 0, sec = 0, params;
    gint time;
    Playlist *playlist = playlist_get_active();

    params = sscanf(gtk_entry_get_text(GTK_ENTRY(entry)), "%u:%u",
                    &min, &sec);
    if (params == 2)
        time = (min * 60) + sec;
    else if (params == 1)
        time = min;
    else
        return;

    if (playlist_get_current_length(playlist) > -1 &&
        time <= (playlist_get_current_length(playlist) / 1000))
    {
        playback_seek(time);
        gtk_widget_destroy(mainwin_jtt);
    }
}


void
mainwin_jump_to_time(void)
{
    GtkWidget *vbox, *hbox_new, *hbox_total;
    GtkWidget *time_entry, *label, *bbox, *jump, *cancel;
    GtkWidget *dialog;
    guint tindex;
    gchar time_str[10];

    if (!playback_get_playing()) {
        dialog =
            gtk_message_dialog_new (NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_CLOSE,
                                    _("Can't jump to time when no track is being played.\n"));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return;
    }

    if (mainwin_jtt) {
        gtk_window_present(GTK_WINDOW(mainwin_jtt));
        return;
    }

    mainwin_jtt = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(mainwin_jtt),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_window_set_title(GTK_WINDOW(mainwin_jtt), _("Jump to Time"));
    gtk_window_set_position(GTK_WINDOW(mainwin_jtt), GTK_WIN_POS_CENTER);

    g_signal_connect(mainwin_jtt, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &mainwin_jtt);
    gtk_container_set_border_width(GTK_CONTAINER(mainwin_jtt), 10);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(mainwin_jtt), vbox);

    hbox_new = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_new, TRUE, TRUE, 5);

    time_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox_new), time_entry, FALSE, FALSE, 5);
    g_signal_connect(time_entry, "activate",
                     G_CALLBACK(mainwin_jump_to_time_cb), time_entry);

    gtk_widget_set_size_request(time_entry, 70, -1);
    label = gtk_label_new(_("minutes:seconds"));
    gtk_box_pack_start(GTK_BOX(hbox_new), label, FALSE, FALSE, 5);

    hbox_total = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_total, TRUE, TRUE, 5);
    gtk_widget_show(hbox_total);

    /* FIXME: Disable display of current track length. It's not
       updated when track changes */
#if 0
    label = gtk_label_new(_("Track length:"));
    gtk_box_pack_start(GTK_BOX(hbox_total), label, FALSE, FALSE, 5);

    len = playlist_get_current_length() / 1000;
    g_snprintf(time_str, sizeof(time_str), "%u:%2.2u", len / 60, len % 60);
    label = gtk_label_new(time_str);

    gtk_box_pack_start(GTK_BOX(hbox_total), label, FALSE, FALSE, 10);
#endif

    bbox = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(vbox), bbox, TRUE, TRUE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(bbox), 5);

    cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(bbox), cancel);
    g_signal_connect_swapped(cancel, "clicked",
                             G_CALLBACK(gtk_widget_destroy), mainwin_jtt);

    jump = gtk_button_new_from_stock(GTK_STOCK_JUMP_TO);
    GTK_WIDGET_SET_FLAGS(jump, GTK_CAN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(bbox), jump);
    g_signal_connect(jump, "clicked",
                     G_CALLBACK(mainwin_jump_to_time_cb), time_entry);

    tindex = playback_get_time() / 1000;
    g_snprintf(time_str, sizeof(time_str), "%u:%2.2u", tindex / 60,
               tindex % 60);
    gtk_entry_set_text(GTK_ENTRY(time_entry), time_str);

    gtk_editable_select_region(GTK_EDITABLE(time_entry), 0, strlen(time_str));

    gtk_widget_show_all(mainwin_jtt);

    gtk_widget_grab_focus(time_entry);
    gtk_widget_grab_default(jump);
}

void
action_jump_to_time( void )
{
    mainwin_jump_to_time();
}

void
action_playback_next( void )
{
    Playlist *playlist = playlist_get_active();
    playlist_next(playlist);
}

void
action_playback_previous( void )
{
    Playlist *playlist = playlist_get_active();
    playlist_prev(playlist);
}

void
action_playback_play( void )
{
    if (ab_position_a != -1)
        playback_seek(ab_position_a / 1000);
    if (playback_get_paused()) {
        playback_pause();
        return;
    }

    if (playlist_get_length(playlist_get_active()))
        playback_initiate();
}

void
action_playback_pause( void )
{
    playback_pause();
}

void
action_playback_stop( void )
{
    playback_stop();
}

void
action_preferences( void )
{
    show_prefs_window();
}

void
action_quit( void )
{
    aud_quit();
}

void action_playlist_track_info(void)
{
    Playlist *playlist = playlist_get_active();

    /* Show the first selected file, or the current file if nothing is
     * selected */
    GList *list = playlist_get_selected(playlist);
    if (list) {
        ui_fileinfo_show(playlist, GPOINTER_TO_INT(list->data));
        g_list_free(list);
    }
    else
        ui_fileinfo_show_current(playlist);
}

void action_queue_toggle(void)
{
    playlist_queue(playlist_get_active());
}

void action_playlist_sort_by_playlist_entry(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_PLAYLIST);
}

void action_playlist_sort_by_track_number(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_TRACK);
}

void action_playlist_sort_by_title(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_TITLE);
}

void action_playlist_sort_by_album(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_ALBUM);
}

void action_playlist_sort_by_artist(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_ARTIST);
}

void action_playlist_sort_by_full_path(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_PATH);
}

void action_playlist_sort_by_date(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_DATE);
}

void action_playlist_sort_by_filename(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_FILENAME);
}

void action_playlist_sort_selected_by_playlist_entry(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_PLAYLIST);
}

void action_playlist_sort_selected_by_track_number(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_TRACK);
}

void action_playlist_sort_selected_by_title(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_TITLE);
}

void action_playlist_sort_selected_by_album(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_ALBUM);
}

void action_playlist_sort_selected_by_artist(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_ARTIST);
}

void action_playlist_sort_selected_by_full_path(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_PATH);
}

void action_playlist_sort_selected_by_date(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_DATE);
}

void action_playlist_sort_selected_by_filename(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_FILENAME);
}

void action_playlist_randomize_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_random(playlist);
}

void action_playlist_reverse_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_reverse(playlist);
}

void
action_playlist_clear_queue(void)
{
    playlist_clear_queue(playlist_get_active());
}

void
action_playlist_remove_unavailable(void)
{
    playlist_remove_dead_files(playlist_get_active());
}

void
action_playlist_remove_dupes_by_title(void)
{
    playlist_remove_duplicates(playlist_get_active(), PLAYLIST_DUPS_TITLE);
}

void
action_playlist_remove_dupes_by_filename(void)
{
    playlist_remove_duplicates(playlist_get_active(), PLAYLIST_DUPS_FILENAME);
}

void
action_playlist_remove_dupes_by_full_path(void)
{
    playlist_remove_duplicates(playlist_get_active(), PLAYLIST_DUPS_PATH);
}

void
action_playlist_remove_all(void)
{
    playlist_clear(playlist_get_active());
}

void
action_playlist_remove_selected(void)
{
    playlist_delete(playlist_get_active(), FALSE);
}

void
action_playlist_remove_unselected(void)
{
    playlist_delete(playlist_get_active(), TRUE);
}

void
action_playlist_add_files(void)
{
    run_filebrowser(FALSE);
}

void
action_playlist_add_url(void)
{
    show_add_url_window();
}

void
action_playlist_new( void )
{
    Playlist *new_pl = playlist_new();
    playlist_add_playlist(new_pl);
    playlist_select_playlist(new_pl);
}

void
action_playlist_prev( void )
{
    playlist_select_prev();
}

void
action_playlist_next( void )
{
    playlist_select_next();
}

void
action_playlist_delete( void )
{
    playlist_remove_playlist( playlist_get_active() );
}

static void
on_static_toggle(GtkToggleButton *button, gpointer data)
{
    Playlist *playlist = playlist_get_active();

    playlist->attribute =
        gtk_toggle_button_get_active(button) ?
        playlist->attribute | PLAYLIST_STATIC :
        playlist->attribute & ~PLAYLIST_STATIC;
}

static void
on_relative_toggle(GtkToggleButton *button, gpointer data)
{
    Playlist *playlist = playlist_get_active();

    playlist->attribute =
        gtk_toggle_button_get_active(button) ?
        playlist->attribute | PLAYLIST_USE_RELATIVE :
        playlist->attribute & ~PLAYLIST_USE_RELATIVE;
}

static gchar *
playlist_file_selection_save(const gchar * title,
                        const gchar * default_filename)
{
    GtkWidget *dialog;
    gchar *filename;
    GtkWidget *hbox;
    GtkWidget *toggle, *toggle2;

    g_return_val_if_fail(title != NULL, NULL);

    dialog = make_filebrowser(title, TRUE);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), cfg.playlist_path);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), default_filename);

    hbox = gtk_hbox_new(FALSE, 5);

    /* static playlist */
    toggle = gtk_check_button_new_with_label(_("Save as Static Playlist"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),
                                 (playlist_get_active()->attribute & PLAYLIST_STATIC) ? TRUE : FALSE);
    g_signal_connect(G_OBJECT(toggle), "toggled", G_CALLBACK(on_static_toggle), dialog);
    gtk_box_pack_start(GTK_BOX(hbox), toggle, FALSE, FALSE, 0);

    /* use relative path */
    toggle2 = gtk_check_button_new_with_label(_("Use Relative Path"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle2),
                                 (playlist_get_active()->attribute & PLAYLIST_USE_RELATIVE) ? TRUE : FALSE);
    g_signal_connect(G_OBJECT(toggle2), "toggled", G_CALLBACK(on_relative_toggle), dialog);
    gtk_box_pack_start(GTK_BOX(hbox), toggle2, FALSE, FALSE, 0);

    gtk_widget_show_all(hbox);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), hbox);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    else
        filename = NULL;

    gtk_widget_destroy(dialog);
    return filename;
}

static void
show_playlist_save_error(GtkWindow *parent,
                         const gchar *filename)
{
    GtkWidget *dialog;

    g_return_if_fail(GTK_IS_WINDOW(parent));
    g_return_if_fail(filename);

    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_OK,
                                    _("Error writing playlist \"%s\": %s"),
                                    filename, strerror(errno));

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER); /* centering */
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static gboolean
show_playlist_overwrite_prompt(GtkWindow * parent,
                               const gchar * filename)
{
    GtkWidget *dialog;
    gint result;

    g_return_val_if_fail(GTK_IS_WINDOW(parent), FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);

    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    _("%s already exist. Continue?"),
                                    filename);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER); /* centering */
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_YES);
}

static void
show_playlist_save_format_error(GtkWindow * parent,
                                const gchar * filename)
{
    const gchar *markup =
        N_("<b><big>Unable to save playlist.</big></b>\n\n"
           "Unknown file type for '%s'.\n");

    GtkWidget *dialog;

    g_return_if_fail(GTK_IS_WINDOW(parent));
    g_return_if_fail(filename != NULL);

    dialog =
        gtk_message_dialog_new_with_markup(GTK_WINDOW(parent),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           _(markup),
                                           filename);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER); /* centering */
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void
playlistwin_save_playlist(const gchar * filename)
{
    PlaylistContainer *plc;
    gchar *ext = strrchr(filename, '.') + 1;

    plc = playlist_container_find(ext);
    if (plc == NULL) {
        show_playlist_save_format_error(NULL, filename);
        return;
    }

    str_replace_in(&cfg.playlist_path, g_path_get_dirname(filename));

    if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
        if (!show_playlist_overwrite_prompt(NULL, filename))
            return;

    if (!playlist_save(playlist_get_active(), filename))
        show_playlist_save_error(NULL, filename);
}

void
action_playlist_save_list(void)
{
    Playlist *playlist = playlist_get_active();

    const gchar *default_filename = playlist_get_current_name(playlist);

    gchar *dot = NULL, *basename = NULL;
    gchar *filename =
        playlist_file_selection_save(_("Save Playlist"), default_filename);

    if (filename) {
        /* Default extension */
        basename = g_path_get_basename(filename);
        dot = strrchr(basename, '.');
        if( dot == NULL || dot == basename) {
            gchar *oldname = filename;
#ifdef HAVE_XSPF_PLAYLIST
            filename = g_strconcat(oldname, ".xspf", NULL);
#else
            filename = g_strconcat(oldname, ".m3u", NULL);
#endif
            g_free(oldname);
        }
        g_free(basename);

        playlistwin_save_playlist(filename);
        g_free(filename);
    }
}

void
action_playlist_save_default_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_save(playlist, aud_paths[BMP_PATH_PLAYLIST_FILE]);
}

static void
playlistwin_load_playlist(const gchar * filename)
{
    const gchar *title;
    Playlist *playlist = playlist_get_active();

    g_return_if_fail(filename != NULL);

    str_replace_in(&cfg.playlist_path, g_path_get_dirname(filename));

    playlist_clear(playlist);

    playlist_load(playlist, filename);
    title = playlist_get_current_name(playlist);
    if(!title || !title[0])
        playlist_set_current_name(playlist, filename);
}

static gchar *
playlist_file_selection_load(const gchar * title,
                        const gchar * default_filename)
{
    GtkWidget *dialog;
    gchar *filename;

    g_return_val_if_fail(title != NULL, NULL);

    dialog = make_filebrowser(title, FALSE);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), cfg.playlist_path);
    if (default_filename)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), default_filename);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER); /* centering */

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    else
        filename = NULL;

    gtk_widget_destroy(dialog);
    return filename;
}

void
action_playlist_load_list(void)
{
    Playlist *playlist = playlist_get_active();

    const gchar *default_filename = playlist_get_current_name(playlist);
    gchar *filename =
        playlist_file_selection_load(_("Load Playlist"), default_filename);

    if (filename) {
        playlistwin_load_playlist(filename);
        g_free(filename);
    }
}

void
action_playlist_refresh_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_read_info_selection(playlist);
}

void
action_open_list_manager(void)
{
    playlist_manager_ui_show();
}

void
action_playlist_search_and_select(void)
{
    //playlistwin_select_search();
}

void
action_playlist_invert_selection(void)
{
    g_message("implement me");
}

void
action_playlist_select_none(void)
{
    g_message("implement me");
}

void
action_playlist_select_all(void)
{
    g_message("implement me");
}

static GtkWidget *equalizerwin_load_window = NULL;
static GtkWidget *equalizerwin_load_auto_window = NULL;
static GtkWidget *equalizerwin_save_window = NULL;
static GtkWidget *equalizerwin_save_entry = NULL;
static GtkWidget *equalizerwin_save_auto_window = NULL;
static GtkWidget *equalizerwin_save_auto_entry = NULL;
static GtkWidget *equalizerwin_delete_window = NULL;
static GtkWidget *equalizerwin_delete_auto_window = NULL;

static GList *equalizer_presets = NULL, *equalizer_auto_presets = NULL;

static void
equalizerwin_apply_preset(EqualizerPreset *preset)
{
    if (preset == NULL)
       return;

    gint i;

    cfg.equalizer_preamp = preset->preamp;
    for (i = 0; i < AUD_EQUALIZER_NBANDS; i++)
        cfg.equalizer_bands[i] = preset->bands[i];

    output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                  cfg.equalizer_bands);
}

static void
free_cb (gpointer data, gpointer user_data)
{
    equalizer_preset_free((EqualizerPreset*)data);
}

static void
equalizerwin_read_winamp_eqf(VFSFile * file)
{
    GList *presets;
    gint i;

    if ((presets = import_winamp_eqf(file)) == NULL)
        return;

    /* just get the first preset --asphyx */
    EqualizerPreset *preset = (EqualizerPreset*)presets->data;
    cfg.equalizer_preamp = preset->preamp;

    for (i = 0; i < AUD_EQUALIZER_NBANDS; i++)
        cfg.equalizer_bands[i] = preset->bands[i];

    g_list_foreach(presets, free_cb, NULL);
    g_list_free(presets);

    output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                  cfg.equalizer_bands);
}

static VFSFile *
open_vfs_file(const gchar *filename, const gchar *mode)
{
    VFSFile *file;
    GtkWidget *dialog;

    if (!(file = vfs_fopen(filename, mode))) {
        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         "Error loading file '%s'",
                                         filename);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
    }

    return file;
}

static void
load_winamp_file(const gchar * filename)
{
    VFSFile *file;

    if (!(file = open_vfs_file(filename, "rb")))
        return;

    equalizerwin_read_winamp_eqf(file);
    vfs_fclose(file);
}

static void
import_winamp_file(const gchar * filename)
{
    VFSFile *file;
    GList *list;

    if (!(file = open_vfs_file(filename, "rb")) ||
        !(list = import_winamp_eqf(file)))
        return;

    equalizer_presets = g_list_concat(equalizer_presets, list);
    equalizer_write_preset_file(equalizer_presets, "eq.preset");

    vfs_fclose(file);
}

static void
save_winamp_file(const gchar * filename)
{
    VFSFile *file;

    gchar name[257];
    gint i;
    guchar bands[11];

    if (!(file = open_vfs_file(filename, "wb")))
        return;

    vfs_fwrite("Winamp EQ library file v1.1\x1a!--", 1, 31, file);

    memset(name, 0, 257);
    g_strlcpy(name, "Entry1", 257);
    vfs_fwrite(name, 1, 257, file);

    for (i = 0; i < AUD_EQUALIZER_NBANDS; i++)
        bands[i] = 63 - (((cfg.equalizer_bands[i] + EQUALIZER_MAX_GAIN) * 63) / EQUALIZER_MAX_GAIN / 2);

    bands[AUD_EQUALIZER_NBANDS] = 63 - (((cfg.equalizer_preamp + EQUALIZER_MAX_GAIN) * 63) / EQUALIZER_MAX_GAIN / 2);

    vfs_fwrite(bands, 1, 11, file);
    vfs_fclose(file);
}

static GtkWidget *
equalizerwin_create_list_window(GList *preset_list,
                                const gchar *title,
                                GtkWidget **window,
                                GtkSelectionMode sel_mode,
                                GtkWidget **entry,
                                const gchar *action_name,
                                GCallback action_func,
                                GCallback select_row_func)
{
    GtkWidget *vbox, *scrolled_window, *bbox, *view;
    GtkWidget *button_cancel, *button_action;
    GList *node;

    GtkListStore *store;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;
    GtkTreeSortable *sortable;



    *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(*window), title);
    gtk_window_set_type_hint(GTK_WINDOW(*window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_default_size(GTK_WINDOW(*window), 350, 300);
    gtk_window_set_position(GTK_WINDOW(*window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(*window), 10);
    g_signal_connect(*window, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), window);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(*window), vbox);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);


    /* fill the store with the names of all available presets */
    store = gtk_list_store_new(1, G_TYPE_STRING);
    for (node = preset_list; node; node = g_list_next(node))
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, ((EqualizerPreset*)node->data)->name,
                           -1);
    }
    model = GTK_TREE_MODEL(store);


    sortable = GTK_TREE_SORTABLE(store);
    gtk_tree_sortable_set_sort_column_id(sortable, 0, GTK_SORT_ASCENDING);


    view = gtk_tree_view_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1,
                                                _("Presets"), renderer,
                                                "text", 0, NULL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
    g_object_unref(model);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_set_mode(selection, sel_mode);




    gtk_container_add(GTK_CONTAINER(scrolled_window), view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    if (entry) {
        *entry = gtk_entry_new();
        g_signal_connect(*entry, "activate", action_func, NULL);
        gtk_box_pack_start(GTK_BOX(vbox), *entry, FALSE, FALSE, 0);
    }

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    button_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect_swapped(button_cancel, "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             GTK_OBJECT(*window));
    gtk_box_pack_start(GTK_BOX(bbox), button_cancel, TRUE, TRUE, 0);

    button_action = gtk_button_new_from_stock(action_name);
    g_signal_connect(button_action, "clicked", G_CALLBACK(action_func), view);
    GTK_WIDGET_SET_FLAGS(button_action, GTK_CAN_DEFAULT);

    if (select_row_func)
        g_signal_connect(view, "row-activated", G_CALLBACK(select_row_func), NULL);

        
    gtk_box_pack_start(GTK_BOX(bbox), button_action, TRUE, TRUE, 0);

    gtk_widget_grab_default(button_action);


    gtk_widget_show_all(*window);

    return *window;
}

static EqualizerPreset *
equalizerwin_find_preset(GList * list, const gchar * name)
{
    GList *node = list;
    EqualizerPreset *preset;

    while (node) {
        preset = node->data;
        if (!strcasecmp(preset->name, name))
            return preset;
        node = g_list_next(node);
    }
    return NULL;
}

static gboolean
equalizerwin_load_preset(GList * list, const gchar * name)
{
    EqualizerPreset *preset;
    gint i;

    if ((preset = equalizerwin_find_preset(list, name)) != NULL) {
        cfg.equalizer_preamp = preset->preamp;
        for (i = 0; i < AUD_EQUALIZER_NBANDS; i++)
            cfg.equalizer_bands[i] = preset->bands[i];

        return TRUE;
    }

    output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                  cfg.equalizer_bands);

    return FALSE;
}

static GList *
equalizerwin_save_preset(GList * list, const gchar * name,
                         const gchar * filename)
{
    gint i;
    EqualizerPreset *preset;

    if (!(preset = equalizerwin_find_preset(list, name))) {
        preset = g_new0(EqualizerPreset, 1);
        preset->name = g_strdup(name);
        list = g_list_append(list, preset);
    }

    preset->preamp = cfg.equalizer_preamp;
    for (i = 0; i < AUD_EQUALIZER_NBANDS; i++)
        preset->bands[i] = cfg.equalizer_bands[i];

    equalizer_write_preset_file(list, filename);

    return list;
}

static GList *
equalizerwin_delete_preset(GList * list, gchar * name, gchar * filename)
{
    EqualizerPreset *preset;
    GList *node;

    if (!(preset = equalizerwin_find_preset(list, name)))
        return list;

    if (!(node = g_list_find(list, preset)))
        return list;

    list = g_list_remove_link(list, node);
    equalizer_preset_free(preset);
    g_list_free_1(node);

    equalizer_write_preset_file(list, filename);

    return list;
}

static void
equalizerwin_delete_selected_presets(GtkTreeView *view, gchar *filename)
{
    gchar *text;

    GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
    GtkTreeModel *model = gtk_tree_view_get_model(view);

    /*
     * first we are making a list of the selected rows, then we convert this
     * list into a list of GtkTreeRowReferences, so that when you delete an
     * item you can still access the other items
     * finally we iterate through all GtkTreeRowReferences, convert them to
     * GtkTreeIters and delete those one after the other
     */

    GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
    GList *rrefs = NULL;
    GList *litr;

    for (litr = list; litr; litr = litr->next)
    {
        GtkTreePath *path = litr->data;
        rrefs = g_list_append(rrefs, gtk_tree_row_reference_new(model, path));
    }

    for (litr = rrefs; litr; litr = litr->next)
    {
        GtkTreeRowReference *ref = litr->data;
        GtkTreePath *path = gtk_tree_row_reference_get_path(ref);
        GtkTreeIter iter;
        gtk_tree_model_get_iter(model, &iter, path);

        gtk_tree_model_get(model, &iter, 0, &text, -1);

        if (!strcmp(filename, "eq.preset"))
            equalizer_presets = equalizerwin_delete_preset(equalizer_presets, text, filename);
        else if (!strcmp(filename, "eq.auto_preset"))
            equalizer_auto_presets = equalizerwin_delete_preset(equalizer_auto_presets, text, filename);

        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
    }
}

static void
equalizerwin_save_ok(GtkWidget * widget, gpointer data)
{
    const gchar *text;

    text = gtk_entry_get_text(GTK_ENTRY(equalizerwin_save_entry));
    if (strlen(text) != 0)
        equalizer_presets =
            equalizerwin_save_preset(equalizer_presets, text, "eq.preset");
    gtk_widget_destroy(equalizerwin_save_window);
}

static void
equalizerwin_save_select(GtkTreeView *treeview, GtkTreePath *path,
                         GtkTreeViewColumn *col, gpointer data)
{
    gchar *text;

    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (selection)
    {
        if (gtk_tree_selection_get_selected(selection, &model, &iter))
        {
            gtk_tree_model_get(model, &iter, 0, &text, -1);
            gtk_entry_set_text(GTK_ENTRY(equalizerwin_save_entry), text);
            equalizerwin_save_ok(NULL, NULL);

            g_free(text);
        }
    }
}

static void
equalizerwin_load_ok(GtkWidget *widget, gpointer data)
{
    gchar *text;

    GtkTreeView* view = GTK_TREE_VIEW(data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (selection)
    {
        if (gtk_tree_selection_get_selected(selection, &model, &iter))
        {
            gtk_tree_model_get(model, &iter, 0, &text, -1);
            equalizerwin_load_preset(equalizer_presets, text);

            g_free(text);
        }
    }
    gtk_widget_destroy(equalizerwin_load_window);
}

static void
equalizerwin_load_select(GtkTreeView *treeview, GtkTreePath *path,
                         GtkTreeViewColumn *col, gpointer data)
{
    equalizerwin_load_ok(NULL, treeview);
}

static void
equalizerwin_delete_delete(GtkWidget *widget, gpointer data)
{
    equalizerwin_delete_selected_presets(GTK_TREE_VIEW(data), "eq.preset");
}

static void
equalizerwin_save_auto_ok(GtkWidget *widget, gpointer data)
{
    const gchar *text;

    text = gtk_entry_get_text(GTK_ENTRY(equalizerwin_save_auto_entry));
    if (strlen(text) != 0)
        equalizer_auto_presets =
            equalizerwin_save_preset(equalizer_auto_presets, text,
                                     "eq.auto_preset");
    gtk_widget_destroy(equalizerwin_save_auto_window);
}

static void
equalizerwin_save_auto_select(GtkTreeView *treeview, GtkTreePath *path,
                              GtkTreeViewColumn *col, gpointer data)
{
    gchar *text;

    GtkTreeSelection *selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (selection)
    {
        if (gtk_tree_selection_get_selected(selection, &model, &iter))
        {
            gtk_tree_model_get(model, &iter, 0, &text, -1);
            gtk_entry_set_text(GTK_ENTRY(equalizerwin_save_auto_entry), text);
            equalizerwin_save_auto_ok(NULL, NULL);

            g_free(text);
        }
    }
}

static void
equalizerwin_load_auto_ok(GtkWidget *widget, gpointer data)
{
    gchar *text;

    GtkTreeView *view = GTK_TREE_VIEW(data);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(view);
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (selection)
    {
        if (gtk_tree_selection_get_selected(selection, &model, &iter))
        {
            gtk_tree_model_get(model, &iter, 0, &text, -1);
            equalizerwin_load_preset(equalizer_auto_presets, text);

            g_free(text);
        }
    }
    gtk_widget_destroy(equalizerwin_load_auto_window);
}

static void
equalizerwin_load_auto_select(GtkTreeView *treeview, GtkTreePath *path,
                              GtkTreeViewColumn *col, gpointer data)
{
    equalizerwin_load_auto_ok(NULL, treeview);
}

static void
equalizerwin_delete_auto_delete(GtkWidget *widget, gpointer data)
{
    equalizerwin_delete_selected_presets(GTK_TREE_VIEW(data), "eq.auto_preset");
}

void
action_equ_load_preset(void)
{
    if (equalizerwin_load_window) {
        gtk_window_present(GTK_WINDOW(equalizerwin_load_window));
        return;
    }

    equalizerwin_create_list_window(equalizer_presets,
                                    Q_("Load preset"),
                                    &equalizerwin_load_window,
                                    GTK_SELECTION_SINGLE, NULL,
                                    GTK_STOCK_OK,
                                    G_CALLBACK(equalizerwin_load_ok),
                                    G_CALLBACK(equalizerwin_load_select));
}

void
action_equ_load_auto_preset(void)
{
    if (equalizerwin_load_auto_window) {
        gtk_window_present(GTK_WINDOW(equalizerwin_load_auto_window));
        return;
    }

    equalizerwin_create_list_window(equalizer_auto_presets,
                                    Q_("Load auto-preset"),
                                    &equalizerwin_load_auto_window,
                                    GTK_SELECTION_SINGLE, NULL,
                                    GTK_STOCK_OK,
                                    G_CALLBACK(equalizerwin_load_auto_ok),
                                    G_CALLBACK(equalizerwin_load_auto_select));
}

void
action_equ_load_default_preset(void)
{
    equalizerwin_load_preset(equalizer_presets, "Default");
}

void
action_equ_zero_preset(void)
{
    gint i;

    cfg.equalizer_preamp = 0;
    for (i = 0; i < AUD_EQUALIZER_NBANDS; i++)
        cfg.equalizer_bands[i] = 0;

    output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                  cfg.equalizer_bands);
}

void
action_equ_load_preset_file(void)
{
    GtkWidget *dialog;
    gchar *file_uri;

    dialog = make_filebrowser(Q_("Load equalizer preset"), FALSE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        file_uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
        EqualizerPreset *preset = load_preset_file(file_uri);
        equalizerwin_apply_preset(preset);
        equalizer_preset_free(preset);
        g_free(file_uri);
    }
    gtk_widget_destroy(dialog);
}

void
action_equ_load_preset_eqf(void)
{
    GtkWidget *dialog;
    gchar *file_uri;

    dialog = make_filebrowser(Q_("Load equalizer preset"), FALSE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        file_uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
        load_winamp_file(file_uri);
        g_free(file_uri);
    }
    gtk_widget_destroy(dialog);
}

void
action_equ_import_winamp_presets(void)
{
    GtkWidget *dialog;
    gchar *file_uri;

    dialog = make_filebrowser(Q_("Load equalizer preset"), FALSE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        file_uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
        import_winamp_file(file_uri);
        g_free(file_uri);
    }
    gtk_widget_destroy(dialog);
}

void
action_equ_save_preset(void)
{
    if (equalizerwin_save_window) {
        gtk_window_present(GTK_WINDOW(equalizerwin_save_window));
        return;
    }
     
    equalizerwin_create_list_window(equalizer_presets,
                                    Q_("Save preset"),
                                    &equalizerwin_save_window,
                                    GTK_SELECTION_SINGLE,
                                    &equalizerwin_save_entry,
                                    GTK_STOCK_OK,
                                    G_CALLBACK(equalizerwin_save_ok),
                                    G_CALLBACK(equalizerwin_save_select));
}

void
action_equ_save_auto_preset(void)
{
    gchar *name;
    Playlist *playlist = playlist_get_active();

    if (equalizerwin_save_auto_window)
        gtk_window_present(GTK_WINDOW(equalizerwin_save_auto_window));
    else
        equalizerwin_create_list_window(equalizer_auto_presets,
                                        Q_("Save auto-preset"),
                                        &equalizerwin_save_auto_window,
                                        GTK_SELECTION_SINGLE,
                                        &equalizerwin_save_auto_entry,
                                        GTK_STOCK_OK,
                                        G_CALLBACK(equalizerwin_save_auto_ok),
                                        G_CALLBACK(equalizerwin_save_auto_select));

    name = playlist_get_filename(playlist, playlist_get_position(playlist));
    if (name) {
        gtk_entry_set_text(GTK_ENTRY(equalizerwin_save_auto_entry),
                           g_basename(name));
        g_free(name);
    }
}

void
action_equ_save_default_preset(void)
{
    equalizer_presets =
        equalizerwin_save_preset(equalizer_presets, Q_("Default"), "eq.preset");
}

void
action_equ_save_preset_file(void)
{
    GtkWidget *dialog;
    gchar *file_uri;
    gchar *songname;
    Playlist *playlist = playlist_get_active();
    int i;

    dialog = make_filebrowser(Q_("Save equalizer preset"), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        file_uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
        EqualizerPreset *preset = g_new0(EqualizerPreset, 1);
        preset->name = g_strdup(file_uri);
        preset->preamp = cfg.equalizer_preamp;
        for (i = 0; i < AUD_EQUALIZER_NBANDS; i++)
            preset->bands[i] = cfg.equalizer_bands[i];
        save_preset_file(preset, file_uri);
        equalizer_preset_free(preset);
        g_free(file_uri);
    }

    songname = playlist_get_filename(playlist, playlist_get_position(playlist));
    if (songname) {
        gchar *eqname = g_strdup_printf("%s.%s", songname,
                                        cfg.eqpreset_extension);
        g_free(songname);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
                                      eqname);
        g_free(eqname);
    }

    gtk_widget_destroy(dialog);
}

void
action_equ_save_preset_eqf(void)
{
    GtkWidget *dialog;
    gchar *file_uri;

    dialog = make_filebrowser(Q_("Save equalizer preset"), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        file_uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
        save_winamp_file(file_uri);
        g_free(file_uri);
    }
    gtk_widget_destroy(dialog);
}

void
action_equ_delete_preset(void)
{
    if (equalizerwin_delete_window) {
        gtk_window_present(GTK_WINDOW(equalizerwin_delete_window));
        return;
    }
    
    equalizerwin_create_list_window(equalizer_presets,
                                    Q_("Delete preset"),
                                    &equalizerwin_delete_window,
                                    GTK_SELECTION_EXTENDED, NULL,
                                    GTK_STOCK_DELETE,
                                    G_CALLBACK(equalizerwin_delete_delete),
                                    NULL);
}

void
action_equ_delete_auto_preset(void)
{
    if (equalizerwin_delete_auto_window) {
        gtk_window_present(GTK_WINDOW(equalizerwin_delete_auto_window));
        return;
    }
    
    equalizerwin_create_list_window(equalizer_auto_presets,
                                    Q_("Delete auto-preset"),
                                    &equalizerwin_delete_auto_window,
                                    GTK_SELECTION_EXTENDED, NULL,
                                    GTK_STOCK_DELETE,
                                    G_CALLBACK(equalizerwin_delete_auto_delete),
                                    NULL);
}
