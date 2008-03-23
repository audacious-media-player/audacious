/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious development team.
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

#ifdef USE_SRC
#  include <samplerate.h>
#endif

#include "effect.h"
#include "general.h"
#include "playback.h"
#include "pluginenum.h"
#include "skin.h"
#include "ui_equalizer.h"
#include "ui_playlist.h"
#include "ui_skinned_window.h"
#include "ui_vis.h"
#include "util.h"
#include "visualization.h"

#include "bmpconfig.h"

BmpConfig bmp_default_config = {
    MAINWIN_DEFAULT_POS_X,      /* mainwin x position */
    MAINWIN_DEFAULT_POS_Y,      /* mainwin y position */
    EQUALIZER_DEFAULT_POS_X,    /* equalizer x position */
    EQUALIZER_DEFAULT_POS_Y,    /* equalizer y position */
    PLAYLISTWIN_DEFAULT_POS_X,  /* playlistwin x position */
    PLAYLISTWIN_DEFAULT_POS_Y,  /* playlistwin y position */
    PLAYLISTWIN_DEFAULT_WIDTH,  /* playlistwin width */
    PLAYLISTWIN_DEFAULT_HEIGHT, /* playlistwin height */
    10,                         /* snap distance */
    FALSE,                      /* real-time priority */
    FALSE, FALSE,               /* shuffle, repeat */
    FALSE,                      /* scaling */
    TRUE,                       /* autoscroll */
    TRUE,                       /* analyzer peaks */
    FALSE,                      /* equalizer autoload */
    FALSE,                      /* easy move */
    FALSE,                      /* equalizer active */
    FALSE,                      /* playlistwin visible */
    FALSE,                      /* equalizer visible */
    TRUE,                       /* player visible */
    FALSE,                      /* player shaded */
    FALSE,                      /* playlistwin shaded */
    FALSE,                      /* equalizer shaded */
    FALSE,                      /* allow multiple instances */
    TRUE,                       /* always show cb */
    TRUE, TRUE, TRUE,           /* convert '_', %20 and '\' */
    TRUE,                       /* show numbers in playlist */
    TRUE,                       /* snap windows */
    TRUE,                       /* save window positions */
    TRUE,                       /* dim titlebar */
    FALSE,                      /* get playlist info on load */
    TRUE,                       /* get playlist info on demand */
    TRUE,                       /* equalizer scale linked */
    FALSE,                      /* sort jump to file */
    FALSE,                      /* use effect plugins */
    FALSE,                      /* always on top */
    FALSE,                      /* sticky */
    FALSE,                      /* no playlist advance */
    FALSE,                      /* stop after current song */
    TRUE,                       /* refresh file list */
    TRUE,                       /* UNUSED (smooth title scrolling) */
    TRUE,                       /* use playlist metadata */
    TRUE,                       /* deprecated */
    TRUE,                       /* warn about windows visibility */
    FALSE,                      /* use \ as directory delimiter */
    FALSE,                      /* random skin on play */
    FALSE,                      /* use fontsets */
    TRUE,                       /* use bitmap font for mainwin */
    TRUE,                       /* use custom cursors */
    TRUE,                       /* close dialog on open */
    TRUE,                       /* close dialog on add */
    0.0,                        /* equalizer preamp */
    {0.0, 0.0, 0.0, 0.0, 0.0,             /* equalizer bands */
     0.0, 0.0, 0.0, 0.0, 0.0},
    1.2,                        /* GUI scale factor, hardcoded for testing purposes --majeru */
    NULL,                       /* skin */
    NULL,                       /* output plugin */
    NULL,                       /* file selector path */
    NULL,                       /* playlist path */
    NULL,                       /* playlist font */
    NULL,                       /* mainwin font */
    NULL,                       /* disabled input plugins */
    NULL,                       /* enabled general plugins */
    NULL,                       /* enabled visualization plugins */
    NULL,                       /* enabled effect plugins */
    NULL,                       /* enabled discovery plugins */
    NULL,                       /* equalizer preset default file */
    NULL,                       /* equalizer preset extension */
    NULL,                       /* URL history */
    0,                          /* timer mode */
    VIS_ANALYZER,               /* visualizer type */
    ANALYZER_NORMAL,            /* analyzer mode */
    ANALYZER_BARS,              /* analyzer type */
    SCOPE_DOT,                  /* scope mode */
    VOICEPRINT_NORMAL,          /* voiceprint mode */
    VU_SMOOTH,                  /* VU mode */
    REFRESH_FULL,               /* visualizer refresh rate */
    FALLOFF_FAST,               /* analyzer fall off rate */
    FALLOFF_SLOW,               /* peaks fall off rate */
    0,                          /* playlist position */
    2,                          /* pause between songs time */
    FALSE,                      /* pause between songs */
    FALSE,                      /* show window decorations */
    8,                          /* mouse wheel scroll step */
    FALSE,                      /* playlist transparent */
    2,                          /* 3rd preset (ARTIST - ALBUM - TITLE) */
    NULL,                       /* title format */
    FALSE,                      /* software volume control enabled */
    TRUE,                       /* UNUSED (XMMS compatibility mode) */
    TRUE,                       /* extra eq filtering */
    3,                          /* scroll pl by */
    FALSE,                      /* resume playback on startup */
    -1,                         /* resume playback on startup time */
    TRUE,                       /* show seperators in pl */
    NULL,           /* chardet_detector */
    NULL,           /* chardet_fallback */
    500,            /* audio buffer size */
    TRUE,           /* whether or not to postpone format detection on initial add */
    TRUE,           /* show filepopup for tuple */
    NULL,           /* words identifying covers */
    NULL,           /* words that might not show up in cover names */
    FALSE,
    0,
    NULL,           /* default session uri base (non-NULL = custom session uri base) */
    150,            /* short side length of the picture in the filepopup */
    20,             /* delay until the filepopup comes up */
    FALSE,          /* use filename.jpg for coverart */
    FALSE,          /* use XMMS-style file selection */
    TRUE,           /* use extension probing */
    255, 255, 255,  /* colorize r, g, b */
    FALSE,          /* internal: whether or not to terminate */
    TRUE,           /* whether show progress bar in filepopup or not */
    TRUE,           /* close jtf dialog on jump */
    TRUE,           /* use back and forth scroll */
    FALSE,          /* use software volume control */
    .warn_about_broken_gtk_engines = TRUE,           /* warn about broken gtk themes */
    FALSE,          /* disable inline themes */
    TRUE,           /* remember jtf text entry */
    16,             /* output bit depth */
    TRUE,           /* enable replay gain */
    TRUE,           /* enable clipping prevention */
    TRUE,           /* track mode */
    FALSE,          /* album mode */
    FALSE,          /* enable adaptive scaler */
    0.0,            /* preamp */
    -9.0,           /* default gain */
    0x6464,         /* saved volume for both channels, 0x64=100 */
#ifdef USE_SRC
    FALSE,          /* enable resampling */
    48000,          /* samplerate */
    SRC_SINC_BEST_QUALITY, /* default interpolation method */
#endif
    FALSE,          /* bypass dsp */
};

