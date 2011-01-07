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

#include <limits.h>
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

#ifdef HAVE_FTS_H
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fts.h>
#endif

#include <libaudcore/audstrings.h>

#include "audconfig.h"
#include "debug.h"
#include "i18n.h"
#include "misc.h"
#include "plugins.h"
#include "util.h"

gboolean
dir_foreach(const gchar * path, DirForeachFunc function,
            gpointer user_data, GError ** error)
{
    GError *error_out = NULL;
    GDir *dir;
    const gchar *entry;
    gchar *entry_fullpath;

    if (!(dir = g_dir_open(path, 0, &error_out))) {
        g_propagate_error(error, error_out);
        return FALSE;
    }

    while ((entry = g_dir_read_name(dir))) {
        entry_fullpath = g_build_filename(path, entry, NULL);

        if ((*function) (entry_fullpath, entry, user_data)) {
            g_free(entry_fullpath);
            break;
        }

        g_free(entry_fullpath);
    }

    g_dir_close(dir);

    return TRUE;
}

/**
 * util_get_localdir:
 *
 * Returns a string with the full path of Audacious local datadir (where config files are placed).
 * It's useful in order to put in the right place custom config files for audacious plugins.
 *
 * Return value: a string with full path of Audacious local datadir (should be freed after use)
 **/
gchar*
util_get_localdir(void)
{
  gchar *datadir;
  gchar *tmp;

  if ( (tmp = getenv("XDG_CONFIG_HOME")) == NULL )
    datadir = g_build_filename( g_get_home_dir() , ".config" , "audacious" ,  NULL );
  else
    datadir = g_build_filename( tmp , "audacious" , NULL );

  return datadir;
}


gchar * construct_uri (const gchar * string, const gchar * playlist_name)
{
    gchar *filename = g_strdup(string);
    gchar *tmp, *path;
    gchar *uri = NULL;

    /* try to translate dos path */
    convert_dos_path(filename); /* in place replacement */

    // make full path uri here
    // case 1: filename is raw full path or uri
    if (filename[0] == '/' || strstr(filename, "://")) {
        uri = g_filename_to_uri(filename, NULL, NULL);
        if(!uri)
            uri = g_strdup(filename);
        g_free(filename);
    }
    // case 2: filename is not raw full path nor uri, playlist path is full path
    // make full path by replacing last part of playlist path with filename. (using g_build_filename)
    else if (playlist_name[0] == '/' || strstr(playlist_name, "://")) {
        path = g_filename_from_uri(playlist_name, NULL, NULL);
        if (!path)
            path = g_strdup(playlist_name);
        tmp = strrchr(path, '/'); *tmp = '\0';
        tmp = g_build_filename(path, filename, NULL);
        g_free(path); g_free(filename);
        uri = g_filename_to_uri(tmp, NULL, NULL);
        g_free(tmp);
    }
    // case 3: filename is not raw full path nor uri, playlist path is not full path
    // just abort.
    else {
        g_free(filename);
        uri = NULL;
    }

    AUDDBG("uri=%s\n", uri);
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
    gchar buf[PATH_MAX];
    gint len;

#ifdef _WIN32
    if (! (len = GetModuleFileName (NULL, buf, sizeof buf)) || len == sizeof buf)
    {
        fprintf (stderr, "GetModuleFileName failed.\n");
        return NULL;
    }
#else
    if ((len = readlink ("/proc/self/exe", buf, sizeof buf)) < 0)
    {
        fprintf (stderr, "Cannot access /proc/self/exe: %s.\n", strerror (errno));
        return NULL;
    }
#endif

    return g_strndup (buf, len);
}

#define URL_HISTORY_MAX_SIZE 30

void
util_add_url_history_entry(const gchar * url)
{
    if (g_list_find_custom(cfg.url_history, url, (GCompareFunc) strcasecmp))
        return;

    cfg.url_history = g_list_prepend(cfg.url_history, g_strdup(url));

    while (g_list_length(cfg.url_history) > URL_HISTORY_MAX_SIZE) {
        GList *node = g_list_last(cfg.url_history);
        g_free(node->data);
        cfg.url_history = g_list_delete_link(cfg.url_history, node);
    }
}

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

static gchar * stream_name (const gchar * name)
{
    if (! strncmp (name, "http://", 7))
        name += 7;
    else if (! strncmp (name, "https://", 8))
        name += 8;
    else if (! strncmp (name, "mms://", 6))
        name += 6;
    else
        return NULL;

    gchar * s = g_strdup (name);
    gchar * c;

    if ((c = strchr (s, '/')))
        * c = 0;
    if ((c = strchr (s, ':')))
        * c = 0;
    if ((c = strchr (s, '?')))
        * c = 0;

    return s;
}

void describe_song (const gchar * name, const Tuple * tuple, gchar * * title,
 gchar * * artist, gchar * * album)
{
    const gchar * _title = tuple_get_string (tuple, FIELD_TITLE, NULL);
    const gchar * _artist = tuple_get_string (tuple, FIELD_ARTIST, NULL);
    const gchar * _album = tuple_get_string (tuple, FIELD_ALBUM, NULL);

    * title = (_title && _title[0]) ? g_strdup (_title) : NULL;
    * artist = (_artist && _artist[0]) ? g_strdup (_artist) : NULL;
    * album = (_album && _album[0]) ? g_strdup (_album) : NULL;

    if (* title && * artist && * album)
        return;

    gchar * copy = uri_to_display (name);

    if (! strncmp (name, "file://", 7))
    {
        gchar * base, * first, * second;
        split_filename (skip_top_folders (copy), & base, & first,
         & second);

        if (! * title)
            * title = g_strdup (base);

        if (first)
        {
            if (second && ! * artist && ! * album)
            {
                * artist = g_strdup (second);
                * album = g_strdup (first);
            }
            else if (! * artist)
                * artist = g_strdup (first);
            else if (! * album)
                * album = g_strdup (first);
        }
    }
    else
    {
        if (! * title)
            * title = stream_name (copy);
        else if (! * artist)
            * artist = stream_name (copy);
        else if (! * album)
            * album = stream_name (copy);
    }

    g_free (copy);
}
