/*
 * util.c
 * Copyright 2009-2012 John Lindgren and Michał Lipski
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
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <errno.h>

#include <libaudcore/audstrings.h>

#include "config.h"
#include "debug.h"
#include "i18n.h"
#include "misc.h"
#include "plugins.h"
#include "util.h"

bool_t dir_foreach (const char * path, DirForeachFunc func, void * user)
{
    DIR * dir = opendir (path);
    if (! dir)
        return FALSE;

    struct dirent * entry;
    while ((entry = readdir (dir)))
    {
        if (entry->d_name[0] == '.')
            continue;

        char * full = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s", path, entry->d_name);
        bool_t stop = func (full, entry->d_name, user);
        g_free (full);

        if (stop)
            break;
    }

    closedir (dir);
    return TRUE;
}

char * construct_uri (const char * string, const char * playlist_name)
{
    /* URI */
    if (strstr (string, "://"))
        return strdup (string);

    /* absolute filename (assumed UTF-8) */
#ifdef _WIN32
    if (string[0] && string[1] == ':' && string[2] == '\\')
#else
    if (string[0] == '/')
#endif
        return filename_to_uri (string);

    /* relative filename (assumed UTF-8) */
    const char * slash = strrchr (playlist_name, '/');
    if (! slash)
        return NULL;

    int pathlen = slash + 1 - playlist_name;
    char buf[pathlen + 3 * strlen (string) + 1];
    memcpy (buf, playlist_name, pathlen);
    str_encode_percent (string, -1, buf + pathlen);
    return strdup (buf);
}

/* local files -- not URI's */
int file_get_mtime (const char * filename)
{
    struct stat info;

    if (stat (filename, & info))
        return -1;

    return info.st_mtime;
}

void
make_directory(const char * path, mode_t mode)
{
    if (g_mkdir_with_parents(path, mode) == 0)
        return;

    g_printerr(_("Could not create directory (%s): %s\n"), path,
               g_strerror(errno));
}

char * write_temp_file (void * data, int64_t len)
{
    char * name = g_strdup_printf ("%s/audacious-temp-XXXXXX", g_get_tmp_dir ());

    int handle = g_mkstemp (name);
    if (handle < 0)
    {
        fprintf (stderr, "Error creating temporary file: %s\n", strerror (errno));
        g_free (name);
        return NULL;
    }

    while (len)
    {
        int64_t written = write (handle, data, len);
        if (written < 0)
        {
            fprintf (stderr, "Error writing %s: %s\n", name, strerror (errno));
            close (handle);
            g_free (name);
            return NULL;
        }

        data = (char *) data + written;
        len -= written;
    }

    if (close (handle) < 0)
    {
        fprintf (stderr, "Error closing %s: %s\n", name, strerror (errno));
        g_free (name);
        return NULL;
    }

    return name;
}

char * get_path_to_self (void)
{
#if defined _WIN32 || defined HAVE_PROC_SELF_EXE
    int size = 256;
    char * buf = g_malloc (size);

    while (1)
    {
        int len;

#ifdef _WIN32
        if (! (len = GetModuleFileName (NULL, buf, size)))
        {
            fprintf (stderr, "GetModuleFileName failed.\n");
            g_free (buf);
            return NULL;
        }
#else
        if ((len = readlink ("/proc/self/exe", buf, size)) < 0)
        {
            fprintf (stderr, "Cannot access /proc/self/exe: %s.\n", strerror (errno));
            g_free (buf);
            return NULL;
        }
#endif

        if (len < size)
        {
            buf[len] = 0;
            return buf;
        }

        size += size;
        buf = g_realloc (buf, size);
    }
#elif defined __APPLE__
    unsigned int size = 256;
    char * buf = g_malloc (size);

    while (1)
    {
        int res;

        if (! (res = _NSGetExecutablePath (buf, &size)))
            return buf;

        if (res == -1)
            buf = g_realloc (buf, size);
        else
        {
            g_free (buf);
            return NULL;
        }
    }
#else
    return NULL;
#endif
}

/* Strips various common top-level folders from a URI.  The string passed will
 * not be modified, but the string returned will share the same memory.
 * Examples:
 *     "file:///home/john/folder/file.mp3" -> "folder/file.mp3"
 *     "file:///folder/file.mp3"           -> "folder/file.mp3" */