typedef struct bmp_cfg_boolent_t {
    char const *be_vname;
    gboolean *be_vloc;
    gboolean be_wrt;
} bmp_cfg_boolent;

typedef struct bmp_cfg_nument_t {
    char const *ie_vname;
    gint *ie_vloc;
    gboolean ie_wrt;
} bmp_cfg_nument;

typedef struct bmp_cfg_strent_t {
    char const *se_vname;
    char **se_vloc;
    gboolean se_wrt;
} bmp_cfg_strent;


/* XXX: case-sensitivity is bad for lazy nenolods. :( -nenolod */
static gchar *pl_candidates[] = {
    PLUGIN_FILENAME("ALSA"),
    PLUGIN_FILENAME("coreaudio"),
    PLUGIN_FILENAME("OSS"),
    PLUGIN_FILENAME("sun"),
    PLUGIN_FILENAME("ESD"),
    PLUGIN_FILENAME("pulse_audio"),
    PLUGIN_FILENAME("disk_writer"),
    NULL
};


static bmp_cfg_boolent bmp_boolents[] = {
    {"allow_multiple_instances", &cfg.allow_multiple_instances, TRUE},
    {"use_realtime", &cfg.use_realtime, TRUE},
    {"always_show_cb", &cfg.always_show_cb, TRUE},
    {"convert_underscore", &cfg.convert_underscore, TRUE},
    {"convert_twenty", &cfg.convert_twenty, TRUE},
    {"convert_slash", &cfg.convert_slash, TRUE },
    {"show_numbers_in_pl", &cfg.show_numbers_in_pl, TRUE},
    {"show_separator_in_pl", &cfg.show_separator_in_pl, TRUE},
    {"snap_windows", &cfg.snap_windows, TRUE},
    {"save_window_positions", &cfg.save_window_position, TRUE},
    {"dim_titlebar", &cfg.dim_titlebar, TRUE},
    {"get_info_on_load", &cfg.get_info_on_load, TRUE},
    {"get_info_on_demand", &cfg.get_info_on_demand, TRUE},
    {"eq_scaled_linked", &cfg.eq_scaled_linked, TRUE},
    {"no_playlist_advance", &cfg.no_playlist_advance, TRUE},
    {"refresh_file_list", &cfg.refresh_file_list, TRUE},
    {"sort_jump_to_file", &cfg.sort_jump_to_file, TRUE},
    {"use_pl_metadata", &cfg.use_pl_metadata, TRUE},
    {"warn_about_win_visibility", &cfg.warn_about_win_visibility, TRUE},
    {"use_backslash_as_dir_delimiter", &cfg.use_backslash_as_dir_delimiter, TRUE},
    {"player_shaded", &cfg.player_shaded, TRUE},
    {"player_visible", &cfg.player_visible, TRUE},
    {"shuffle", &cfg.shuffle, TRUE},
    {"repeat", &cfg.repeat, TRUE},
    {"scaled", &cfg.scaled, TRUE},   /* toggles custom scale */
    {"autoscroll_songname", &cfg.autoscroll, TRUE},
    {"stop_after_current_song", &cfg.stopaftersong, TRUE},
    {"playlist_shaded", &cfg.playlist_shaded, TRUE},
    {"playlist_visible", &cfg.playlist_visible, TRUE},
    {"use_fontsets", &cfg.use_fontsets, TRUE},
    {"mainwin_use_bitmapfont", &cfg.mainwin_use_bitmapfont, TRUE},
    {"equalizer_visible", &cfg.equalizer_visible, TRUE},
    {"equalizer_active", &cfg.equalizer_active, TRUE},
    {"equalizer_shaded", &cfg.equalizer_shaded, TRUE},
    {"equalizer_autoload", &cfg.equalizer_autoload, TRUE},
    {"easy_move", &cfg.easy_move, TRUE},
    {"use_eplugins", &cfg.use_eplugins, TRUE},
    {"always_on_top", &cfg.always_on_top, TRUE},
    {"sticky", &cfg.sticky, TRUE},
    {"random_skin_on_play", &cfg.random_skin_on_play, TRUE},
    {"pause_between_songs", &cfg.pause_between_songs, TRUE},
    {"show_wm_decorations", &cfg.show_wm_decorations, TRUE},
    {"eq_extra_filtering", &cfg.eq_extra_filtering, TRUE},
    {"analyzer_peaks", &cfg.analyzer_peaks, TRUE},
    {"custom_cursors", &cfg.custom_cursors, TRUE},
    {"close_dialog_open", &cfg.close_dialog_open, TRUE},
    {"close_dialog_add", &cfg.close_dialog_add, TRUE},
    {"resume_playback_on_startup", &cfg.resume_playback_on_startup, TRUE},
    {"playlist_detect", &cfg.playlist_detect, TRUE},
    {"show_filepopup_for_tuple", &cfg.show_filepopup_for_tuple, TRUE},
    {"recurse_for_cover", &cfg.recurse_for_cover, TRUE},
    {"use_file_cover", &cfg.use_file_cover, TRUE},
    {"use_xmms_style_fileselector", &cfg.use_xmms_style_fileselector, TRUE},
    {"use_extension_probing", &cfg.use_extension_probing, TRUE},
    {"filepopup_showprogressbar", &cfg.filepopup_showprogressbar, TRUE},
    {"close_jtf_dialog", &cfg.close_jtf_dialog, TRUE},
    {"twoway_scroll", &cfg.twoway_scroll, TRUE},
    {"software_volume_control", &cfg.software_volume_control, TRUE},
    {"warn_about_broken_gtk_engines", &cfg.warn_about_broken_gtk_engines, TRUE},
    {"disable_inline_gtk", &cfg.disable_inline_gtk, TRUE},
    {"remember_jtf_entry", &cfg.remember_jtf_entry, TRUE},
    {"enable_replay_gain",         &cfg.enable_replay_gain, TRUE},
    {"enable_clipping_prevention", &cfg.enable_clipping_prevention, TRUE},
    {"replay_gain_track",          &cfg.replay_gain_track, TRUE},
    {"replay_gain_album",          &cfg.replay_gain_album, TRUE},
    {"enable_adaptive_scaler",     &cfg.enable_adaptive_scaler, TRUE},
#ifdef USE_SRC
    {"enable_src",                 &cfg.enable_src, TRUE},
#endif
    {"bypass_dsp",                 &cfg.bypass_dsp, TRUE},
};

