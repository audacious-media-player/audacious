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

#include <locale.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <new>

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <glib.h>
#include <libintl.h>

#include "audstrings.h"
#include "drct.h"
#include "hook.h"
#include "internal.h"
#include "mainloop.h"
#include "playlist-internal.h"
#include "plugins-internal.h"
#include "scanner.h"

#define AUTOSAVE_INTERVAL 300000 /* milliseconds, autosave every 5 minutes */

#ifdef WORDS_BIGENDIAN
#define UTF16_NATIVE "UTF-16BE"
#else
#define UTF16_NATIVE "UTF-16LE"
#endif

#ifdef S_IRGRP
#define DIRMODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#else
#define DIRMODE (S_IRWXU)
#endif

static bool_t headless_mode;
static bool_t verbose_mode;

static String aud_paths[AUD_PATH_COUNT];

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

static StringBuf get_path_to_self (void)
{
#ifdef HAVE_PROC_SELF_EXE

    StringBuf buf (-1);
    int len = readlink ("/proc/self/exe", buf, buf.len ());

    if (len < 0)
    {
        perror ("/proc/self/exe");
        return StringBuf ();
    }

    if (len == buf.len ())
        throw std::bad_alloc ();

    buf.resize (len);
    return buf;

#elif defined _WIN32

    StringBuf buf (-1);
    wchar_t * bufw = (wchar_t *) (char *) buf;
    int sizew = buf.len () / sizeof (wchar_t);
    int lenw = GetModuleFileNameW (NULL, bufw, sizew);

    if (! lenw)
    {
        fprintf (stderr, "GetModuleFileName failed.\n");
        return StringBuf ();
    }

    if (lenw == sizew)
        throw std::bad_alloc ();

    buf.resize (lenw * sizeof (wchar_t));
    buf.steal (str_convert (buf, buf.len (), UTF16_NATIVE, "UTF-8"));
    return buf;

#elif defined __APPLE__

    StringBuf buf (-1);
    uint32_t size = buf.len ();

    if (_NSGetExecutablePath (buf, & size) < 0)
        throw std::bad_alloc ();

    buf.resize (strlen (buf));
    return buf;

#else

    return StringBuf ();

#endif
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

static String relocate_path (const char * path, const char * from, const char * to)
{
    int oldlen = strlen (from);
    int newlen = strlen (to);

    if (oldlen && from[oldlen - 1] == G_DIR_SEPARATOR)
        oldlen --;
    if (newlen && to[newlen - 1] == G_DIR_SEPARATOR)
        newlen --;

#ifdef _WIN32
    if (g_ascii_strncasecmp (path, from, oldlen) || (path[oldlen] && path[oldlen] != G_DIR_SEPARATOR))
#else
    if (strncmp (path, from, oldlen) || (path[oldlen] && path[oldlen] != G_DIR_SEPARATOR))
#endif
        return String (path);

    return String (str_printf ("%.*s%s", newlen, to, path + oldlen));
}

static void set_default_paths (void)
{
    aud_paths[AUD_PATH_BIN_DIR] = String (HARDCODE_BINDIR);
    aud_paths[AUD_PATH_DATA_DIR] = String (HARDCODE_DATADIR);
    aud_paths[AUD_PATH_PLUGIN_DIR] = String (HARDCODE_PLUGINDIR);
    aud_paths[AUD_PATH_LOCALE_DIR] = String (HARDCODE_LOCALEDIR);
    aud_paths[AUD_PATH_DESKTOP_FILE] = String (HARDCODE_DESKTOPFILE);
    aud_paths[AUD_PATH_ICON_FILE] = String (HARDCODE_ICONFILE);
}

static void relocate_all_paths (void)
{
    StringBuf bindir = str_copy (HARDCODE_BINDIR);
    filename_normalize (bindir);

    StringBuf datadir = str_copy (HARDCODE_DATADIR);
    filename_normalize (datadir);

    StringBuf plugindir = str_copy (HARDCODE_PLUGINDIR);
    filename_normalize (plugindir);

    StringBuf localedir = str_copy (HARDCODE_LOCALEDIR);
    filename_normalize (localedir);

    StringBuf desktopfile = str_copy (HARDCODE_DESKTOPFILE);
    filename_normalize (desktopfile);

    StringBuf iconfile = str_copy (HARDCODE_ICONFILE);
    filename_normalize (iconfile);

    StringBuf from = str_copy (bindir);

    /* get path to current executable */
    StringBuf to = get_path_to_self ();

    if (! to)
    {
        set_default_paths ();
        return;
    }

    filename_normalize (to);

    char * base = (char *) last_path_element (to);

    if (! base)
    {
        set_default_paths ();
        return;
    }

    cut_path_element (to, base);

    /* trim trailing path elements common to old and new paths */
    /* at the end, the old and new installation prefixes are left */
    char * a, * b;
    while ((a = (char *) last_path_element (from)) &&
     (b = (char *) last_path_element (to)) &&
#ifdef _WIN32
     ! g_ascii_strcasecmp (a, b))
#else
     ! strcmp (a, b))
#endif
    {
        cut_path_element (from, a);
        cut_path_element (to, b);
    }

    /* replace old prefix with new one in each path */
    aud_paths[AUD_PATH_BIN_DIR] = relocate_path (bindir, from, to);
    aud_paths[AUD_PATH_DATA_DIR] = relocate_path (datadir, from, to);
    aud_paths[AUD_PATH_PLUGIN_DIR] = relocate_path (plugindir, from, to);
    aud_paths[AUD_PATH_LOCALE_DIR] = relocate_path (localedir, from, to);
    aud_paths[AUD_PATH_DESKTOP_FILE] = relocate_path (desktopfile, from, to);
    aud_paths[AUD_PATH_ICON_FILE] = relocate_path (iconfile, from, to);
}

