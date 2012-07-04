/*
 * playlist-api.h
 * Copyright 2010-2012 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

/* Do not include this file directly; use playlist.h instead. */

/* Any functions in this API with a return type of (char *) return pooled
 * strings that must not be modified and must be released with str_unref() when
 * no longer needed. */

/* --- PLAYLIST CORE API --- */

/* Returns the number of playlists currently open.  There will always be at
 * least one playlist open.  The playlists are numbered starting from zero. */
AUD_FUNC0 (int, playlist_count)

/* Adds a new playlist before the one numbered <at>.  If <at> is negative or
 * equal to the number of playlists, adds a new playlist after the last one. */
AUD_VFUNC1 (playlist_insert, int, at)

/* Moves a contiguous block of <count> playlists starting with the one numbered
 * <from> such that that playlist ends up at the position <to>. */
AUD_VFUNC3 (playlist_reorder, int, from, int, to, int, count)

/* Closes a playlist.  CAUTION: The playlist is not saved, and no confirmation
 * is presented to the user.  If <playlist> is the only playlist, a new playlist
 * is added.  If <playlist> is the active playlist, another playlist is marked
 * active.  If <playlist> is the currently playing playlist, playback is
 * stopped.  In this case, calls to playlist_get_playing() will return -1. */
AUD_VFUNC1 (playlist_delete, int, playlist)

/* Returns a unique non-negative integer which can be used to identify a given
 * playlist even if its numbering changes (as when playlists are reordered).
 * On error, returns -1. */
AUD_FUNC1 (int, playlist_get_unique_id, int, playlist)

/* Returns the number of the playlist identified by a given integer ID as
 * returned by playlist_get_unique_id().  If the playlist no longer exists,
 * returns -1. */
AUD_FUNC1 (int, playlist_by_unique_id, int, id)

/* Sets the filename associated with a playlist.  (Audacious currently makes no
 * use of the filename.) */
AUD_VFUNC2 (playlist_set_filename, int, playlist, const char *, filename)

/* Returns the filename associated with a playlist. */
AUD_FUNC1 (char *, playlist_get_filename, int, playlist)

/* Sets the title associated with a playlist. */
AUD_VFUNC2 (playlist_set_title, int, playlist, const char *, title)

/* Returns the title associated with a playlist. */
AUD_FUNC1 (char *, playlist_get_title, int, playlist)

/* Sets the active playlist.  This is the playlist that user interfaces will
 * show to the user. */
AUD_VFUNC1 (playlist_set_active, int, playlist)

/* Returns the number of the active playlist. */
AUD_FUNC0 (int, playlist_get_active)

/* Sets the currently playing playlist.  Any information needed to resume
 * playback from the previously playing playlist is saved, and playback is
 * resumed in the newly set playlist if possible.  (See playlist_set_position()
 * for a way to prevent this resuming.) */
AUD_VFUNC1 (playlist_set_playing, int, playlist)

/* Returns the number of the currently playing playlist, or -1 if there is none. */
AUD_FUNC0 (int, playlist_get_playing)

/* Returns the number of a "blank" playlist.  The active playlist is returned if
 * it has the default title and has no entries; otherwise, a new playlist is
 * added and returned. */
AUD_FUNC0 (int, playlist_get_blank)

/* Returns the number of the "temporary" playlist (which is no different from
 * any other playlist except in name).  If the playlist does not exist, a
 * "blank" playlist is obtained from playlist_get_blank() and is renamed to
 * become the temporary playlist. */
AUD_FUNC0 (int, playlist_get_temporary)

/* Returns the number of entries in a playlist.  The entries are numbered
 * starting from zero. */
AUD_FUNC1 (int, playlist_entry_count, int, playlist)

/* Adds a song file, playlist file, or folder to a playlist before the entry
 * numbered <at>.  If <at> is negative or equal to the number of entries, the
 * item is added after the last entry.  <tuple> may be NULL, in which case
 * Audacious will attempt to read metadata from the song file.  The caller gives
 * up one reference count to <tuple>.  If <play> is nonzero, Audacious will
 * begin playback of the items once they have been added.
 *
 * Because adding items to the playlist can be a slow process, this function may
 * return before the process is complete.  Hence, the caller must not assume
 * that there will be new entries in the playlist immediately. */
