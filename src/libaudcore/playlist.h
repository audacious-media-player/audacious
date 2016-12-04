/*
 * playlist.h
 * Copyright 2010-2013 John Lindgren
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

#ifndef LIBAUDCORE_PLAYLIST_H
#define LIBAUDCORE_PLAYLIST_H

#include <stdint.h>

#include <libaudcore/index.h>
#include <libaudcore/tuple.h>

namespace Playlist {

/* The values which can be passed to the "playlist update" hook.  Selection
 * means that entries have been selected or unselected, or that entries have
 * been added to or removed from the queue.  Metadata means that new metadata
 * has been read for some entries, or that the title or filename of a playlist
 * has changed, and implies Selection.  Structure covers any other change, and
 * implies both Selection and Metadata. */
enum UpdateLevel {
    NoUpdate = 0,
    Selection,
    Metadata,
    Structure
};

struct Update {
    UpdateLevel level;   // type of update
    int before;          // number of unaffected entries at playlist start
    int after;           // number of unaffected entries at playlist end
    bool queue_changed;  // true if entries have been added to/removed from queue
};

/* The values which can be passed to playlist_sort_by_scheme(),
 * playlist_sort_selected_by_scheme(), and
 * playlist_remove_duplicates_by_scheme().  PlaylistSort::Path means the entire
 * URI of a song file; PlaylistSort::Filename means the portion after the last
 * "/" (forward slash).  PlaylistSort::Date means the song's release date (not
 * the file's modification time). */
enum SortType {
    Path,
    Filename,
    Title,
    Album,
    Artist,
    AlbumArtist,
    Date,
    Genre,
    Track,
    FormattedTitle,
    Length,
    Comment,
    n_sort_types
};

/* Possible behaviors for playlist_entry_get_{decoder, tuple}. */
enum GetMode {
    NoWait,  // non-blocking call; returned tuple will be in Initial state if not yet scanned
    Wait     // blocking call; returned tuple will be either Valid or Failed
};

/* Format descriptor returned by playlist_save_formats() */
struct SaveFormat {
    String name;         // human-readable format name
    Index<String> exts;  // supported filename extensions
};

} // namespace Playlist

typedef bool (* PlaylistFilterFunc) (const char * filename, void * user);
typedef int (* PlaylistStringCompareFunc) (const char * a, const char * b);
typedef int (* PlaylistTupleCompareFunc) (const Tuple & a, const Tuple & b);

/* --- PLAYLIST CORE API --- */

/* Returns the number of playlists currently open.  There will always be at
 * least one playlist open.  The playlists are numbered starting from zero. */
int aud_playlist_count ();

/* Adds a new playlist before the one numbered <at>.  If <at> is -1 or equal to
 * the number of playlists, adds a new playlist after the last one. */
void aud_playlist_insert (int at);

/* Moves a contiguous block of <count> playlists starting with the one numbered
 * <from> such that that playlist ends up at the position <to>. */
void aud_playlist_reorder (int from, int to, int count);

/* Closes a playlist.  CAUTION: The playlist is not saved, and no confirmation
 * is presented to the user.  If <playlist> is the only playlist, a new playlist
 * is added.  If <playlist> is the active playlist, another playlist is marked
 * active.  If <playlist> is the currently playing playlist, playback is
 * stopped. */
void aud_playlist_delete (int playlist);

/* Returns a unique non-negative integer which can be used to identify a given
 * playlist even if its numbering changes (as when playlists are reordered).
 * On error, returns -1. */
int aud_playlist_get_unique_id (int playlist);

/* Returns the number of the playlist identified by a given integer ID as
 * returned by playlist_get_unique_id().  If the playlist no longer exists,
 * returns -1. */
int aud_playlist_by_unique_id (int id);

/* Sets the filename associated with a playlist.  (Audacious currently makes no
 * use of the filename.) */
void aud_playlist_set_filename (int playlist, const char * filename);

/* Returns the filename associated with a playlist. */
String aud_playlist_get_filename (int playlist);

