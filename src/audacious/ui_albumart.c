/*
 * ui_albumart.c
 * Copyright 2006 Michael Hanselmann and Yoshiki Yazawa
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
#include "misc.h"

static bool_t
has_front_cover_extension(const char *name)
{
    char *ext;

    ext = strrchr(name, '.');
    if (!ext) {
        /* No file extension */
        return FALSE;
    }

    return g_ascii_strcasecmp(ext, ".jpg") == 0 ||
           g_ascii_strcasecmp(ext, ".jpeg") == 0 ||
           g_ascii_strcasecmp(ext, ".png") == 0;
}

static bool_t
cover_name_filter(const char *name, const char *filter, const bool_t ret_on_empty)
{
    bool_t result = FALSE;
    char **splitted;
    char *current;
    char *lname;
    int i;

    if (!filter || strlen(filter) == 0) {
        return ret_on_empty;
    }

    splitted = g_strsplit(filter, ",", 0);
    lname = g_ascii_strdown (name, -1);

    for (i = 0; ! result && (current = splitted[i]); i ++)
    {
        char * stripped = g_strstrip (g_ascii_strdown (current, -1));
        result = result || strstr(lname, stripped);
        g_free(stripped);
    }

    g_free(lname);
    g_strfreev(splitted);

    return result;
}

/* Check wether it's an image we want */
static bool_t is_front_cover_image (const char * file)
{
    char * include = get_string (NULL, "cover_name_include");
    char * exclude = get_string (NULL, "cover_name_exclude");
    bool_t accept = cover_name_filter (file, include, TRUE) &&
     ! cover_name_filter (file, exclude, FALSE);
    g_free (include);
    g_free (exclude);
    return accept;
}

static bool_t
is_file_image(const char *imgfile, const char *file_name)
{
    char *imgfile_ext, *file_name_ext;
    size_t imgfile_len, file_name_len;

    imgfile_ext = strrchr(imgfile, '.');
    if (!imgfile_ext) {
        /* No file extension */
        return FALSE;
    }

    file_name_ext = strrchr(file_name, '.');
    if (!file_name_ext) {
        /* No file extension */
        return FALSE;
    }

    imgfile_len = (imgfile_ext - imgfile);
    file_name_len = (file_name_ext - file_name);

    if (imgfile_len == file_name_len) {
        return (g_ascii_strncasecmp(imgfile, file_name, imgfile_len) == 0);
    } else {
        return FALSE;
    }
}

static char * fileinfo_recursive_get_image (const char * path, const char *
 file_name, int depth)
{
    GDir *d;

    if (get_bool (NULL, "recurse_for_cover") && depth > get_int (NULL, "recurse_for_cover_depth"))
        return NULL;

    d = g_dir_open(path, 0, NULL);

    if (d) {
        const char *f;

        if (get_bool (NULL, "use_file_cover") && file_name)
        {
            /* Look for images matching file name */
            while((f = g_dir_read_name(d))) {
                char *newpath = g_strconcat(path, "/", f, NULL);

                if (!g_file_test(newpath, G_FILE_TEST_IS_DIR) &&
                    has_front_cover_extension(f) &&
                    is_file_image(f, file_name)) {
                    g_dir_close(d);
                    return newpath;
                }

                g_free(newpath);
            }
            g_dir_rewind(d);
        }

        /* Search for files using filter */
        while ((f = g_dir_read_name(d))) {
            char *newpath = g_strconcat(path, "/", f, NULL);

            if (!g_file_test(newpath, G_FILE_TEST_IS_DIR) &&
                has_front_cover_extension(f) &&
                is_front_cover_image(f)) {
                g_dir_close(d);
                return newpath;
            }

            g_free(newpath);
        }
        g_dir_rewind(d);

        /* checks whether recursive or not. */
        if (! get_bool (NULL, "recurse_for_cover"))
        {
            g_dir_close(d);
            return NULL;
        }

        /* Descend into directories recursively. */
        while ((f = g_dir_read_name(d))) {
            char *newpath = g_strconcat(path, "/", f, NULL);

            if(g_file_test(newpath, G_FILE_TEST_IS_DIR)) {
                char *tmp = fileinfo_recursive_get_image(newpath,
                    NULL, depth + 1);
                if(tmp) {
                    g_free(newpath);
                    g_dir_close(d);
                    return tmp;
                }
            }

            g_free(newpath);
        }

        g_dir_close(d);
    }

    return NULL;
}

char * get_associated_image_file (const char * filename)
{
    if (strncmp (filename, "file://", 7))
        return NULL;

    char * unesc = uri_to_filename (filename);
    if (! unesc)
        return NULL;

    char * path = g_path_get_dirname (unesc);
    char * base = g_path_get_basename (unesc);
    char * image_unesc = fileinfo_recursive_get_image (path, base, 0);
    char * image_file = image_unesc ? filename_to_uri (image_unesc) : NULL;

    g_free (unesc);
    g_free (path);
    g_free (base);
    g_free (image_unesc);

    return image_file;
}
