/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2010  Audacious development team.
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

#include <glib.h>
#include <libaudcore/hook.h>

#include "audconfig.h"
#include "configdb.h"
#include "playback.h"

AudConfig cfg = {
    .shuffle = FALSE,
    .repeat = FALSE,
    .equalizer_autoload = FALSE,
    .equalizer_active = FALSE,
    .playlist_visible = FALSE,
    .equalizer_visible = FALSE,
    .player_visible = TRUE,
    .show_numbers_in_pl = TRUE,
    .leading_zero = TRUE,
    .no_playlist_advance = FALSE,
    .advance_on_delete = FALSE,
    .clear_playlist = TRUE,
    .open_to_temporary = FALSE,
    .stopaftersong = FALSE,
    .close_dialog_open = TRUE,
    .equalizer_preamp = 0.0,
    .equalizer_bands = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    .filesel_path = NULL,
    .playlist_path = NULL,
    .eqpreset_default_file = NULL,
    .eqpreset_extension = NULL,
    .url_history = NULL,
    .resume_playback_on_startup = FALSE,
    .resume_state = 0,
    .resume_playback_on_startup_time = 0,
    .chardet_detector = NULL,
    .chardet_fallback = NULL,
    .chardet_fallback_s = NULL,
    .output_buffer_size = 500,
    .show_filepopup_for_tuple = TRUE,
    .cover_name_include = NULL,        /* words identifying covers */
    .cover_name_exclude = NULL,        /* words that might not show up in cover names */
    .recurse_for_cover = FALSE,
    .recurse_for_cover_depth = 0,
    .filepopup_pixelsize = 150,        /* short side length of the picture in the filepopup */
    .filepopup_delay = 20,             /* delay until the filepopup comes up */
    .use_file_cover = FALSE,           /* use filename.jpg for coverart */
    .filepopup_showprogressbar = TRUE,
    .close_jtf_dialog = TRUE,          /* close jtf dialog on jump */
    .software_volume_control = FALSE,
    .remember_jtf_entry = TRUE,
    .output_bit_depth = 16,
    .enable_replay_gain = TRUE,
    .enable_clipping_prevention = TRUE,
    .replay_gain_track = TRUE,         /* track mode */
    .replay_gain_album = FALSE,        /* album mode */
    .replay_gain_preamp = 0,
    .default_gain = 0,
    .sw_volume_left = 100, .sw_volume_right = 100,

    /* libaudgui stuff */
    .no_confirm_playlist_delete = FALSE,
    .playlist_manager_x = 0,
    .playlist_manager_y = 0,
    .playlist_manager_width = 0,
    .playlist_manager_height = 0,
    .playlist_manager_close_on_activate = FALSE,

    /* not saved */
    .verbose = FALSE,
};

typedef struct aud_cfg_boolent_t {
    char const *be_vname;
    gboolean *be_vloc;
    gboolean be_wrt;
} aud_cfg_boolent;

typedef struct aud_cfg_nument_t {
    char const *ie_vname;
    gint *ie_vloc;
    gboolean ie_wrt;
} aud_cfg_nument;

typedef struct aud_cfg_strent_t {
    char const *se_vname;
    char **se_vloc;
    gboolean se_wrt;
} aud_cfg_strent;

