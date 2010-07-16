/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2008 Audacious development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <string.h>
#include <assert.h>
#ifdef HAVE_SYS_TYPES_H
#  include "sys/types.h"
#endif

#if defined(USE_REGEX_ONIGURUMA)
  #include <onigposix.h>
#elif defined(USE_REGEX_PCRE)
  #include <pcreposix.h>
#else
  #include <regex.h>
#endif

#include <audacious/debug.h>
#include <audacious/playlist.h>

#include "ui_jumptotrack_cache.h"

// Struct to keep information about matches from searches.
typedef struct
{
    GArray* track_entries; // JumpToTrackEntry*
    GArray* normalized_titles; // gchar*
} KeywordMatches;

static void ui_jump_to_track_cache_init (JumpToTrackCache * cache);

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
    gchar **words = NULL;
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
        regex_t *regex = g_malloc(sizeof(regex_t));
    #if defined(USE_REGEX_PCRE)
        if ( regcomp( regex , words[i] , REG_NOSUB | REG_UTF8 ) == 0 )
    #else
        if ( regcomp( regex , words[i] , REG_NOSUB ) == 0 )
    #endif
            regex_list = g_slist_append( regex_list , regex );
        else
            g_free( regex );
    }

    g_strfreev(words);

    return regex_list;
}

/**
 * Frees the regular expression list used in searches.
 */
static void
ui_jump_to_track_cache_regex_list_free(GSList* regex_list)
{
    if ( regex_list != NULL )
    {
        GSList* regex_list_tmp = regex_list;
        while ( regex_list != NULL )
        {
            regex_t *regex = regex_list->data;
            regfree( regex );
            g_free( regex );
            regex_list = g_slist_next(regex_list);
        }
        g_slist_free( regex_list_tmp );
    }
}

/**
 * Checks if 'song' matches all regular expressions in 'regex_list'.
 */
static gboolean
ui_jump_to_track_match(const gchar * song, GSList *regex_list)
{
    if ( song == NULL )
        return FALSE;

    for ( ; regex_list ; regex_list = g_slist_next(regex_list) )
    {
        regex_t *regex = regex_list->data;
        if ( regexec( regex , song , 0 , NULL , 0 ) != 0 )
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
    GArray * track_entries = g_array_new (FALSE, FALSE, sizeof (gint));
    GArray* normalized_titles = g_array_new(FALSE, FALSE, sizeof(gchar*));
    gboolean match = FALSE;
    int i = 0;

    for (i = 0; i < search_space->normalized_titles->len; i++)
    {
        gchar* title = g_array_index(search_space->normalized_titles, gchar*, i);

        if (regex_list != NULL)
            match = ui_jump_to_track_match(title, regex_list);
        else
            match = TRUE;

        if (match) {
            g_array_append_val (track_entries, g_array_index
             (search_space->track_entries, gint, i));
            g_array_append_val(normalized_titles, title);
        }
    }

    KeywordMatches* keyword_matches = g_new(KeywordMatches, 1);
    keyword_matches->track_entries = track_entries;
    keyword_matches->normalized_titles = normalized_titles;

    g_hash_table_insert(cache->keywords,
                        GINT_TO_POINTER(g_string_hash(keyword)),
                        keyword_matches);

    ui_jump_to_track_cache_regex_list_free(regex_list);
    return track_entries;
}

/**
 * Normalizes the search string to be more suitable for searches.
 *
 * Basically this does Unicode NFKD normalization to for example match
 * half-width and full-width characters and case folding mainly to match
 * upper- and lowercase letters.
 *
 * String returned by this function should be freed manually.
 */
static gchar *
normalize_search_string(const gchar* string)
{
    gchar* normalized_string = g_utf8_normalize(string, -1, G_NORMALIZE_NFKD);
    gchar* folded_string = g_utf8_casefold(normalized_string, -1);
    g_free(normalized_string);
    return folded_string;
}

/**
 * Frees the possibly allocated data in KeywordMatches.
 */
static void
ui_jump_to_track_cache_free_keywordmatch_data(KeywordMatches* match_entry)
{
    int i = 0;
    assert(match_entry->normalized_titles->len == match_entry->track_entries->len);
    for (i = 0; i < match_entry->normalized_titles->len; i++)
        g_free(g_array_index(match_entry->normalized_titles, gchar*, i));
}

/**
 * Frees the memory reserved for an search result.
 */
static void
ui_jump_to_track_cache_free_cache_entry(gpointer entry)
{
    KeywordMatches* match_entry = (KeywordMatches*)entry;
    g_array_free(match_entry->track_entries, TRUE);
    g_array_free(match_entry->normalized_titles, TRUE);
}

/**
 * Creates a new song search cache.
 *
 * Returned value should be freed with ui_jump_to_track_cache_free() function.
 */
JumpToTrackCache*
ui_jump_to_track_cache_new()
{
    JumpToTrackCache* cache = g_new(JumpToTrackCache, 1);

    cache->keywords = g_hash_table_new_full(NULL, NULL, NULL,
                                            ui_jump_to_track_cache_free_cache_entry);
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
        gint playlist, entries, entry;
        GArray * track_entries = g_array_new (FALSE, FALSE, sizeof (gint));
        GArray* normalized_titles = g_array_new(FALSE, FALSE, sizeof(gchar*));
        GString* empty_keyword = g_string_new("");

        // Reset cache state
        ui_jump_to_track_cache_clear(cache);

        // Initialize cache with playlist data
        playlist = aud_playlist_get_active ();
        entries = aud_playlist_entry_count (playlist);

        for (entry = 0; entry < entries; entry ++)
        {
            gchar * title = normalize_search_string (aud_playlist_entry_get_title
             (playlist, entry, TRUE));

            g_array_append_val (track_entries, entry);
            g_array_append_val (normalized_titles, title);
        }

        // Finally insert all titles into cache into an empty key "" so that
        // the matchable data has specified place to be.
        KeywordMatches* keyword_data = g_new(KeywordMatches, 1);
        keyword_data->track_entries = track_entries;
        keyword_data->normalized_titles = normalized_titles;
        g_hash_table_insert(cache->keywords,
                            GINT_TO_POINTER(g_string_hash(empty_keyword)),
                            keyword_data);
        g_string_free(empty_keyword,
                      TRUE);
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
 * Return: GArray of gint
 */
const GArray * ui_jump_to_track_cache_search (JumpToTrackCache * cache, const
 gchar * keyword)
{
    gchar* normalized_keyword = normalize_search_string(keyword);
    GString* keyword_string = g_string_new(normalized_keyword);
    GString* match_string = g_string_new(normalized_keyword);
    gsize match_string_length = keyword_string->len;

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
                g_free(normalized_keyword);
                return matched_entries->track_entries;
            }

            // Do normal search by using the result of previous search
            // as search space.
            GArray* result = ui_jump_to_track_cache_match_keyword(cache,
                                                                  matched_entries,
                                                                  keyword_string);
            g_string_free(keyword_string, TRUE);
            g_string_free(match_string, TRUE);
            g_free(normalized_keyword);
            return result;
        }
        match_string_length--;
        g_string_set_size(match_string, match_string_length);
    }
    // This should never, ever get to this point because there is _always_
    // the empty string to match against.
    AUDDBG("One should never get to this point. Something is really wrong with \
cache->keywords hash table.");
    assert(FALSE);
    g_return_val_if_fail(FALSE, (GArray*)-1);
}

void ui_jump_to_track_cache_free (JumpToTrackCache * cache)
{
    ui_jump_to_track_cache_clear (cache);
    g_hash_table_unref (cache->keywords);
    g_free (cache);
}
