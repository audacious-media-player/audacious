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

#ifndef AUDCONFIG_H
#define AUDCONFIG_H

#include <glib.h>

struct _AudConfig {
    gint player_x, player_y;
    gint equalizer_x, equalizer_y;
    gint playlist_x, playlist_y;
    gint playlist_width, playlist_height;
    gint snap_distance;
    gboolean use_realtime;
    gboolean shuffle, repeat;
    gboolean scaled, autoscroll;
    gboolean analyzer_peaks, equalizer_autoload, easy_move, equalizer_active;
    gboolean playlist_visible, equalizer_visible, player_visible;
    gboolean player_shaded, playlist_shaded, equalizer_shaded;
    gboolean allow_multiple_instances, always_show_cb;
    gboolean convert_underscore, convert_twenty, convert_slash;
    gboolean show_numbers_in_pl;
    gboolean snap_windows, save_window_position;
    gboolean dim_titlebar;
    gboolean get_info_on_load, get_info_on_demand;
    gboolean eq_scaled_linked;
    gboolean sort_jump_to_file;
    gboolean use_eplugins;
    gboolean always_on_top, sticky;
    gboolean no_playlist_advance;
    gboolean stopaftersong;
    gboolean refresh_file_list;
    gboolean use_pl_metadata;
    gboolean warn_about_win_visibility;
    gboolean use_backslash_as_dir_delimiter;
    gboolean random_skin_on_play;
    gboolean use_fontsets;
    gboolean mainwin_use_bitmapfont;
    gboolean allow_broken_skins;
    gboolean close_dialog_open;
    gboolean close_dialog_add;
    gfloat equalizer_preamp, equalizer_bands[10];
    gfloat scale_factor;
    gchar *skin;
    gchar *outputplugin;
    gchar *filesel_path;
    gchar *playlist_path;
    gchar *playlist_font, *mainwin_font;
    gchar *disabled_iplugins;
    gchar *enabled_gplugins, *enabled_vplugins, *enabled_eplugins, *enabled_dplugins ;
    gchar *eqpreset_default_file, *eqpreset_extension;
    GList *url_history;
    gint timer_mode;
    gint vis_type;
    gint analyzer_mode, analyzer_type;
    gint scope_mode;
    gint voiceprint_mode;
    gint vu_mode, vis_refresh;
    gint analyzer_falloff, peaks_falloff;
    gint playlist_position;
    gint pause_between_songs_time;
    gboolean pause_between_songs;
    gboolean show_wm_decorations;
    gint mouse_change;
    gboolean playlist_transparent;
    gint titlestring_preset;
    gchar *gentitle_format;
    gboolean softvolume_enable;
    gboolean eq_extra_filtering;
    gint scroll_pl_by;
    gboolean resume_playback_on_startup;
    gint resume_playback_on_startup_time;
    gboolean show_separator_in_pl;
    gchar *chardet_detector;
    gchar *chardet_fallback;
    gint output_buffer_size;
    gboolean playlist_detect;
    gboolean show_filepopup_for_tuple;
    gchar *cover_name_include, *cover_name_exclude;
    gboolean recurse_for_cover;
    gint recurse_for_cover_depth;
    gchar *session_uri_base;
    gint filepopup_pixelsize;
    gint filepopup_delay;
    gboolean use_file_cover;
    gboolean use_xmms_style_fileselector;
    gboolean use_extension_probing;
    gint colorize_r; gint colorize_g; gint colorize_b;
    gboolean filepopup_showprogressbar;
    gboolean close_jtf_dialog;
    gboolean twoway_scroll;
    gboolean software_volume_control;
    gboolean warn_about_broken_gtk_engines;
    gboolean disable_inline_gtk;
    gboolean remember_jtf_entry;
    gint output_bit_depth;
    gboolean enable_replay_gain;
    gboolean enable_clipping_prevention;
    gboolean replay_gain_track;
    gboolean replay_gain_album;
    gboolean enable_adaptive_scaler;
    gfloat replay_gain_preamp;
    gfloat default_gain;
    gint saved_volume;
#ifdef USE_SAMPLERATE
    gboolean enable_src;
    gint src_rate;
    gint src_type;
#endif
    gboolean bypass_dsp;
};

typedef struct _AudConfig AudConfig;

extern AudConfig cfg;
extern AudConfig aud_default_config;

void aud_config_free(void);
void aud_config_load(void);
void aud_config_save(void);

#endif