static aud_cfg_boolent aud_boolents[] = {
    {"show_numbers_in_pl", &cfg.show_numbers_in_pl, TRUE},
    {"leading_zero", & cfg.leading_zero, TRUE},
    {"no_playlist_advance", &cfg.no_playlist_advance, TRUE},
    {"advance_on_delete", & cfg.advance_on_delete, TRUE},
    {"clear_playlist", & cfg.clear_playlist, TRUE},
    {"open_to_temporary", & cfg.open_to_temporary, TRUE},
    {"player_visible", &cfg.player_visible, TRUE},
    {"shuffle", &cfg.shuffle, TRUE},
    {"repeat", &cfg.repeat, TRUE},
    {"stop_after_current_song", &cfg.stopaftersong, TRUE},
    {"playlist_visible", &cfg.playlist_visible, TRUE},
    {"equalizer_visible", &cfg.equalizer_visible, TRUE},
    {"equalizer_active", &cfg.equalizer_active, TRUE},
    {"equalizer_autoload", &cfg.equalizer_autoload, TRUE},
    {"close_dialog_open", &cfg.close_dialog_open, TRUE},
    {"resume_playback_on_startup", &cfg.resume_playback_on_startup, TRUE},
    {"show_filepopup_for_tuple", &cfg.show_filepopup_for_tuple, TRUE},
    {"recurse_for_cover", &cfg.recurse_for_cover, TRUE},
    {"use_file_cover", &cfg.use_file_cover, TRUE},
    {"filepopup_showprogressbar", &cfg.filepopup_showprogressbar, TRUE},
    {"close_jtf_dialog", &cfg.close_jtf_dialog, TRUE},
    {"software_volume_control", &cfg.software_volume_control, TRUE},
    {"remember_jtf_entry", &cfg.remember_jtf_entry, TRUE},
    {"enable_replay_gain", &cfg.enable_replay_gain, TRUE},
    {"enable_clipping_prevention", &cfg.enable_clipping_prevention, TRUE},
    {"replay_gain_track", &cfg.replay_gain_track, TRUE},
    {"replay_gain_album", &cfg.replay_gain_album, TRUE},
    {"no_confirm_playlist_delete", &cfg.no_confirm_playlist_delete, TRUE},
    {"playlist_manager_close_on_activate",
     & cfg.playlist_manager_close_on_activate, TRUE},
};

static gint ncfgbent = G_N_ELEMENTS(aud_boolents);

static aud_cfg_nument aud_numents[] = {
    {"titlestring_preset", &cfg.titlestring_preset, TRUE},
    {"resume_state", & cfg.resume_state, TRUE},
    {"resume_playback_on_startup_time", &cfg.resume_playback_on_startup_time, TRUE},
    {"output_buffer_size", &cfg.output_buffer_size, TRUE},
    {"recurse_for_cover_depth", &cfg.recurse_for_cover_depth, TRUE},
    {"filepopup_pixelsize", &cfg.filepopup_pixelsize, TRUE},
    {"filepopup_delay", &cfg.filepopup_delay, TRUE},
    {"output_bit_depth", &cfg.output_bit_depth, TRUE},
    {"sw_volume_left", & cfg.sw_volume_left, TRUE},
    {"sw_volume_right", & cfg.sw_volume_right, TRUE},
    {"playlist_manager_x", & cfg.playlist_manager_x, TRUE},
    {"playlist_manager_y", & cfg.playlist_manager_y, TRUE},
    {"playlist_manager_width", & cfg.playlist_manager_width, TRUE},
    {"playlist_manager_height", & cfg.playlist_manager_height, TRUE},
};

static gint ncfgient = G_N_ELEMENTS(aud_numents);

static aud_cfg_strent aud_strents[] = {
    {"eqpreset_default_file", &cfg.eqpreset_default_file, TRUE},
    {"eqpreset_extension", &cfg.eqpreset_extension, TRUE},
    {"filesel_path", &cfg.filesel_path, FALSE},
    {"playlist_path", &cfg.playlist_path, FALSE},
    {"generic_title_format", &cfg.gentitle_format, TRUE},
    {"chardet_detector", &cfg.chardet_detector, TRUE},
    {"chardet_fallback", &cfg.chardet_fallback, TRUE},
    {"cover_name_include", &cfg.cover_name_include, TRUE},
    {"cover_name_exclude", &cfg.cover_name_exclude, TRUE},
};

static gint ncfgsent = G_N_ELEMENTS(aud_strents);

void aud_config_chardet_update(void)
{
    if (cfg.chardet_fallback_s != NULL)
        g_strfreev(cfg.chardet_fallback_s);
    cfg.chardet_fallback_s = g_strsplit_set(cfg.chardet_fallback, " ,:;|/", 0);
}


