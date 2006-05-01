/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2006  Audacious development team.
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "libaudacious/configdb.h"
#include "libaudacious/beepctrl.h"
#include "libaudacious/util.h"
#include "libaudacious/vfs.h"

#include "controlsocket.h"
#include "dnd.h"
#include "dock.h"
#include "effect.h"
#include "equalizer.h"
#include "general.h"
#include "genevent.h"
#include "hints.h"
#include "input.h"
#include "logger.h"
#include "mainwin.h"
#include "output.h"
#include "playback.h"
#include "playlist.h"
#include "ui_playlist.h"
#include "pluginenum.h"
#include "prefswin.h"
#include "skin.h"
#include "skinwin.h"
#include "util.h"
#include "visualization.h"
#include "build_stamp.h"

#include "pixmaps.h"
#include "images/audacious_player.xpm"

gboolean has_x11_connection = FALSE; 	/* do we have an X11 connection? */

/* Translatable string for beep.desktop's comment field */
const gchar *desktop_comment = N_("Audacious");

const gchar *application_name = N_("Audacious");


struct _BmpCmdLineOpt {
    GList *filenames;
    gint session;
    gboolean play, stop, pause, fwd, rew, play_pause, playcd;
    gboolean enqueue, mainwin, remote, activate;
    gboolean load_skins;
    gboolean headless;
    gchar *previous_session_id;
};

typedef struct _BmpCmdLineOpt BmpCmdLineOpt;

BmpCmdLineOpt options;

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
    FALSE,                      /* UNUSED (double size) */
    TRUE,                       /* autoscroll */
    FALSE,                      /* stop after current song */
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
    TRUE, TRUE,                 /* convert '_' and %20 */
    TRUE,                       /* show numbers in playlist */
    TRUE,                       /* snap windows */
    TRUE,                       /* save window positions */
    TRUE,                       /* dim titlebar */
    FALSE,                      /* get playlist info on load */
    TRUE,                       /* get playlist info on demand */
    TRUE,                       /* UNUSED (equalizer doublesize linked) */
    FALSE,                      /* sort jump to file */
    FALSE,                      /* use effect plugins */
    FALSE,                      /* always on top */
    FALSE,                      /* sticky */
    FALSE,                      /* no playlist advance */
#ifdef HAVE_GNOME_VFS
    FALSE,                      /* refresh file list - should be unneeded with VFS */
#else
    TRUE,                       /* refresh file list */
#endif
    TRUE,                       /* UNUSED (smooth title scrolling) */
    TRUE,                       /* use playlist metadata */
    TRUE,                       /* warn about unplayables */
    FALSE,                      /* use \ as directory delimiter */
    FALSE,                      /* random skin on play */
    FALSE,                      /* use fontsets */
    FALSE,                      /* use X font for mainwin */
    TRUE,			/* use custom cursors */
    TRUE,			/* close dialog on open */
    TRUE,			/* close dialog on add */
    0.0,                        /* equalizer preamp */
    {0, 0, 0, 0, 0,             /* equalizer bands */
     0, 0, 0, 0, 0},
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
    NULL,                       /* equalizer preset default file */
    NULL,                       /* equalizer preset extension */
    NULL,                       /* URL history */
    0,                          /* timer mode */
    VIS_ANALYZER,               /* visualizer type */
    ANALYZER_NORMAL,            /* analyzer mode */
    ANALYZER_BARS,              /* analyzer type */
    SCOPE_DOT,                  /* scope mode */
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
    4,                          /* 5th preset (ARTIST [ ALBUM ] - TRACK. TITLE) */
    NULL,                       /* title format */
    FALSE,                      /* software volume control enabled */
    TRUE,                       /* UNUSED (XMMS compatibility mode) */
    TRUE,                       /* extra eq filtering */
    3,                          /* scroll pl by */
    FALSE,                      /* resume playback on startup */
    -1,                         /* resume playback on startup time */
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
    "%t",
    "%{p:%p - %}%t",
    "%{p:%p - %}%{a:%a - %}%t",
    "%{p:%p - %}%{a:%a - %}%{n:%n. %}%t",
    "%{p:%p %}%{a:[ %a ] %}%{p:- %}%{n:%n. %}%{t:%t%}",
    "%{a:%a - %}%t"
};

