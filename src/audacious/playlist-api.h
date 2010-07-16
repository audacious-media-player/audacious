/*
 * playlist-api.h
 * Copyright 2010 John Lindgren
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

/* Do not include this file directly; use playlist.h instead. */

/* CAUTION: These functions are not thread safe. */

/* --- PLAYLIST CORE API --- */

/* Returns the number of playlists currently open.  There will always be at
 * least one playlist open.  The playlists are numbered starting from zero. */
AUD_FUNC0 (gint, playlist_count)

/* Adds a new playlist before the one numbered <at>.  If <at> is negative or
 * equal to the number of playlists, adds a new playlist after the last one. */
AUD_FUNC1 (void, playlist_insert, gint, at)

/* Moves a contiguous block of <count> playlists starting with the one numbered
 * <from> such that that playlist ends up at the position <to>. */
AUD_FUNC3 (void, playlist_reorder, gint, from, gint, to, gint, count)

/* Closes a playlist.  CAUTION: The playlist is not saved, and no confirmation
 * is presented to the user. */
AUD_FUNC1 (void, playlist_delete, gint, playlist)

/* Sets the filename associated with a playlist.  (Audacious currently makes no
 * use of the filename.) */
AUD_FUNC2 (void, playlist_set_filename, gint, playlist, const gchar *, filename)

/* Returns the filename associated with a playlist. */
AUD_FUNC1 (const gchar *, playlist_get_filename, gint, playlist)

/* Sets the title associated with a playlist. */
AUD_FUNC2 (void, playlist_set_title, gint, playlist, const gchar *, title)

/* Returns the title associated with a playlist. */
AUD_FUNC1 (const gchar *, playlist_get_title, gint, playlist)

/* Marks a playlist as active.  This is the playlist which the user will see and
 * on which most DRCT functions will take effect. */
AUD_FUNC1 (void, playlist_set_active, gint, playlist)

/* Returns the number of the playlist marked active. */
AUD_FUNC0 (gint, playlist_get_active)

/* Sets the playlist in which playback will begin when drct_play() is called.
 * This does not mark the playlist as active.  If a song is already playing, it
 * will be stopped.  If <playlist> is negative, calls to playlist_get_playing()
 * will return -1, and the behavior of drct_play() is unspecified. */
AUD_FUNC1 (void, playlist_set_playing, gint, playlist)

/* Returns the number of the playlist from which the last song played was taken,
 * or -1 if that cannot be determined.  Note that this playlist may not be
 * marked active. */
AUD_FUNC0 (gint, playlist_get_playing)

/* Returns the number of entries in a playlist.  The entries are numbered
 * starting from zero. */
AUD_FUNC1 (gint, playlist_entry_count, gint, playlist)

/* Adds a song file to a playlist before the entry numbered <at>.  If <at> is
 * negative or equal to the number of entries, the song is added after the last
 * entry.  <tuple> may be NULL, in which case Audacious will attempt to read
 * metadata from the song file.  Audacious will free the memory used by the
 * filename and the tuple when they are no longer needed.  NOTE: This function
 * cannot be used to insert playlist files or entire folders.  To do that, see
 * playlist_insert_playlist or playlist_insert_folder. */
AUD_FUNC4 (void, playlist_entry_insert, gint, playlist, gint, at, gchar *,
 filename, Tuple *, tuple)

/* Similar to playlist_entry_insert, adds multiple song files to a playlist.
 * The filenames are stored as (gchar *) in an index (see libaudcore/index.h).
 * Tuples are likewise stored as (Tuple *) in an index of the same length.
 * <tuples> may be NULL, or individual pointers within it may be NULL.
 * Audacious will free both indexes, the filenames, and the tuples when they are
 * no longer needed. */
AUD_FUNC4 (void, playlist_entry_insert_batch, gint, playlist, gint, at,
 struct index *, filenames, struct index *, tuples)

/* Removes a contiguous block of <number> entries starting from the one numbered
 * <at> from a playlist. */
AUD_FUNC3 (void, playlist_entry_delete, gint, playlist, gint, at, gint, number)

/* Returns the filename of an entry.  The returned string is valid until another
 * playlist function is called or control returns to the program's main loop. */
AUD_FUNC2 (const gchar *, playlist_entry_get_filename, gint, playlist, gint,
 entry)

