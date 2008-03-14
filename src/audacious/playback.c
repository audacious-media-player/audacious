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
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "configdb.h"

#include "eventqueue.h"
#include "hook.h"
#include "input.h"
#include "main.h"
#include "output.h"
#include "playlist.h"
#include "pluginenum.h"
#include "skin.h"
#include "ui_equalizer.h"
#include "ui_main.h"
#include "ui_skinned_playstatus.h"
#include "util.h"
#include "visualization.h"

#include "playback.h"
#include "playback_evlisteners.h"

static gint song_info_timeout_source = 0;
static gint update_vis_timeout_source = 0;

/* XXX: there has to be a better way than polling here! */
static gboolean
playback_update_vis_func(gpointer unused)
{
    if (!playback_get_playing())
        return FALSE;

    input_update_vis(playback_get_time());

    return TRUE;
}

static gint
playback_set_pb_ready(InputPlayback *playback)
{
    g_mutex_lock(playback->pb_ready_mutex);
    playback->pb_ready_val = 1;
    g_cond_signal(playback->pb_ready_cond);
    g_mutex_unlock(playback->pb_ready_mutex);
    return 0;
}

static gint
playback_wait_for_pb_ready(InputPlayback *playback)
{
    g_mutex_lock(playback->pb_ready_mutex);
    while (playback->pb_ready_val != 1)
        g_cond_wait(playback->pb_ready_cond, playback->pb_ready_mutex);
    g_mutex_unlock(playback->pb_ready_mutex);
    return 0;
}

static void
playback_set_pb_change(InputPlayback *playback)
{
    g_mutex_lock(playback->pb_change_mutex);
    g_cond_signal(playback->pb_change_cond);
    g_mutex_unlock(playback->pb_change_mutex);
}

static void
playback_set_pb_params(InputPlayback *playback, gchar *title,
    gint length, gint rate, gint freq, gint nch)
{
    playback->title = g_strdup(title);
    playback->length = length;
    playback->rate = rate;
    playback->freq = freq;
    playback->nch = nch;

    /* XXX: this can be removed/merged here someday */
    plugin_set_current((Plugin *)(playback->plugin));
    playback->plugin->set_info(title, length, rate, freq, nch);
}

static void
playback_set_pb_title(InputPlayback *playback, gchar *title)
{
    playback->title = g_strdup(title);

    plugin_set_current((Plugin *)(playback->plugin));
    playback->plugin->set_info_text(title);
}

void
playback_eof(void)
{
    event_queue("playback eof", playlist_get_active());
}

void
playback_error(void)
{
    event_queue("playback audio error", NULL);
}

gint
playback_get_time(void)
{
    InputPlayback *playback;
    g_return_val_if_fail(playback_get_playing(), -1);
    playback = get_current_input_playback();

    if (!playback) /* playback can be NULL during init even if playing is TRUE */              
        return -1;
    plugin_set_current((Plugin *)(playback->plugin));
    if (playback->plugin->get_time)
    {
        plugin_set_current((Plugin *)(playback->plugin));
        return playback->plugin->get_time(playback);
    }
    if (playback->error)
        return -2;
    if (!playback->playing || 
    (playback->eof && !playback->output->buffer_playing()))
        return -1;
    return playback->output->output_time();
}

gint
playback_get_length(void)
{
    InputPlayback *playback;
    g_return_val_if_fail(playback_get_playing(), -1);
    playback = get_current_input_playback();

    if (playback->length)
        return playback->length;

    if (playback && playback->plugin->get_song_tuple) {
        plugin_set_current((Plugin *)(playback->plugin));
        Tuple *tuple = playback->plugin->get_song_tuple(playback->filename);
        if (tuple_get_value_type(tuple, FIELD_LENGTH, NULL) == TUPLE_INT)
            return tuple_get_int(tuple, FIELD_LENGTH, NULL);
    }

    return -1;
}

void
playback_initiate(void)
{
    PlaylistEntry *entry = NULL;
    Playlist *playlist = playlist_get_active();

    g_return_if_fail(playlist_get_length(playlist) != 0);

    /* initialize playback event listeners if not done already */
    playback_evlistener_init();

    if (playback_get_playing())
        playback_stop();

    ui_vis_clear_data(mainwin_vis);
    ui_svis_clear_data(mainwin_svis);
    mainwin_disable_seekbar();

    entry = playlist_get_entry_to_play(playlist);
    g_return_if_fail(entry != NULL);
#ifdef USE_DBUS
    mpris_emit_track_change(mpris);
#endif
    playback_play_file(entry);

//    if (playback_get_time() != -1) {
        equalizerwin_load_auto_preset(entry->filename);
        input_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                     cfg.equalizer_bands);
        output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                      cfg.equalizer_bands);
//    }

    playlist_check_pos_current(playlist);
    mainwin_update_song_info();

    /* FIXME: use g_timeout_add_seconds when glib-2.14 is required */
    song_info_timeout_source = g_timeout_add(1000,
        (GSourceFunc) mainwin_update_song_info, NULL);

    update_vis_timeout_source = g_timeout_add(10,
        (GSourceFunc) playback_update_vis_func, NULL);

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
    InputPlayback *playback;

    if (!playback_get_playing())
        return;

    if ((playback = get_current_input_playback()) == NULL)
        return;

    ip_data.paused = !ip_data.paused;

    if (playback->plugin->pause)
    {
        plugin_set_current((Plugin *)(playback->plugin));
        playback->plugin->pause(playback, ip_data.paused);
    }

    if (ip_data.paused)
        hook_call("playback pause", NULL);
    else
        hook_call("playback unpause", NULL);

    g_return_if_fail(mainwin_playstatus != NULL);

    if (ip_data.paused) {
        ui_skinned_playstatus_set_status(mainwin_playstatus, STATUS_PAUSE);
#ifdef USE_DBUS
        mpris_emit_status_change(mpris, MPRIS_STATUS_PAUSE);
#endif
    } else {
        ui_skinned_playstatus_set_status(mainwin_playstatus, STATUS_PLAY);
#ifdef USE_DBUS
        mpris_emit_status_change(mpris, MPRIS_STATUS_PLAY);
#endif
    }
}