const guint n_titlestring_presets = G_N_ELEMENTS(bmp_titlestring_presets);

static bmp_cfg_boolent bmp_boolents[] = {
    {"allow_multiple_instances", &cfg.allow_multiple_instances, TRUE},
    {"use_realtime", &cfg.use_realtime, TRUE},
    {"always_show_cb", &cfg.always_show_cb, TRUE},
    {"convert_underscore", &cfg.convert_underscore, TRUE},
    {"convert_twenty", &cfg.convert_twenty, TRUE},
    {"show_numbers_in_pl", &cfg.show_numbers_in_pl, TRUE},
    {"snap_windows", &cfg.snap_windows, TRUE},
    {"save_window_positions", &cfg.save_window_position, TRUE},
    {"dim_titlebar", &cfg.dim_titlebar, TRUE},
    {"get_info_on_load", &cfg.get_info_on_load, TRUE},
    {"get_info_on_demand", &cfg.get_info_on_demand, TRUE},
    {"no_playlist_advance", &cfg.no_playlist_advance, TRUE},
    {"refresh_file_list", &cfg.refresh_file_list, TRUE},
    {"sort_jump_to_file", &cfg.sort_jump_to_file, TRUE},
    {"use_pl_metadata", &cfg.use_pl_metadata, TRUE},
    {"warn_about_unplayables", &cfg.warn_about_unplayables, TRUE},
    {"use_backslash_as_dir_delimiter", &cfg.use_backslash_as_dir_delimiter, TRUE},
    {"player_shaded", &cfg.player_shaded, TRUE},
    {"player_visible", &cfg.player_visible, TRUE},
    {"shuffle", &cfg.shuffle, TRUE},
    {"repeat", &cfg.repeat, TRUE},
    {"autoscroll_songname", &cfg.autoscroll, TRUE},
    {"stop_after_current_song", &cfg.stopaftersong, TRUE},
    {"playlist_shaded", &cfg.playlist_shaded, TRUE},
    {"playlist_visible", &cfg.playlist_visible, TRUE},
    {"playlist_transparent", &cfg.playlist_transparent, TRUE},
    {"use_fontsets", &cfg.use_fontsets, TRUE},
    {"mainwin_use_xfont", &cfg.mainwin_use_xfont, FALSE},
    {"equalizer_visible", &cfg.equalizer_visible, TRUE},
    {"equalizer_active", &cfg.equalizer_active, TRUE},
    {"equalizer_shaded", &cfg.equalizer_shaded, TRUE},
    {"equalizer_autoload", &cfg.equalizer_autoload, TRUE},
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
};

static gint ncfgsent = G_N_ELEMENTS(bmp_strents);

gchar *bmp_paths[BMP_PATH_COUNT] = {};

GList *dock_window_list = NULL;

gboolean pposition_broken = FALSE;

gboolean starting_up = TRUE;

