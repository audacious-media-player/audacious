/*
 * ui_jumptotrack_cache.c
 * Copyright 2008-2012 Jussi Judin and John Lindgren
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

#include <glib.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <audacious/debug.h>
#include <audacious/playlist.h>
#include <libaudcore/audstrings.h>

#include "ui_jumptotrack_cache.h"

// Struct to keep information about matches from searches.
typedef struct
{
    GArray * entries; // int
    GArray * titles, * artists, * albums, * paths; // char * (pooled)
} KeywordMatches;

static void ui_jump_to_track_cache_init (JumpToTrackCache * cache);

static KeywordMatches * keyword_matches_new (void)
{
    KeywordMatches * k = g_slice_new (KeywordMatches);
    k->entries = g_array_new (FALSE, FALSE, sizeof (int));
    k->titles = g_array_new (FALSE, FALSE, sizeof (char *));
    k->artists = g_array_new (FALSE, FALSE, sizeof (char *));
    k->albums = g_array_new (FALSE, FALSE, sizeof (char *));
    k->paths = g_array_new (FALSE, FALSE, sizeof (char *));
    return k;
}

static void keyword_matches_free (KeywordMatches * k)
{
    g_array_free (k->entries, TRUE);
    g_array_free (k->titles, TRUE);
    g_array_free (k->artists, TRUE);
    g_array_free (k->albums, TRUE);
    g_array_free (k->paths, TRUE);
    g_slice_free (KeywordMatches, k);
}

/**
 * Creates an regular expression list usable in searches from search keyword.
 *
 * In searches, every regular expression on this list is matched against
 * the search title and if they all match, the title is declared as
 * matching one.
 *
 * Regular expressions in list are formed by splitting the 'keyword' to words
 * by splitting the keyword string with space character.
 */
static GSList*
ui_jump_to_track_cache_regex_list_create(const GString* keyword)
{
    GSList *regex_list = NULL;
    char **words = NULL;
    int i = -1;
    /* Chop the key string into ' '-separated key regex-pattern strings */
    words = g_strsplit(keyword->str, " ", 0);

    /* create a list of regex using the regex-pattern strings */
    while ( words[++i] != NULL )
    {
        // Ignore empty words.
        if (words[i][0] == 0) {
            continue;
        }

        GRegex * regex = g_regex_new (words[i], G_REGEX_CASELESS, 0, NULL);
        if (regex)
            regex_list = g_slist_append (regex_list, regex);
    }

    g_strfreev(words);

    return regex_list;
}

/**
 * Checks if 'song' matches all regular expressions in 'regex_list'.
 */
static bool_t
ui_jump_to_track_match(const char * song, GSList *regex_list)
{
    if ( song == NULL )
        return FALSE;

    for ( ; regex_list ; regex_list = g_slist_next(regex_list) )
    {
        GRegex * regex = regex_list->data;
        if (! g_regex_match (regex, song, 0, NULL))
            return FALSE;
    }

    return TRUE;
}

/**
 * Returns all songs that match 'keyword'.
 *
 * Searches are conducted against entries in 'search_space' variable
 * and after the search, search result is added to 'cache'.
 *
 * @param cache The result of this search is added to cache.
 * @param search_space Entries inside which the search is conducted.
 * @param keyword Normalized string for searches.
 */
static GArray*
ui_jump_to_track_cache_match_keyword(JumpToTrackCache* cache,
                                     const KeywordMatches* search_space,
                                     const GString* keyword)
{
    GSList* regex_list = ui_jump_to_track_cache_regex_list_create(keyword);

    KeywordMatches * k = keyword_matches_new ();

    for (int i = 0; i < search_space->entries->len; i ++)
    {
        char * title = g_array_index (search_space->titles, char *, i);
        char * artist = g_array_index (search_space->artists, char *, i);
        char * album = g_array_index (search_space->albums, char *, i);
        char * path = g_array_index (search_space->paths, char *, i);
        bool_t match;

        if (regex_list != NULL)
            match = ui_jump_to_track_match (title, regex_list)
             || ui_jump_to_track_match (artist, regex_list)
             || ui_jump_to_track_match (album, regex_list)
             || ui_jump_to_track_match (path, regex_list);
        else
            match = TRUE;

        if (match) {
            g_array_append_val (k->entries, g_array_index (search_space->entries, int, i));
            g_array_append_val (k->titles, title);
            g_array_append_val (k->artists, artist);
            g_array_append_val (k->albums, album);
            g_array_append_val (k->paths, path);
        }
    }

    g_hash_table_insert (cache->keywords, GINT_TO_POINTER (g_string_hash (keyword)), k);

    g_slist_free_full (regex_list, (GDestroyNotify) g_regex_unref);

    return k->entries;
}

/**
 * Frees the possibly allocated data in KeywordMatches.
 */
static void
ui_jump_to_track_cache_free_keywordmatch_data(KeywordMatches* match_entry)
{
    for (int i = 0; i < match_entry->entries->len; i ++)
    {
        str_unref (g_array_index (match_entry->titles, char *, i));
        str_unref (g_array_index (match_entry->artists, char *, i));
        str_unref (g_array_index (match_entry->albums, char *, i));
        str_unref (g_array_index (match_entry->paths, char *, i));
    }
}

/**
 * Creates a new song search cache.
 *
 * Returned value should be freed with ui_jump_to_track_cache_free() function.
 */
