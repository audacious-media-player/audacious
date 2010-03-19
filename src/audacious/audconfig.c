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

#include "audconfig.h"

#include "effect.h"
#include "general.h"
#include "playback.h"
#include "playlist-new.h"
#include "playlist-utils.h"
#include "pluginenum.h"
#include "plugin-registry.h"
#include "util.h"
#include "visualization.h"

AudConfig cfg;

AudConfig aud_default_config = {
    .shuffle = FALSE,
    .repeat = FALSE,
    .equalizer_autoload = FALSE,
    .equalizer_active = FALSE,
    .playlist_visible = FALSE,
    .equalizer_visible = FALSE,
    .player_visible = TRUE,
    .show_numbers_in_pl = TRUE,
    .no_playlist_advance = FALSE,
    .stopaftersong = FALSE,
    .close_dialog_open = TRUE,
    .equalizer_preamp = 0.0,
    .equalizer_bands = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    .filesel_path = NULL,
    .playlist_path = NULL,
    .enabled_gplugins = NULL,          /* enabled general plugins */
    .enabled_vplugins = NULL,          /* enabled visualization plugins */
    .enabled_eplugins = NULL,          /* enabled effect plugins */
    .enabled_dplugins = NULL,          /* enabled discovery plugins */
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
    .clear_playlist = FALSE,
    .output_path = NULL,
    .output_number = -1,
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
    {"no_playlist_advance", &cfg.no_playlist_advance, TRUE},
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
    {"clear_playlist", & cfg.clear_playlist, TRUE},
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
    {"output_number", & cfg.output_number, TRUE},
};

static gint ncfgient = G_N_ELEMENTS(aud_numents);

static aud_cfg_strent aud_strents[] = {
    {"eqpreset_default_file", &cfg.eqpreset_default_file, TRUE},
    {"eqpreset_extension", &cfg.eqpreset_extension, TRUE},
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
    {"output_path", & cfg.output_path, TRUE},
};

static gint ncfgsent = G_N_ELEMENTS(aud_strents);


static gboolean
save_extra_playlist(const gchar * path, const gchar * basename,
        gpointer savedlist)
{
    gint playlists, playlist;
    GList **saved;
    int found;
    const gchar * filename;

    playlists = playlist_count ();
    saved = (GList **) savedlist;

    found = 0;

    for (playlist = 0; playlist < playlists; playlist ++)
    {
        if (g_list_find (* saved, GINT_TO_POINTER (playlist)) != NULL)
            continue;

        filename = playlist_get_filename (playlist);

        if (filename == NULL)
            continue;

        if (strcmp(filename, path) == 0) {
            /* Save playlist */
            playlist_save(playlist, path);
            * saved = g_list_prepend (* saved, GINT_TO_POINTER (playlist));
            found = 1;
            break;
        }
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
    gint playlists, playlist;
    gchar * pos, * ext, * basename, * newbasename, * new_filename;
    const gchar * filename;
    int i, num, isdigits;

    playlists = playlist_count ();

    for (playlist = 0; playlist < playlists; playlist ++)
    {
        if (g_list_find (saved, GINT_TO_POINTER (playlist)) != NULL)
            continue;

        filename = playlist_get_filename (playlist);

        if (filename == NULL || g_file_test (filename, G_FILE_TEST_IS_DIR))
        {
            /* default basename */
#ifdef HAVE_XSPF_PLAYLIST
            basename = g_strdup("playlist_01.xspf");
#else
            basename = g_strdup("playlist_01.m3u");
#endif
        } else {
            basename = g_path_get_basename(filename);
        }

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
        new_filename = NULL;

        do {
            g_free (new_filename);

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

            new_filename = g_build_filename (aud_paths[BMP_PATH_PLAYLISTS_DIR],
                    newbasename, NULL);
            g_free(newbasename);
        }
        while (g_file_test (new_filename, G_FILE_TEST_EXISTS));

        playlist_save (playlist, new_filename);
cleanup:
        g_free (new_filename);
        g_free(basename);
    }
}


void
aud_config_free(void)
{
  gint i;
  for (i = 0; i < ncfgsent; ++i) {
    if ( *(aud_strents[i].se_vloc) != NULL )
    {
      g_free( *(aud_strents[i].se_vloc) );
      *(aud_strents[i].se_vloc) = NULL;
    }
  }
}

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

    memcpy(&cfg, &aud_default_config, sizeof(AudConfig));

    db = cfg_db_open();
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
        cfg.cover_name_include = g_strdup("");

    if (!cfg.cover_name_exclude)
        cfg.cover_name_exclude = g_strdup("back");
}

void save_all_playlists (void)
{
    GList * saved;

    /* Main playlist becomes #0 at load, so save #0 as main. -jlindgren */
    if (! playlist_save (0, aud_paths[BMP_PATH_PLAYLIST_FILE]))
        g_warning ("Could not save main playlist\n");

    /* Save extra playlists that were loaded from PLAYLISTS_DIR  */
    saved = g_list_append (NULL, GINT_TO_POINTER (0));

    if (! dir_foreach (aud_paths[BMP_PATH_PLAYLISTS_DIR], save_extra_playlist,
     & saved, NULL))
        g_warning ("Could not save extra playlists\n");

    /* Save other playlists to PLAYLISTS_DIR */
    save_other_playlists (saved);
    g_list_free (saved);
}

static void save_output_path (void)
{
    const gchar * path = NULL;
    gint type, number = -1;

    if (current_output_plugin != NULL)
        plugin_get_path (current_output_plugin, & path, & type, & number);

    cfg.output_path = (path != NULL) ? g_strdup (path) : NULL;
    cfg.output_number = number;
}

void
aud_config_save(void)
{
    GList *node;
    gchar *str;
    gint i;
    mcs_handle_t *db;

    cfg.resume_state = playback_get_playing () ? (playback_get_paused () ? 2 :
     1) : 0;
    cfg.resume_playback_on_startup_time = playback_get_playing () ?
     playback_get_time () : 0;

    save_output_path ();

    db = cfg_db_open();

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

    hook_call("config save", db);

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

    cfg_db_close(db);

    save_all_playlists ();
}