static gint ncfgbent = G_N_ELEMENTS(bmp_boolents);

static bmp_cfg_nument bmp_numents[] = {
    {"player_x", &cfg.player_x, TRUE},
    {"player_y", &cfg.player_y, TRUE},
    {"timer_mode", &cfg.timer_mode, TRUE},
    {"vis_type", &cfg.vis_type, TRUE},
    {"analyzer_mode", &cfg.analyzer_mode, TRUE},
    {"analyzer_type", &cfg.analyzer_type, TRUE},
    {"scope_mode", &cfg.scope_mode, TRUE},
    {"vu_mode", &cfg.vu_mode, TRUE},
    {"voiceprint_mode", &cfg.voiceprint_mode, TRUE},
    {"vis_refresh_rate", &cfg.vis_refresh, TRUE},
    {"analyzer_falloff", &cfg.analyzer_falloff, TRUE},
    {"peaks_falloff", &cfg.peaks_falloff, TRUE},
    {"playlist_x", &cfg.playlist_x, TRUE},
    {"playlist_y", &cfg.playlist_y, TRUE},
    {"playlist_width", &cfg.playlist_width, TRUE},
    {"playlist_height", &cfg.playlist_height, TRUE},
    {"playlist_position", &cfg.playlist_position, TRUE},
    {"equalizer_x", &cfg.equalizer_x, TRUE},
    {"equalizer_y", &cfg.equalizer_y, TRUE},
    {"snap_distance", &cfg.snap_distance, TRUE},
    {"pause_between_songs_time", &cfg.pause_between_songs_time, TRUE},
    {"mouse_wheel_change", &cfg.mouse_change, TRUE},
    {"scroll_pl_by", &cfg.scroll_pl_by, TRUE},
    {"titlestring_preset", &cfg.titlestring_preset, TRUE},
    {"resume_playback_on_startup_time", &cfg.resume_playback_on_startup_time, TRUE},
    {"output_buffer_size", &cfg.output_buffer_size, TRUE},
    {"recurse_for_cover_depth", &cfg.recurse_for_cover_depth, TRUE},
    {"filepopup_pixelsize", &cfg.filepopup_pixelsize, TRUE},
    {"filepopup_delay", &cfg.filepopup_delay, TRUE},
    {"colorize_r", &cfg.colorize_r, TRUE},
    {"colorize_g", &cfg.colorize_g, TRUE},
    {"colorize_b", &cfg.colorize_b, TRUE},
    {"output_bit_depth", &cfg.output_bit_depth, TRUE},
    {"saved_volume", &cfg.saved_volume, TRUE},
#ifdef USE_SRC
    {"src_rate", &cfg.src_rate, TRUE},
    {"src_type", &cfg.src_type, TRUE},
#endif
};

