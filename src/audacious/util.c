/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2011  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <errno.h>

#include <libaudcore/audstrings.h>

#include "debug.h"
#include "i18n.h"
#include "misc.h"
#include "plugins.h"
#include "util.h"

gboolean dir_foreach (const gchar * path, DirForeachFunc func, void * user)
{
    DIR * dir = opendir (path);
    if (! dir)
        return FALSE;

    struct dirent * entry;
    while ((entry = readdir (dir)))
    {
        if (entry->d_name[0] == '.')
            continue;

        gchar * full = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s", path, entry->d_name);
        gboolean stop = func (full, entry->d_name, user);
        g_free (full);

        if (stop)
            break;
    }

    closedir (dir);
    return TRUE;
}

gchar * construct_uri (const gchar * string, const gchar * playlist_name)
{
    gchar *filename = g_strdup(string);
    gchar *uri = NULL;

    // make full path uri here
    // case 1: filename is raw full path or uri
    if (filename[0] == '/' || strstr(filename, "://")) {
        uri = g_filename_to_uri(filename, NULL, NULL);
        if(!uri)
            uri = g_strdup(filename);
    }
    // case 2: filename is not raw full path nor uri
    // make full path by replacing last part of playlist path with filename.
    else
    {
        const gchar * slash = strrchr (playlist_name, '/');
        if (slash)
            uri = g_strdup_printf ("%.*s/%s", (gint) (slash - playlist_name),
             playlist_name, filename);
    }

    g_free (filename);
    return uri;
}

/* local files -- not URI's */
gint file_get_mtime (const gchar * filename)
{
    struct stat info;

    if (stat (filename, & info))
        return -1;

    return info.st_mtime;
}

void
make_directory(const gchar * path, mode_t mode)
{
    if (g_mkdir_with_parents(path, mode) == 0)
        return;

    g_printerr(_("Could not create directory (%s): %s\n"), path,
               g_strerror(errno));
}

gchar * get_path_to_self (void)
{
#if defined _WIN32 || defined HAVE_PROC_SELF_EXE
    gint size = 256;
    gchar * buf = g_malloc (size);

    while (1)
    {
        gint len;

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
#else
    return NULL;
#endif
}

/* Strips various common top-level folders from a file name (not URI).  The
 * string passed will not be modified, but the string returned will share the
 * same memory.  Examples:
 *     "/home/john/folder/file.mp3"    -> "folder/file.mp3"
 *     "/folder/file.mp3"              -> "folder/file.mp3"
 *     "C:\Users\John\folder\file.mp3" -> "folder\file.mp3"
 *     "E:\folder\file.mp3"            -> "folder\file.mp3" */

static gchar * skip_top_folders (gchar * name)
{
    const gchar * home = getenv ("HOME");
    if (! home)
        goto NO_HOME;

    gint len = strlen (home);
    if (len > 0 && home[len - 1] == G_DIR_SEPARATOR)
        len --;

#ifdef _WIN32
    if (! strncasecmp (name, home, len) && name[len] == '\\')
#else
    if (! strncmp (name, home, len) && name[len] == '/')
#endif
        return name + len + 1;

NO_HOME:
#ifdef _WIN32
    return (name[0] && name[1] == ':' && name[2] == '\\') ? name + 3 : name;
#else
    return (name[0] == '/') ? name + 1 : name;
#endif
}

/* Divides a file name (not URI) into the base name, the lowest folder, and the
 * second lowest folder.  The string passed will be modified, and the strings
 * returned will use the same memory.  May return NULL for <first> and <second>.
 * Examples:
 *     "a/b/c/d/e.mp3" -> "e", "d",  "c"
 *     "d/e.mp3"       -> "e", "d",  NULL
 *     "e.mp3"         -> "e", NULL, NULL */

static void split_filename (gchar * name, gchar * * base, gchar * * first,
 gchar * * second)
{
    * first = * second = NULL;

    gchar * c;

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

static gchar * stream_name (gchar * name)
{
    if (! strncmp (name, "http://", 7))
        name += 7;
    else if (! strncmp (name, "https://", 8))
        name += 8;
    else if (! strncmp (name, "mms://", 6))
        name += 6;
    else
        return NULL;

    gchar * c;

    if ((c = strchr (name, '/')))
        * c = 0;
    if ((c = strchr (name, ':')))
        * c = 0;
    if ((c = strchr (name, '?')))
        * c = 0;

    return name;
}

static gchar * get_nonblank_field (const Tuple * tuple, gint field)
{
    gchar * str = tuple ? tuple_get_str (tuple, field, NULL) : NULL;

    if (str && ! str[0])
    {
        str_unref (str);
        str = NULL;
    }

    return str;
}

/* Derives best guesses of title, artist, and album from a file name (URI) and
 * tuple (which may be NULL).  The returned strings are stringpooled or NULL. */

void describe_song (const gchar * name, const Tuple * tuple, gchar * * _title,
 gchar * * _artist, gchar * * _album)
{
    /* Common folder names to skip */
    static const gchar * const skip[] = {"music"};

    gchar * title = get_nonblank_field (tuple, FIELD_TITLE);
    gchar * artist = get_nonblank_field (tuple, FIELD_ARTIST);
    gchar * album = get_nonblank_field (tuple, FIELD_ALBUM);

    gchar * copy = NULL;

    if (title && artist && album)
        goto DONE;

    copy = uri_to_display (name);

    if (! strncmp (name, "file://", 7))
    {
        gchar * base, * first, * second;
        split_filename (skip_top_folders (copy), & base, & first, & second);

        if (! title)
            title = str_get (base);

        for (gint i = 0; i < G_N_ELEMENTS (skip); i ++)
        {
            if (first && ! strcasecmp (first, skip[i]))
                first = NULL;
            if (second && ! strcasecmp (second, skip[i]))
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
    }
    else
    {
        if (! title)
            title = str_get (stream_name (copy));
        else if (! artist)
            artist = str_get (stream_name (copy));
        else if (! album)
            album = str_get (stream_name (copy));
    }

DONE:
    * _title = title;
    * _artist = artist;
    * _album = album;

    g_free (copy);
}