/* Sets the title associated with a playlist. */
void aud_playlist_set_title (int playlist, const char * title);

/* Returns the title associated with a playlist. */
String aud_playlist_get_title (int playlist);

/* Sets the active playlist.  This is the playlist that user interfaces will
 * show to the user. */
void aud_playlist_set_active (int playlist);

/* Returns the number of the active playlist. */
int aud_playlist_get_active ();

/* Convenience function which adds a new playlist after the active one and then
 * sets the new one as active.  Returns the number of the new playlist. */
int aud_playlist_new ();

/* Starts playback of a playlist, resuming from the position last played if
 * possible.  If <playlist> is -1 or if the requested playlist is empty, stops
 * playback.  If <paused> is true, starts playback in a paused state. */
void aud_playlist_play (int playlist, bool paused = false);

/* Returns the number of the currently playing playlist.  If no playlist is
 * playing, returns -1. */
int aud_playlist_get_playing ();

/* Returns the number of a "blank" playlist.  The active playlist is returned if
 * it has the default title and has no entries; otherwise, a new playlist is
 * added and returned. */
int aud_playlist_get_blank ();

/* Returns the number of the "temporary" playlist (which is no different from
 * any other playlist except in name).  If the playlist does not exist, a
 * "blank" playlist is obtained from playlist_get_blank() and is renamed to
 * become the temporary playlist. */
int aud_playlist_get_temporary ();

/* Returns the number of entries in a playlist.  The entries are numbered
 * starting from zero. */
int aud_playlist_entry_count (int playlist);

/* Adds a song file, playlist file, or folder to a playlist before the entry
 * numbered <at>.  If <at> is negative or equal to the number of entries, the
 * item is added after the last entry.  <tuple> may be nullptr, in which case
 * Audacious will attempt to read metadata from the song file.  If <play> is
 * true, Audacious will begin playback of the items once they have been
 * added.
 *
 * Because adding items to the playlist can be a slow process, this function may
 * return before the process is complete.  Hence, the caller must not assume
 * that there will be new entries in the playlist immediately. */
void aud_playlist_entry_insert (int playlist, int at, const char * filename,
 Tuple && tuple, bool play);

/* Similar to playlist_entry_insert, adds multiple song files, playlist files,
 * or folders to a playlist. */
void aud_playlist_entry_insert_batch (int playlist, int at,
 Index<PlaylistAddItem> && items, bool play);

/* Similar to playlist_entry_insert_batch, but allows the caller to prevent some
 * items from being added by returning false from the <filter> callback.  Useful
 * for searching a folder and adding only new files to the playlist.  <user> is
 * an additional, untyped pointer passed to the callback. */
void aud_playlist_entry_insert_filtered (int playlist, int at,
 Index<PlaylistAddItem> && items, PlaylistFilterFunc filter, void * user,
 bool play);

/* Removes entries (at) to (at + number - 1) from the playlist.  If <number> is
 * negative, removes entries up to the end of the playlist.  If necessary, the
 * playback position is moved elsewhere in the playlist and playback is
 * restarted (or stopped). */
void aud_playlist_entry_delete (int playlist, int at, int number);

/* Returns the filename of an entry. */
String aud_playlist_entry_get_filename (int playlist, int entry);

/* Returns a handle to the decoder plugin associated with an entry.  On error,
 * or if the entry has not yet been scanned, returns nullptr according to
 * <mode>.  On error, an error message is optionally returned. */
PluginHandle * aud_playlist_entry_get_decoder (int playlist, int entry,
 Playlist::GetMode mode = Playlist::Wait, String * error = nullptr);

/* Returns the tuple associated with an entry.  The state of the returned tuple
 * may indicate that the entry has not yet been scanned, or an error occurred,
 * according to <mode>.  On error, an error message is optionally returned. */
Tuple aud_playlist_entry_get_tuple (int playlist, int entry,
 Playlist::GetMode mode = Playlist::Wait, String * error = nullptr);

/* Moves the playback position to the beginning of the entry at <position>.  If
 * <position> is -1, unsets the playback position.  If <playlist> is the
 * currently playing playlist, playback is restarted (or stopped). */
