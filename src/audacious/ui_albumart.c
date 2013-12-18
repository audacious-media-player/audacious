/*
 * ui_albumart.c
 * Copyright 2006-2012 Michael Hanselmann, Yoshiki Yazawa, and John Lindgren
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

#include <dirent.h>
#include <string.h>

#include <glib.h>

#include <libaudcore/audstrings.h>

#include "i18n.h"
#include "main.h"
#include "misc.h"

static bool_t has_front_cover_extension (const char * name)
{
    char * ext = strrchr (name, '.');
    if (! ext)
        return FALSE;

    return ! g_ascii_strcasecmp (ext, ".jpg") ||
     ! g_ascii_strcasecmp (ext, ".jpeg") || ! g_ascii_strcasecmp (ext, ".png");
}

static bool_t cover_name_filter (const char * name, const char * filter, bool_t ret_on_empty)
{
    if (! filter[0])
        return ret_on_empty;

    bool_t result = FALSE;

    char * * splitted = g_strsplit (filter, ",", 0);
    char * lname = g_ascii_strdown (name, -1);
    char * current;

    for (int i = 0; ! result && (current = splitted[i]); i ++)
    {
        char * stripped = g_strstrip (g_ascii_strdown (current, -1));
        result = result || strstr (lname, stripped);
        g_free (stripped);
    }

    g_free (lname);
    g_strfreev (splitted);
    return result;
}

static bool_t is_front_cover_image (const char * file)
{
    char * include = get_str (NULL, "cover_name_include");
    char * exclude = get_str (NULL, "cover_name_exclude");
    bool_t accept = cover_name_filter (file, include, TRUE) &&
     ! cover_name_filter (file, exclude, FALSE);

    str_unref (include);
    str_unref (exclude);
    return accept;
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

static char * fileinfo_recursive_get_image (const char * path, const char * file_name, int depth)
{
    DIR * d = opendir (path);
    if (! d)
        return NULL;

    struct dirent * entry;

    if (get_bool (NULL, "use_file_cover") && file_name)
    {
        /* Look for images matching file name */
        while ((entry = readdir (d)))
        {
            if (entry->d_name[0] == '.')
                continue;

            char * newpath = g_build_filename (path, entry->d_name, NULL);

            if (! g_file_test (newpath, G_FILE_TEST_IS_DIR) &&
             has_front_cover_extension (entry->d_name) &&
             is_file_image (entry->d_name, file_name))
            {
                closedir (d);
                return newpath;
            }

            g_free (newpath);
        }

        rewinddir (d);
    }

    /* Search for files using filter */
    while ((entry = readdir (d)))
    {
        if (entry->d_name[0] == '.')
            continue;

        char * newpath = g_build_filename (path, entry->d_name, NULL);

        if (! g_file_test (newpath, G_FILE_TEST_IS_DIR) &&
         has_front_cover_extension (entry->d_name) &&
         is_front_cover_image (entry->d_name))
        {
            closedir (d);
            return newpath;
        }

        g_free (newpath);
    }

    rewinddir (d);

    if (get_bool (NULL, "recurse_for_cover") && depth < get_int (NULL, "recurse_for_cover_depth"))
    {
        /* Descend into directories recursively. */
        while ((entry = readdir (d)))
        {
            if (entry->d_name[0] == '.')
                continue;

            char * newpath = g_build_filename (path, entry->d_name, NULL);

            if (g_file_test (newpath, G_FILE_TEST_IS_DIR))
            {
                char * tmp = fileinfo_recursive_get_image (newpath, NULL, depth + 1);

                if (tmp)
                {
                    g_free (newpath);
                    closedir (d);
                    return tmp;
                }
            }

            g_free (newpath);
        }
    }

    closedir (d);
    return NULL;
}

char * get_associated_image_file (const char * filename)
{
    if (strncmp (filename, "file://", 7))
        return NULL;

    char * local = uri_to_filename (filename);
    if (! local)
        return NULL;

    char * path = g_path_get_dirname (local);
    char * base = g_path_get_basename (local);
    char * image_local = fileinfo_recursive_get_image (path, base, 0);
    char * image_uri = image_local ? filename_to_uri (image_local) : NULL;

    str_unref (local);
    g_free (path);
    g_free (base);
    g_free (image_local);

    return image_uri;
}