EXPORT void aud_init_paths (void)
{
    relocate_all_paths ();

    const char * xdg_config_home = g_get_user_config_dir ();

    aud_paths[AUD_PATH_USER_DIR] = String (filename_build ({xdg_config_home, "audacious"}));
    aud_paths[AUD_PATH_PLAYLISTS_DIR] = String (filename_build
     ({aud_paths[AUD_PATH_USER_DIR], "playlists"}));

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
    for (String & path : aud_paths)
        path = String ();
}

EXPORT const char * aud_get_path (int id)
{
    g_return_val_if_fail (id >= 0 && id < AUD_PATH_COUNT, NULL);
    return aud_paths[id];
}

EXPORT void aud_init_i18n (void)
{
    const char * localedir = aud_get_path (AUD_PATH_LOCALE_DIR);

    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, localedir);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    bindtextdomain (PACKAGE "-plugins", localedir);
    bind_textdomain_codeset (PACKAGE "-plugins", "UTF-8");
    textdomain (PACKAGE);
}

EXPORT void aud_init (void)
{
    g_thread_pool_set_max_idle_time (100);

    config_load ();

    art_init ();
    chardet_init ();
    eq_init ();
    playlist_init ();

    start_plugins_one ();

    scanner_init ();

    load_playlists ();
}

static void do_autosave (void * unused)
{
    hook_call ("config save", NULL);
    save_playlists (FALSE);
    config_save ();
}

EXPORT void aud_run (void)
{
    start_plugins_two ();

    static QueuedFunc autosave;
    autosave.start (AUTOSAVE_INTERVAL, do_autosave, NULL);

    /* calls "config save" before returning */
    interface_run ();

    autosave.stop ();

    stop_plugins_two ();
}

EXPORT void aud_cleanup (void)
{
    save_playlists (TRUE);

    if (aud_drct_get_playing ())
        aud_drct_stop ();

    adder_cleanup ();
    scanner_cleanup ();

    stop_plugins_one ();

    art_cleanup ();
    chardet_cleanup ();
    eq_cleanup ();
    playlist_end ();

    event_queue_cancel_all ();

    config_save ();
    config_cleanup ();
}
