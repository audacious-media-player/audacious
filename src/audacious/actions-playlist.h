/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team.
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

#ifndef AUDACIOUS_ACTIONS_PLAYLIST_H
#define AUDACIOUS_ACTIONS_PLAYLIST_H

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

/* void action_playlist_add_cd(void); - this is no longer needed, as the respective menu entry is added from within the cdaudio plugin */
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

#endif /* AUDACIOUS_ACTIONS_PLAYLIST_H */