static gint ncfgient = G_N_ELEMENTS(bmp_numents);

static bmp_cfg_strent bmp_strents[] = {
    {"playlist_font", &cfg.playlist_font, TRUE},
    {"mainwin_font", &cfg.mainwin_font, TRUE},
    {"eqpreset_default_file", &cfg.eqpreset_default_file, TRUE},
    {"eqpreset_extension", &cfg.eqpreset_extension, TRUE},
    {"skin", &cfg.skin, FALSE},
    {"output_plugin", &cfg.outputplugin, FALSE},
    {"disabled_iplugins", &cfg.disabled_iplugins, TRUE},
    {"enabled_gplugins", &cfg.enabled_gplugins, FALSE},
    {"enabled_vplugins", &cfg.enabled_vplugins, FALSE},
    {"enabled_eplugins", &cfg.enabled_eplugins, FALSE},
    {"filesel_path", &cfg.filesel_path, FALSE},
    {"playlist_path", &cfg.playlist_path, FALSE},
    {"generic_title_format", &cfg.gentitle_format, TRUE},
    {"chardet_detector", &cfg.chardet_detector, TRUE},
    {"chardet_fallback", &cfg.chardet_fallback, TRUE},
    {"cover_name_include", &cfg.cover_name_include, TRUE},
    {"cover_name_exclude", &cfg.cover_name_exclude, TRUE},
    {"session_uri_base", &cfg.session_uri_base, TRUE}
};

