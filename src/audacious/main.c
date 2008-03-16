/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team.
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

#include "main.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <time.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#ifdef USE_SRC
#  include <samplerate.h>
#endif

#include "platform/smartinclude.h"

#include "configdb.h"
#include "vfs.h"

#ifdef USE_DBUS
#  include "dbus-service.h"
#  include "audctrl.h"
#endif

#include "auddrct.h"
#include "build_stamp.h"
#include "dnd.h"
#include "effect.h"
#include "general.h"
#include "hints.h"
#include "input.h"
#include "logger.h"
#include "output.h"
#include "playback.h"
#include "playlist.h"
#include "pluginenum.h"
#include "signals.h"
#include "skin.h"
#include "ui_equalizer.h"
#include "ui_fileinfo.h"
#include "ui_main.h"
#include "ui_manager.h"
#include "ui_playlist.h"
#include "ui_preferences.h"
#include "ui_skinned_window.h"
#include "ui_skinselector.h"
#include "util.h"
#include "visualization.h"

#include "libSAD.h"
#include "eggsmclient.h"
#include "eggdesktopfile.h"

#include "icons-stock.h"
#include "images/audacious_player.xpm"

gboolean has_x11_connection = FALSE;    /* do we have an X11 connection? */
const gchar *application_name = N_("Audacious");

struct _AudCmdLineOpt {
    gchar **filenames;
    gint session;
    gboolean play, stop, pause, fwd, rew, play_pause, show_jump_box;
    gboolean enqueue, mainwin, remote, activate;
    gboolean load_skins;
    gboolean headless;
    gboolean no_log;
    gboolean enqueue_to_temp;
    gboolean version;
    gchar *previous_session_id;
	gboolean macpack;
};

typedef struct _AudCmdLineOpt AudCmdLineOpt;

AudCmdLineOpt options;

BmpConfig cfg;

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

const gchar *bmp_titlestring_presets[] = {
    "${title}",
    "${?artist:${artist} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${?track-number:${track-number}. }${title}",
    "${?artist:${artist} }${?album:[ ${album} ] }${?artist:- }${?track-number:${track-number}. }${title}",
    "${?album:${album} - }${title}"
};

const guint n_titlestring_presets = G_N_ELEMENTS(bmp_titlestring_presets);

const gchar *chardet_detector_presets[] = {
    N_("None"),
    N_("Japanese"),
    N_("Taiwanese"),
    N_("Chinese"),
    N_("Korean"),
    N_("Russian"),
    N_("Greek"),
    N_("Hebrew"),
    N_("Turkish"),
    N_("Arabic"),
#ifdef HAVE_UDET
    N_("Universal")
#endif
};

const guint n_chardet_detector_presets = G_N_ELEMENTS(chardet_detector_presets);    

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

gchar *bmp_paths[BMP_PATH_COUNT] = {};

GList *dock_window_list = NULL;

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

GCond *cond_scan;
GMutex *mutex_scan;
#ifdef USE_DBUS
MprisPlayer *mpris;
#endif

static GSList *
get_feature_list(void)
{
    GSList *features = NULL;
    
#ifdef HAVE_GCONF
    features = g_slist_append(features, "GConf");
#endif

    return features;
}

static void
dump_version(void)
{
    GSList *features;

    g_printf("%s %s [%s]", _(application_name), VERSION, svn_stamp);

    features = get_feature_list();

    if (features) {
        GSList *item;

        g_printf(" (");

        for (item = features; g_slist_next(item); item = g_slist_next(item))
            g_printf("%s, ", (const gchar *) item->data);

        g_printf("%s)", (const gchar *) item->data);

        g_slist_free(features);
    }

    g_printf("\n");
}

const gchar *
get_gentitle_format(void)
{
    guint titlestring_preset = cfg.titlestring_preset;

    if (titlestring_preset < n_titlestring_presets)
        return bmp_titlestring_presets[titlestring_preset];

    return cfg.gentitle_format;
}

void
make_directory(const gchar * path, mode_t mode)
{
    if (g_mkdir_with_parents(path, mode) == 0)
        return;

    g_printerr(_("Could not create directory (%s): %s\n"), path,
               g_strerror(errno));
}

