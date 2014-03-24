/*
 * ui_albumart.c
 * Copyright 2006-2013 Michael Hanselmann, Yoshiki Yazawa, and John Lindgren
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
#include <string.h>

#include <libaudcore/audstrings.h>

#include "i18n.h"
#include "main.h"
#include "misc.h"
#include "util.h"

typedef struct {
    const char * basename;
    Index * include, * exclude;
} SearchParams;

static bool_t has_front_cover_extension (const char * name)
{
    char * ext = strrchr (name, '.');
    if (! ext)
        return FALSE;

    return ! g_ascii_strcasecmp (ext, ".jpg") ||
     ! g_ascii_strcasecmp (ext, ".jpeg") || ! g_ascii_strcasecmp (ext, ".png");
}

static bool_t cover_name_filter (const char * name, Index * keywords, bool_t ret_on_empty)
{
    int count = index_count (keywords);
    if (! count)
        return ret_on_empty;

    for (int i = 0; i < count; i ++)
    {
        if (strstr_nocase (name, index_get (keywords, i)))
            return TRUE;
    }

    return FALSE;
}

static bool_t is_file_image (const char * imgfile, const char * file_name)
{
    char * imgfile_ext = strrchr (imgfile, '.');
    if (! imgfile_ext)
        return FALSE;

    char * file_name_ext = strrchr (file_name, '.');
    if (! file_name_ext)
        return FALSE;

    size_t imgfile_len = imgfile_ext - imgfile;
    size_t file_name_len = file_name_ext - file_name;

    return imgfile_len == file_name_len && ! g_ascii_strncasecmp (imgfile, file_name, imgfile_len);
}

static char * fileinfo_recursive_get_image (const char * path,
 const SearchParams * params, int depth)
{
    GDir * d = g_dir_open (path, 0, NULL);
    if (! d)
        return NULL;

    const char * name;

    if (get_bool (NULL, "use_file_cover") && ! depth)
    {
        /* Look for images matching file name */
        while ((name = g_dir_read_name (d)))
        {
            char * newpath = filename_build (path, name);

            if (! g_file_test (newpath, G_FILE_TEST_IS_DIR) &&
             has_front_cover_extension (name) &&
             is_file_image (name, params->basename))
            {
                g_dir_close (d);
                return newpath;
            }

            str_unref (newpath);
        }

        g_dir_rewind (d);
    }

    /* Search for files using filter */
    while ((name = g_dir_read_name (d)))
    {
        char * newpath = filename_build (path, name);

        if (! g_file_test (newpath, G_FILE_TEST_IS_DIR) &&
         has_front_cover_extension (name) &&
         cover_name_filter (name, params->include, TRUE) &&
         ! cover_name_filter (name, params->exclude, FALSE))
        {
            g_dir_close (d);
            return newpath;
        }

        str_unref (newpath);
    }

    g_dir_rewind (d);

    if (get_bool (NULL, "recurse_for_cover") && depth < get_int (NULL, "recurse_for_cover_depth"))
    {
        /* Descend into directories recursively. */
        while ((name = g_dir_read_name (d)))
        {
            char * newpath = filename_build (path, name);

            if (g_file_test (newpath, G_FILE_TEST_IS_DIR))
            {
                char * tmp = fileinfo_recursive_get_image (newpath, params, depth + 1);

                if (tmp)
                {
                    str_unref (newpath);
                    g_dir_close (d);
                    return tmp;
                }
            }

            str_unref (newpath);
        }
    }

    g_dir_close (d);
    return NULL;
}

char * get_associated_image_file (const char * filename)
{
    char * image_uri = NULL;

    char * local = uri_to_filename (filename);
    char * base = local ? last_path_element (local) : NULL;

    if (local && base)
    {
        char * include = get_str (NULL, "cover_name_include");
        char * exclude = get_str (NULL, "cover_name_exclude");

        SearchParams params = {
            .basename = base,
            .include = str_list_to_index (include, ", "),
            .exclude = str_list_to_index (exclude, ", ")
        };

        str_unref (include);
        str_unref (exclude);

        SNCOPY (path, local, base - 1 - local);

        char * image_local = fileinfo_recursive_get_image (path, & params, 0);
        if (image_local)
            image_uri = filename_to_uri (image_local);

        str_unref (image_local);

        index_free_full (params.include, (IndexFreeFunc) str_unref);
        index_free_full (params.exclude, (IndexFreeFunc) str_unref);
    }

    str_unref (local);

    return image_uri;
}