/* Returns the tuple associated with an entry, or NULL if one is not available.
 * The returned tuple is read-only and valid until another playlist function is
 * called or control returns to the program's main loop.  If <fast> is nonzero,
 * returns NULL if metadata for the entry has not yet been read from the song
 * file. */
AUD_FUNC3 (const Tuple *, playlist_entry_get_tuple, gint, playlist, gint, entry,
 gboolean, fast)

/* Returns a formatted title string for an entry.  This may include information
 * such as the filename, song title, and/or artist.  The returned string is
 * valid until another playlist function is called or control returns to the
 * program's main loop.  <fast> is as in playlist_entry_get_tuple(). */
AUD_FUNC3 (const gchar *, playlist_entry_get_title, gint, playlist, gint, entry,
 gboolean, fast)

/* Returns the length in milliseconds of an entry, or -1 if the length is not
 * known.  <fast> is as in playlist_entry_get_tuple(). */
AUD_FUNC3 (gint, playlist_entry_get_length, gint, playlist, gint, entry,
 gboolean, fast)

/* Sets the entry which will be played (if this playlist is selected with
 * playlist_set_playing()) when drct_play() is called.  If a song from this
 * playlist is already playing, it will be stopped.  If <position> is negative,
 * calls to playlist_get_position() will return -1, and the behavior of
 * drct_play() is unspecified. */
AUD_FUNC2 (void, playlist_set_position, gint, playlist, gint, position)

/* Returns the number of the entry which was last played from this playlist, or
 * -1 if that cannot be determined. */
AUD_FUNC1 (gint, playlist_get_position, gint, playlist)

/* Sets whether an entry is selected. */
AUD_FUNC3 (void, playlist_entry_set_selected, gint, playlist, gint, entry,
 gboolean, selected)

/* Returns whether an entry is selected. */
AUD_FUNC2 (gboolean, playlist_entry_get_selected, gint, playlist, gint, entry)

/* Returns the number of selected entries in a playlist. */
AUD_FUNC1 (gint, playlist_selected_count, gint, playlist)

/* Selects all (or none) of the entries in a playlist. */
AUD_FUNC2 (void, playlist_select_all, gint, playlist, gboolean, selected)

/* Moves an entry, along with selected entries near it, within a playlist, by an
 * offset of <distance> entries.  For an exact definition of "near it", read the
 * source code.  Returns the offset by which the entry was actually moved, which
 * may be less (in absolute value) than the requested offset. */
AUD_FUNC3 (gint, playlist_shift, gint, playlist, gint, position, gint, distance)

/* Removes the selected entries from a playlist. */
AUD_FUNC1 (void, playlist_delete_selected, gint, playlist)

/* Sorts the entries in a playlist based on filename.  The callback function
 * should return negative if the first filename comes before the second,
 * positive if it comes after, or zero if the two are indistinguishable. */
AUD_FUNC2 (void, playlist_sort_by_filename, gint, playlist,
 PlaylistFilenameCompareFunc, compare)

/* Sorts the entries in a playlist based on tuple. */
AUD_FUNC2 (void, playlist_sort_by_tuple, gint, playlist,
 PlaylistTupleCompareFunc, compare)

/* Sorts only the selected entries in a playlist based on filename. */
AUD_FUNC2 (void, playlist_sort_selected_by_filename, gint, playlist,
 PlaylistFilenameCompareFunc, compare)

/* Sorts only the selected entries in a playlist based on tuple. */
AUD_FUNC2 (void, playlist_sort_selected_by_tuple, gint, playlist,
 PlaylistTupleCompareFunc, compare)

/* Reverses the order of the entries in a playlist. */
AUD_FUNC1 (void, playlist_reverse, gint, playlist)

/* Discards the metadata stored for all the entries in a playlist and starts
 * reading it afresh from the song files in the background. */
AUD_FUNC1 (void, playlist_rescan, gint, playlist)

/* Discards the metadata stored for all the entries that refer to a particular
 * song file, in whatever playlist they appear, and starts reading it afresh
 * from that file in the background. */
AUD_FUNC1 (void, playlist_rescan_file, const gchar *, filename)

/* Calculates the total length in milliseconds of all the entries in a playlist.
 * <fast> is as in playlist_entry_get_tuple(). */
