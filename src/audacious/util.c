/*
 * util.c
 * Copyright 2009-2013 John Lindgren and Michał Lipski
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

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <glib.h>

#include <libaudcore/audstrings.h>

#include "debug.h"
#include "i18n.h"
#include "misc.h"
#include "plugins.h"
#include "util.h"

bool_t dir_foreach (const char * path, DirForeachFunc func, void * user)
{
    GDir * dir = g_dir_open (path, 0, NULL);
    if (! dir)
        return FALSE;

    const char * name;
    while ((name = g_dir_read_name (dir)))
    {
        char * full = filename_build (path, name);
        bool_t stop = func (full, name, user);
        str_unref (full);

        if (stop)
            break;
    }

    g_dir_close (dir);
    return TRUE;
}

char * construct_uri (const char * path, const char * reference)
{
    /* URI */
    if (strstr (path, "://"))
        return str_get (path);

    /* absolute filename */
#ifdef _WIN32
    if (path[0] && path[1] == ':' && path[2] == '\\')
#else
    if (path[0] == '/')
#endif
        return filename_to_uri (path);

    /* relative path */
    const char * slash = strrchr (reference, '/');
    if (! slash)
        return NULL;

    char * utf8 = str_to_utf8 (path, -1);
    if (! utf8)
        return NULL;

    int pathlen = slash + 1 - reference;

    char buf[pathlen + 3 * strlen (utf8) + 1];
    memcpy (buf, reference, pathlen);

    if (get_bool (NULL, "convert_backslash"))
    {
        SCOPY (tmp, utf8);
        str_replace_char (tmp, '\\', '/');
        str_encode_percent (tmp, -1, buf + pathlen);
    }
    else
        str_encode_percent (utf8, -1, buf + pathlen);

    str_unref (utf8);
    return str_get (buf);
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
    char * temp = filename_build (g_get_tmp_dir (), "audacious-temp-XXXXXX");
    SCOPY (name, temp);
    str_unref (temp);

    int handle = g_mkstemp (name);
    if (handle < 0)
    {
        fprintf (stderr, "Error creating temporary file: %s\n", strerror (errno));
        return NULL;
    }

    while (len)
    {
        int64_t written = write (handle, data, len);
        if (written < 0)
        {
            fprintf (stderr, "Error writing %s: %s\n", name, strerror (errno));
            close (handle);
            return NULL;
        }

        data = (char *) data + written;
        len -= written;
    }

    if (close (handle) < 0)
    {
        fprintf (stderr, "Error closing %s: %s\n", name, strerror (errno));
        return NULL;
    }

    return str_get (name);
}

char * get_path_to_self (void)
{
#ifdef HAVE_PROC_SELF_EXE
    int size = 256;

    while (1)
    {
        char buf[size];
        int len;

        if ((len = readlink ("/proc/self/exe", buf, size)) < 0)
        {
            fprintf (stderr, "Cannot access /proc/self/exe: %s.\n", strerror (errno));
            return NULL;
        }

        if (len < size)
        {
            buf[len] = 0;
            return str_get (buf);
        }

        size += size;
    }
#elif defined _WIN32
    int size = 256;

    while (1)
    {
        wchar_t buf[size];
        int len;

        if (! (len = GetModuleFileNameW (NULL, buf, size)))
        {
            fprintf (stderr, "GetModuleFileName failed.\n");
            return NULL;
        }

        if (len < size)
        {
            char * temp = g_utf16_to_utf8 (buf, len, NULL, NULL, NULL);
            char * path = str_get (temp);
            g_free (temp);
            return path;
        }

        size += size;
    }
#elif defined __APPLE__
    unsigned int size = 256;

    while (1)
    {
        char buf[size];
        int res;

        if (! (res = _NSGetExecutablePath (buf, &size)))
            return str_get (buf);

        if (res != -1)
            return NULL;
    }
#else
    return NULL;
#endif
}

#ifdef _WIN32
void get_argv_utf8 (int * argc, char * * * argv)
{
    wchar_t * combined = GetCommandLineW ();
    wchar_t * * split = CommandLineToArgvW (combined, argc);

    * argv = g_new (char *, argc + 1);

    for (int i = 0; i < * argc; i ++)
        (* argv)[i] = g_utf16_to_utf8 (split[i], -1, NULL, NULL, NULL);

    (* argv)[* argc] = 0;

    LocalFree (split);
}

void free_argv_utf8 (int * argc, char * * * argv)
{
    g_strfreev (* argv);
    * argc = 0;
    * argv = NULL;
}
#endif

/* Strips various common top-level folders from a filename.  The string passed
 * will not be modified, but the string returned will share the same memory.
 * Examples:
 *     "/home/john/folder/file.mp3" -> "folder/file.mp3"
 *     "/folder/file.mp3"           -> "folder/file.mp3" */

static char * skip_top_folders (char * name)
{
    static const char * home;
    static int len;

    if (! home)
    {
        home = g_get_home_dir ();
        len = strlen (home);

        if (len > 0 && home[len - 1] == G_DIR_SEPARATOR)
            len --;
    }

#ifdef _WIN32
    if (! g_ascii_strncasecmp (name, home, len) && name[len] == '\\')
#else
    if (! strncmp (name, home, len) && name[len] == '/')
#endif
        return name + len + 1;

#ifdef _WIN32
    if (g_ascii_isalpha (name[0]) && name[1] == ':' && name[2] == '\\')
        return name + 3;
#else
    if (name[0] == '/')
        return name + 1;
#endif

    return name;
}

/* Divides a filename into the base name, the lowest folder, and the
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

    if ((c = strrchr (name, G_DIR_SEPARATOR)))
    {
        * base = c + 1;
        * c = 0;
    }
    else
    {
        * base = name;
        goto DONE;
    }

    if ((c = strrchr (name, G_DIR_SEPARATOR)))
    {
        * first = c + 1;
        * c = 0;
    }
    else
    {
        * first = name;
        goto DONE;
    }

    if ((c = strrchr (name, G_DIR_SEPARATOR)))
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
    char * str = tuple ? tuple_get_str (tuple, field) : NULL;

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

    if (! strncmp (name, "file:///", 8))
    {
        char * filename = uri_to_display (name);
        if (! filename)
            goto DONE;

        SCOPY (buf, filename);

        char * base, * first, * second;
        split_filename (skip_top_folders (buf), & base, & first, & second);

        if (! title)
            title = str_get (base);

        for (int i = 0; i < ARRAY_LEN (skip); i ++)
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
                artist = str_get (second);
                album = str_get (first);
            }
            else if (! artist)
                artist = str_get (first);
            else if (! album)
                album = str_get (first);
        }

        str_unref (filename);
    }
    else
    {
        SCOPY (buf, name);

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

char * last_path_element (char * path)
{
    char * slash = strrchr (path, G_DIR_SEPARATOR);
    return (slash && slash[1]) ? slash + 1 : NULL;
}

void cut_path_element (char * path, char * elem)
{
#ifdef _WIN32
    if (elem > path + 3)
#else
    if (elem > path + 1)
#endif
        elem[-1] = 0; /* overwrite slash */
    else
        elem[0] = 0; /* leave [drive letter and] leading slash */
}