static GSList *
get_feature_list(void)
{
    GSList *features = NULL;
    
#ifdef HAVE_GNOME_VFS
    features = g_slist_append(features, "GNOME VFS");
#endif

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
xmms_get_gentitle_format(void)
{
    guint titlestring_preset = cfg.titlestring_preset;

    if (titlestring_preset < n_titlestring_presets)
	return bmp_titlestring_presets[titlestring_preset];

    return cfg.gentitle_format;
}

void
make_directory(const gchar * path, mode_t mode)
{
    if (mkdir(path, mode) == 0)
        return;

    if (errno == EEXIST)
        return;

    g_printerr(_("Could not create directory (%s): %s"), path,
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


#define USER_PATH(path) \
    g_build_filename(bmp_paths[BMP_PATH_USER_DIR], path, NULL);

static void
bmp_init_paths(void)
{
    bmp_paths[BMP_PATH_USER_DIR] = g_build_filename(g_get_home_dir(), BMP_RCPATH, NULL);

    bmp_paths[BMP_PATH_USER_PLUGIN_DIR] = USER_PATH(BMP_USER_PLUGIN_DIR_BASENAME);
    bmp_paths[BMP_PATH_USER_SKIN_DIR] = USER_PATH(BMP_SKIN_DIR_BASENAME);
    bmp_paths[BMP_PATH_SKIN_THUMB_DIR] = USER_PATH(BMP_SKIN_THUMB_DIR_BASENAME);
    bmp_paths[BMP_PATH_CONFIG_FILE] = USER_PATH(BMP_CONFIG_BASENAME);
    bmp_paths[BMP_PATH_PLAYLIST_FILE] = USER_PATH(BMP_PLAYLIST_BASENAME);
    bmp_paths[BMP_PATH_ACCEL_FILE] = USER_PATH(BMP_ACCEL_BASENAME);
    bmp_paths[BMP_PATH_LOG_FILE] = USER_PATH(BMP_LOG_BASENAME);

    g_atexit(bmp_free_paths);
}


void
bmp_config_load(void)
{
    ConfigDb *db;
    gint i, length;

    memcpy(&cfg, &bmp_default_config, sizeof(BmpConfig));

    db = bmp_cfg_db_open();
    for (i = 0; i < ncfgbent; ++i) {
        bmp_cfg_db_get_bool(db, NULL,
                            bmp_boolents[i].be_vname,
                            bmp_boolents[i].be_vloc);
    }

    for (i = 0; i < ncfgient; ++i) {
        bmp_cfg_db_get_int(db, NULL,
                           bmp_numents[i].ie_vname,
                           bmp_numents[i].ie_vloc);
    }

    for (i = 0; i < ncfgsent; ++i) {
        bmp_cfg_db_get_string(db, NULL,
                              bmp_strents[i].se_vname,
                              bmp_strents[i].se_vloc);
    }

    /* Preset */
    bmp_cfg_db_get_float(db, NULL, "equalizer_preamp", &cfg.equalizer_preamp);
    for (i = 0; i < 10; i++) {
        gchar eqtext[18];

        g_snprintf(eqtext, sizeof(eqtext), "equalizer_band%d", i);
        bmp_cfg_db_get_float(db, NULL, eqtext, &cfg.equalizer_bands[i]);
    }

    /* History */
    if (bmp_cfg_db_get_int(db, NULL, "url_history_length", &length)) {
        for (i = 1; i <= length; i++) {
            gchar str[19], *tmp;

            g_snprintf(str, sizeof(str), "url_history%d", i);
            if (bmp_cfg_db_get_string(db, NULL, str, &tmp))
                cfg.url_history = g_list_append(cfg.url_history, tmp);
        }
    }

    bmp_cfg_db_close(db);


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
        cfg.gentitle_format = g_strdup("%{p:%p %}%{a:[ %a ] %}%{p:- %}%{n:%n. %}%{t:%t%}");

    if (!cfg.outputplugin) {
#ifdef HAVE_OSS
        cfg.outputplugin = g_build_filename(PLUGIN_DIR, plugin_dir_list[0],
                                            PLUGIN_FILENAME("OSS"), NULL);
#else
        /* FIXME: This implicitly means the output plugin that is
         * first in the alphabet will be used (usually the disk writer
         * plugin) */
        cfg.outputplugin = g_strdup("");
#endif
    }

    if (!cfg.eqpreset_default_file)
        cfg.eqpreset_default_file = g_strdup(EQUALIZER_DEFAULT_DIR_PRESET);

    if (!cfg.eqpreset_extension)
        cfg.eqpreset_extension = g_strdup(EQUALIZER_DEFAULT_PRESET_EXT);
}


void
bmp_config_save(void)
{
    GList *node;
    gchar *str;
    gint i, cur_pb_time;
    ConfigDb *db;

    cfg.disabled_iplugins = input_stringify_disabled_list();


    db = bmp_cfg_db_open();

    for (i = 0; i < ncfgbent; ++i)
        if (bmp_boolents[i].be_wrt)
            bmp_cfg_db_set_bool(db, NULL,
                                bmp_boolents[i].be_vname,
                                *bmp_boolents[i].be_vloc);

    /* HACK: Work-around for bug #129 */
    /*
    bmp_cfg_db_set_bool(db, NULL, "playlist_shaded",
		        FALSE);
    */
    for (i = 0; i < ncfgient; ++i)
        if (bmp_numents[i].ie_wrt)
            bmp_cfg_db_set_int(db, NULL,
                               bmp_numents[i].ie_vname,
                               *bmp_numents[i].ie_vloc);

    /* This is a bit lame .. it'll end up being written twice,
     * could do with being done a bit neater.  -larne   */
    bmp_cfg_db_set_int(db, NULL, "playlist_position",
                       playlist_get_position());

    bmp_cfg_db_set_bool(db, NULL, "mainwin_use_xfont",
			cfg.mainwin_use_xfont);

    for (i = 0; i < ncfgsent; ++i) {
        if (bmp_strents[i].se_wrt)
            bmp_cfg_db_set_string(db, NULL,
                                  bmp_strents[i].se_vname,
                                  *bmp_strents[i].se_vloc);
    }

    bmp_cfg_db_set_float(db, NULL, "equalizer_preamp", cfg.equalizer_preamp);

    for (i = 0; i < 10; i++) {
        str = g_strdup_printf("equalizer_band%d", i);
        bmp_cfg_db_set_float(db, NULL, str, cfg.equalizer_bands[i]);
        g_free(str);
    }

    if (bmp_active_skin != NULL)
    {
        if (bmp_active_skin->path)
            bmp_cfg_db_set_string(db, NULL, "skin", bmp_active_skin->path);
        else
            bmp_cfg_db_unset_key(db, NULL, "skin");
    }

    if (get_current_output_plugin())
        bmp_cfg_db_set_string(db, NULL, "output_plugin",
                              get_current_output_plugin()->filename);
    else
        bmp_cfg_db_unset_key(db, NULL, "output_plugin");

    str = general_stringify_enabled_list();
    if (str) {
        bmp_cfg_db_set_string(db, NULL, "enabled_gplugins", str);
        g_free(str);
    }
    else
        bmp_cfg_db_unset_key(db, NULL, "enabled_gplugins");

    str = vis_stringify_enabled_list();
    if (str) {
        bmp_cfg_db_set_string(db, NULL, "enabled_vplugins", str);
        g_free(str);
    }
    else
        bmp_cfg_db_unset_key(db, NULL, "enabled_vplugins");

    str = effect_stringify_enabled_list();
    if (str) {
        bmp_cfg_db_set_string(db, NULL, "enabled_eplugins", str);
        g_free(str);
    }
    else
        bmp_cfg_db_unset_key(db, NULL, "enabled_eplugins");

    if (cfg.filesel_path)
        bmp_cfg_db_set_string(db, NULL, "filesel_path", cfg.filesel_path);

    if (cfg.playlist_path)
        bmp_cfg_db_set_string(db, NULL, "playlist_path", cfg.playlist_path);

    bmp_cfg_db_set_int(db, NULL, "url_history_length",
                       g_list_length(cfg.url_history));

    for (node = cfg.url_history, i = 1; node; node = g_list_next(node), i++) {
        str = g_strdup_printf("url_history%d", i);
        bmp_cfg_db_set_string(db, NULL, str, node->data);
        g_free(str);
    }

    if (bmp_playback_get_playing()) {
	    cur_pb_time = bmp_playback_get_time();
    } else
	    cur_pb_time = -1;
    cfg.resume_playback_on_startup_time = cur_pb_time;
    bmp_cfg_db_set_int(db, NULL, "resume_playback_on_startup_time",
		       cfg.resume_playback_on_startup_time);

    bmp_cfg_db_close(db);

    playlist_save(bmp_paths[BMP_PATH_PLAYLIST_FILE], FALSE);
}

static void
bmp_set_default_icon(void)
{
    GdkPixbuf *icon;

    icon = gdk_pixbuf_new_from_xpm_data((const gchar **) audacious_player_xpm);
    gtk_window_set_default_icon(icon);
    g_object_unref(icon);
}

static void
display_usage(void)
{
    g_print(_("Usage: audacious [options] [files] ...\n\n"
              "Options:\n"
              "--------\n"));
    g_print("\n-h, --help             ");
    /* -h, --help switch */
    g_print(_("Display this text and exit"));
    g_print("\n-n, --session          ");
    /* -n, --session switch */
    g_print(_("Select Audacious/BMP/XMMS session (Default: 0)"));
    g_print("\n-r, --rew              ");
    /* -r, --rew switch */
    g_print(_("Skip backwards in playlist"));
    g_print("\n-p, --play             ");
    /* -p, --play switch */
    g_print(_("Start playing current playlist"));
    g_print("\n-u, --pause            ");
    /* -u, --pause switch */
    g_print(_("Pause current song"));
    g_print("\n-s, --stop             ");
    /* -s, --stop switch */
    g_print(_("Stop current song"));
    g_print("\n-t, --play-pause       ");
    /* -t, --play-pause switch */
    g_print(_("Pause if playing, play otherwise"));
    g_print("\n-f, --fwd              ");
    /* -f, --fwd switch */
    g_print(_("Skip forward in playlist"));
    g_print("\n-e, --enqueue          ");
    /* -e, --enqueue switch */
    g_print(_("Don't clear the playlist"));
    g_print("\n-m, --show-main-window ");
    /* -m, --show-main-window switch */
    g_print(_("Show the main window"));
    g_print("\n-a, --activate         ");
    /* -a, --activate switch */
    g_print(_("Activate Audacious"));
    g_print("\n-i, --sm-client-id     ");
    /* -i, --sm-client-id switch */
    g_print(_("Previous session ID"));
    g_print("\n-H, --headless         ");
    /* -h, --headless switch */
    g_print(_("Headless operation [experimental]"));
    g_print("\n-v, --version          ");
    /* -v, --version switch */
    g_print(_("Print version number and exit\n"));

    exit(EXIT_SUCCESS);
}

static void
parse_cmd_line(gint argc,
               gchar ** argv,
               BmpCmdLineOpt * options)
{
    static struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"session", 1, NULL, 'n'},
        {"rew", 0, NULL, 'r'},
        {"play", 0, NULL, 'p'},
        {"pause", 0, NULL, 'u'},
        {"play-pause", 0, NULL, 't'},
        {"stop", 0, NULL, 's'},
        {"fwd", 0, NULL, 'f'},
        {"enqueue", 0, NULL, 'e'},
        {"show-main-window", 0, NULL, 'm'},
        {"activate", 0, NULL, 'a'},
        {"version", 0, NULL, 'v'},
        {"sm-client-id", 1, NULL, 'i'},
        {"xmms", 0, NULL, 'x'},
        {"headless", 0, NULL, 'H'},
        {0, 0, 0, 0}
    };

    gchar *filename, *current_dir;
    gint c, i;

    memset(options, 0, sizeof(BmpCmdLineOpt));
    options->session = -1;

    while ((c = getopt_long(argc, argv, "chn:HrpusfemavtLS", long_options,
                            NULL)) != -1) {
        switch (c) {
        case 'h':
            display_usage();
            break;
        case 'n':
            options->session = atoi(optarg);
            break;
        case 'H':
            options->headless = TRUE;
            break;
        case 'r':
            options->rew = TRUE;
            break;
        case 'p':
            options->play = TRUE;
            break;
        case 'u':
            options->pause = TRUE;
            break;
        case 's':
            options->stop = TRUE;
            break;
        case 'f':
            options->fwd = TRUE;
            break;
        case 't':
            options->play_pause = TRUE;
            break;
        case 'm':
            options->mainwin = TRUE;
            break;
        case 'a':
            options->activate = TRUE;
	    break;
        case 'e':
            options->enqueue = TRUE;
            break;
        case 'v':
            dump_version();
            exit(EXIT_SUCCESS);
            break;
        case 'i':
            options->previous_session_id = g_strdup(optarg);
            break;
        case 'c':
            options->playcd = TRUE;
            break;
        case 'S':
            options->load_skins = TRUE;
            break;
        }
    }

    current_dir = g_get_current_dir();

    for (i = optind; i < argc; i++) {
        if (argv[i][0] == '/' || strstr(argv[i], "://"))
            filename = g_strdup(argv[i]);
        else
            filename = g_build_filename(current_dir, argv[i], NULL);

        options->filenames = g_list_prepend(options->filenames, filename);
    }

    options->filenames = g_list_reverse(options->filenames);

    g_free(current_dir);
}

static void
handle_cmd_line_options(BmpCmdLineOpt * options,
                        gboolean remote)
{
    GList *filenames = options->filenames;
    gint session = options->session;

    if (session == -1) {
        if (!remote)
            session = ctrlsocket_get_session_id();
        else
            session = 0;
    }

    if (filenames) {
        gint pos = 0;

        if (options->load_skins) {
            xmms_remote_set_skin(session, filenames->data);
            skin_install_skin(filenames->data);
        }
        else {
            if (options->enqueue && options->play)
                pos = xmms_remote_get_playlist_length(session);

            if (!options->enqueue)
                xmms_remote_playlist_clear(session);

            xmms_remote_playlist_add(session, filenames);

            if (options->enqueue && options->play &&
                xmms_remote_get_playlist_length(session) > pos)
                xmms_remote_set_playlist_pos(session, pos);

            if (!options->enqueue)
                xmms_remote_play(session);
        }

        g_list_foreach(filenames, (GFunc) g_free, NULL);
        g_list_free(filenames);
    }

    if (options->rew)
        xmms_remote_playlist_prev(session);

    if (options->play)
        xmms_remote_play(session);

    if (options->pause)
        xmms_remote_pause(session);

    if (options->stop)
        xmms_remote_stop(session);

    if (options->fwd)
        xmms_remote_playlist_next(session);

    if (options->play_pause)
        xmms_remote_play_pause(session);

    if (options->mainwin)
        xmms_remote_main_win_toggle(session, TRUE);

    if (options->activate)
        xmms_remote_activate(session);

    if (options->playcd)
        play_medium();
}

static void
segfault_handler(gint sig)
{
    g_printerr(_("\nReceived SIGSEGV\n\n"
                 "This could be a bug in Audacious. If you don't know why this happened, "
                 "file a bug at http://bugs.nenolod.net/\n\n"));
#ifdef HANDLE_SIGSEGV
    exit(EXIT_FAILURE);
#else
    abort();
#endif
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

// use a format string?
void report_error(const gchar *error_text)
{
    fprintf(stderr,error_text);
	if (options.headless!=1) {
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(err),
                                                 error_text);
        gtk_dialog_run(GTK_DIALOG(err));
        gtk_widget_hide(err);
    }
}

gint
main(gint argc, gchar ** argv)
{
    gboolean gtk_init_check_ok;

    /* Setup l10n early so we can print localized error messages */
    gtk_set_locale();
    bindtextdomain(PACKAGE_NAME, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
    textdomain(PACKAGE_NAME);

    bmp_init_paths();
    bmp_make_user_dir();
    bmp_setup_logger();

    /* Check GTK version. Really, this is only needed for binary
     * distribution since configure already checks. */
    if (!GTK_CHECK_VERSION(2, 6, 0)) {
        g_printerr(_("Sorry, your GTK+ version (%d.%d.%d) does not work with Audacious.\n"
                     "Please use GTK+ %s or newer.\n"),
                   gtk_major_version, gtk_minor_version, gtk_micro_version,
                   "2.6.0");
        exit(EXIT_FAILURE);
    }

    g_set_application_name(_(application_name));

    g_thread_init(NULL);
    if (!g_thread_supported()) {
        g_printerr(_("Sorry, threads isn't supported on your platform.\n\n"
                     "If you're on a libc5 based linux system and installed Glib & GTK+ before you\n"
                     "installed LinuxThreads you need to recompile Glib & GTK+.\n"));
        exit(EXIT_FAILURE);
    }

    gdk_threads_init();

    gtk_init_check_ok = gtk_init_check(&argc, &argv);
    /* Now let's parse the command line options first. */
    parse_cmd_line(argc, argv, &options);
    if (!gtk_init_check_ok) {
        if (argc < 2) {
            /* GTK check failed, and no arguments passed to indicate
               that user is intending to only remote control a running
               session */
            g_printerr(_("audacious: Unable to open display, exiting.\n"));
            exit(EXIT_FAILURE);
        }

        handle_cmd_line_options(&options, TRUE);

        /* we could be running headless, so GTK probably wont matter */
        if (options.headless != 1)
          exit(EXIT_SUCCESS);
    }

    if (!vfs_init()) {
        g_printerr(Q_("Could not initialize VFS.\n"));
        exit(EXIT_FAILURE);
    }

    signal(SIGPIPE, SIG_IGN);   /* for controlsocket.c */
    signal(SIGSEGV, segfault_handler);

    g_random_set_seed(time(NULL));

    bmp_config_load();

    if (options.session != -1 || !ctrlsocket_setup()) {
        handle_cmd_line_options(&options, TRUE);
        exit(EXIT_SUCCESS);
    }

    if (options.headless != 1)
    {

        bmp_set_default_icon();

        gtk_accel_map_load(bmp_paths[BMP_PATH_ACCEL_FILE]);

        mainwin_create();

        playlistwin_create();
        equalizerwin_create();

        if (!init_skins(cfg.skin)) {
            run_load_skin_error_dialog(cfg.skin);
            exit(EXIT_FAILURE);
        }

        GDK_THREADS_ENTER();
    }

    plugin_system_init();

    playlist_load(bmp_paths[BMP_PATH_PLAYLIST_FILE]);
    playlist_set_position(cfg.playlist_position);

    /* this needs to be called after all 3 windows are created and
     * input plugins are setup'ed 
     * but not if we're running headless --nenolod
     */
    mainwin_setup_menus();

    if (options.headless != 1)
        GDK_THREADS_LEAVE();

    ctrlsocket_start();

    handle_cmd_line_options(&options, FALSE);

    if (options.headless != 1)
    {
        GDK_THREADS_ENTER();

        read_volume(VOLSET_STARTUP);
        mainwin_set_info_text();

        /* FIXME: delayed, because it deals directly with the plugin
         * interface to set menu items */
        create_prefs_window();

        if (cfg.player_visible)
            mainwin_show(TRUE);
        else if (!cfg.playlist_visible && !cfg.equalizer_visible)
            mainwin_show(TRUE);

        if (cfg.equalizer_visible)
            equalizerwin_show(TRUE);

        if (cfg.playlist_visible)
            playlistwin_show();

        hint_set_always(cfg.always_on_top);

        playlist_start_get_info_thread();
        mainwin_attach_idle_func();

	starting_up = FALSE;

	has_x11_connection = TRUE;

	if (cfg.resume_playback_on_startup) {
		if (cfg.resume_playback_on_startup_time != -1) {
			while (gtk_events_pending()) gtk_main_iteration();
			bmp_playback_initiate();
			while (!ip_data.playing)
				g_usleep(10000);
			bmp_playback_seek(cfg.resume_playback_on_startup_time /
					  1000);
		}
	}

        gtk_main();

        GDK_THREADS_LEAVE();

        return EXIT_SUCCESS;
    }
    else
    {
        mainwin_set_info_text();
        playlist_start_get_info_thread();

	starting_up = FALSE;

        for (;;)
	{
	    /* headless eventloop */
	    audcore_generic_events();
	    free_vis_data();          /* to prevent buffer overflow -- paranoia */
            xmms_usleep(10000);
        }

        return EXIT_SUCCESS;
    }
}
