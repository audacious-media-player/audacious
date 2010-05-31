/*
 * playlist-new.h
 * Copyright 2009 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
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
 * and indexes passed to playlist_entry_insert, playlist_entry_insert_batch and
 * playlist_entry_set_tuple will be freed when no longer needed. Returned
 * pointers refer to static data which should not be freed.
 *
 * Clarification: Code calling the functions mentioned above must own a
 * reference to each tuple passed. Passing a tuple to one of these functions
 * means that the reference is given to the playlist. Tuples returned by
 * playlist functions, in contrast, are read-only, and no reference is given to
 * the calling code.
 *
 * IMPORTANT: The playlist is not thread-safe.  If you want to examine or
 * manipulate it, you must do so in the main thread.  Not following this rule
 * will (and has) led to crashes.
 */

void playlist_init (void);
void playlist_end (void);
void playlist_load_state (void);
void playlist_save_state (void);

gint playlist_count (void);
void playlist_insert (gint at);
void playlist_reorder (gint from, gint to, gint count);
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
void playlist_entry_set_tuple (gint playlist, gint entry, Tuple * tuple);
const Tuple * playlist_entry_get_tuple (gint playlist, gint entry);
const gchar * playlist_entry_get_title (gint playlist, gint entry);
gint playlist_entry_get_length (gint playlist, gint entry);
void playlist_entry_set_segmentation (gint playlist, gint entry, gint start, gint end);
gboolean playlist_entry_is_segmented (gint playlist, gint entry);
gint playlist_entry_get_start_time (gint playlist, gint entry);
gint playlist_entry_get_end_time (gint playlist, gint entry);

void playlist_set_position (gint playlist, gint position);
gint playlist_get_position (gint playlist);

void playlist_entry_set_selected (gint playlist, gint entry, gboolean selected);
gboolean playlist_entry_get_selected (gint playlist, gint entry);
gint playlist_selected_count (gint playlist);
void playlist_select_all (gint playlist, gboolean selected);

gint playlist_shift (gint playlist, gint position, gint distance);
gint playlist_shift_selected (gint playlist, gint distance);
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

void playlist_reformat_titles (void);
void playlist_rescan (gint playlist);
void playlist_rescan_file (const gchar * filename);

gint64 playlist_get_total_length (gint playlist);
gint64 playlist_get_selected_length (gint playlist);

gint playlist_queue_count (gint playlist);
void playlist_queue_insert (gint playlist, gint at, gint entry);
void playlist_queue_insert_selected (gint playlist, gint at);
gint playlist_queue_get_entry (gint playlist, gint at);
gint playlist_queue_find_entry (gint playlist, gint entry);
void playlist_queue_delete (gint playlist, gint at, gint number);

gboolean playlist_prev_song (gint playlist);
gboolean playlist_next_song (gint playlist, gboolean repeat);

/* TRUE if called within "playlist update" hook */
gboolean playlist_update_pending (void);

#endif
