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

#include "internal.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <glib.h>

#include "audstrings.h"
#include "runtime.h"
#include "tuple.h"

bool dir_foreach (const char * path, DirForeachFunc func, void * user)
{
    GDir * dir = g_dir_open (path, 0, nullptr);
    if (! dir)
        return false;

    const char * name;
    while ((name = g_dir_read_name (dir)))
    {
        if (func (filename_build ({path, name}), name, user))
            break;
    }

    g_dir_close (dir);
    return true;
}

String write_temp_file (void * data, int64_t len)
{
    StringBuf name = filename_build ({g_get_tmp_dir (), "audacious-temp-XXXXXX"});

    int handle = g_mkstemp (name);
    if (handle < 0)
    {
        fprintf (stderr, "Error creating temporary file: %s\n", strerror (errno));
        return String ();
    }

    while (len)
    {
        int64_t written = write (handle, data, len);
        if (written < 0)
        {
            fprintf (stderr, "Error writing %s: %s\n", (const char *) name, strerror (errno));
            close (handle);
            return String ();
        }

        data = (char *) data + written;
        len -= written;
    }

    if (close (handle) < 0)
    {
        fprintf (stderr, "Error closing %s: %s\n", (const char *) name, strerror (errno));
        return String ();
    }

    return String (name);
}

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
 * returned will use the same memory.  May return nullptr for <first> and <second>.
 * Examples:
 *     "a/b/c/d/e.mp3" -> "e", "d",  "c"
 *     "d/e.mp3"       -> "e", "d",  nullptr
 *     "e.mp3"         -> "e", nullptr, nullptr */

static void split_filename (char * name, char * * base, char * * first, char * * second)
{
    * first = * second = nullptr;

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
 * nullptr.  Examples:
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
        return nullptr;

    char * c;

    if ((c = strchr (name, '/')))
        * c = 0;
    if ((c = strchr (name, ':')))
        * c = 0;
    if ((c = strchr (name, '?')))
        * c = 0;

    return name;
}

static String get_nonblank_field (const Tuple & tuple, int field)
{
    String str = tuple.get_str (field);
    return (str && str[0]) ? str : String ();
}

static String str_get_decoded (char * str)
{
    if (! str)
        return String ();

    return String (str_decode_percent (str));
}

/* Derives best guesses of title, artist, and album from a file name (URI) and
 * tuple (which may be nullptr).  The returned strings are stringpooled or nullptr. */

void describe_song (const char * name, const Tuple & tuple, String & title,
 String & artist, String & album)
{
    title = get_nonblank_field (tuple, FIELD_TITLE);
    artist = get_nonblank_field (tuple, FIELD_ARTIST);
    album = get_nonblank_field (tuple, FIELD_ALBUM);

    if (title && artist && album)
        return;

    if (! strncmp (name, "file:///", 8))
    {
        StringBuf filename = uri_to_display (name);
        if (! filename)
            return;

        // fill in song info from folder path
        char * base, * first, * second;
        split_filename (skip_top_folders (filename), & base, & first, & second);

        if (! title)
            title = String (base);

        // skip common strings and avoid duplicates
        for (auto skip : (const char *[]) {"music", artist, album})
        {
            if (first && skip && ! g_ascii_strcasecmp (first, skip))
            {
                first = second;
                second = nullptr;
            }

            if (second && skip && ! g_ascii_strcasecmp (second, skip))
                second = nullptr;
        }

        if (first)
        {
            if (second && ! artist && ! album)
            {
                artist = String (second);
                album = String (first);
            }
            else if (! artist)
                artist = String (first);
            else if (! album)
                album = String (first);
        }
    }
    else
    {
        StringBuf buf = str_copy (name);

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
}

const char * last_path_element (const char * path)
{
    const char * slash = strrchr (path, G_DIR_SEPARATOR);
    return (slash && slash[1]) ? slash + 1 : nullptr;
}

/* Thomas Wang's 32-bit mix function.  See:
 * http://web.archive.org/web/20070307172248/http://www.concentric.net/~Ttwang/tech/inthash.htm */

unsigned int32_hash (unsigned val)
{
    val = ~val + (val << 15);
    val = val ^ (val >> 12);
    val = val + (val << 2);
    val = val ^ (val >> 4);
    val = val * 2057;
    val = val ^ (val >> 16);
    return val;
}