static char * skip_top_folders (char * name)
{
    static char * home;
    static int len;

    if (! home)
    {
        home = filename_to_uri (g_get_home_dir ());
        len = strlen (home);

        if (len > 0 && home[len - 1] == '/')
            len --;
    }

#ifdef _WIN32
    if (! g_ascii_strncasecmp (name, home, len) && name[len] == '/')
#else
    if (! strncmp (name, home, len) && name[len] == '/')
#endif
        return name + len + 1;

    if (! strncmp (name, "file:///", 8))
        return name + 8;

    return name;
}

/* Divides a URI into the base name, the lowest folder, and the
 * second lowest folder.  The string passed will be modified, and the strings
 * returned will use the same memory.  May return NULL for <first> and <second>.
 * Examples:
 *     "a/b/c/d/e.mp3" -> "e", "d",  "c"
 *     "d/e.mp3"       -> "e", "d",  NULL
 *     "e.mp3"         -> "e", NULL, NULL */

static void split_filename (char * name, char * * base, char * * first,
 char * * second)
{
    * first = * second = NULL;

    char * c;

    if ((c = strrchr (name, '/')))
    {
        * base = c + 1;
        * c = 0;
    }
    else
    {
        * base = name;
        goto DONE;
    }

    if ((c = strrchr (name, '/')))
    {
        * first = c + 1;
        * c = 0;
    }
    else
    {
        * first = name;
        goto DONE;
    }

    if ((c = strrchr (name, '/')))
        * second = c + 1;
    else
        * second = name;

DONE:
    if ((c = strrchr (* base, '.')))
        * c = 0;
}

/* Separates the domain name from an internet URI.  The string passed will be
 * modified, and the string returned will share the same memory.  May return
 * NULL.  Examples:
 *     "http://some.domain.org/folder/file.mp3" -> "some.domain.org"
 *     "http://some.stream.fm:8000"             -> "some.stream.fm" */

static char * stream_name (char * name)
{
    if (! strncmp (name, "http://", 7))
        name += 7;
    else if (! strncmp (name, "https://", 8))
        name += 8;
    else if (! strncmp (name, "mms://", 6))
        name += 6;
    else
        return NULL;

    char * c;

    if ((c = strchr (name, '/')))
        * c = 0;
    if ((c = strchr (name, ':')))
        * c = 0;
    if ((c = strchr (name, '?')))
        * c = 0;

    return name;
}

static char * get_nonblank_field (const Tuple * tuple, int field)
{
    char * str = tuple ? tuple_get_str (tuple, field, NULL) : NULL;

    if (str && ! str[0])
    {
        str_unref (str);
        str = NULL;
    }

    return str;
}

static char * str_get_decoded (char * str)
{
    if (! str)
        return NULL;

    str_decode_percent (str, -1, str);
    return str_get (str);
}

/* Derives best guesses of title, artist, and album from a file name (URI) and
 * tuple (which may be NULL).  The returned strings are stringpooled or NULL. */

void describe_song (const char * name, const Tuple * tuple, char * * _title,
 char * * _artist, char * * _album)
{
    /* Common folder names to skip */
    static const char * const skip[] = {"music"};

    char * title = get_nonblank_field (tuple, FIELD_TITLE);
    char * artist = get_nonblank_field (tuple, FIELD_ARTIST);
    char * album = get_nonblank_field (tuple, FIELD_ALBUM);

    if (title && artist && album)
    {
DONE:
        * _title = title;
        * _artist = artist;
        * _album = album;
        return;
    }

    char buf[strlen (name) + 1];
    memcpy (buf, name, sizeof buf);

    if (! strncmp (buf, "file:///", 8))
    {
        char * base, * first, * second;
        split_filename (skip_top_folders (buf), & base, & first, & second);

        if (! title)
            title = str_get_decoded (base);

        for (int i = 0; i < G_N_ELEMENTS (skip); i ++)
        {
            if (first && ! g_ascii_strcasecmp (first, skip[i]))
                first = NULL;
            if (second && ! g_ascii_strcasecmp (second, skip[i]))
                second = NULL;
        }

        if (first)
        {
            if (second && ! artist && ! album)
            {
                artist = str_get_decoded (second);
                album = str_get_decoded (first);
            }
            else if (! artist)
                artist = str_get_decoded (first);
            else if (! album)
                album = str_get_decoded (first);
        }
    }
    else
    {
        if (! title)
        {
            title = str_get_decoded (stream_name (buf));

            if (! title)
                title = str_get_decoded (buf);
        }
        else if (! artist)
            artist = str_get_decoded (stream_name (buf));
        else if (! album)
            album = str_get_decoded (stream_name (buf));
    }

    goto DONE;
}
