/*
 * playlist.h
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

#ifndef AUDACIOUS_PLAYLIST_H
#define AUDACIOUS_PLAYLIST_H

#include <stdint.h>

#include <audacious/api.h>
#include <audacious/types.h>
#include <libaudcore/index.h>
#include <libaudcore/tuple.h>

/* The values which can be passed (packed into a pointer) to the "playlist
 * update" hook.  PLAYLIST_UPDATE_SELECTION means that entries have been
 * selected or unselected, or that entries have been added to or removed from
 * the queue.  PLAYLIST_UPDATE_METADATA means that new metadata has been read
 * for some entries, or that the title or filename of a playlist has changed,
 * and implies PLAYLIST_UPDATE_SELECTION.  PLAYLIST_UPDATE_STRUCTURE covers any
 * change not listed under the other types, and implies both
 * PLAYLIST_UPDATE_SELECTION and PLAYLIST_UPDATE_METADATA. */
enum {
 PLAYLIST_UPDATE_SELECTION = 1,
 PLAYLIST_UPDATE_METADATA,
 PLAYLIST_UPDATE_STRUCTURE};

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

typedef bool_t (* PlaylistFilterFunc) (const char * filename, void * user);
typedef int (* PlaylistStringCompareFunc) (const char * a, const char * b);
typedef int (* PlaylistTupleCompareFunc) (const Tuple * a, const Tuple * b);

#define AUD_API_NAME PlaylistAPI
#define AUD_API_SYMBOL playlist_api

#ifdef _AUDACIOUS_CORE

#include "api-local-begin.h"
#include "playlist-api.h"
#include "api-local-end.h"

/* playlist-files.c */
bool_t playlist_load (const char * filename, char * * title,
 Index * * filenames, Index * * tuples);
bool_t playlist_insert_playlist_raw (int list, int at,
 const char * filename);

/* playlist-new.c */
void playlist_init (void);
void playlist_end (void);

void playlist_insert_with_id (int at, int id);
void playlist_set_modified (int playlist, bool_t modified);
bool_t playlist_get_modified (int playlist);

void playlist_load_state (void);
void playlist_save_state (void);
void playlist_resume (void);

void playlist_reformat_titles (void);
void playlist_trigger_scan (void);

void playlist_entry_insert_batch_raw (int playlist, int at,
 Index * filenames, Index * tuples, Index * decoders);

bool_t playlist_prev_song (int playlist);
bool_t playlist_next_song (int playlist, bool_t repeat);

int playback_entry_get_position (void);
PluginHandle * playback_entry_get_decoder (void);
Tuple * playback_entry_get_tuple (void);
char * playback_entry_get_title (void);
int playback_entry_get_length (void);

void playback_entry_set_tuple (Tuple * tuple);

int playback_entry_get_start_time (void);
int playback_entry_get_end_time (void);

/* playlist-utils.c */
void load_playlists (void);
void save_playlists (bool_t exiting);

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
