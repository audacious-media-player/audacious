/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious development team
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

#define AUD_EQUALIZER_NBANDS    (10)

struct _AudConfig {
    gboolean shuffle, repeat;
    gboolean equalizer_autoload, equalizer_active;
    gboolean playlist_visible, equalizer_visible, player_visible;
    gboolean show_numbers_in_pl;
    gboolean get_info_on_load;
    gboolean no_playlist_advance;
    gboolean stopaftersong;
    gboolean use_pl_metadata;
    gboolean close_dialog_open;
    gfloat equalizer_preamp, equalizer_bands[AUD_EQUALIZER_NBANDS];
    gchar *outputplugin;
    gchar *filesel_path;
    gchar *playlist_path;
    gchar *disabled_iplugins;
    gchar *enabled_gplugins, *enabled_vplugins, *enabled_eplugins, *enabled_dplugins ;
    gchar *eqpreset_default_file, *eqpreset_extension;
    GList *url_history;
    gint playlist_position;
    gint titlestring_preset;
    gchar *gentitle_format;
    gboolean resume_playback_on_startup;
    gint resume_state;
    gint resume_playback_on_startup_time;
    gchar *chardet_detector;
    gchar *chardet_fallback;
    gint output_buffer_size;
    gboolean playlist_detect;
    gboolean show_filepopup_for_tuple;
    gchar *cover_name_include, *cover_name_exclude;
    gboolean recurse_for_cover;
    gint recurse_for_cover_depth;
    gint filepopup_pixelsize;
    gint filepopup_delay;
    gboolean use_file_cover;
    gboolean use_extension_probing;
    gboolean filepopup_showprogressbar;
    gboolean close_jtf_dialog;
    gboolean software_volume_control;
    gboolean remember_jtf_entry;
    gint output_bit_depth;
    gboolean enable_replay_gain;
    gboolean enable_clipping_prevention;
    gboolean replay_gain_album;
    gboolean enable_adaptive_scaler;
    gfloat replay_gain_preamp;
    gfloat default_gain;
#ifdef USE_SAMPLERATE
    gboolean enable_src;
    gint src_rate;
    gint src_type;
#endif
    gboolean bypass_dsp;
    gint sw_volume_left, sw_volume_right;
    gboolean no_dithering;
};

typedef struct _AudConfig AudConfig;

extern AudConfig cfg;
extern AudConfig aud_default_config;

void aud_config_free(void);
void aud_config_load(void);
void aud_config_save(void);

#endif /* AUDACIOUS_AUDCONFIG_H */
