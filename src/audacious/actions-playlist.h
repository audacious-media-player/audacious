/*
 * audacious: Cross-platform multimedia player.
 * actions-playlist.h: Definition of common actions.      
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

#ifndef ACTIONS_PLAYLIST_H
#define ACTIONS_PLAYLIST_H

void action_playlist_load_list(void);
void action_playlist_save_list(void);
void action_playlist_save_default_list(void);
void action_playlist_refresh_list(void);
void action_open_list_manager(void);

void action_playlist_prev(void);
void action_playlist_new(void);
void action_playlist_next(void);
void action_playlist_delete(void);

void action_playlist_search_and_select(void);
void action_playlist_invert_selection(void);
void action_playlist_select_all(void);
void action_playlist_select_none(void);

void action_playlist_clear_queue(void);
void action_playlist_remove_unavailable(void);
void action_playlist_remove_dupes_by_title(void);
void action_playlist_remove_dupes_by_filename(void);
void action_playlist_remove_dupes_by_full_path(void);
void action_playlist_remove_all(void);
void action_playlist_remove_selected(void);
void action_playlist_remove_unselected(void);

void action_playlist_add_cd(void);
void action_playlist_add_url(void);
void action_playlist_add_files(void);

void action_playlist_randomize_list(void);
void action_playlist_reverse_list(void);

void action_playlist_sort_by_title(void);
void action_playlist_sort_by_artist(void);
void action_playlist_sort_by_filename(void);
void action_playlist_sort_by_full_path(void);
void action_playlist_sort_by_date(void);
void action_playlist_sort_by_track_number(void);
void action_playlist_sort_by_playlist_entry(void);

void action_playlist_sort_selected_by_title(void);
void action_playlist_sort_selected_by_artist(void);
void action_playlist_sort_selected_by_filename(void);
void action_playlist_sort_selected_by_full_path(void);
void action_playlist_sort_selected_by_date(void);
void action_playlist_sort_selected_by_track_number(void);
void action_playlist_sort_selected_by_playlist_entry(void);

void action_playlist_track_info(void);
void action_queue_toggle(void);

#endif
