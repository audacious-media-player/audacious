/*  Audacious
 *  Copyright (c) 2005-2007 Audacious team
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

#ifndef ACTIONS_PLAYLIST_H
#define ACTIONS_PLAYLIST_H

void action_new_list(void);
void action_load_list(void);
void action_save_list(void);
void action_save_default_list(void);
void action_open_list_manager(void);
void action_refresh_list(void);

void action_search_and_select(void);
void action_invert_selection(void);
void action_select_all(void);
void action_select_none(void);

void action_clear_queue(void);
void action_remove_unavailable(void);
void action_remove_dupes_by_title(void);
void action_remove_dupes_by_filename(void);
void action_remove_dupes_by_full_path(void);
void action_remove_all(void);
void action_remove_selected(void);
void action_remove_unselected(void);

void action_add_cd(void);
void action_add_url(void);
void action_add_files(void);

void action_randomize_list(void);
void action_reverse_list(void);

void action_sort_by_title(void);
void action_sort_by_artist(void);
void action_sort_by_filename(void);
void action_sort_by_full_path(void);
void action_sort_by_date(void);
void action_sort_by_track_number(void);
void action_sort_by_playlist_entry(void);

void action_sort_selected_by_title(void);
void action_sort_selected_by_artist(void);
void action_sort_selected_by_filename(void);
void action_sort_selected_by_full_path(void);
void action_sort_selected_by_date(void);
void action_sort_selected_by_track_number(void);
void action_sort_selected_by_playlist_entry(void);

void action_view_track_info(void);
void action_queue_toggle(void);

#endif
