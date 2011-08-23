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
    .show_numbers_in_pl = TRUE,
    .leading_zero = TRUE,
    .resume_state = 0,
    .resume_playback_on_startup_time = 0,
    .chardet_detector = NULL,
    .chardet_fallback = NULL,
    .chardet_fallback_s = NULL,
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
};

static gint ncfgbent = G_N_ELEMENTS(aud_boolents);

static aud_cfg_nument aud_numents[] = {
    {"titlestring_preset", &cfg.titlestring_preset, TRUE},
    {"resume_state", & cfg.resume_state, TRUE},
    {"resume_playback_on_startup_time", &cfg.resume_playback_on_startup_time, TRUE},
};

static gint ncfgient = G_N_ELEMENTS(aud_numents);

static aud_cfg_strent aud_strents[] = {
    {"generic_title_format", &cfg.gentitle_format, TRUE},
    {"chardet_detector", &cfg.chardet_detector, TRUE},
    {"chardet_fallback", &cfg.chardet_fallback, TRUE},
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
    gint i;

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

    cfg_db_close(db);

    if (!cfg.gentitle_format)
        cfg.gentitle_format = g_strdup("${?artist:${artist} - }${?album:${album} - }${title}");

    if (!cfg.chardet_detector)
        cfg.chardet_detector = g_strdup("");

    if (!cfg.chardet_fallback)
        cfg.chardet_fallback = g_strdup("");

    aud_config_chardet_update();
}

void
aud_config_save(void)
{
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

    cfg_db_close(db);
}
