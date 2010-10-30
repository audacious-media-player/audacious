/*
 * playlist.h
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

#ifndef AUDACIOUS_PLAYLIST_H
#define AUDACIOUS_PLAYLIST_H

#include <glib.h>
#include <audacious/api.h>
#include <audacious/types.h>
#include <libaudcore/index.h>
#include <libaudcore/tuple.h>

/* The values which can be passed (packed into a pointer) to the "playlist
 * update" hook.  PLAYLIST_UPDATE_SELECTION means that entries have been
 * selected or unselected, entries have been added or removed from the queue,
 * or that the current song has changed.  PLAYLIST_UPDATE_METADATA means that
 * new metadata has been read for some entries, and implies
 * PLAYLIST_UPDATE_SELECTION.  PLAYLIST_UPDATE_STRUCTURE covers any change not
 * listed under the other types, and implies both PLAYLIST_UPDATE_SELECTION and
 * PLAYLIST_UPDATE_METADATA. */
enum {
 PLAYLIST_UPDATE_SELECTION,
 PLAYLIST_UPDATE_METADATA,
 PLAYLIST_UPDATE_STRUCTURE,
 PLAYLIST_UPDATE_TYPES};

/* The values which can be passed to playlist_sort_by_scheme(),
 * playlist_sort_selected_by_scheme(), and
 * playlist_remove_duplicates_by_scheme().  PLAYLIST_SORT_PATH means the entire
 * URI of a song file; PLAYLIST_SORT_FILENAME means the portion after the last
 * "/" (forward slash).  PLAYLIST_SORT_DATE means the song's release date (not
 * the file's modification time). */
enum {
 PLAYLIST_SORT_PATH,
 PLAYLIST_SORT_FILENAME,
 PLAYLIST_SORT_TITLE,
 PLAYLIST_SORT_ALBUM,
 PLAYLIST_SORT_ARTIST,
 PLAYLIST_SORT_DATE,
 PLAYLIST_SORT_TRACK,
 PLAYLIST_SORT_FORMATTED_TITLE,
 PLAYLIST_SORT_SCHEMES};

#define PlaylistFilenameCompareFunc PlaylistStringCompareFunc /* deprecated */
typedef gint (* PlaylistStringCompareFunc) (const gchar * a, const gchar * b);
typedef gint (* PlaylistTupleCompareFunc) (const Tuple * a, const Tuple * b);

#define AUD_API_NAME PlaylistAPI
#define AUD_API_SYMBOL playlist_api

#ifdef _AUDACIOUS_CORE

#include "api-local-begin.h"
#include "playlist-api.h"
#include "api-local-end.h"

/* playlist-new.c */
void playlist_init (void);
void playlist_end (void);
void playlist_load_state (void);
void playlist_save_state (void);

void playlist_reformat_titles (void);

void playlist_entry_insert_batch_with_decoders (gint playlist, gint at,
 struct index * filenames, struct index * decoders, struct index * tuples);
void playlist_entry_set_tuple (gint playlist, gint entry, Tuple * tuple);

gboolean playlist_entry_is_segmented (gint playlist, gint entry);
gint playlist_entry_get_start_time (gint playlist, gint entry);
gint playlist_entry_get_end_time (gint playlist, gint entry);

gboolean playlist_prev_song (gint playlist);
gboolean playlist_next_song (gint playlist, gboolean repeat);

/* playlist-utils.c */
void save_playlists (void);
void load_playlists (void);

#else

#include <audacious/api-define-begin.h>
#include <audacious/playlist-api.h>
#include <audacious/api-define-end.h>

#include <audacious/api-alias-begin.h>
#include <audacious/playlist-api.h>
#include <audacious/api-alias-end.h>

#endif

#undef AUD_API_NAME
#undef AUD_API_SYMBOL

#endif

#ifdef AUD_API_DECLARE

#define AUD_API_NAME PlaylistAPI
#define AUD_API_SYMBOL playlist_api

#include "api-define-begin.h"
#include "playlist-api.h"
#include "api-define-end.h"

#include "api-declare-begin.h"
#include "playlist-api.h"
#include "api-declare-end.h"

#undef AUD_API_NAME
#undef AUD_API_SYMBOL

#endif
