/*
 * Audacious
 * Copyright (c) 2006-2007 Audacious development team.
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

#include "ui_playlist_evlisteners.h"

#include <glib.h>
#include <math.h>

#include "hook.h"
#include "playback.h"
#include "playlist.h"
#include "playlist_evmessages.h"
#include "visualization.h"

#include "ui_credits.h"
#include "ui_equalizer.h"
#include "ui_fileopener.h"
#include "ui_urlopener.h"
#include "ui_jumptotrack.h"
#include "ui_main.h"
#include "ui_playlist.h"
#include "ui_preferences.h"
#include "ui_skinned_playstatus.h"
#include "ui_skinned_textbox.h"
#include "ui_skinned_window.h"

static gint song_info_timeout_source = 0;
static gint update_vis_timeout_source = 0;

/* XXX: there has to be a better way than polling here! */
/* also: where should this function go? should it stay here? --mf0102 */
static gboolean
update_vis_func(gpointer unused)
{
    if (!playback_get_playing())
        return FALSE;

    input_update_vis(playback_get_time());

    return TRUE;
}

static void
ui_main_evlistener_title_change(gpointer hook_data, gpointer user_data)
{
    gchar *text = (gchar *) hook_data;

    ui_skinned_textbox_set_text(mainwin_info, text);
    playlistwin_update_list(playlist_get_active());
}

static void
ui_main_evlistener_hide_seekbar(gpointer hook_data, gpointer user_data)
{
    mainwin_disable_seekbar();
}

static void
ui_main_evlistener_volume_change(gpointer hook_data, gpointer user_data)
{
    gint *h_vol = (gint *) hook_data;
    gint vl, vr, b, v;

    vl = CLAMP(h_vol[0], 0, 100);
    vr = CLAMP(h_vol[1], 0, 100);
    v = MAX(vl, vr);
    if (vl > vr)
        b = (gint) rint(((gdouble) vr / vl) * 100) - 100;
    else if (vl < vr)
        b = 100 - (gint) rint(((gdouble) vl / vr) * 100);
    else
        b = 0;

    mainwin_set_volume_slider(v);
    equalizerwin_set_volume_slider(v);
    mainwin_set_balance_slider(b);
    equalizerwin_set_balance_slider(b);
}

static void
ui_main_evlistener_playback_initiate(gpointer hook_data, gpointer user_data)
{
    playback_initiate();
}

static void
ui_main_evlistener_playback_begin(gpointer hook_data, gpointer user_data)
{
    PlaylistEntry *entry = (PlaylistEntry*)hook_data;
    g_return_if_fail(entry != NULL);

    equalizerwin_load_auto_preset(entry->filename);
    hook_call("equalizer changed", NULL);

    ui_vis_clear_data(mainwin_vis);
    ui_svis_clear_data(mainwin_svis);
    mainwin_disable_seekbar();
    mainwin_update_song_info();

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

    song_info_timeout_source = 
        g_timeout_add_seconds(1, (GSourceFunc) mainwin_update_song_info, NULL);

    update_vis_timeout_source =
        g_timeout_add(10, (GSourceFunc) update_vis_func, NULL);

    vis_playback_start();

    ui_skinned_playstatus_set_status(mainwin_playstatus, STATUS_PLAY);
}

static void
ui_main_evlistener_playback_stop(gpointer hook_data, gpointer user_data)
{
    if (song_info_timeout_source)
        g_source_remove(song_info_timeout_source);

    vis_playback_stop();
    free_vis_data();

    ui_skinned_playstatus_set_buffering(mainwin_playstatus, FALSE);
}

static void
ui_main_evlistener_playback_pause(gpointer hook_data, gpointer user_data)
{
    ui_skinned_playstatus_set_status(mainwin_playstatus, STATUS_PAUSE);
}

static void
ui_main_evlistener_playback_unpause(gpointer hook_data, gpointer user_data)
{
    ui_skinned_playstatus_set_status(mainwin_playstatus, STATUS_PLAY);
}

static void
ui_main_evlistener_playback_seek(gpointer hook_data, gpointer user_data)
{
    free_vis_data();
}

static void
ui_main_evlistener_playback_play_file(gpointer hook_data, gpointer user_data)
{
    if (cfg.random_skin_on_play)
        skin_set_random_skin();
}

static void
ui_main_evlistener_playlist_end_reached(gpointer hook_data, gpointer user_data)
{
    mainwin_clear_song_info();

    if (cfg.stopaftersong)
        mainwin_set_stopaftersong(FALSE);
}

static void
ui_main_evlistener_playlist_info_change(gpointer hook_data, gpointer user_data)
{
    PlaylistEventInfoChange *msg = (PlaylistEventInfoChange *) hook_data;

    mainwin_set_song_info(msg->bitrate, msg->samplerate, msg->channels);
}

static void
ui_main_evlistener_mainwin_set_always_on_top(gpointer hook_data, gpointer user_data)
{
    gboolean *ontop = (gboolean*)hook_data;
    mainwin_set_always_on_top(*ontop);
}

