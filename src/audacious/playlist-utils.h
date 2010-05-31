/*
 * playlist-utils.h
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef AUDACIOUS_PLAYLIST_UTILS_H
#define AUDACIOUS_PLAYLIST_UTILS_H

extern const gint n_titlestring_presets;

const gchar * get_gentitle_format (void);

void playlist_sort_by_scheme (gint playlist, gint scheme);
void playlist_sort_selected_by_scheme (gint playlist, gint scheme);
void playlist_remove_duplicates_by_scheme (gint playlist, gint scheme);
void playlist_remove_failed (gint playlist);
void playlist_select_by_patterns (gint playlist, const Tuple * patterns);

gboolean filename_is_playlist (const gchar * filename);

gboolean playlist_insert_playlist (gint playlist, gint at, const gchar *
 filename);
gboolean playlist_save (gint playlist, const gchar * filename);
void playlist_insert_folder (gint playlist, gint at, const gchar * folder);
void playlist_insert_folder_v2 (gint playlist, gint at, const gchar * folder,
 gboolean play);

void save_playlists (void);
void load_playlists (void);

#endif