static gint ncfgsent = G_N_ELEMENTS(bmp_strents);


static gboolean
save_extra_playlist(const gchar * path, const gchar * basename,
        gpointer savedlist)
{
    GList *playlists, *iter;
    GList **saved;
    Playlist *playlist;
    int found;
    gchar *filename;

    playlists = playlist_get_playlists();
    saved = (GList **) savedlist;

    found = 0;
    for (iter = playlists; iter; iter = iter->next) {
        playlist = (Playlist *) iter->data;
        if (g_list_find(*saved, playlist)) continue;
        filename = playlist_filename_get(playlist);
        if (!filename) continue;
        if (strcmp(filename, path) == 0) {
            /* Save playlist */
            playlist_save(playlist, path);
            *saved = g_list_prepend(*saved, playlist);
            found = 1;
            g_free(filename);
            break;
        }
        g_free(filename);
    }

    if(!found) {
        /* Remove playlist */
        unlink(path);
    }

    return FALSE; /* process other playlists */
}

static void
save_other_playlists(GList *saved)
{
    GList *playlists, *iter;
    Playlist *playlist;
    gchar *pos, *ext, *basename, *filename, *newbasename;
    int i, num, isdigits;

    playlists = playlist_get_playlists();
    for(iter = playlists; iter; iter = iter->next) {
        playlist = (Playlist *) iter->data;
        if (g_list_find(saved, playlist)) continue;
        filename = playlist_filename_get(playlist);
        if (!filename || !filename[0]
                || g_file_test(filename, G_FILE_TEST_IS_DIR)) {
            /* default basename */
#ifdef HAVE_XSPF_PLAYLIST
            basename = g_strdup("playlist_01.xspf");
#else
            basename = g_strdup("playlist_01.m3u");
#endif
        } else {
            basename = g_path_get_basename(filename);
        }
        g_free(filename);
        if ((pos = strrchr(basename, '.'))) {
            *pos = '\0';
        }
#ifdef HAVE_XSPF_PLAYLIST
        ext = ".xspf";
#else
        ext = ".m3u";
#endif
        num = -1;
        if ((pos = strrchr(basename, '_'))) {
            isdigits = 0;
            for (i=1; pos[i]; i++) {
                if (!g_ascii_isdigit(pos[i])) {
                    isdigits = 0;
                    break;
                }
                isdigits = 1;
            }
            if (isdigits) {
                num = atoi(pos+1) + 1;
                *pos = '\0';
            }
        }
        /* attempt to generate unique filename */
        filename = NULL;
        do {
            g_free(filename);
            if (num < 0) {
                /* try saving without number first */
                newbasename = g_strdup_printf("%s%s", basename, ext);
                num = 1;
            } else {
                newbasename = g_strdup_printf("%s_%02d%s", basename, num, ext);
                num++;
                if (num < 0) {
                    g_warning("Playlist number in filename overflowed."
                            " Not saving playlist.\n");
                    goto cleanup;
                }
            }
            filename = g_build_filename(bmp_paths[BMP_PATH_PLAYLISTS_DIR],
                    newbasename, NULL);
            g_free(newbasename);
        } while (g_file_test(filename, G_FILE_TEST_EXISTS));

        playlist_save(playlist, filename);
cleanup:
        g_free(filename);
        g_free(basename);
    }
}