AUD_VFUNC5 (playlist_entry_insert, int, playlist, int, at, const char *,
 filename, Tuple *, tuple, bool_t, play)

/* Similar to playlist_entry_insert, adds multiple song files, playlist files,
 * or folders to a playlist.  The filenames, stored as (char *) in an index
 * (see libaudcore/index.h), must be pooled with str_get(); the caller gives up
 * one reference count to each filename.  Tuples are likewise stored as
 * (Tuple *) in an index of the same length; the caller gives up one reference
 * count to each tuple.  <tuples> may be NULL, or individual pointers within it
 * may be NULL.  Finally, the caller also gives up ownership of the indexes
 * themselves and should not access them after the call.   */
AUD_VFUNC5 (playlist_entry_insert_batch, int, playlist, int, at,
 Index *, filenames, Index *, tuples, bool_t, play)

/* Similar to playlist_entry_insert_batch, but allows the caller to prevent some
 * items from being added by returning false from the <filter> callback.  Useful
 * for searching a folder and adding only new files to the playlist.  <user> is
 * an untyped pointer passed to the <filter> callback. */
AUD_VFUNC7 (playlist_entry_insert_filtered, int, playlist, int, at,
 Index *, filenames, Index *, tuples, PlaylistFilterFunc, filter,
 void *, user, bool_t, play)

/* Removes a contiguous block of <number> entries starting from the one numbered
 * <at> from a playlist.  If the last song played is in this block, playback is
 * stopped.  In this case, calls to playlist_get_position() will return -1, and
 * the behavior of drct_play() is unspecified. */
AUD_VFUNC3 (playlist_entry_delete, int, playlist, int, at, int, number)

/* Returns the filename of an entry. */
AUD_FUNC2 (char *, playlist_entry_get_filename, int, playlist, int, entry)

/* Returns a handle to the decoder plugin associated with an entry, or NULL if
 * none can be found.  If <fast> is nonzero, returns NULL if no decoder plugin
 * has yet been found. */
AUD_FUNC3 (PluginHandle *, playlist_entry_get_decoder, int, playlist, int,
 entry, bool_t, fast)

/* Returns the tuple associated with an entry, or NULL if one is not available.
 * The reference count of the tuple is incremented.  If <fast> is nonzero,
 * returns NULL if metadata for the entry has not yet been read from the song
 * file. */
AUD_FUNC3 (Tuple *, playlist_entry_get_tuple, int, playlist, int, entry,
 bool_t, fast)

/* Returns a formatted title string for an entry.  This may include information
 * such as the filename, song title, and/or artist.  If <fast> is nonzero,
 * returns a "best guess" based on the entry's filename if metadata for the
 * entry has not yet been read. */
AUD_FUNC3 (char *, playlist_entry_get_title, int, playlist, int, entry,
 bool_t, fast)

/* Returns three strings (title, artist, and album) describing an entry.  The
 * strings are pooled, and the usual cautions apply.  If <fast> is nonzero,
 * returns a "best guess" based on the entry's filename if metadata for the
 * entry has not yet been read.  The caller may pass NULL for any values that
 * are not needed; NULL may also be returned for any values that are not
 * available. */
AUD_VFUNC6 (playlist_entry_describe, int, playlist, int, entry,
 char * *, title, char * *, artist, char * *, album, bool_t, fast)

/* Returns the length in milliseconds of an entry, or -1 if the length is not
 * known.  <fast> is as in playlist_entry_get_tuple(). */
AUD_FUNC3 (int, playlist_entry_get_length, int, playlist, int, entry,
 bool_t, fast)

/* Sets the entry which will be played (if this playlist is selected with
 * playlist_set_playing()) when drct_play() is called.  If a song from this
 * playlist is already playing, it will be stopped.  If the playlist has resume
 * information set, it will be cleared.  It is possible to set a position of -1,
 * meaning that no entry is set to be played. */
