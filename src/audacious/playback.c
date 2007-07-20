/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
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
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "configdb.h"

#include "hook.h"
#include "input.h"
#include "main.h"
#include "ui_equalizer.h"
#include "output.h"
#include "playlist.h"
#include "ui_main.h"
#include "ui_playlist.h"
#include "ui_skinselector.h"
#include "ui_skinned_playstatus.h"
#include "util.h"
#include "visualization.h"

#include "playback.h"

static int song_info_timeout_source = 0;

gint
playback_get_time(void)
{
    InputPlayback *playback;
    g_return_val_if_fail(playback_get_playing(), -1);
    playback = get_current_input_playback();
    g_return_val_if_fail(playback, -1);

    if (playback->plugin->get_time)
        return playback->plugin->get_time(playback);
    if (playback->error)
        return -2;
    if (!playback->playing || 
	(playback->eof && !playback->output->buffer_playing()))
        return -1;
    return playback->output->output_time();
}

void
playback_initiate(void)
{
    PlaylistEntry *entry = NULL;
    Playlist *playlist = playlist_get_active();

    g_return_if_fail(playlist_get_length(playlist) != 0);

    if (playback_get_playing())
        playback_stop();

    ui_vis_clear_data(mainwin_vis);
    ui_svis_clear_data(mainwin_svis);
    mainwin_disable_seekbar();

    entry = playlist_get_entry_to_play(playlist);
    g_return_if_fail(entry != NULL);
    playback_play_file(entry);

    if (playback_get_time() != -1) {
        equalizerwin_load_auto_preset(entry->filename);
        input_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                     cfg.equalizer_bands);
        output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                      cfg.equalizer_bands);
    }

    playlist_check_pos_current(playlist);
    mainwin_set_info_text();
    mainwin_update_song_info();

    /* FIXME: use g_timeout_add_seconds when glib-2.14 is required */
    song_info_timeout_source = g_timeout_add(1000,
        (GSourceFunc) mainwin_update_song_info, NULL);

    if (cfg.player_shaded) {
        gtk_widget_show(mainwin_stime_min);
        gtk_widget_show(mainwin_stime_sec);
        gtk_widget_show(mainwin_sposition);
    } else {
        gtk_widget_show(mainwin_minus_num);
        gtk_widget_show(mainwin_10min_num);
        gtk_widget_show(mainwin_min_num);
        gtk_widget_show(mainwin_10sec_num);
        gtk_widget_show(mainwin_sec_num);
        gtk_widget_show(mainwin_position);
    }

    vis_playback_start();

    hook_call("playback begin", entry);
}

void
playback_pause(void)
{
    if (!playback_get_playing())
        return;

    if (!get_current_input_playback())
        return;

    ip_data.paused = !ip_data.paused;

    if (get_current_input_playback()->plugin->pause)
        get_current_input_playback()->plugin->pause(get_current_input_playback(),
						    ip_data.paused);

    if (ip_data.paused)
        hook_call("playback pause", NULL);
    else
        hook_call("playback unpause", NULL);

    g_return_if_fail(mainwin_playstatus != NULL);

    if (ip_data.paused)
        ui_skinned_playstatus_set_status(mainwin_playstatus, STATUS_PAUSE);
    else
        ui_skinned_playstatus_set_status(mainwin_playstatus, STATUS_PLAY);
}

void
playback_stop(void)
{
    if (ip_data.playing && get_current_input_playback()) {

        if (playback_get_paused()) {
            output_flush(get_written_time()); /* to avoid noise */
            playback_pause();
        }

        ip_data.playing = FALSE; 

        if (get_current_input_playback()->plugin->stop)
            get_current_input_playback()->plugin->stop(get_current_input_playback());

        free_vis_data();
        ip_data.paused = FALSE;

        if (input_info_text) {
            g_free(input_info_text);
            input_info_text = NULL;
            mainwin_set_info_text();
        }

	g_free(get_current_input_playback()->filename);
	g_free(get_current_input_playback());
	set_current_input_playback(NULL);
    }

    ip_data.buffering = FALSE;
    ip_data.playing = FALSE;

    if (song_info_timeout_source)
        g_source_remove(song_info_timeout_source);

    vis_playback_stop();

    g_return_if_fail(mainwin_playstatus != NULL);
    ui_skinned_playstatus_set_buffering(mainwin_playstatus, FALSE);
}

static void
run_no_output_plugin_dialog(void)
{
    const gchar *markup = 
        N_("<b><big>No output plugin selected.</big></b>\n"
           "You have not selected an output plugin.");

    GtkWidget *dialog =
        gtk_message_dialog_new_with_markup(GTK_WINDOW(mainwin),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           _(markup));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

gboolean
playback_play_file(PlaylistEntry *entry)
{
    InputPlayback * playback;
    g_return_val_if_fail(entry != NULL, FALSE);

    if (!get_current_output_plugin()) {
        run_no_output_plugin_dialog();
        mainwin_stop_pushed();
        return FALSE;
    }

    if (cfg.random_skin_on_play)
        skin_set_random_skin();

    /*
     * This is slightly uglier than the original version, but should
     * fix the "crash" issues as seen in 0.2 when dealing with this situation.
     *  - nenolod
     */
    if (!entry->decoder && 
    (((entry->decoder = input_check_file(entry->filename, FALSE)) == NULL) ||
        !input_is_enabled(entry->decoder->filename)))
    {
        set_current_input_playback(NULL);
        mainwin_set_info_text();

        return FALSE;
    }

    playback = g_new0(InputPlayback, 1);
    
    entry->decoder->output = &psuedo_output_plugin;

    playback->plugin = entry->decoder;
    playback->output = &psuedo_output_plugin;
    playback->filename = g_strdup(entry->filename);
    
    set_current_input_playback(playback);

    entry->decoder->play_file(playback);

    ip_data.playing = TRUE;

    return TRUE;
}

gboolean
playback_get_playing(void)
{
    return ip_data.playing;
}

gboolean
playback_get_paused(void)
{
    return ip_data.paused;
}

void
playback_seek(gint time)
{
    gboolean restore_pause = FALSE;
    gint l=0, r=0;

    g_return_if_fail(ip_data.playing);
    g_return_if_fail(get_current_input_playback());

    /* FIXME WORKAROUND...that should work with all plugins
     * mute the volume, start playback again, do the seek, then pause again
     * -Patrick Sudowe 
     */
    if (ip_data.paused)
    {
        restore_pause = TRUE;
        output_get_volume(&l, &r);
        output_set_volume(0,0);
        playback_pause();
    }
    
    free_vis_data();
    get_current_input_playback()->plugin->seek(get_current_input_playback(), time);
    
    if (restore_pause)
    {
        playback_pause();
        output_set_volume(l, r);
    }
}

void
playback_seek_relative(gint offset)
{
    gint time = CLAMP(playback_get_time() / 1000 + offset,
                      0, playlist_get_current_length(playlist_get_active()) / 1000 - 1);
    playback_seek(time);
}
