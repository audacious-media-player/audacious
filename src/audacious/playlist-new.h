/*
 * playlist-new.h
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

#ifndef AUDACIOUS_PLAYLIST_NEW_H
#define AUDACIOUS_PLAYLIST_NEW_H

#include "index.h"
#include "plugin.h"

/*
 * "Insert at position -1" means "append". Strings passed to
 * playlist_set_filename and playlist_set_title are duplicated. Strings, tuples,
 * and indexes passed to playlist_entry_insert and playlist_entry_insert_batch
 * will be freed when no longer needed. Returned pointers refer to static data
 * which should not be freed.
 */

void playlist_init (void);
void playlist_end (void);

gint playlist_count (void);
void playlist_insert (gint at);
void playlist_delete (gint playlist);

void playlist_set_filename (gint playlist, const gchar * filename);
const gchar * playlist_get_filename (gint playlist);
void playlist_set_title (gint playlist, const gchar * title);
const gchar * playlist_get_title (gint playlist);

void playlist_set_active (gint playlist);
gint playlist_get_active (void);
void playlist_set_playing (gint playlist);
gint playlist_get_playing (void);

gint playlist_entry_count (gint playlist);
void playlist_entry_insert (gint playlist, gint at, gchar * filename, Tuple *
 tuple);
void playlist_entry_insert_batch (gint playlist, gint at, struct index *
 filenames, struct index * tuples);
void playlist_entry_delete (gint playlist, gint at, gint number);

const gchar * playlist_entry_get_filename (gint playlist, gint entry);
InputPlugin * playlist_entry_get_decoder (gint playlist, gint entry);
const Tuple * playlist_entry_get_tuple (gint playlist, gint entry);
const gchar * playlist_entry_get_title (gint playlist, gint entry);
glong playlist_entry_get_length (gint playlist, gint entry);

void playlist_set_position (gint playlist, gint position);
gint playlist_get_position (gint playlist);

void playlist_entry_set_selected (gint playlist, gint entry, gboolean selected);
gboolean playlist_entry_get_selected (gint playlist, gint entry);
gint playlist_selected_count (gint playlist);
void playlist_select_all (gint playlist, gboolean selected);

gint playlist_shift (gint playlist, gint position, gint distance);
void playlist_delete_selected (gint playlist);
void playlist_reverse (gint playlist);
void playlist_randomize (gint playlist);

void playlist_sort_by_filename (gint playlist, gint (* compare) (const gchar *
 a, const gchar * b));
void playlist_sort_by_tuple (gint playlist, gint (* compare) (const Tuple * a,
 const Tuple * b));
void playlist_sort_selected_by_filename (gint playlist, gint (* compare) (const
 gchar * a, const gchar * b));
void playlist_sort_selected_by_tuple (gint playlist, gint (* compare) (const
 Tuple * a, const Tuple * b));

void playlist_reformat_titles ();
void playlist_rescan (gint playlist);

glong playlist_get_total_length (gint playlist);
glong playlist_get_selected_length (gint playlist);

void playlist_set_shuffle (gboolean shuffle);

gint playlist_queue_count (gint playlist);
void playlist_queue_insert (gint playlist, gint at, gint entry);
void playlist_queue_insert_selected (gint playlist, gint at);
gint playlist_queue_get_entry (gint playlist, gint at);
gint playlist_queue_find_entry (gint playlist, gint entry);
void playlist_queue_delete (gint playlist, gint at, gint number);

gboolean playlist_prev_song (gint playlist);
gboolean playlist_next_song (gint playlist, gboolean repeat);

#endif