AUD_VFUNC2 (playlist_set_position, int, playlist, int, position)

/* Returns the number of the entry set to be played (which may or may not be
 * currently playing), or -1 if there is none. */
AUD_FUNC1 (int, playlist_get_position, int, playlist)

/* Sets whether an entry is selected. */
AUD_VFUNC3 (playlist_entry_set_selected, int, playlist, int, entry,
 bool_t, selected)

/* Returns whether an entry is selected. */
AUD_FUNC2 (bool_t, playlist_entry_get_selected, int, playlist, int, entry)

/* Returns the number of selected entries in a playlist. */
AUD_FUNC1 (int, playlist_selected_count, int, playlist)

/* Selects all (or none) of the entries in a playlist. */
AUD_VFUNC2 (playlist_select_all, int, playlist, bool_t, selected)

/* Moves a selected entry within a playlist by an offset of <distance> entries.
 * Other selected entries are gathered around it.  Returns the offset by which
 * the entry was actually moved, which may be less in absolute value than the
 * requested offset. */
AUD_FUNC3 (int, playlist_shift, int, playlist, int, position, int, distance)

/* Removes the selected entries from a playlist.  If the currently playing entry
 * is among these, playback is stopped. */
AUD_VFUNC1 (playlist_delete_selected, int, playlist)

/* Sorts the entries in a playlist based on filename.  The callback function
 * should return negative if the first filename comes before the second,
 * positive if it comes after, or zero if the two are indistinguishable. */
AUD_VFUNC2 (playlist_sort_by_filename, int, playlist,
 PlaylistStringCompareFunc, compare)

/* Sorts the entries in a playlist based on tuple.  May fail if metadata
 * scanning is still in progress (or has been disabled). */
AUD_VFUNC2 (playlist_sort_by_tuple, int, playlist,
 PlaylistTupleCompareFunc, compare)

/* Sorts the entries in a playlist based on formatted title string.  May fail if
 * metadata scanning is still in progress (or has been disabled). */
AUD_VFUNC2 (playlist_sort_by_title, int, playlist,
 PlaylistStringCompareFunc, compare)

/* Sorts only the selected entries in a playlist based on filename. */
AUD_VFUNC2 (playlist_sort_selected_by_filename, int, playlist,
 PlaylistStringCompareFunc, compare)

/* Sorts only the selected entries in a playlist based on tuple.  May fail if
 * metadata scanning is still in progress (or has been disabled). */
AUD_VFUNC2 (playlist_sort_selected_by_tuple, int, playlist,
 PlaylistTupleCompareFunc, compare)

/* Sorts only the selected entries in a playlist based on formatted title
 * string.  May fail if metadata scanning is still in progress (or has been
 * disabled). */
AUD_VFUNC2 (playlist_sort_selected_by_title, int, playlist,
 PlaylistStringCompareFunc, compare)

/* Reverses the order of the entries in a playlist. */
AUD_VFUNC1 (playlist_reverse, int, playlist)

/* Reorders the entries in a playlist randomly. */
AUD_VFUNC1 (playlist_randomize, int, playlist)

/* Discards the metadata stored for all the entries in a playlist and starts
 * reading it afresh from the song files in the background. */
AUD_VFUNC1 (playlist_rescan, int, playlist)

/* Like playlist_rescan, but applies only to the selected entries in a playlist. */
AUD_VFUNC1 (playlist_rescan_selected, int, playlist)

/* Discards the metadata stored for all the entries that refer to a particular
 * song file, in whatever playlist they appear, and starts reading it afresh
 * from that file in the background. */
AUD_VFUNC1 (playlist_rescan_file, const char *, filename)

/* Calculates the total length in milliseconds of all the entries in a playlist.
 * Only takes into account entries for which metadata has already been read. */
AUD_FUNC1 (int64_t, playlist_get_total_length, int, playlist)

/* Calculates the total length in milliseconds of only the selected entries in a
 * playlist.  Only takes into account entries for which metadata has already
 * been read. */