void aud_playlist_set_position (int playlist, int position);

/* Returns the playback position, or -1 if it is not set.  Note that the
 * position may be set even if <playlist> is not currently playing. */
int aud_playlist_get_position (int playlist);

/* Sets the entry which has keyboard focus (-1 means no entry). */
void aud_playlist_set_focus (int playlist, int entry);

/* Gets the entry which has keyboard focus (-1 means no entry). */
int aud_playlist_get_focus (int playlist);

/* Sets whether an entry is selected. */
void aud_playlist_entry_set_selected (int playlist, int entry, bool selected);

/* Returns whether an entry is selected. */
bool aud_playlist_entry_get_selected (int playlist, int entry);

/* Returns the number of selected entries in a playlist. */
int aud_playlist_selected_count (int playlist);

/* Counts selected entries between (at) and (at + number - 1), inclusive.  If
 * <number> is negative, counts entries up to the end of the playlist. */
int aud_playlist_selected_count (int playlist, int at, int number);

/* Selects all (or none) of the entries in a playlist. */
void aud_playlist_select_all (int playlist, bool selected);

/* Moves a selected entry within a playlist by an offset of <distance> entries.
 * Other selected entries are gathered around it.  Returns the offset by which
 * the entry was actually moved, which may be less in absolute value than the
 * requested offset. */
int aud_playlist_shift (int playlist, int position, int distance);

/* Removes the selected entries from a playlist.  If necessary, the playback
 * position is moved elsewhere in the playlist and playback is restarted (or
 * stopped). */
void aud_playlist_delete_selected (int playlist);

/* Sorts the entries in a playlist based on filename.  The callback function
 * should return negative if the first filename comes before the second,
 * positive if it comes after, or zero if the two are indistinguishable. */
void aud_playlist_sort_by_filename (int playlist, PlaylistStringCompareFunc compare);

/* Sorts the entries in a playlist based on tuple.  May fail if metadata
 * scanning is still in progress (or has been disabled). */
void aud_playlist_sort_by_tuple (int playlist, PlaylistTupleCompareFunc compare);

/* Sorts the entries in a playlist based on formatted title string.  May fail if
 * metadata scanning is still in progress (or has been disabled). */
void aud_playlist_sort_by_title (int playlist, PlaylistStringCompareFunc compare);

/* Sorts only the selected entries in a playlist based on filename. */
void aud_playlist_sort_selected_by_filename (int playlist, PlaylistStringCompareFunc compare);

/* Sorts only the selected entries in a playlist based on tuple.  May fail if
 * metadata scanning is still in progress (or has been disabled). */
void aud_playlist_sort_selected_by_tuple (int playlist, PlaylistTupleCompareFunc compare);

/* Sorts only the selected entries in a playlist based on formatted title
 * string.  May fail if metadata scanning is still in progress (or has been
 * disabled). */
void aud_playlist_sort_selected_by_title (int playlist, PlaylistStringCompareFunc compare);

/* Reverses the order of the entries in a playlist. */
void aud_playlist_reverse (int playlist);

/* Reorders the entries in a playlist randomly. */
void aud_playlist_randomize (int playlist);

/* Reverses the order of the selected entries in a playlist. */
void aud_playlist_reverse_selected (int playlist);

/* Reorders the selected entries in a playlist randomly. */
void aud_playlist_randomize_selected (int playlist);

/* Discards the metadata stored for all the entries in a playlist and starts
 * reading it afresh from the song files in the background. */
void aud_playlist_rescan (int playlist);

/* Like playlist_rescan, but applies only to the selected entries in a playlist. */
void aud_playlist_rescan_selected (int playlist);

/* Discards the metadata stored for all the entries that refer to a particular
 * song file, in whatever playlist they appear, and starts reading it afresh
 * from that file in the background. */
void aud_playlist_rescan_file (const char * filename);

/* Calculates the total length in milliseconds of all the entries in a playlist.
 * Only takes into account entries for which metadata has already been read. */
int64_t aud_playlist_get_total_length (int playlist);

