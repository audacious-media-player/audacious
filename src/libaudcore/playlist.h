/*
 * playlist.h
 * Copyright 2010-2017 John Lindgren
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

/*
 * Persistent handle attached to a playlist.
 * Follows the same playlist even if playlists are reordered.
 * Does not prevent the playlist from being deleted (check exists()).
 */
class Playlist
{
public:
    /* --- TYPES --- */

    /* Opaque type which uniquely identifies a playlist */
    struct ID;

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

    /* Preset sorting "schemes" */
    enum SortType {
        Path,            // entry's entire URI
        Filename,        // base name (no folder path)
        Title,
        Album,
        Artist,
        AlbumArtist,
        Date,            // release date (not modification time)
        Genre,
        Track,
        FormattedTitle,
        Length,
        Comment,
        n_sort_types
    };

    /* Possible behaviors for entry_{decoder, tuple}. */
    enum GetMode {
        NoWait,  // non-blocking call; returned tuple will be in Initial state if not yet scanned
        Wait     // blocking call; returned tuple will be either Valid or Failed
    };

    /* Format descriptor returned by save_formats() */
    struct SaveFormat {
        String name;         // human-readable format name
        Index<String> exts;  // supported filename extensions
    };

    typedef bool (* FilterFunc) (const char * filename, void * user);
    typedef int (* StringCompareFunc) (const char * a, const char * b);
    typedef int (* TupleCompareFunc) (const Tuple & a, const Tuple & b);

    /* --- CONSTRUCTOR ETC. --- */

    /* Default constructor; indicates "no playlist" */
    constexpr Playlist () : m_id (nullptr) {}

    bool operator== (const Playlist & b) const { return m_id == b.m_id; }
    bool operator!= (const Playlist & b) const { return m_id != b.m_id; }

    /* The number of the playlist in display order, starting from 0.
     * Returns -1 if the playlist no longer exists. */
    int index () const;

    /* True if the playlist exists (i.e. has not been deleted). */
    bool exists () const { return index () >= 0; }

    /* --- CORE (STATIC) API --- */

    /* Returns the number of playlists currently open (>= 1). */
    static int n_playlists ();

    /* Looks up a playlist by display order. */
    static Playlist by_index (int at);

    /* Adds a new playlist before the one numbered <at> (-1 = insert at end). */
    static Playlist insert_playlist (int at);

    /* Moves a contiguous block of <count> playlists starting with the one
     * numbered <from> such that that playlist ends up at the position <to>. */
    static void reorder_playlists (int from, int to, int count);

    /* Returns the active (i.e. displayed) playlist. */
    static Playlist active_playlist ();

    /* Convenience function which adds a new playlist after the active one and
     * then sets the new one as active.  Returns the new playlist. */
    static Playlist new_playlist ();

    /* Returns the currently playing playlist.  If no playlist is playing,
     * returns Playlist(). */
    static Playlist playing_playlist ();

    /* Returns the number of a "blank" playlist.  The active playlist is
     * returned if it has the default title and has no entries; otherwise, a new
     * playlist is added and returned. */
    static Playlist blank_playlist ();

    /* Returns the number of the "temporary" playlist (which is no different
     * from any other playlist except in name).  If the playlist does not exist,
     * a "blank" playlist is renamed to become the temporary playlist. */
    static Playlist temporary_playlist ();

    /* Discards the metadata stored for all the entries that refer to a
     * particular song file, in whatever playlist they appear, and starts
     * reading it afresh from that file in the background. */
    static void rescan_file (const char * filename);

    /* --- CORE (NON-STATIC) API --- */

    /* Gets/sets the filename associated with this playlist.
     * (Audacious currently makes no use of the filename.) */
    String get_filename () const;
    void set_filename (const char * filename) const;

    /* Gets/sets the title of the playlist. */
    String get_title () const;
    void set_title (const char * title) const;

    /* Closes the playlist.
     * The playlist is not saved, and no confirmation is presented to the user.
     * When the last playlist is closed, a new one is added in its place.
     * When the active playlist is closed, another is made active.
     * When the playing playlist is closed, playback stops. */
    void remove_playlist () const;

    /* Makes this the active (i.e. displayed) playlist. */
    void activate () const;

