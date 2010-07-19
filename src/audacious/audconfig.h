/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2010  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
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

#ifndef AUDACIOUS_AUDCONFIG_H
#define AUDACIOUS_AUDCONFIG_H

#include <glib.h>
#include <audacious/types.h>

#ifndef _AUDACIOUS_CORE
#include <audacious/api.h>
#define aud_cfg (_aud_api_table->cfg)
#endif

struct _AudConfig {
    gboolean shuffle, repeat;
    gboolean equalizer_autoload, equalizer_active;
    gboolean playlist_visible, equalizer_visible, player_visible;
    gboolean show_numbers_in_pl;
    gboolean no_playlist_advance;
    gboolean stopaftersong;
    gboolean close_dialog_open;
    gfloat equalizer_preamp, equalizer_bands[AUD_EQUALIZER_NBANDS];
    gchar *filesel_path;
    gchar *playlist_path;
    gchar *eqpreset_default_file, *eqpreset_extension;
    GList *url_history;
    gint titlestring_preset;
    gchar *gentitle_format;
    gboolean resume_playback_on_startup;
    gint unused, unused2; /* for compatibility with v2.3 binary API */
    gint resume_state;
    gint resume_playback_on_startup_time;
    gchar *chardet_detector;
    gchar *chardet_fallback;
    gchar **chardet_fallback_s;
    gint output_buffer_size;
    gboolean show_filepopup_for_tuple;
    gchar *cover_name_include, *cover_name_exclude;
    gboolean recurse_for_cover;
    gint recurse_for_cover_depth;
    gint filepopup_pixelsize;
    gint filepopup_delay;
    gboolean use_file_cover;
    gboolean filepopup_showprogressbar;
    gboolean close_jtf_dialog;
    gboolean software_volume_control;
    gboolean remember_jtf_entry;
    gint output_bit_depth;
    gboolean enable_replay_gain;
    gboolean enable_clipping_prevention;
    gboolean replay_gain_track;
    gboolean replay_gain_album;
    gfloat replay_gain_preamp;
    gfloat default_gain;
    gint sw_volume_left, sw_volume_right;
    gboolean clear_playlist;
    gchar * output_path;
    gint output_number;
    gchar * iface_path;
    gint iface_number;

    /* libaudgui stuff */
    gboolean no_confirm_playlist_delete;
    gint playlist_manager_x, playlist_manager_y, playlist_manager_width,
     playlist_manager_height;
    gboolean playlist_manager_close_on_activate;
};

typedef struct _AudConfig AudConfig;

extern AudConfig cfg;
extern AudConfig aud_default_config;

void aud_config_free(void);
void aud_config_load(void);
void aud_config_save(void);

void aud_config_chardet_update(void);

#endif /* AUDACIOUS_AUDCONFIG_H */
