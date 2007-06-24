/*
 * audacious: Cross-platform multimedia player.
 * actions-mainwin.h: Definition of common actions.      
 *
 * Copyright (c) 2005-2007 Audacious development team.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
void action_preferences(void);
void action_quit(void);
void action_current_track_info(void);


#endif