    /* Starts playback of this playlist, unless it is empty.
     * Resumes from the position last played if possible.
     * If <paused> is true, starts playback in a paused state. */
    void start_playback (bool paused = false) const;

    /* Returns the number of entries (numbered from 0). */
    int n_entries () const;

    /* Adds a single song file, playlist file, or folder before the entry <at>.
     * If <at> is negative or equal to the number of entries, the item is added
     * after the last entry.  <tuple> may be null, in which case Audacious will
     * attempt to read metadata from the song file.  If <play> is true,
     * Audacious will begin playback of the items once they have been added.
     *
     * This function is asynchronous (the items are added in the background). */
    void insert_entry (int at, const char * filename, Tuple && tuple, bool play) const;

    /* Adds multiple song files, playlist files, or folders to a playlist. */
    void insert_items (int at, Index<PlaylistAddItem> && items, bool play) const;

    /* Similar to insert_items() but allows the caller to prevent some items
     * from being added by returning false from the <filter> callback.  Useful
     * for searching a folder and adding only new files to the playlist.  <user>
     * is an opaque pointer passed to the callback. */
    void insert_filtered (int at, Index<PlaylistAddItem> && items,
     FilterFunc filter, void * user, bool play) const;

    /* Removes entries from the playlist.  The playback position may be moved,
     * or playback may be stopped (according to user preference). */
    void remove_entries (int at, int number) const;
    void remove_entry (int at) const { remove_entries (at, 1); }
    void remove_all_entries () const { remove_entries (0, -1); }

    /* Returns an entry's filename. */
    String entry_filename (int entry) const;

    /* Returns an entry's decoder plugin.  On error, or if the entry has not yet
     * been scanned, returns nullptr according to <mode>.  An optional error
     * message may be returned. */
    PluginHandle * entry_decoder (int entry, GetMode mode = Wait, String * error = nullptr) const;

    /* Returns an entry's metadata.  The state of the returned tuple may
     * indicate that the entry has not yet been scanned, or an error occurred,
     * according to <mode>.  An optional error message may be returned. */
    Tuple entry_tuple (int entry, GetMode mode = Wait, String * error = nullptr) const;

    /* Gets/sets the playing or last-played entry (-1 = no entry).
     * Affects playback only if this playlist is currently playing.
     * set_position(get_position()) restarts playback from 0:00.
     * set_position(-1) stops playback. */
    int get_position () const;
    void set_position (int position) const;

    /* Advances the playlist position to the next entry in playback order,
     * taking current shuffle settings into account.  At the end of the
     * playlist, wraps around to the beginning if <repeat> is true.  Returns
     * true on success, false if playlist position was not changed. */
    bool next_song (bool repeat) const;

    /* Returns the playlist position to the previous entry in playback order.
     * Does not support wrapping past the beginning of the playlist.  Returns
     * true on success, false if playlist position was not changed. */
    bool prev_song () const;

    /* Gets/sets the entry which has keyboard focus (-1 = no entry). */
    int get_focus () const;
    void set_focus (int entry) const;

    /* Gets/sets whether an entry is selected. */
    bool entry_selected (int entry) const;
    void select_entry (int entry, bool selected) const;

    /* Returns the number of selected entries.
     * An optional range of entries to examine may be specified. */
    int n_selected (int at = 0, int number = -1) const;

    /* Selects all (or none) of the entries in a playlist. */
    void select_all (bool selected) const;

    /* Moves a selected entry within a playlist by an offset of <distance>
     * entries.  Other selected entries are gathered around it.  Returns the
     * offset by which the entry was actually moved (which may be less than the
     * requested offset. */
    int shift_entries (int position, int distance) const;

    /* Removes all selected entries. */
    void remove_selected () const;

    /* Sorts the entries in a playlist based on filename.  The callback function
     * should return negative if the first filename comes before the second,
     * positive if it comes after, or zero if the two are indistinguishable. */
    void sort_by_filename (StringCompareFunc compare) const;

    /* Sorts the entries in a playlist based on tuple.  May fail if metadata
     * scanning is still in progress (or has been disabled). */
    void sort_by_tuple (TupleCompareFunc compare) const;

