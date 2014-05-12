/*
 * art-search.c
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

#include "internal.h"

#include <glib.h>
#include <string.h>

#include "audstrings.h"
#include "index.h"
#include "runtime.h"

struct SearchParams {
    const char * basename;
    Index<String> include, exclude;
};

static bool_t has_front_cover_extension (const char * name)
{
    const char * ext = strrchr (name, '.');
    if (! ext)
        return FALSE;

    return ! g_ascii_strcasecmp (ext, ".jpg") ||
     ! g_ascii_strcasecmp (ext, ".jpeg") || ! g_ascii_strcasecmp (ext, ".png");
}

static bool_t cover_name_filter (const char * name,
 const Index<String> & keywords, bool_t ret_on_empty)
{
    if (! keywords.len ())
        return ret_on_empty;

    for (const String & keyword : keywords)
    {
        if (strstr_nocase (name, keyword))
            return TRUE;
    }

    return FALSE;
}

static bool_t is_file_image (const char * imgfile, const char * file_name)
{
    const char * imgfile_ext = strrchr (imgfile, '.');
    if (! imgfile_ext)
        return FALSE;

    const char * file_name_ext = strrchr (file_name, '.');
    if (! file_name_ext)
        return FALSE;

    size_t imgfile_len = imgfile_ext - imgfile;
    size_t file_name_len = file_name_ext - file_name;

    return imgfile_len == file_name_len && ! g_ascii_strncasecmp (imgfile, file_name, imgfile_len);
}

static String fileinfo_recursive_get_image (const char * path,
 const SearchParams * params, int depth)
{
    GDir * d = g_dir_open (path, 0, NULL);
    if (! d)
        return String ();

    const char * name;

    if (aud_get_bool (NULL, "use_file_cover") && ! depth)
    {
        /* Look for images matching file name */
        while ((name = g_dir_read_name (d)))
        {
            StringBuf newpath = filename_build ({path, name});

            if (! g_file_test (newpath, G_FILE_TEST_IS_DIR) &&
             has_front_cover_extension (name) &&
             is_file_image (name, params->basename))
            {
                g_dir_close (d);
                return String (newpath);
            }
        }

        g_dir_rewind (d);
    }

    /* Search for files using filter */
    while ((name = g_dir_read_name (d)))
    {
        StringBuf newpath = filename_build ({path, name});

        if (! g_file_test (newpath, G_FILE_TEST_IS_DIR) &&
         has_front_cover_extension (name) &&
         cover_name_filter (name, params->include, TRUE) &&
         ! cover_name_filter (name, params->exclude, FALSE))
        {
            g_dir_close (d);
            return String (newpath);
        }
    }

    g_dir_rewind (d);

    if (aud_get_bool (NULL, "recurse_for_cover") && depth < aud_get_int (NULL, "recurse_for_cover_depth"))
    {
        /* Descend into directories recursively. */
        while ((name = g_dir_read_name (d)))
        {
            StringBuf newpath = filename_build ({path, name});

            if (g_file_test (newpath, G_FILE_TEST_IS_DIR))
            {
                String tmp = fileinfo_recursive_get_image (newpath, params, depth + 1);

                if (tmp)
                {
                    g_dir_close (d);
                    return tmp;
                }
            }
        }
    }

    g_dir_close (d);
    return String ();
}

String art_search (const char * filename)
{
    StringBuf local = uri_to_filename (filename);
    if (! local)
        return String ();

    const char * base = last_path_element (local);
    if (! base)
        return String ();

    String include = aud_get_str (NULL, "cover_name_include");
    String exclude = aud_get_str (NULL, "cover_name_exclude");

    SearchParams params = {
        base,
        str_list_to_index (include, ", "),
        str_list_to_index (exclude, ", ")
    };

    StringBuf path = str_copy (local, base - 1 - local);

    String image_local = fileinfo_recursive_get_image (path, & params, 0);
    if (! image_local)
        return String ();

    return String (filename_to_uri (image_local));
}