static void
ui_main_evlistener_mainwin_show(gpointer hook_data, gpointer user_data)
{
    gboolean *show = (gboolean*)hook_data;
    mainwin_show(*show);
}

static void
ui_main_evlistener_equalizerwin_show(gpointer hook_data, gpointer user_data)
{
    gboolean *show = (gboolean*)hook_data;
    equalizerwin_show(*show);
}

static void
ui_main_evlistener_prefswin_show(gpointer hook_data, gpointer user_data)
{
    gboolean *show = (gboolean*)hook_data;
    if (*show == TRUE)
        show_prefs_window();
    else
        hide_prefs_window();
}

static void
ui_main_evlistener_aboutwin_show(gpointer hook_data, gpointer user_data)
{
    gboolean *show = (gboolean*)hook_data;
    if (*show == TRUE)
        show_about_window();
    else
        hide_about_window();
}


static void
ui_main_evlistener_ui_jump_to_track_show(gpointer hook_data, gpointer user_data)
{
    gboolean *show = (gboolean*)hook_data;
    if (*show == TRUE)
        ui_jump_to_track();
    else
        ui_jump_to_track_hide();
}

static void
ui_main_evlistener_filebrowser_show(gpointer hook_data, gpointer user_data)
{
    gboolean *play_button = (gboolean*)hook_data;
    run_filebrowser(*play_button);
}

static void
ui_main_evlistener_filebrowser_hide(gpointer hook_data, gpointer user_data)
{
    hide_filebrowser();
}

static void
ui_main_evlistener_visualization_timeout(gpointer hook_data, gpointer user_data)
{
    if (hook_data == NULL) {
        if (cfg.player_shaded && cfg.player_visible)
            ui_svis_timeout_func(mainwin_svis, NULL);
        else
            ui_vis_timeout_func(mainwin_vis, NULL);
        return;
    }

    VisNode *vis = (VisNode*) hook_data;

    guint8 intern_vis_data[512];
    gint16 mono_freq[2][256];
    gboolean mono_freq_calced = FALSE;
    gint16 mono_pcm[2][512], stereo_pcm[2][512];
    gboolean mono_pcm_calced = FALSE, stereo_pcm_calced = FALSE;
    gint i;

    if (cfg.vis_type == VIS_OFF)
        return;

    if (cfg.vis_type == VIS_ANALYZER) {
            /* Spectrum analyzer */
            /* 76 values */
            const gint long_xscale[] =
                { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                17, 18,
                19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
                34,
                35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
                50, 51,
                52, 53, 54, 55, 56, 57, 58, 61, 66, 71, 76, 81, 87, 93,
                100, 107,
                114, 122, 131, 140, 150, 161, 172, 184, 255
            };
            /* 20 values */
            const int short_xscale[] =
                { 0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 15, 20, 27,
                36, 47, 62, 82, 107, 141, 184, 255
            };
            const double y_scale = 3.60673760222;   /* 20.0 / log(256) */
            const int *xscale;
            gint j, y, max;

            if (!mono_freq_calced)
                calc_mono_freq(mono_freq, vis->data, vis->nch);

            memset(intern_vis_data, 0, 75);

            if (cfg.analyzer_type == ANALYZER_BARS) {
                if (cfg.player_shaded) {
                    max = 13;
                }
                else {
                    max = 19;
                }
                xscale = short_xscale;
            }
            else {
                if (cfg.player_shaded) {
                    max = 37;
                }
                else {
                    max = 75;
                }
                xscale = long_xscale;
            }

            for (i = 0; i < max; i++) {
                for (j = xscale[i], y = 0; j < xscale[i + 1]; j++) {
                    if (mono_freq[0][j] > y)
                        y = mono_freq[0][j];
                }
                y >>= 7;
                if (y != 0) {
                    intern_vis_data[i] = log(y) * y_scale;
                    if (intern_vis_data[i] > 15)
                        intern_vis_data[i] = 15;
                }
                else
                    intern_vis_data[i] = 0;
            }
        
    }
    else if(cfg.vis_type == VIS_VOICEPRINT){
        if (cfg.player_shaded && cfg.player_visible) {
            /* VU */
            gint vu, val;

            if (!stereo_pcm_calced)
                calc_stereo_pcm(stereo_pcm, vis->data, vis->nch);
            vu = 0;
            for (i = 0; i < 512; i++) {
                val = abs(stereo_pcm[0][i]);
                if (val > vu)
                    vu = val;
            }
            intern_vis_data[0] = (vu * 37) >> 15;
            if (intern_vis_data[0] > 37)
                intern_vis_data[0] = 37;
            if (vis->nch == 2) {
                vu = 0;
                for (i = 0; i < 512; i++) {
                    val = abs(stereo_pcm[1][i]);
                    if (val > vu)
                        vu = val;
                }
                intern_vis_data[1] = (vu * 37) >> 15;
                if (intern_vis_data[1] > 37)
                    intern_vis_data[1] = 37;
            }
            else
                intern_vis_data[1] = intern_vis_data[0];
        }
        else { /*Voiceprint*/
            if (!mono_freq_calced)
                calc_mono_freq(mono_freq, vis->data, vis->nch);
            memset(intern_vis_data, 0, 256);

            /* For the values [0-16] we use the frequency that's 3/2 as much.
               If we assume the 512 values calculated by calc_mono_freq to
               cover 0-22kHz linearly we get a range of
               [0-16] * 3/2 * 22000/512 = [0-1,031] Hz.
               Most stuff above that is harmonics and we want to utilize the
               16 samples we have to the max[tm]
               */
            for (i = 0; i < 50 ; i+=3){
                intern_vis_data[i/3] += (mono_freq[0][i/2] >> 5);

                /*Boost frequencies above 257Hz a little*/
                //if(i > 4 * 3)
                //  intern_vis_data[i/3] += 8;
            }
        }
    }
    else { /* (cfg.vis_type == VIS_SCOPE) */

        /* Oscilloscope */
        gint pos, step;

        if (!mono_pcm_calced)
            calc_mono_pcm(mono_pcm, vis->data, vis->nch);

        step = (vis->length << 8) / 74;
        for (i = 0, pos = 0; i < 75; i++, pos += step) {
            intern_vis_data[i] = ((mono_pcm[0][pos >> 8]) >> 12) + 7;
            if (intern_vis_data[i] == 255)
                intern_vis_data[i] = 0;
            else if (intern_vis_data[i] > 12)
                intern_vis_data[i] = 12;
            /* Do not see the point of that? (comparison always false) -larne.
               if (intern_vis_data[i] < 0)
               intern_vis_data[i] = 0; */
        }
    }

    if (cfg.player_shaded && cfg.player_visible)
        ui_svis_timeout_func(mainwin_svis, intern_vis_data);
    else
        ui_vis_timeout_func(mainwin_vis, intern_vis_data);
}

