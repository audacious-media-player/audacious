/*
 * runtime.c
 * Copyright 2010-2014 John Lindgren
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

#include "runtime.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <glib.h>

#include "audstrings.h"

static bool_t headless_mode;
static bool_t verbose_mode;

static char * aud_paths[AUD_PATH_COUNT];

#ifdef S_IRGRP
#define DIRMODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#else
#define DIRMODE (S_IRWXU)
#endif

EXPORT void aud_set_headless_mode (bool_t headless)
{
    headless_mode = headless;
}

EXPORT bool_t aud_get_headless_mode (void)
{
    return headless_mode;
}

EXPORT void aud_set_verbose_mode (bool_t verbose)
{
    verbose_mode = verbose;
}

EXPORT bool_t aud_get_verbose_mode (void)
{
    return verbose_mode;
}

static char * get_path_to_self (void)
{
#ifdef HAVE_PROC_SELF_EXE
    int size = 256;

    while (1)
    {
        char buf[size];
        int len;

        if ((len = readlink ("/proc/self/exe", buf, size)) < 0)
        {
            perror ("/proc/self/exe");
            return NULL;
        }

        if (len < size)
            return g_strndup (buf, len);

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
            return g_utf16_to_utf8 (buf, len, NULL, NULL, NULL);

        size += size;
    }
#elif defined __APPLE__
    unsigned int size = 256;

    while (1)
    {
        char buf[size];
        int res;

        if (! (res = _NSGetExecutablePath (buf, & size)))
            return g_strdup (buf);

        if (res != -1)
            return NULL;
    }
#else
#error No known method to identify run-time executable path!
#endif
}

static char * last_path_element (char * path)
{
    char * slash = strrchr (path, G_DIR_SEPARATOR);
    return (slash && slash[1]) ? slash + 1 : NULL;
}

static void cut_path_element (char * path, char * elem)
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

static void relocation_error (void)
{
    fprintf (stderr, "Unable to identify installation path.\n");
    abort ();
}

static char * relocate_path (const char * path, const char * old, const char * new)
{
    int oldlen = strlen (old);
    int newlen = strlen (new);

    if (oldlen && old[oldlen - 1] == G_DIR_SEPARATOR)
        oldlen --;
    if (newlen && new[newlen - 1] == G_DIR_SEPARATOR)
        newlen --;

#ifdef _WIN32
    if (g_ascii_strncasecmp (path, old, oldlen) || (path[oldlen] && path[oldlen] != G_DIR_SEPARATOR))
#else
    if (strncmp (path, old, oldlen) || (path[oldlen] && path[oldlen] != G_DIR_SEPARATOR))
#endif
        relocation_error ();

    return str_printf ("%.*s%s", newlen, new, path + oldlen);
}

static void relocate_all_paths (void)
{
    char bindir[] = HARDCODE_BINDIR;
    char datadir[] = HARDCODE_DATADIR;
    char plugindir[] = HARDCODE_PLUGINDIR;
    char localedir[] = HARDCODE_LOCALEDIR;
    char desktopfile[] = HARDCODE_DESKTOPFILE;
    char iconfile[] = HARDCODE_ICONFILE;

    filename_normalize (bindir);
    filename_normalize (datadir);
    filename_normalize (plugindir);
    filename_normalize (localedir);
    filename_normalize (desktopfile);
    filename_normalize (iconfile);

    SCOPY (old, bindir);

    /* get path to current executable */
    char * new = get_path_to_self ();
    if (! new)
        relocation_error ();

    filename_normalize (new);

    char * base = last_path_element (new);
    if (! base)
        relocation_error ();

    cut_path_element (new, base);

    /* trim trailing path elements common to old and new paths */
    /* at the end, the old and new installation prefixes are left */
    char * a, * b;
    while ((a = last_path_element (old)) && (b = last_path_element (new)) &&
#ifdef _WIN32
     ! g_ascii_strcasecmp (a, b))
#else
     ! strcmp (a, b))
#endif
    {
        cut_path_element (old, a);
        cut_path_element (new, b);
    }

    /* replace old prefix with new one in each path */
    aud_paths[AUD_PATH_BIN_DIR] = relocate_path (bindir, old, new);
    aud_paths[AUD_PATH_DATA_DIR] = relocate_path (datadir, old, new);
    aud_paths[AUD_PATH_PLUGIN_DIR] = relocate_path (plugindir, old, new);
    aud_paths[AUD_PATH_LOCALE_DIR] = relocate_path (localedir, old, new);
    aud_paths[AUD_PATH_DESKTOP_FILE] = relocate_path (desktopfile, old, new);
    aud_paths[AUD_PATH_ICON_FILE] = relocate_path (iconfile, old, new);

    g_free (new);
}

EXPORT void aud_init_paths (void)
{
    relocate_all_paths ();

    const char * xdg_config_home = g_get_user_config_dir ();

    aud_paths[AUD_PATH_USER_DIR] = filename_build (xdg_config_home, "audacious");
    aud_paths[AUD_PATH_PLAYLISTS_DIR] = filename_build (aud_paths[AUD_PATH_USER_DIR], "playlists");

    /* create ~/.config/audacious/playlists */
    if (g_mkdir_with_parents (aud_paths[AUD_PATH_PLAYLISTS_DIR], DIRMODE) < 0)
        perror (aud_paths[AUD_PATH_PLAYLISTS_DIR]);

#ifdef _WIN32
    /* set some UNIX-style environment variables */
    g_setenv ("HOME", g_get_home_dir (), TRUE);
    g_setenv ("XDG_CONFIG_HOME", xdg_config_home, TRUE);
    g_setenv ("XDG_DATA_HOME", g_get_user_data_dir (), TRUE);
    g_setenv ("XDG_CACHE_HOME", g_get_user_cache_dir (), TRUE);
#endif
}

EXPORT void aud_cleanup_paths (void)
{
    for (int i = 0; i < AUD_PATH_COUNT; i ++)
        str_unref (aud_paths[i]);
}

EXPORT const char * aud_get_path (int id)
{
    g_return_val_if_fail (id >= 0 && id < AUD_PATH_COUNT, NULL);
    return aud_paths[id];
}