    /* Sorts the entries in a playlist based on formatted title string.  May fail if
     * metadata scanning is still in progress (or has been disabled). */
    void sort_by_title (StringCompareFunc compare) const;

    /* Sorts only the selected entries in a playlist based on filename. */
    void sort_selected_by_filename (StringCompareFunc compare) const;

    /* Sorts only the selected entries in a playlist based on tuple.  May fail if
     * metadata scanning is still in progress (or has been disabled). */
    void sort_selected_by_tuple (TupleCompareFunc compare) const;

    /* Sorts only the selected entries in a playlist based on formatted title
     * string.  May fail if metadata scanning is still in progress (or has been
     * disabled). */
    void sort_selected_by_title (StringCompareFunc compare) const;

    /* Reverses the order of the entries in a playlist. */
    void reverse_order () const;

    /* Reorders the entries in a playlist randomly. */
    void randomize_order () const;

    /* Reverses the order of the selected entries in a playlist. */
    void reverse_selected () const;

    /* Reorders the selected entries in a playlist randomly. */
    void randomize_selected () const;

    /* Discards the metadata stored for entries in a playlist and starts reading
     * it fresh from the song files in the background. */
    void rescan_all () const;
    void rescan_selected () const;

    /* Calculates the length in milliseconds of entries in a playlist.  Only
     * takes into account entries for which metadata has already been read. */
    int64_t total_length_ms () const;
    int64_t selected_length_ms () const;

    /* Returns the number of entries in a playlist queue. */
    int n_queued () const;

    /* Adds an entry to the queue at <pos> (-1 = at end of queue).
     * The same entry cannot be added to the queue more than once. */
    void queue_insert (int pos, int entry) const;
    void queue_insert_selected (int pos) const;

    /* Returns the entry at the given queue position. */
    int queue_get_entry (int pos) const;

    /* Returns the queue position of the given entry (-1 if not queued). */
    int queue_find_entry (int entry) const;

    /* Removes entries from the queue. */
    void queue_remove (int pos, int number = 1) const;
    void queue_remove_all () const { queue_remove (0, -1); }

    /* Removes the selected entries in a playlist from the queue, if they are in it. */
    void queue_remove_selected () const;

    /* Returns true if a "playlist update" hook call is pending.
     * A running hook call is not considered pending. */
    bool update_pending () const;
    static bool update_pending_any ();

    /* May be called within the "playlist update" hook to determine the update
     * level and number of entries changed in a playlist. */
    Update update_detail () const;

    /* Returns true if entries are being added in the background. */
    bool add_in_progress () const;
    static bool add_in_progress_any ();

    /* Returns true if entries are being scanned in the background. */
    bool scan_in_progress () const;
    static bool scan_in_progress_any ();

    /* --- UTILITY API --- */

    /* Sorts entries according to a preset scheme. */
    void sort_entries (SortType scheme) const;
    void sort_selected (SortType scheme) const;

    /* Removes duplicate entries according to a preset scheme.
     * The current implementation also sorts the playlist. */
    void remove_duplicates (SortType scheme) const;

    /* Removes all entries referring to inaccessible files in a playlist. */
    void remove_unavailable () const;

    /* Selects entries by matching regular expressions.
     * Example: To select all titles starting with the letter "A",
     * create a blank tuple and set its title field to "^A". */
    void select_by_patterns (const Tuple & patterns) const;

    /* Saves metadata for the selected entries to an internal cache.
     * This will speed up adding those entries to another playlist. */
    void cache_selected () const;

    /* Saves the entries in a playlist to a playlist file.
     * The format of the file is determined from the file extension.
     * <mode> specifies whether to wait for metadata scanning to complete.
     * Returns true on success. */
    bool save_to_file (const char * filename, GetMode mode) const;

    /* Checks a filename for an extension matching a known playlist format. */
    static bool filename_is_playlist (const char * filename);

    /* Generates a list of the currently supported formats for saving playlists.
     * The list should not be cached since it may change as plugins are enabled or
     * disabled. */
    static Index<SaveFormat> save_formats ();

    /* --- IMPLEMENTATION --- */

private:
    ID * m_id;

    explicit constexpr Playlist (ID * id) :
        m_id (id) {}

    friend class PlaylistEx;
};

#endif