void
aud_config_load(void)
{
    mcs_handle_t *db;
    gint i, length;

    if (! (db = cfg_db_open ()))
        return;

    for (i = 0; i < ncfgbent; ++i) {
        cfg_db_get_bool(db, NULL,
                            aud_boolents[i].be_vname,
                            aud_boolents[i].be_vloc);
    }

    for (i = 0; i < ncfgient; ++i) {
        cfg_db_get_int(db, NULL,
                           aud_numents[i].ie_vname,
                           aud_numents[i].ie_vloc);
    }

    for (i = 0; i < ncfgsent; ++i) {
        cfg_db_get_string(db, NULL,
                              aud_strents[i].se_vname,
                              aud_strents[i].se_vloc);
    }

    /* Preset */
    cfg_db_get_float(db, NULL, "equalizer_preamp", &cfg.equalizer_preamp);
    for (i = 0; i < AUD_EQUALIZER_NBANDS; i++) {
        gchar eqtext[32];

        g_snprintf(eqtext, sizeof(eqtext), "equalizer_band%d", i);
        cfg_db_get_float(db, NULL, eqtext, &cfg.equalizer_bands[i]);
    }

    /* History */
    if (cfg_db_get_int(db, NULL, "url_history_length", &length)) {
        for (i = 1; i <= length; i++) {
            gchar str[19], *tmp;

            g_snprintf(str, sizeof(str), "url_history%d", i);
            if (cfg_db_get_string(db, NULL, str, &tmp))
                cfg.url_history = g_list_append(cfg.url_history, tmp);
        }
    }

    /* RG settings */
    cfg_db_get_float(db, NULL, "replay_gain_preamp", &cfg.replay_gain_preamp);
    cfg_db_get_float(db, NULL, "default_gain", &cfg.default_gain);

    cfg_db_close(db);

    if (!cfg.gentitle_format)
        cfg.gentitle_format = g_strdup("${?artist:${artist} - }${?album:${album} - }${title}");

    if (!cfg.chardet_detector)
        cfg.chardet_detector = g_strdup("");

    if (!cfg.chardet_fallback)
        cfg.chardet_fallback = g_strdup("");

    aud_config_chardet_update();

    if (!cfg.cover_name_include)
        cfg.cover_name_include = g_strdup("album,folder");

    if (!cfg.cover_name_exclude)
        cfg.cover_name_exclude = g_strdup("back");
}

void
aud_config_save(void)
{
    GList *node;
    gchar *str;
    gint i;
    mcs_handle_t *db;

    hook_call ("config save", NULL);

    cfg.resume_state = playback_get_playing () ? (playback_get_paused () ? 2 :
     1) : 0;
    cfg.resume_playback_on_startup_time = playback_get_playing () ?
     playback_get_time () : 0;

    if (! (db = cfg_db_open ()))
        return;

    for (i = 0; i < ncfgbent; ++i)
        if (aud_boolents[i].be_wrt)
            cfg_db_set_bool(db, NULL,
                                aud_boolents[i].be_vname,
                                *aud_boolents[i].be_vloc);

    for (i = 0; i < ncfgient; ++i)
        if (aud_numents[i].ie_wrt)
            cfg_db_set_int(db, NULL,
                               aud_numents[i].ie_vname,
                               *aud_numents[i].ie_vloc);

    for (i = 0; i < ncfgsent; ++i) {
        if (aud_strents[i].se_wrt)
            cfg_db_set_string(db, NULL,
                                  aud_strents[i].se_vname,
                                  *aud_strents[i].se_vloc);
    }

    cfg_db_set_float(db, NULL, "equalizer_preamp", cfg.equalizer_preamp);

    /* RG settings */
    cfg_db_set_float(db, NULL, "replay_gain_preamp", cfg.replay_gain_preamp);
    cfg_db_set_float(db, NULL, "default_gain",       cfg.default_gain);

    for (i = 0; i < 10; i++) {
        str = g_strdup_printf("equalizer_band%d", i);
        cfg_db_set_float(db, NULL, str, cfg.equalizer_bands[i]);
        g_free(str);
    }

    if (cfg.filesel_path)
        cfg_db_set_string(db, NULL, "filesel_path", cfg.filesel_path);

    if (cfg.playlist_path)
        cfg_db_set_string(db, NULL, "playlist_path", cfg.playlist_path);

    cfg_db_set_int(db, NULL, "url_history_length",
                       g_list_length(cfg.url_history));

    for (node = cfg.url_history, i = 1; node; node = g_list_next(node), i++) {
        str = g_strdup_printf("url_history%d", i);
        cfg_db_set_string(db, NULL, str, node->data);
        g_free(str);
    }

    cfg_db_close(db);
}