void
playback_stop(void)
{
    InputPlayback *playback;
    
    if ((playback = get_current_input_playback()) == NULL)
        return;

    if (ip_data.playing)
    {
        /* wait for plugin to signal it has reached
           the 'playback safe state' before stopping */
        playback_wait_for_pb_ready(playback);

        if (playback_get_paused() == TRUE)
        {
            if (get_written_time() > 0)
              output_flush(get_written_time()); /* to avoid noise */
            playback_pause();
        }

        ip_data.playing = FALSE;

        /* TODO: i'm unsure if this will work. we might have to
           signal the change in stop() (e.g. in the plugins
           directly.) --nenolod */
        playback->set_pb_change(playback);

        if (playback->plugin->stop)
	{
	    plugin_set_current((Plugin *)(playback->plugin));
            playback->plugin->stop(playback);
	}

        if (playback->thread != NULL)
        {
            g_thread_join(playback->thread);
            playback->thread = NULL;
        }

        free_vis_data();
        ip_data.paused = FALSE;

        playback_free(playback);
        set_current_input_playback(NULL);
#ifdef USE_DBUS
        mpris_emit_status_change(mpris, MPRIS_STATUS_STOP);
#endif
    }

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

static gpointer
playback_monitor_thread(gpointer data)
{
    InputPlayback *playback = (InputPlayback *) data;

    plugin_set_current((Plugin *)(playback->plugin));
    playback->plugin->play_file(playback);

    /* if play_file has not reached the 'safe state' before returning (an error
       occurred), set the playback ready value to 1 now, to allow for proper stop */
    playback_set_pb_ready(playback);

    if (!playback->error && ip_data.playing)
        playback_eof();
    else if (playback->error)
        playback_error();

    playback->thread = NULL;
    g_thread_exit(NULL);
    return NULL;
}

InputPlayback *
playback_new(void)
{
    InputPlayback *playback = (InputPlayback *) g_slice_new0(InputPlayback);

    playback->pb_ready_mutex = g_mutex_new();
    playback->pb_ready_cond = g_cond_new();
    playback->pb_ready_val = 0;

    playback->pb_change_mutex = g_mutex_new();
    playback->pb_change_cond = g_cond_new();

    /* init vtable functors */
    playback->set_pb_ready = playback_set_pb_ready;
    playback->set_pb_change = playback_set_pb_change;
    playback->set_params = playback_set_pb_params;
    playback->set_title = playback_set_pb_title;
    playback->pass_audio = output_pass_audio;
    playback->set_replaygain_info = output_set_replaygain_info;

    return playback;
}

/**
 * Destroys InputPlayback.
 *
 * Playback comes from playback_new() function but there can be also
 * other sources for allocated playback data (like filename and title)
 * and this tries to deallocate all that data.
 */
void playback_free(InputPlayback *playback)
{
    g_free(playback->filename);
    g_free(playback->title);

    g_mutex_free(playback->pb_ready_mutex);
    g_cond_free(playback->pb_ready_cond);

    g_mutex_free(playback->pb_change_mutex);
    g_cond_free(playback->pb_change_cond);

    g_slice_free(InputPlayback, playback);
}

void
playback_run(InputPlayback *playback)
{
    playback->thread = g_thread_create(playback_monitor_thread, playback, TRUE, NULL);
}

gboolean
playback_play_file(PlaylistEntry *entry)
{
    InputPlayback *playback;

    g_return_val_if_fail(entry != NULL, FALSE);

    if (!get_current_output_plugin()) {
        run_no_output_plugin_dialog();
        mainwin_stop_pushed();
        return FALSE;
    }

    if (cfg.random_skin_on_play)
        skin_set_random_skin();

    if (!entry->decoder)
    {
        ProbeResult *pr = input_check_file(entry->filename, FALSE);

        if (pr != NULL)
        {
            entry->decoder = pr->ip;
            entry->tuple = pr->tuple;

            g_free(pr);
        }
    }

    if (!entry->decoder || !entry->decoder->enabled)
    {
        set_current_input_playback(NULL);

        return FALSE;
    }

    ip_data.playing = TRUE;

    playback = playback_new();
    
    entry->decoder->output = &psuedo_output_plugin;

    playback->plugin = entry->decoder;
    playback->filename = g_strdup(entry->filename);
    playback->output = &psuedo_output_plugin;
    
    set_current_input_playback(playback);

    playback_run(playback);

#ifdef USE_DBUS
    mpris_emit_status_change(mpris, MPRIS_STATUS_PLAY);
#endif

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
    InputPlayback *playback = get_current_input_playback();
    gboolean restore_pause = FALSE;
    gint l=0, r=0;

    g_return_if_fail(ip_data.playing);
    g_return_if_fail(playback != NULL);

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
    
    plugin_set_current((Plugin *)(playback->plugin));
    playback->plugin->seek(playback, time);
    playback->set_pb_change(playback);
    free_vis_data();
    
    if (restore_pause)
    {
        playback_pause();
        output_set_volume(l, r);
    }

    event_queue_timed(100, "playback seek", playback);
}

void
playback_seek_relative(gint offset)
{
    gint time = CLAMP(playback_get_time() / 1000 + offset,
                      0, playlist_get_current_length(playlist_get_active()) / 1000 - 1);
    playback_seek(time);
}
