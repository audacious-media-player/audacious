/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ACTIONS_MAINWIN_H
#define ACTIONS_MAINWIN_H

#include <glib.h>
#include <gtk/gtk.h>

/* actions below are handled in mainwin.c */


/* toggle actions */
void action_anamode_peaks(GtkToggleAction*);
void action_autoscroll_songname(GtkToggleAction*);
void action_playback_noplaylistadvance(GtkToggleAction*);
void action_playback_repeat(GtkToggleAction*);
void action_playback_shuffle(GtkToggleAction*);
void action_stop_after_current_song(GtkToggleAction*);
void action_view_always_on_top(GtkToggleAction*);
void action_view_doublesize(GtkToggleAction*);
void action_view_easymove(GtkToggleAction*);
void action_view_on_all_workspaces(GtkToggleAction*);
void action_roll_up_equalizer(GtkToggleAction*);
void action_roll_up_player(GtkToggleAction*);
void action_roll_up_playlist_editor(GtkToggleAction*);
void action_show_equalizer(GtkToggleAction*);
void action_show_player(GtkToggleAction*);
void action_show_playlist_editor(GtkToggleAction*);

/* radio actions (one for each radio action group) */
void action_anafoff(GtkAction*,GtkRadioAction*);
void action_anamode(GtkAction*,GtkRadioAction*);
void action_anatype(GtkAction*,GtkRadioAction*);
void action_peafoff(GtkAction*,GtkRadioAction*);
void action_refrate(GtkAction*,GtkRadioAction*);
void action_scomode(GtkAction*,GtkRadioAction*);
void action_vismode(GtkAction*,GtkRadioAction*);
void action_vprmode(GtkAction*,GtkRadioAction*);
void action_wshmode(GtkAction*,GtkRadioAction*);
void action_viewtime(GtkAction*,GtkRadioAction*);

/* normal actions */
void action_about_audacious(void);
void action_ab_clear(void);
void action_ab_set(void);
void action_jump_to_file(void);
void action_jump_to_playlist_start(void);
void action_jump_to_time(void);
void action_play_file(void);
void action_play_location(void);
void action_playback_next(void);
void action_playback_pause(void);
void action_playback_play(void);
void action_playback_playcd(void);
void action_playback_previous(void);
void action_playback_stop(void);
void action_playlist_prev(void);
void action_playlist_new(void);
void action_playlist_next(void);
void action_playlist_delete(void);
void action_preferences(void);
void action_quit(void);
void action_track_info(void);


#endif