AUD_FUNC2 (gint64, playlist_get_total_length, gint, playlist, gboolean, fast)

/* Calculates the total length in milliseconds of only the selected entries in a
 * playlist.  <fast> is as in playlist_entry_get_tuple(). */
AUD_FUNC2 (gint64, playlist_get_selected_length, gint, playlist, gboolean, fast)

/* Returns the number of entries in a playlist queue.  The entries are numbered
 * starting from zero, lower numbers being played first. */
AUD_FUNC1 (gint, playlist_queue_count, gint, playlist)

/* Adds an entry to a playlist's queue before the entry numbered <at> in the
 * queue.  If <at> is negative or equal to the number of entries in the queue,
 * adds the entry after the last one in the queue.  The same entry cannot be
 * added to the queue more than once. */
AUD_FUNC3 (void, playlist_queue_insert, gint, playlist, gint, at, gint, entry)

/* Adds the selected entries in a playlist to the queue, if they are not already
 * in it. */
AUD_FUNC2 (void, playlist_queue_insert_selected, gint, playlist, gint, at)

/* Returns the position in the playlist of the entry at a given position in the
 * queue. */
AUD_FUNC2 (gint, playlist_queue_get_entry, gint, playlist, gint, at)

/* Returns the position in the queue of the entry at a given position in the
 * playlist.  If it is not in the queue, returns -1. */
AUD_FUNC2 (gint, playlist_queue_find_entry, gint, playlist, gint, entry)

/* Removes a contiguous block of <number> entries starting with the one numbered
 * <at> from the queue. */
AUD_FUNC3 (void, playlist_queue_delete, gint, playlist, gint, at, gint, number)

/* Removes the selected entries in a playlist from the queue, if they are in it. */
AUD_FUNC1 (void, playlist_queue_delete_selected, gint, playlist)

/* Returns nonzero if any playlist has been changed since the last call of the
 * "playlist update" hook.  If called from within the hook, returns nonzero. */
AUD_FUNC0 (gboolean, playlist_update_pending)

/* --- PLAYLIST UTILITY API --- */

/* Sorts the entries in a playlist according to one of the schemes listed in
 * playlist.h. */
AUD_FUNC2 (void, playlist_sort_by_scheme, gint, playlist, gint, scheme)

/* Sorts only the selected entries in a playlist according to one of those
 * schemes. */
AUD_FUNC2 (void, playlist_sort_selected_by_scheme, gint, playlist, gint, scheme)

/* Removes duplicate entries in a playlist according to one of those schemes.
 * As currently implemented, first sorts the playlist. */
AUD_FUNC2 (void, playlist_remove_duplicates_by_scheme, gint, playlist, gint,
 scheme)

/* Removes from a playlist entries of files that do not exists or were not
 * recognized. */
AUD_FUNC1 (void, playlist_remove_failed, gint, playlist)

/* Selects all the entries in a playlist that match regular expressions stored
 * in the fields of a tuple.  Does not free the memory used by the tuple.
 * Example: To select all the songs whose title starts with the letter "A",
 * create a blank tuple and set its title field to "^A". */
AUD_FUNC2 (void, playlist_select_by_patterns, gint, playlist, const Tuple *,
 patterns)

/* Returns nonzero if <filename> refers to a playlist file. */
AUD_FUNC1 (gboolean, filename_is_playlist, const gchar *, filename)

/* Reads entries from a playlist file and add them to a playlist.  <at> is as in
 * playlist_entry_insert().  Returns nonzero on success. */
AUD_FUNC3 (gboolean, playlist_insert_playlist, gint, playlist, gint, at,
 const gchar *, filename)

/* Saves the entries in a playlist to a playlist file.  The format of the file
 * is determined from the file extension.  Returns nonzero on success. */
AUD_FUNC2 (gboolean, playlist_save, gint, playlist, const gchar *, filename)

/* Begins searching a folder recursively for supported files (including playlist
 * files) and adding them to a playlist.  The search continues in the
 * background.  If <play> is nonzero, begins playback of the first entry added
 * (or a random entry if shuffle is enabled) once the search is complete.
 * CAUTION: Editing the playlist while the search is in progress may lead to
 * unexpected results. */
AUD_FUNC4 (void, playlist_insert_folder, gint, playlist, gint, at,
 const gchar *, folder, gboolean, play)