/* Calculates the total length in milliseconds of only the selected entries in a
 * playlist.  Only takes into account entries for which metadata has already
 * been read. */
int64_t aud_playlist_get_selected_length (int playlist);

/* Returns the number of entries in a playlist queue.  The entries are numbered
 * starting from zero, lower numbers being played first. */
int aud_playlist_queue_count (int playlist);

/* Adds an entry to a playlist's queue before the entry numbered <at> in the
 * queue.  If <at> is negative or equal to the number of entries in the queue,
 * adds the entry after the last one in the queue.  The same entry cannot be
 * added to the queue more than once. */
void aud_playlist_queue_insert (int playlist, int at, int entry);

/* Adds the selected entries in a playlist to the queue, if they are not already
 * in it. */
void aud_playlist_queue_insert_selected (int playlist, int at);

/* Returns the position in the playlist of the entry at a given position in the
 * queue. */
int aud_playlist_queue_get_entry (int playlist, int at);

/* Returns the position in the queue of the entry at a given position in the
 * playlist.  If it is not in the queue, returns -1. */
int aud_playlist_queue_find_entry (int playlist, int entry);

/* Removes a contiguous block of <number> entries starting with the one numbered
 * <at> from the queue. */
void aud_playlist_queue_delete (int playlist, int at, int number);

/* Removes the selected entries in a playlist from the queue, if they are in it. */
void aud_playlist_queue_delete_selected (int playlist);

/* Returns true if a "playlist update" hook call is pending for the given
 * playlist (or for any playlist, if <playlist> is -1).  If called from within
 * the hook, the current hook call is not considered pending. */
bool aud_playlist_update_pending (int playlist = -1);

/* May be called within the "playlist update" hook to determine the update level
 * and number of entries changed in a playlist. */
Playlist::Update aud_playlist_update_detail (int playlist);

/* Returns true if entries are being added to a playlist in the background.
 * If <playlist> is -1, checks all playlists. */
bool aud_playlist_add_in_progress (int playlist);

/* Returns true if entries in a playlist are being scanned for metadata in
 * the background.  If <playlist> is -1, checks all playlists. */
bool aud_playlist_scan_in_progress (int playlist);

/* --- PLAYLIST UTILITY API --- */

/* Sorts the entries in a playlist according to one of the schemes listed in
 * playlist.h. */
void aud_playlist_sort_by_scheme (int playlist, Playlist::SortType scheme);

/* Sorts only the selected entries in a playlist according to one of those
 * schemes. */
void aud_playlist_sort_selected_by_scheme (int playlist, Playlist::SortType scheme);

/* Removes duplicate entries in a playlist according to one of those schemes.
 * As currently implemented, first sorts the playlist. */
void aud_playlist_remove_duplicates_by_scheme (int playlist, Playlist::SortType scheme);

/* Removes all entries referring to unavailable files in a playlist.  ("Remove
 * failed" is something of a misnomer for the current behavior.) */
void aud_playlist_remove_failed (int playlist);

/* Selects all the entries in a playlist that match regular expressions stored
 * in the fields of a tuple.  Does not free the memory used by the tuple.
 * Example: To select all the songs whose title starts with the letter "A",
 * create a blank tuple and set its title field to "^A". */
void aud_playlist_select_by_patterns (int playlist, const Tuple & patterns);

/* Saves metadata for the selected entries in a playlist to an internal cache,
 * which is used to speed up adding these entries to another playlist. */
void aud_playlist_cache_selected (int playlist);

/* Returns true if <filename> refers to a playlist file. */
bool aud_filename_is_playlist (const char * filename);

/* Saves the entries in a playlist to a playlist file.  The format of the file
 * is determined from the file extension.  Returns true on success. */
bool aud_playlist_save (int playlist, const char * filename, Playlist::GetMode mode);

/* Generates a list of the currently supported formats for saving playlists.
 * The list should not be cached since it may change as plugins are enabled or
 * disabled. */
Index<Playlist::SaveFormat> aud_playlist_save_formats ();

#endif