void
bmp_config_free(void)
{
  gint i;
  for (i = 0; i < ncfgsent; ++i) {
    if ( *(bmp_strents[i].se_vloc) != NULL )
    {
      g_free( *(bmp_strents[i].se_vloc) );
      *(bmp_strents[i].se_vloc) = NULL;
    }
  }
}

void
bmp_config_load(void)
{
    ConfigDb *db;
    gint i, length;

    memcpy(&cfg, &bmp_default_config, sizeof(BmpConfig));

    db = cfg_db_open();
    for (i = 0; i < ncfgbent; ++i) {
        cfg_db_get_bool(db, NULL,
                            bmp_boolents[i].be_vname,
                            bmp_boolents[i].be_vloc);
    }

    for (i = 0; i < ncfgient; ++i) {
        cfg_db_get_int(db, NULL,
                           bmp_numents[i].ie_vname,
                           bmp_numents[i].ie_vloc);
    }

    for (i = 0; i < ncfgsent; ++i) {
        cfg_db_get_string(db, NULL,
                              bmp_strents[i].se_vname,
                              bmp_strents[i].se_vloc);
    }

    /* Preset */
    cfg_db_get_float(db, NULL, "equalizer_preamp", &cfg.equalizer_preamp);
    for (i = 0; i < 10; i++) {
        gchar eqtext[18];

        g_snprintf(eqtext, sizeof(eqtext), "equalizer_band%d", i);
        cfg_db_get_float(db, NULL, eqtext, &cfg.equalizer_bands[i]);
    }

    cfg_db_get_int(db,NULL, "saved_volume", &cfg.saved_volume);

    /* custom scale factor */
    cfg_db_get_float(db, NULL, "scale_factor", &cfg.scale_factor);

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


    if (cfg.playlist_font && strlen(cfg.playlist_font) == 0) {
        g_free(cfg.playlist_font);
        cfg.playlist_font = NULL;
    }

    if (cfg.mainwin_font && strlen(cfg.mainwin_font) == 0) {
        g_free(cfg.mainwin_font);
        cfg.mainwin_font = NULL;
    }

    if (!cfg.playlist_font)
        cfg.playlist_font = g_strdup(MAINWIN_DEFAULT_FONT);

    if (!cfg.mainwin_font)
        cfg.mainwin_font = g_strdup(PLAYLISTWIN_DEFAULT_FONT);

    if (!cfg.gentitle_format)
        cfg.gentitle_format = g_strdup("${?artist:${artist} - }${?album:${album} - }${title}");

    if (!cfg.outputplugin) {
    gint iter;
        gchar *pl_path = g_build_filename(PLUGIN_DIR, plugin_dir_list[0], NULL);

        for (iter = 0; pl_candidates[iter] != NULL && cfg.outputplugin == NULL; iter++)
    {
         cfg.outputplugin = find_file_recursively(pl_path, pl_candidates[iter]);
    }

        g_free(pl_path);
    }

    if (!cfg.eqpreset_default_file)
        cfg.eqpreset_default_file = g_strdup(EQUALIZER_DEFAULT_DIR_PRESET);

    if (!cfg.eqpreset_extension)
        cfg.eqpreset_extension = g_strdup(EQUALIZER_DEFAULT_PRESET_EXT);

    if (!cfg.chardet_detector)
        cfg.chardet_detector = g_strdup("");

    if (!cfg.chardet_fallback)
        cfg.chardet_fallback = g_strdup("");

    if (!cfg.cover_name_include)
        cfg.cover_name_include = g_strdup("");

    if (!cfg.cover_name_exclude)
        cfg.cover_name_exclude = g_strdup("back");

    if (!cfg.session_uri_base)
        cfg.session_uri_base = g_strdup("");

    /* at least one of these should be true */
    if ((!cfg.get_info_on_demand) && (!cfg.get_info_on_load))
        cfg.get_info_on_demand = TRUE;

    /* playlist width and height can't be smaller than minimum */
    cfg.playlist_width = MAX(cfg.playlist_width, PLAYLISTWIN_MIN_WIDTH);
    cfg.playlist_height = MAX(cfg.playlist_height, PLAYLISTWIN_MIN_HEIGHT);
}