static void
ui_main_evlistener_config_save(gpointer hook_data, gpointer user_data)
{
    ConfigDb *db = (ConfigDb *) hook_data;

    if (SKINNED_WINDOW(mainwin)->x != -1 &&
        SKINNED_WINDOW(mainwin)->y != -1 )
    {
        cfg_db_set_int(db, NULL, "player_x", SKINNED_WINDOW(mainwin)->x);
        cfg_db_set_int(db, NULL, "player_y", SKINNED_WINDOW(mainwin)->y);
    }

    cfg_db_set_bool(db, NULL, "mainwin_use_bitmapfont",
                    cfg.mainwin_use_bitmapfont);
}

static void
ui_main_evlistener_equalizer_changed(gpointer hook_data, gpointer user_data)
{
    output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                  cfg.equalizer_bands);
}

static void
ui_main_evlistener_urlopener_show(gpointer hook_data, gpointer user_data)
{
    show_add_url_window();
}

void
ui_main_evlistener_init(void)
{
    hook_associate("title change", ui_main_evlistener_title_change, NULL);
    hook_associate("hide seekbar", ui_main_evlistener_hide_seekbar, NULL);
    hook_associate("volume set", ui_main_evlistener_volume_change, NULL);
    hook_associate("playback initiate", ui_main_evlistener_playback_initiate, NULL);
    hook_associate("playback begin", ui_main_evlistener_playback_begin, NULL);
    hook_associate("playback stop", ui_main_evlistener_playback_stop, NULL);
    hook_associate("playback pause", ui_main_evlistener_playback_pause, NULL);
    hook_associate("playback unpause", ui_main_evlistener_playback_unpause, NULL);
    hook_associate("playback seek", ui_main_evlistener_playback_seek, NULL);
    hook_associate("playback play file", ui_main_evlistener_playback_play_file, NULL);
    hook_associate("playlist end reached", ui_main_evlistener_playlist_end_reached, NULL);
    hook_associate("playlist info change", ui_main_evlistener_playlist_info_change, NULL);
    hook_associate("mainwin set always on top", ui_main_evlistener_mainwin_set_always_on_top, NULL);
    hook_associate("mainwin show", ui_main_evlistener_mainwin_show, NULL);
    hook_associate("equalizerwin show", ui_main_evlistener_equalizerwin_show, NULL);
    hook_associate("prefswin show", ui_main_evlistener_prefswin_show, NULL);
    hook_associate("aboutwin show", ui_main_evlistener_aboutwin_show, NULL);
    hook_associate("ui jump to track show", ui_main_evlistener_ui_jump_to_track_show, NULL);
    hook_associate("filebrowser show", ui_main_evlistener_filebrowser_show, NULL);
    hook_associate("filebrowser hide", ui_main_evlistener_filebrowser_hide, NULL);
    hook_associate("visualization timeout", ui_main_evlistener_visualization_timeout, NULL);
    hook_associate("config save", ui_main_evlistener_config_save, NULL);
    hook_associate("urlopener show", ui_main_evlistener_urlopener_show, NULL);

    hook_associate("equalizer changed", ui_main_evlistener_equalizer_changed, NULL);
}

