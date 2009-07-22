/*
 * playlist-utils.c
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

#include <dirent.h>
#include <glib.h>
#include <regex.h>
#include <string.h>
#include <sys/stat.h>

#include "input.h"
#include "playlist_container.h"
#include "playlist-new.h"
#include "playlist-utils.h"
#include "plugin.h"

extern GHashTable * ext_hash;

const gchar * aud_titlestring_presets[] =
{
    "${title}",
    "${?artist:${artist} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }"
     "${?track-number:${track-number}. }${title}",
    "${?artist:${artist} }${?album:[ ${album} ] }${?artist:- }"
     "${?track-number:${track-number}. }${title}",
    "${?album:${album} - }${title}",
};

const gint n_titlestring_presets = G_N_ELEMENTS (aud_titlestring_presets);

static const gchar * get_basename (const gchar * filename)
{
    const gchar * slash = strrchr (filename, '/');

    return (slash == NULL) ? filename : slash + 1;
}

static gint filename_compare_basename (const gchar * a, const gchar * b)
{
    return strcmp (get_basename (a), get_basename (b));
}

static gint tuple_compare_string (const Tuple * a, const Tuple * b, gint field)
{
    const gchar * string_a = tuple_get_string ((Tuple *) a, field, NULL);
    const gchar * string_b = tuple_get_string ((Tuple *) b, field, NULL);

    if (string_a == NULL)
        return (string_b == NULL) ? 0 : -1;
    if (string_b == NULL)
        return 1;

    return strcmp (string_a, string_b);
}

static gint tuple_compare_int (const Tuple * a, const Tuple * b, gint field)
{
    gint int_a = tuple_get_int ((Tuple *) a, field, NULL);
    gint int_b = tuple_get_int ((Tuple *) b, field, NULL);

    return (int_a < int_b) ? -1 : (int_a > int_b);
}

static gint tuple_compare_title (const Tuple * a, const Tuple * b)
{
    return tuple_compare_string (a, b, FIELD_TITLE);
}

static gint tuple_compare_album (const Tuple * a, const Tuple * b)
{
    return tuple_compare_string (a, b, FIELD_ALBUM);
}

static gint tuple_compare_artist (const Tuple * a, const Tuple * b)
{
    return tuple_compare_string (a, b, FIELD_ARTIST);
}

static gint tuple_compare_date (const Tuple * a, const Tuple * b)
{
    return tuple_compare_int (a, b, FIELD_MTIME);
}

static gint tuple_compare_track (const Tuple * a, const Tuple * b)
{
    return tuple_compare_int (a, b, FIELD_TRACK_NUMBER);
}

static gint (* filename_comparisons[PLAYLIST_SORT_SCHEMES]) (const gchar * a,
 const gchar * b) =
{
    strcmp,
    filename_compare_basename,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static gint (* tuple_comparisons[PLAYLIST_SORT_SCHEMES]) (const Tuple * a, const
 Tuple * b) =
{
    NULL,
    NULL,
    tuple_compare_title,
    tuple_compare_album,
    tuple_compare_artist,
    tuple_compare_date,
    tuple_compare_track,
};

const gchar * get_gentitle_format (void)
{
    if (cfg.titlestring_preset >= 0 && cfg.titlestring_preset <
     n_titlestring_presets)
        return aud_titlestring_presets[cfg.titlestring_preset];

    return cfg.gentitle_format;
}

void playlist_sort_by_scheme (gint playlist, gint scheme)
{
    if (filename_comparisons[scheme] != NULL)
        playlist_sort_by_filename (playlist, filename_comparisons[scheme]);
    else if (tuple_comparisons[scheme] != NULL)
        playlist_sort_by_tuple (playlist, tuple_comparisons[scheme]);
}

void playlist_sort_selected_by_scheme (gint playlist, gint scheme)
{
    if (filename_comparisons[scheme] != NULL)
        playlist_sort_selected_by_filename (playlist,
         filename_comparisons[scheme]);
    else if (tuple_comparisons[scheme] != NULL)
        playlist_sort_selected_by_tuple (playlist, tuple_comparisons[scheme]);
}

void playlist_remove_duplicates_by_scheme (gint playlist, gint scheme)
{
    gint entries = playlist_entry_count (playlist);
    gint count;

    if (entries < 1)
        return;

    playlist_select_all (playlist, FALSE);

    if (filename_comparisons[scheme] != NULL)
    {
        gint (* compare) (const gchar * a, const gchar * b) =
         filename_comparisons[scheme];
        const gchar * last, * current;

        playlist_sort_by_filename (playlist, compare);
        last = playlist_entry_get_filename (playlist, 0);

        for (count = 1; count < entries; count ++)
        {
            current = playlist_entry_get_filename (playlist, count);

            if (compare (last, current) == 0)
                playlist_entry_set_selected (playlist, count, TRUE);

            last = current;
        }
    }
    else if (tuple_comparisons[scheme] != NULL)
    {
        gint (* compare) (const Tuple * a, const Tuple * b) =
         tuple_comparisons[scheme];
        const Tuple * last, * current;

        playlist_sort_by_tuple (playlist, compare);
        last = playlist_entry_get_tuple (playlist, 0);

        for (count = 1; count < entries; count ++)
        {
            current = playlist_entry_get_tuple (playlist, count);

            if (last != NULL && current != NULL && compare (last, current) == 0)
                playlist_entry_set_selected (playlist, count, TRUE);

            last = current;
        }
    }

    playlist_delete_selected (playlist);
}

void playlist_remove_failed (gint playlist)
{
    gint entries = playlist_entry_count (playlist);
    gint count;

    playlist_select_all (playlist, FALSE);

    for (count = 0; count < entries; count ++)
    {
        if (playlist_entry_get_decoder (playlist, count) == NULL ||
         playlist_entry_get_tuple (playlist, count) == NULL)
            playlist_entry_set_selected (playlist, count, TRUE);
    }

    playlist_delete_selected (playlist);
}

void playlist_select_by_patterns (gint playlist, const Tuple * patterns)
{
    const gint fields[] = {FIELD_TITLE, FIELD_ALBUM, FIELD_ARTIST,
     FIELD_FILE_NAME};

    gint entries = playlist_entry_count (playlist);
    gint field, entry;

    playlist_select_all (playlist, TRUE);

    for (field = 0; field < G_N_ELEMENTS (fields); field ++)
    {
        const gchar * pattern = tuple_get_string ((Tuple *) patterns,
         fields[field], NULL);
        regex_t regex;

        if (pattern == NULL || pattern[0] == 0)
            continue;

        if (regcomp (& regex, pattern, REG_ICASE) != 0)
            continue;

        for (entry = 0; entry < entries; entry ++)
        {
            const Tuple * tuple;
            const gchar * string;

            if (! playlist_entry_get_selected (playlist, entry))
                continue;

            tuple = playlist_entry_get_tuple (playlist, entry);

            if (tuple == NULL)
                goto NO_MATCH;

            string = tuple_get_string ((Tuple *) tuple, fields[field], NULL);

            if (string == NULL)
                goto NO_MATCH;

            if (regexec (& regex, string, 0, NULL, 0) == 0)
                continue;

        NO_MATCH:
            playlist_entry_set_selected (playlist, entry, FALSE);
        }

        regfree (& regex);
    }
}

gboolean filename_is_playlist (const gchar * filename)
{
	const gchar * period = strrchr (filename, '.');

    return (period != NULL && playlist_container_find ((gchar *) period + 1) !=
     NULL);
}

InputPlugin * filename_find_decoder (const gchar * filename)
{
    InputPlugin * decoder = NULL;
    gchar * temp = g_strdup (filename);
    gchar * temp2;
    GList * * index;

    decoder = uri_get_plugin (temp);

    if (decoder != NULL)
        goto DONE;

    temp2 = strrchr (temp, '?');

    if (temp2 != NULL)
        * temp2 = 0;

    temp2 = strrchr (temp, '.');

    if (temp2 == NULL)
        goto DONE;

    temp2 = g_ascii_strdown (temp2 + 1, -1);
    g_free (temp);
    temp = temp2;

    index = g_hash_table_lookup (ext_hash, temp);

    if (index != NULL)
        decoder = (* index)->data;

DONE:
     g_free (temp);
     return decoder;
}

gboolean playlist_insert_playlist (gint playlist, gint at, const gchar *
 filename)
{
    const gchar * period = strrchr (filename, '.');
    PlaylistContainer * container;
    gint last;

    if (period == NULL)
        return FALSE;

    container = playlist_container_find ((gchar *) period + 1);

    if (container == NULL || container->plc_read == NULL)
        return FALSE;

    last = playlist_get_active ();
    playlist_set_active (playlist);
    container->plc_read (filename, at);
    playlist_set_active (last);
    return TRUE;
}

gboolean playlist_save (gint playlist, const gchar * filename)
{
    const gchar * period = strrchr (filename, '.');
    PlaylistContainer * container;
    gint last;

    if (period == NULL)
        return FALSE;

    container = playlist_container_find ((gchar *) period + 1);

    if (container == NULL || container->plc_write == NULL)
        return FALSE;

    last = playlist_get_active ();
    playlist_set_active (playlist);
    container->plc_write (filename, 0);
    playlist_set_active (last);
    return TRUE;
}

static gboolean add_folder_cb (void * data)
{
    GList * * stack_ptr = data;
    GList * stack = * stack_ptr;
    struct index * add = index_new ();
    gint count;

    for (count = 0; count < 30; count ++)
    {
        struct stat info;
        struct dirent * entry;

        if (stat (stack->data, & info) == 0)
        {
            if (S_ISREG (info.st_mode))
            {
                gchar * filename = g_filename_to_uri (stack->data, NULL, NULL);

                if (filename != NULL && filename_find_decoder (filename) != NULL)
                    index_append (add, filename);
                else
                    g_free (filename);
            }
            else if (S_ISDIR (info.st_mode))
            {
                DIR * folder = opendir (stack->data);

                if (folder != NULL)
                {
                    stack = g_list_prepend (stack, folder);
                    goto READ;
                }
            }
        }

        g_free (stack->data);
        stack = g_list_delete_link (stack, stack);

    READ:
        if (stack == NULL)
            break;

        entry = readdir (stack->data);

        if (entry != NULL)
        {
            if (entry->d_name[0] == '.')
                goto READ;

            stack = g_list_prepend (stack, g_strdup_printf ("%s/%s", (gchar *)
             stack->next->data, entry->d_name));
        }
        else
        {
            closedir (stack->data);
            stack = g_list_delete_link (stack, stack);
            g_free (stack->data);
            stack = g_list_delete_link (stack, stack);
            goto READ;
        }
    }

    if (index_count (add) > 0)
        playlist_entry_insert_batch (playlist_get_active (), -1, add, NULL);
    else
        index_free (add);

    if (stack == NULL)
    {
        g_free (stack_ptr);
        return FALSE;
    }

    * stack_ptr = stack;
    return TRUE;
}

void playlist_add_folder (const gchar * folder)
{
    gchar * unix_name = g_filename_from_uri (folder, NULL, NULL);
    GList * * stack_ptr;

    if (unix_name == NULL)
        return;

    stack_ptr = g_malloc (sizeof (GList *));
    * stack_ptr = g_list_prepend (NULL, unix_name);
    g_idle_add (add_folder_cb, stack_ptr);
}