void
bmp_config_save(void)
{
    GList *node;
    gchar *str;
    gint i, cur_pb_time, vol_l, vol_r;
    ConfigDb *db;
    GList *saved;
    Playlist *playlist = playlist_get_active();

    cfg.disabled_iplugins = input_stringify_disabled_list();


    db = cfg_db_open();

    for (i = 0; i < ncfgbent; ++i)
        if (bmp_boolents[i].be_wrt)
            cfg_db_set_bool(db, NULL,
                                bmp_boolents[i].be_vname,
                                *bmp_boolents[i].be_vloc);

    for (i = 0; i < ncfgient; ++i)
        if (bmp_numents[i].ie_wrt)
            cfg_db_set_int(db, NULL,
                               bmp_numents[i].ie_vname,
                               *bmp_numents[i].ie_vloc);

    /* This is a bit lame .. it'll end up being written twice,
     * could do with being done a bit neater.  -larne   */
    cfg_db_set_int(db, NULL, "playlist_position",
                       playlist_get_position(playlist));

    /* FIXME: we're looking up SkinnedWindow::x &c ourselves here.
     * this isn't exactly right. -nenolod
     */
    if ( playlistwin &&
         SKINNED_WINDOW(playlistwin)->x != -1 &&
         SKINNED_WINDOW(playlistwin)->y != -1 )
    {
        cfg_db_set_int(db, NULL, "playlist_x",
                           SKINNED_WINDOW(playlistwin)->x);
        cfg_db_set_int(db, NULL, "playlist_y",
                           SKINNED_WINDOW(playlistwin)->y);
    }
    
    if ( mainwin &&
         SKINNED_WINDOW(mainwin)->x != -1 &&
         SKINNED_WINDOW(mainwin)->y != -1 )
    {
        cfg_db_set_int(db, NULL, "player_x",
                           SKINNED_WINDOW(mainwin)->x);
        cfg_db_set_int(db, NULL, "player_y",
                           SKINNED_WINDOW(mainwin)->y);
    }

    if ( equalizerwin &&
         SKINNED_WINDOW(equalizerwin)->x != -1 &&
         SKINNED_WINDOW(equalizerwin)->y != -1 )
    {
        cfg_db_set_int(db, NULL, "equalizer_x",
                           SKINNED_WINDOW(equalizerwin)->x);
        cfg_db_set_int(db, NULL, "equalizer_y",
                           SKINNED_WINDOW(equalizerwin)->y);
    }

    cfg_db_set_bool(db, NULL, "mainwin_use_bitmapfont",
            cfg.mainwin_use_bitmapfont);

    for (i = 0; i < ncfgsent; ++i) {
        if (bmp_strents[i].se_wrt)
            cfg_db_set_string(db, NULL,
                                  bmp_strents[i].se_vname,
                                  *bmp_strents[i].se_vloc);
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

    cfg_db_set_float(db, NULL, "scale_factor", cfg.scale_factor);

    if (bmp_active_skin != NULL)
    {
        if (bmp_active_skin->path)
            cfg_db_set_string(db, NULL, "skin", bmp_active_skin->path);
        else
            cfg_db_unset_key(db, NULL, "skin");
    }

    if (get_current_output_plugin())
        cfg_db_set_string(db, NULL, "output_plugin",
                              get_current_output_plugin()->filename);
    else
        cfg_db_unset_key(db, NULL, "output_plugin");

    str = general_stringify_enabled_list();
    if (str) {
        cfg_db_set_string(db, NULL, "enabled_gplugins", str);
        g_free(str);
    }
    else
        cfg_db_unset_key(db, NULL, "enabled_gplugins");

    str = vis_stringify_enabled_list();
    if (str) {
        cfg_db_set_string(db, NULL, "enabled_vplugins", str);
        g_free(str);
    }
    else
        cfg_db_unset_key(db, NULL, "enabled_vplugins");

    str = effect_stringify_enabled_list();
    if (str) {
        cfg_db_set_string(db, NULL, "enabled_eplugins", str);
        g_free(str);
    }
    else
        cfg_db_unset_key(db, NULL, "enabled_eplugins");

    if (cfg.filesel_path)
        cfg_db_set_string(db, NULL, "filesel_path", cfg.filesel_path);

    if (cfg.playlist_path)
        cfg_db_set_string(db, NULL, "playlist_path", cfg.playlist_path);

    cfg_db_set_int(db, NULL, "url_history_length",
                       g_list_length(cfg.url_history));
    
    input_get_volume(&vol_l,&vol_r);
    cfg_db_set_int(db, NULL, "saved_volume", (vol_l<<8) | vol_r );
    
    for (node = cfg.url_history, i = 1; node; node = g_list_next(node), i++) {
        str = g_strdup_printf("url_history%d", i);
        cfg_db_set_string(db, NULL, str, node->data);
        g_free(str);
    }

    if (playback_get_playing()) {
        cur_pb_time = playback_get_time();
    } else
        cur_pb_time = -1;
    cfg.resume_playback_on_startup_time = cur_pb_time;
    cfg_db_set_int(db, NULL, "resume_playback_on_startup_time",
               cfg.resume_playback_on_startup_time);

    cfg_db_close(db);

    playlist_save(playlist, bmp_paths[BMP_PATH_PLAYLIST_FILE]);

    /* Save extra playlists that were loaded from PLAYLISTS_DIR  */
    saved = NULL;
    saved = g_list_prepend(saved, playlist); /* don't save default again */
    if(!dir_foreach(bmp_paths[BMP_PATH_PLAYLISTS_DIR], save_extra_playlist,
            &saved, NULL)) {
        g_warning("Could not save extra playlists\n");
    }

    /* Save other playlists to PLAYLISTS_DIR */
    save_other_playlists(saved);
    g_list_free(saved);
}