JumpToTrackCache*
ui_jump_to_track_cache_new()
{
    JumpToTrackCache * cache = g_slice_new (JumpToTrackCache);

    cache->keywords = g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify) keyword_matches_free);
    ui_jump_to_track_cache_init (cache);
    return cache;
}

/**
 * Clears the search cache.
 */
static void
ui_jump_to_track_cache_clear(JumpToTrackCache* cache)
{
    GString* empty_keyword = g_string_new("");
    gpointer found_keyword = NULL;

    // All normalized titles reside in an empty key "" so we'll free them
    // first.
    found_keyword = g_hash_table_lookup(cache->keywords,
                                        GINT_TO_POINTER(g_string_hash(empty_keyword)));
    g_string_free(empty_keyword,
                  TRUE);
    if (found_keyword != NULL)
    {
        KeywordMatches* all_titles = (KeywordMatches*)found_keyword;
        ui_jump_to_track_cache_free_keywordmatch_data(all_titles);
    }
    // Now when all normalized strings are freed, no need to worry about
    // double frees or memory leaks.
    g_hash_table_remove_all(cache->keywords);
}

/**
 * Initializes the search cache if cache is empty or has wrong playlist.
 */
static void ui_jump_to_track_cache_init (JumpToTrackCache * cache)
{
    // Reset cache state
    ui_jump_to_track_cache_clear(cache);

    // Initialize cache with playlist data
    int playlist = aud_playlist_get_active ();
    int entries = aud_playlist_entry_count (playlist);

    KeywordMatches * k = keyword_matches_new ();

    for (int entry = 0; entry < entries; entry ++)
    {
        char * title, * artist, * album;
        aud_playlist_entry_describe (playlist, entry, & title, & artist, & album, TRUE);

        char * uri = aud_playlist_entry_get_filename (playlist, entry);
        char * decoded = uri_to_display (uri);
        str_unref (uri);

        g_array_append_val (k->entries, entry);
        g_array_append_val (k->titles, title);
        g_array_append_val (k->artists, artist);
        g_array_append_val (k->albums, album);
        g_array_append_val (k->paths, decoded);
    }

    // Finally insert all titles into cache into an empty key "" so that
    // the matchable data has specified place to be.
    GString * empty_keyword = g_string_new ("");
    g_hash_table_insert (cache->keywords, GINT_TO_POINTER (g_string_hash (empty_keyword)), k);
    g_string_free (empty_keyword, TRUE);
}

/**
 * Searches 'keyword' inside 'playlist' by using 'cache' to speed up searching.
 *
 * Searches are basically conducted as follows:
 *
 * Cache is checked if it has the information about right playlist and
 * initialized with playlist data if needed.
 *
 * Keyword is normalized for searching (Unicode NFKD, case folding)
 *
 * Cache is checked if it has keyword and if it has, we can immediately get
 * the search results and return. If not, searching goes as follows:
 *
 * Search for the longest word that is in cache that matches the beginning
 * of keyword and use the cached matches as base for the current search.
 * The shortest word that can be matched against is the empty string "", so
 * there should always be matches in cache.
 *
 * After that conduct the search by splitting keyword into words separated
 * by space and using regular expressions.
 *
 * When the keyword is searched, search result is added to cache to
 * corresponding keyword that can be used as base for new searches.
 *
 * The motivation for caching is that to search word 'some cool song' one
 * has to type following strings that are all searched individually:
 *
 * s
 * so
 * som
 * some
 * some
 * some c
 * some co
 * some coo
 * some cool
 * some cool
 * some cool s
 * some cool so
 * some cool son
 * some cool song
 *
 * If the search results are cached in every phase and the result of
 * the maximum length matching string is used as base for concurrent
 * searches, we can probably get the matches reduced to some hundreds
 * after a few letters typed on playlists with thousands of songs and
 * reduce useless iteration quite a lot.
 *
 * Return: GArray of int
 */
const GArray * ui_jump_to_track_cache_search (JumpToTrackCache * cache, const
 char * keyword)
{
    GString* keyword_string = g_string_new(keyword);
    GString* match_string = g_string_new(keyword);
    int match_string_length = keyword_string->len;

    while (match_string_length >= 0)
    {
        gpointer string_ptr =  GINT_TO_POINTER(g_string_hash(match_string));
        gpointer result_entries = g_hash_table_lookup(cache->keywords,
                                                      string_ptr);
        if (result_entries != NULL)
        {
            KeywordMatches* matched_entries = (KeywordMatches*)result_entries;
            // if keyword matches something we have, we'll just return the list
            // of matches that the keyword has.
            if (match_string_length == keyword_string->len) {
                g_string_free(keyword_string, TRUE);
                g_string_free(match_string, TRUE);
                return matched_entries->entries;
            }

            // Do normal search by using the result of previous search
            // as search space.
            GArray* result = ui_jump_to_track_cache_match_keyword(cache,
                                                                  matched_entries,
                                                                  keyword_string);
            g_string_free(keyword_string, TRUE);
            g_string_free(match_string, TRUE);
            return result;
        }
        match_string_length--;
        g_string_set_size(match_string, match_string_length);
    }
    // This should never, ever get to this point because there is _always_
    // the empty string to match against.
    AUDDBG("One should never get to this point. Something is really wrong with \
cache->keywords hash table.");
    abort ();
}

void ui_jump_to_track_cache_free (JumpToTrackCache * cache)
{
    ui_jump_to_track_cache_clear (cache);
    g_hash_table_unref (cache->keywords);
    g_slice_free (JumpToTrackCache, cache);
}