static void
bmp_make_user_dir(void)
{
    const mode_t mode755 = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    make_directory(bmp_paths[BMP_PATH_USER_DIR], mode755);
    make_directory(bmp_paths[BMP_PATH_USER_PLUGIN_DIR], mode755);
    make_directory(bmp_paths[BMP_PATH_USER_SKIN_DIR], mode755);
    make_directory(bmp_paths[BMP_PATH_SKIN_THUMB_DIR], mode755);
    make_directory(bmp_paths[BMP_PATH_PLAYLISTS_DIR], mode755);
}

static void
bmp_free_paths(void)
{
    int i;

    for (i = 0; i < BMP_PATH_COUNT; i++)
    {
        g_free(bmp_paths[i]);
        bmp_paths[i] = 0;
    }
}

static void
bmp_init_paths()
{
    char *xdg_config_home;
    char *xdg_data_home;
    char *xdg_cache_home;

    xdg_config_home = (getenv("XDG_CONFIG_HOME") == NULL
        ? g_build_filename(g_get_home_dir(), ".config", NULL)
        : g_strdup(getenv("XDG_CONFIG_HOME")));
    xdg_data_home = (getenv("XDG_DATA_HOME") == NULL
        ? g_build_filename(g_get_home_dir(), ".local", "share", NULL)
        : g_strdup(getenv("XDG_DATA_HOME")));
    xdg_cache_home = (getenv("XDG_CACHE_HOME") == NULL
        ? g_build_filename(g_get_home_dir(), ".cache", NULL)
        : g_strdup(getenv("XDG_CACHE_HOME")));

    bmp_paths[BMP_PATH_USER_DIR] =
        g_build_filename(xdg_config_home, "audacious", NULL);
    bmp_paths[BMP_PATH_USER_SKIN_DIR] =
        g_build_filename(xdg_data_home, "audacious", "Skins", NULL);
    bmp_paths[BMP_PATH_USER_PLUGIN_DIR] =
        g_build_filename(xdg_data_home, "audacious", "Plugins", NULL);

    bmp_paths[BMP_PATH_SKIN_THUMB_DIR] =
        g_build_filename(xdg_cache_home, "audacious", "thumbs", NULL);

    bmp_paths[BMP_PATH_PLAYLISTS_DIR] =
        g_build_filename(bmp_paths[BMP_PATH_USER_DIR], "playlists", NULL);

    bmp_paths[BMP_PATH_CONFIG_FILE] =
        g_build_filename(bmp_paths[BMP_PATH_USER_DIR], "config", NULL);
#ifdef HAVE_XSPF_PLAYLIST
    bmp_paths[BMP_PATH_PLAYLIST_FILE] =
        g_build_filename(bmp_paths[BMP_PATH_USER_DIR],
        "playlist.xspf", NULL);
#else
    bmp_paths[BMP_PATH_PLAYLIST_FILE] =
        g_build_filename(bmp_paths[BMP_PATH_USER_DIR],
        "playlist.m3u", NULL);
#endif
    bmp_paths[BMP_PATH_ACCEL_FILE] =
        g_build_filename(bmp_paths[BMP_PATH_USER_DIR], "accels", NULL);
    bmp_paths[BMP_PATH_LOG_FILE] =
        g_build_filename(bmp_paths[BMP_PATH_USER_DIR], "log", NULL);

    bmp_paths[BMP_PATH_GTKRC_FILE] =
        g_build_filename(bmp_paths[BMP_PATH_USER_DIR], "gtkrc", NULL);

    g_free(xdg_config_home);
    g_free(xdg_data_home);
    g_free(xdg_cache_home);

    g_atexit(bmp_free_paths);
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
        cfg.playlist_font = g_strdup(PLAYLISTWIN_DEFAULT_FONT);

    if (!cfg.mainwin_font)
        cfg.mainwin_font = g_strdup(MAINWIN_DEFAULT_FONT);

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
bmp_config_save(void)
{
    GList *node;
    gchar *str;
    gint i, cur_pb_time;
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
    if ( SKINNED_WINDOW(playlistwin)->x != -1 &&
         SKINNED_WINDOW(playlistwin)->y != -1 )
    {
        cfg_db_set_int(db, NULL, "playlist_x",
                           SKINNED_WINDOW(playlistwin)->x);
        cfg_db_set_int(db, NULL, "playlist_y",
                           SKINNED_WINDOW(playlistwin)->y);
    }
    
    if ( SKINNED_WINDOW(mainwin)->x != -1 &&
         SKINNED_WINDOW(mainwin)->y != -1 )
    {
        cfg_db_set_int(db, NULL, "player_x",
                           SKINNED_WINDOW(mainwin)->x);
        cfg_db_set_int(db, NULL, "player_y",
                           SKINNED_WINDOW(mainwin)->y);
    }

    if ( SKINNED_WINDOW(equalizerwin)->x != -1 &&
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

static void
bmp_set_default_icon(void)
{
    GdkPixbuf *icon;

    icon = gdk_pixbuf_new_from_xpm_data((const gchar **) audacious_player_xpm);
    gtk_window_set_default_icon(icon);
    g_object_unref(icon);
}

#ifdef GDK_WINDOWING_QUARTZ
static void
set_dock_icon(void)
{
    GdkPixbuf *icon, *pixbuf;
    CGColorSpaceRef colorspace;
    CGDataProviderRef data_provider;
    CGImageRef image;
    gpointer data;
    gint rowstride, pixbuf_width, pixbuf_height;
    gboolean has_alpha;

    icon = gdk_pixbuf_new_from_xpm_data((const gchar **) audacious_player_xpm);
    pixbuf = gdk_pixbuf_scale_simple(icon, 128, 128, GDK_INTERP_BILINEAR);

    data = gdk_pixbuf_get_pixels(pixbuf);
    pixbuf_width = gdk_pixbuf_get_width(pixbuf);
    pixbuf_height = gdk_pixbuf_get_height(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);

    /* create the colourspace for the CGImage. */
    colorspace = CGColorSpaceCreateDeviceRGB();
    data_provider = CGDataProviderCreateWithData(NULL, data, pixbuf_height * rowstride, NULL);
    image = CGImageCreate(pixbuf_width, pixbuf_height, 8,
	has_alpha ? 32 : 24, rowstride, colorspace,
	has_alpha ? kCGImageAlphaLast : 0,
	data_provider, NULL, FALSE,
	kCGRenderingIntentDefault);

    /* release the colourspace and data provider, we have what we want. */
    CGDataProviderRelease(data_provider);
    CGColorSpaceRelease(colorspace);

    /* set the dock tile images */
    SetApplicationDockTileImage(image);

#if 0
    /* and release */
    CGImageRelease(image);
    g_object_unref(icon);
    g_object_unref(pixbuf);
#endif
}
#endif

static GOptionEntry cmd_entries[] = {
    {"rew", 'r', 0, G_OPTION_ARG_NONE, &options.rew, N_("Skip backwards in playlist"), NULL},
    {"play", 'p', 0, G_OPTION_ARG_NONE, &options.play, N_("Start playing current playlist"), NULL},
    {"pause", 'u', 0, G_OPTION_ARG_NONE, &options.pause, N_("Pause current song"), NULL},
    {"stop", 's', 0, G_OPTION_ARG_NONE, &options.stop, N_("Stop current song"), NULL},
    {"play-pause", 't', 0, G_OPTION_ARG_NONE, &options.play_pause, N_("Pause if playing, play otherwise"), NULL},
    {"fwd", 'f', 0, G_OPTION_ARG_NONE, &options.fwd, N_("Skip forward in playlist"), NULL},
    {"show-jump-box", 'j', 0, G_OPTION_ARG_NONE, &options.show_jump_box, N_("Display Jump to File dialog"), NULL},
    {"enqueue", 'e', 0, G_OPTION_ARG_NONE, &options.enqueue, N_("Don't clear the playlist"), NULL},
    {"enqueue-to-temp", 'E', 0, G_OPTION_ARG_NONE, &options.enqueue_to_temp, N_("Add new files to a temporary playlist"), NULL},
    {"show-main-window", 'm', 0, G_OPTION_ARG_NONE, &options.mainwin, N_("Display the main window"), NULL},
    {"activate", 'a', 0, G_OPTION_ARG_NONE, &options.activate, N_("Display all open Audacious windows"), NULL},
    {"headless", 'H', 0, G_OPTION_ARG_NONE, &options.headless, N_("Enable headless operation"), NULL},
    {"no-log", 'N', 0, G_OPTION_ARG_NONE, &options.no_log, N_("Print all errors and warnings to stdout"), NULL},
    {"version", 'v', 0, G_OPTION_ARG_NONE, &options.version, N_("Show version and builtin features"), NULL},
	#ifdef GDK_WINDOWING_QUARTZ
	{"macpack", 'n', 0, G_OPTION_ARG_NONE, &options.macpack, N_("Used in macpacking"), NULL}, /* Make this hidden */
	#endif
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &options.filenames, N_("FILE..."), NULL},
    {NULL},
};

static gboolean
aud_start_playback(gpointer unused)
{
    drct_play();
    return FALSE;
}

static void
handle_cmd_line_options(AudCmdLineOpt * options,
                        gboolean remote)
{
    gchar **filenames = options->filenames;
#ifdef USE_DBUS
    DBusGProxy *session = audacious_get_dbus_proxy();
    gboolean is_running = audacious_remote_is_running(session);
#else
    gboolean is_running = FALSE;
#endif

    if (options->version)
    {
        dump_version();
        exit(EXIT_SUCCESS);
    }

#ifdef USE_DBUS
    if (is_running)
    {
        if (filenames != NULL)
        {
            gint pos = 0;
            gint i = 0;
            GList *fns = NULL;

            for (i = 0; filenames[i] != NULL; i++)
            {
                gchar *filename;
                gchar *current_dir = g_get_current_dir();

		if (!strstr(filenames[i], "://"))
		{
                    if (filenames[i][0] == '/')
   	               filename = g_strdup_printf("file:///%s", filenames[i]);
                    else
	               filename = g_strdup_printf("file:///%s/%s", current_dir, filenames[i]);
		}
                else
                    filename = g_strdup(filenames[i]);

                fns = g_list_prepend(fns, filename);

                g_free(current_dir);
            }

            fns = g_list_reverse(fns);

            if (options->load_skins)
            {
                audacious_remote_set_skin(session, filenames[0]);
                skin_install_skin(filenames[0]);
            }
            else
            {
                GList *i;

                if (options->enqueue_to_temp)
                    audacious_remote_playlist_enqueue_to_temp(session, filenames[0]);

                if (options->enqueue && options->play)
                    pos = audacious_remote_get_playlist_length(session);

                if (!options->enqueue)
                {
                    audacious_remote_playlist_clear(session);
                    audacious_remote_stop(session);
                }

                for (i = fns; i != NULL; i = i->next)
                    audacious_remote_playlist_add_url_string(session, i->data);

                if (options->enqueue && options->play &&
                    audacious_remote_get_playlist_length(session) > pos)
                    audacious_remote_set_playlist_pos(session, pos);

                if (!options->enqueue)
                    audacious_remote_play(session);
            }

            g_list_foreach(fns, (GFunc) g_free, NULL);
            g_list_free(fns);

            g_strfreev(filenames);
        } /* filename */

        if (options->rew)
            audacious_remote_playlist_prev(session);

        if (options->play)
            audacious_remote_play(session);

        if (options->pause)
            audacious_remote_pause(session);

        if (options->stop)
            audacious_remote_stop(session);

        if (options->fwd)
            audacious_remote_playlist_next(session);

        if (options->play_pause)
            audacious_remote_play_pause(session);

        if (options->show_jump_box)
            audacious_remote_show_jtf_box(session);

        if (options->mainwin)
            audacious_remote_main_win_toggle(session, TRUE);

        if (options->activate)
            audacious_remote_activate(session);

        exit(EXIT_SUCCESS);
    } /* is_running */
    else
#endif
    { /* !is_running */
        if (filenames != NULL)
        {
            gint pos = 0;
            gint i = 0;
            GList *fns = NULL;

            for (i = 0; filenames[i] != NULL; i++)
            {
                gchar *filename;
                gchar *current_dir = g_get_current_dir();

		if (!strstr(filenames[i], "://"))
		{
                    if (filenames[i][0] == '/')
   	               filename = g_strdup_printf("file:///%s", filenames[i]);
                    else
	               filename = g_strdup_printf("file:///%s/%s", current_dir, filenames[i]);
		}
                else
                    filename = g_strdup(filenames[i]);

                fns = g_list_prepend(fns, filename);

                g_free(current_dir);
            }

            fns = g_list_reverse(fns);

            {
                if (options->enqueue_to_temp)
                    drct_pl_enqueue_to_temp(filenames[0]);

                if (options->enqueue && options->play)
                    pos = drct_pl_get_length();

                if (!options->enqueue)
                {
                    drct_pl_clear();
                    drct_stop();
                }

                drct_pl_add(fns);

                if (options->enqueue && options->play &&
                    drct_pl_get_length() > pos)
                    drct_pl_set_pos(pos);

                if (!options->enqueue)
                    g_idle_add(aud_start_playback, NULL);
            }

            g_list_foreach(fns, (GFunc) g_free, NULL);
            g_list_free(fns);

            g_strfreev(filenames);
        } /* filename */

        if (options->rew)
            drct_pl_prev();

        if (options->play)
            drct_play();

        if (options->pause)
            drct_pause();

        if (options->stop)
            drct_stop();

        if (options->fwd)
            drct_pl_next();

        if (options->play_pause) {
            if (drct_get_paused())
                drct_play();
            else
                drct_pause();
        }

        if (options->show_jump_box)
            drct_jtf_show();

        if (options->mainwin)
            drct_main_win_toggle(TRUE);

        if (options->activate)
            drct_activate();
    } /* !is_running */
}

static void
bmp_setup_logger(void)
{
    if (!bmp_logger_start(bmp_paths[BMP_PATH_LOG_FILE]))
        return;

    g_atexit(bmp_logger_stop);
}

static void
run_load_skin_error_dialog(const gchar * skin_path)
{
    const gchar *markup =
        N_("<b><big>Unable to load skin.</big></b>\n"
           "\n"
           "Check that skin at '%s' is usable and default skin is properly "
           "installed at '%s'\n");

    GtkWidget *dialog =
        gtk_message_dialog_new_with_markup(NULL,
                                           GTK_DIALOG_MODAL,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_CLOSE,
                                           _(markup),
                                           skin_path,
                                           BMP_DEFAULT_SKIN_PATH);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static gboolean
aud_headless_iteration(gpointer unused)
{
    free_vis_data();
    return TRUE;
}

static gboolean
load_extra_playlist(const gchar * path, const gchar * basename,
        gpointer def)
{
    const gchar *title;
    Playlist *playlist;
    Playlist *deflist;

    deflist = (Playlist *)def;
    playlist = playlist_new();
    if (!playlist) {
        g_warning("Couldn't create new playlist\n");
        return FALSE;
    }

    playlist_add_playlist(playlist);
    playlist_load(playlist, path);

    title = playlist_get_current_name(playlist);

    return FALSE; /* keep loading other playlists */
}

gint
main(gint argc, gchar ** argv)
{
    gboolean gtk_init_check_ok;
    Playlist *playlist;
    GOptionContext *context;
    GError *error = NULL;

    /* glib-2.13.0 requires g_thread_init() to be called before all
       other GLib functions */
    g_thread_init(NULL);
    if (!g_thread_supported()) {
        g_printerr(_("Sorry, threads isn't supported on your platform.\n\n"
                     "If you're on a libc5 based linux system and installed Glib & GTK+ before you\n"
                     "installed LinuxThreads you need to recompile Glib & GTK+.\n"));
        exit(EXIT_FAILURE);
    }

    gdk_threads_init();

    /* Setup l10n early so we can print localized error messages */
    gtk_set_locale();
    bindtextdomain(PACKAGE_NAME, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
    bindtextdomain(PACKAGE_NAME "-plugins", LOCALEDIR);
    bind_textdomain_codeset(PACKAGE_NAME "-plugins", "UTF-8");
    textdomain(PACKAGE_NAME);

#ifndef _WIN32
    egg_set_desktop_file(AUDACIOUS_DESKTOP_FILE);
#endif
    bmp_init_paths();
    bmp_make_user_dir();

    cond_scan = g_cond_new();
    mutex_scan = g_mutex_new();
    gtk_rc_add_default_file(bmp_paths[BMP_PATH_GTKRC_FILE]);

    gtk_init_check_ok = gtk_init_check(&argc, &argv);

    memset(&options, '\0', sizeof(AudCmdLineOpt));
    options.session = -1;

    context = g_option_context_new(_("- play multimedia files"));
    g_option_context_add_main_entries(context, cmd_entries, PACKAGE_NAME);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    g_option_context_add_group(context, egg_sm_client_get_option_group());
    g_option_context_parse(context, &argc, &argv, &error);

    if (error != NULL)
    {
        if(error->message)
        {   /* checking for MacOS X -psn_0_* errors*/
            char* s = g_strrstr(error->message,"-psn_0_");
            if(!s)
            {
                g_printerr(_("%s: %s\nTry `%s --help' for more information.\n"),
                        argv[0], error->message, argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }

    if (!gtk_init_check_ok) {
        if (argc < 2) {
            /* GTK check failed, and no arguments passed to indicate
               that user is intending to only remote control a running
               session */
            g_printerr(_("%s: Unable to open display, exiting.\n"), argv[0]);
            exit(EXIT_FAILURE);
        }

        handle_cmd_line_options(&options, TRUE);

        /* we could be running headless, so GTK probably wont matter */
        if (options.headless != 1)
            exit(EXIT_SUCCESS);
    }

    if (options.no_log == FALSE)
        bmp_setup_logger();

    signal_handlers_init();

    g_random_set_seed(time(NULL));
    SAD_dither_init_rand((gint32)time(NULL));

    bmp_config_load();

    mowgli_init();

    if (options.headless != 1)
    {
      ui_main_check_theme_engine();

      /* register icons in stock
         NOTE: should be called before UIManager */
      register_aud_stock_icons();

      /* UIManager
         NOTE: this needs to be called before plugin init, cause
         plugin init functions may want to add custom menu entries */
      ui_manager_init();
      ui_manager_create_menus();
    }

    plugin_system_init();

    /* Initialize the playlist system. */
    playlist_init();
    playlist = playlist_get_active();
    playlist_load(playlist, bmp_paths[BMP_PATH_PLAYLIST_FILE]);
    playlist_set_position(playlist, cfg.playlist_position);

    handle_cmd_line_options(&options, TRUE);

#ifdef USE_DBUS
    init_dbus();
#endif

    if (options.headless != 1)
    {
        bmp_set_default_icon();
#ifdef GDK_WINDOWING_QUARTZ
        set_dock_icon();
#endif

        gtk_accel_map_load(bmp_paths[BMP_PATH_ACCEL_FILE]);

        if (!init_skins(cfg.skin)) {
            run_load_skin_error_dialog(cfg.skin);
            exit(EXIT_FAILURE);
        }

        GDK_THREADS_ENTER();
    }

    /* Load extra playlists */
    if(!dir_foreach(bmp_paths[BMP_PATH_PLAYLISTS_DIR], load_extra_playlist,
            playlist, NULL)) {
        g_warning("Could not load extra playlists\n");
    }

    /* this needs to be called after all 3 windows are created and
     * input plugins are setup'ed 
     * but not if we're running headless --nenolod
     */
    mainwin_setup_menus();

    if (options.headless != 1)
    {
        ui_main_set_initial_volume();

        /* FIXME: delayed, because it deals directly with the plugin
         * interface to set menu items */
        create_prefs_window();

        create_fileinfo_window();


        if (cfg.player_visible)
            mainwin_show(TRUE);
        else if (!cfg.playlist_visible && !cfg.equalizer_visible)
        {
          /* all of the windows are hidden... warn user about this */
          mainwin_show_visibility_warning();
        }

        if (cfg.equalizer_visible)
            equalizerwin_show(TRUE);

        if (cfg.playlist_visible)
            playlistwin_show();

        hint_set_always(cfg.always_on_top);

        playlist_start_get_info_thread();

        has_x11_connection = TRUE;

        if (cfg.resume_playback_on_startup)
        {
            if (cfg.resume_playback_on_startup_time != -1 &&
                playlist_get_length(playlist) > 0)
            {
                int i;
                gint l = 0, r = 0;
                while (gtk_events_pending()) gtk_main_iteration();
                output_get_volume(&l, &r);
                output_set_volume(0,0);
                playback_initiate();

                /* Busy wait; loop is fairly tight to minimize duration of
                 * "frozen" GUI. Feel free to tune. --chainsaw */
                for (i = 0; i < 20; i++)
                { 
                    g_usleep(1000);
                    if (!ip_data.playing)
                        break;
                }
                playback_seek(cfg.resume_playback_on_startup_time / 1000);
                output_set_volume(l, r);
            }
        }
        
        gtk_main();

        GDK_THREADS_LEAVE();

        g_cond_free(cond_scan);
        g_mutex_free(mutex_scan);

        return EXIT_SUCCESS;
    }
    // if we are running headless
    else
    {
        GMainLoop *loop;

        playlist_start_get_info_thread();

        loop = g_main_loop_new(NULL, TRUE);
        g_timeout_add(10, aud_headless_iteration, NULL);
        g_main_loop_run(loop);

        return EXIT_SUCCESS;
    }
}