AUD_FUNC1 (int64_t, playlist_get_selected_length, int, playlist)

/* Returns the number of entries in a playlist queue.  The entries are numbered
 * starting from zero, lower numbers being played first. */
AUD_FUNC1 (int, playlist_queue_count, int, playlist)

/* Adds an entry to a playlist's queue before the entry numbered <at> in the
 * queue.  If <at> is negative or equal to the number of entries in the queue,
 * adds the entry after the last one in the queue.  The same entry cannot be
 * added to the queue more than once. */
AUD_VFUNC3 (playlist_queue_insert, int, playlist, int, at, int, entry)

/* Adds the selected entries in a playlist to the queue, if they are not already
 * in it. */
AUD_VFUNC2 (playlist_queue_insert_selected, int, playlist, int, at)

/* Returns the position in the playlist of the entry at a given position in the
 * queue. */
AUD_FUNC2 (int, playlist_queue_get_entry, int, playlist, int, at)

/* Returns the position in the queue of the entry at a given position in the
 * playlist.  If it is not in the queue, returns -1. */
AUD_FUNC2 (int, playlist_queue_find_entry, int, playlist, int, entry)

/* Removes a contiguous block of <number> entries starting with the one numbered
 * <at> from the queue. */
AUD_VFUNC3 (playlist_queue_delete, int, playlist, int, at, int, number)

/* Removes the selected entries in a playlist from the queue, if they are in it. */
AUD_VFUNC1 (playlist_queue_delete_selected, int, playlist)

/* Returns nonzero if a "playlist update" hook call is pending.  If called from
 * within the hook, the current hook call is not considered pending. */
AUD_FUNC0 (bool_t, playlist_update_pending)

/* May be called within the "playlist update" hook to determine the update level
 * and number of entries changed in a playlist.  Returns the update level for
 * the playlist, storing the number of the first entry changed in <at> and the
 * number of contiguous entries to be updated in <count>.  Note that entries may
 * have been added or removed within this range.  If no entries in the playlist
 * have changed, returns zero. */
AUD_FUNC3 (int, playlist_updated_range, int, playlist, int *, at, int *, count)

/* Returns nonzero if entries are being added to a playlist in the background. */
AUD_FUNC1 (bool_t, playlist_add_in_progress, int, playlist)

/* Returns nonzero if entries in a playlist are being scanned for metadata in
 * the background. */
AUD_FUNC1 (bool_t, playlist_scan_in_progress, int, playlist)

/* --- PLAYLIST UTILITY API --- */

/* Sorts the entries in a playlist according to one of the schemes listed in
 * playlist.h. */
AUD_VFUNC2 (playlist_sort_by_scheme, int, playlist, int, scheme)

/* Sorts only the selected entries in a playlist according to one of those
 * schemes. */
AUD_VFUNC2 (playlist_sort_selected_by_scheme, int, playlist, int, scheme)

/* Removes duplicate entries in a playlist according to one of those schemes.
 * As currently implemented, first sorts the playlist. */
AUD_VFUNC2 (playlist_remove_duplicates_by_scheme, int, playlist, int,
 scheme)

/* Removes all entries referring to unavailable files in a playlist.  ("Remove
 * failed" is something of a misnomer for the current behavior.)  As currently
 * implemented, only works for file:// URIs. */
AUD_VFUNC1 (playlist_remove_failed, int, playlist)

/* Selects all the entries in a playlist that match regular expressions stored
 * in the fields of a tuple.  Does not free the memory used by the tuple.
 * Example: To select all the songs whose title starts with the letter "A",
 * create a blank tuple and set its title field to "^A". */
AUD_VFUNC2 (playlist_select_by_patterns, int, playlist, const Tuple *,
 patterns)

/* Returns nonzero if <filename> refers to a playlist file. */
AUD_FUNC1 (bool_t, filename_is_playlist, const char *, filename)

/* Saves the entries in a playlist to a playlist file.  The format of the file
 * is determined from the file extension.  Returns nonzero on success. */
AUD_FUNC2 (bool_t, playlist_save, int, playlist, const char *, filename)
