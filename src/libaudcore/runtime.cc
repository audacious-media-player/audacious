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

#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
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
#include "output.h"
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

size_t misc_bytes_allocated;

static bool headless_mode;
static int instance_number = 1;

#if defined(USE_QT) && ! defined(USE_GTK)
static MainloopType mainloop_type = MainloopType::Qt;
#else
static MainloopType mainloop_type = MainloopType::GLib;
#endif

static aud::array<AudPath, String> aud_paths;

EXPORT void aud_set_headless_mode (bool headless)
    { headless_mode = headless; }
EXPORT bool aud_get_headless_mode ()
    { return headless_mode; }

EXPORT void aud_set_instance (int instance)
    { instance_number = instance; }
EXPORT int aud_get_instance ()
    { return instance_number; }

EXPORT void aud_set_mainloop_type (MainloopType type)
    { mainloop_type = type; }
EXPORT MainloopType aud_get_mainloop_type ()
    { return mainloop_type; }

static StringBuf get_path_to_self ()
{
#ifdef __linux__

    StringBuf buf (-1);
    int len = readlink ("/proc/self/exe", buf, buf.len ());

    if (len < 0)
    {
        AUDERR ("Failed to read /proc/self/exe: %s\n", strerror (errno));
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
    int lenw = GetModuleFileNameW (nullptr, bufw, sizew);

    if (! lenw)
    {
        AUDERR ("GetModuleFileName failed.\n");
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

static String relocate_path (const char * path, const char * from, const char * to)
{
    int oldlen = strlen (from);
    int newlen = strlen (to);

    if (oldlen && from[oldlen - 1] == G_DIR_SEPARATOR)
        oldlen --;
    if (newlen && to[newlen - 1] == G_DIR_SEPARATOR)
        newlen --;

#ifdef _WIN32
    if (strcmp_nocase (path, from, oldlen) || (path[oldlen] && path[oldlen] != G_DIR_SEPARATOR))
#else
    if (strncmp (path, from, oldlen) || (path[oldlen] && path[oldlen] != G_DIR_SEPARATOR))
#endif
        return String (path);

    return String (str_printf ("%.*s%s", newlen, to, path + oldlen));
}

static void set_default_paths ()
{
    aud_paths[AudPath::BinDir] = String (HARDCODE_BINDIR);
    aud_paths[AudPath::DataDir] = String (HARDCODE_DATADIR);
    aud_paths[AudPath::PluginDir] = String (HARDCODE_PLUGINDIR);
    aud_paths[AudPath::LocaleDir] = String (HARDCODE_LOCALEDIR);
    aud_paths[AudPath::DesktopFile] = String (HARDCODE_DESKTOPFILE);
    aud_paths[AudPath::IconFile] = String (HARDCODE_ICONFILE);
}

static void set_install_paths ()
{
    StringBuf bindir = filename_normalize (str_copy (HARDCODE_BINDIR));
    StringBuf datadir = filename_normalize (str_copy (HARDCODE_DATADIR));
    StringBuf plugindir = filename_normalize (str_copy (HARDCODE_PLUGINDIR));
    StringBuf localedir = filename_normalize (str_copy (HARDCODE_LOCALEDIR));
    StringBuf desktopfile = filename_normalize (str_copy (HARDCODE_DESKTOPFILE));
    StringBuf iconfile = filename_normalize (str_copy (HARDCODE_ICONFILE));

    StringBuf from = str_copy (bindir);

    /* get path to current executable */
    StringBuf to = get_path_to_self ();

    if (! to)
    {
        set_default_paths ();
        return;
    }

    to.steal (filename_normalize (std::move (to)));

    const char * base = last_path_element (to);

    if (! base)
    {
        set_default_paths ();
        return;
    }

    cut_path_element (to, base - to);

    /* trim trailing path elements common to old and new paths */
    /* at the end, the old and new installation prefixes are left */
    const char * a, * b;
    while ((a = last_path_element (from)) &&
     (b = last_path_element (to)) &&
#ifdef _WIN32
     ! strcmp_nocase (a, b))
#else
     ! strcmp (a, b))
#endif
    {
        cut_path_element (from, a - from);
        cut_path_element (to, b - to);
    }

    /* replace old prefix with new one in each path */
    aud_paths[AudPath::BinDir] = relocate_path (bindir, from, to);
    aud_paths[AudPath::DataDir] = relocate_path (datadir, from, to);
    aud_paths[AudPath::PluginDir] = relocate_path (plugindir, from, to);
    aud_paths[AudPath::LocaleDir] = relocate_path (localedir, from, to);
    aud_paths[AudPath::DesktopFile] = relocate_path (desktopfile, from, to);
    aud_paths[AudPath::IconFile] = relocate_path (iconfile, from, to);
}

static void set_config_paths ()
{
    const char * xdg_config_home = g_get_user_config_dir ();
    StringBuf name = (instance_number == 1) ? str_copy ("audacious") :
                     str_printf ("audacious-%d", instance_number);

    aud_paths[AudPath::UserDir] = String (filename_build ({xdg_config_home, name}));
    aud_paths[AudPath::PlaylistDir] = String (filename_build
     ({aud_paths[AudPath::UserDir], "playlists"}));

    if (g_mkdir_with_parents (aud_paths[AudPath::PlaylistDir], DIRMODE) < 0)
        AUDERR ("Failed to create %s: %s\n",
         (const char *) aud_paths[AudPath::PlaylistDir], strerror (errno));

#ifdef _WIN32
    /* set some UNIX-style environment variables */
    g_setenv ("HOME", g_get_home_dir (), true);
    g_setenv ("XDG_CONFIG_HOME", xdg_config_home, true);
    g_setenv ("XDG_DATA_HOME", g_get_user_data_dir (), true);
    g_setenv ("XDG_CACHE_HOME", g_get_user_cache_dir (), true);
#endif
}

EXPORT const char * aud_get_path (AudPath id)
{
    if (! aud_paths[id])
    {
        if (id <= AudPath::IconFile)
            set_install_paths ();
        else
            set_config_paths ();
    }

    return aud_paths[id];
}

EXPORT void aud_init_i18n ()
{
    const char * localedir = aud_get_path (AudPath::LocaleDir);

    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, localedir);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    bindtextdomain (PACKAGE "-plugins", localedir);
    bind_textdomain_codeset (PACKAGE "-plugins", "UTF-8");
    textdomain (PACKAGE);
}

EXPORT void aud_init ()
{
    g_thread_pool_set_max_idle_time (100);

    config_load ();

    chardet_init ();
    eq_init ();
    output_init ();
    playlist_init ();

    start_plugins_one ();

    record_init ();
    scanner_init ();
    load_playlists ();
}

static void do_autosave (void *)
{
    hook_call ("config save", nullptr);
    save_playlists (false);
    plugin_registry_save ();
    config_save ();
}

EXPORT void aud_run ()
{
    /* playlist_enable_scan() should be after aud_resume(); the intent is to
     * avoid scanning until the currently playing entry is known, at which time
     * it can be scanned more efficiently (album art read in the same pass). */
    playlist_enable_scan (true);
    playlist_clear_updates ();
    start_plugins_two ();

    static QueuedFunc autosave;
    autosave.start (AUTOSAVE_INTERVAL, do_autosave, nullptr);

    /* calls "config save" before returning */
    interface_run ();

    autosave.stop ();

    stop_plugins_two ();
    playlist_enable_scan (false);
}

EXPORT void aud_cleanup ()
{
    save_playlists (true);

    aud_drct_stop ();
    playback_stop (true);

    adder_cleanup ();
    scanner_cleanup ();
    record_cleanup ();

    stop_plugins_one ();

    art_cleanup ();
    chardet_cleanup ();
    eq_cleanup ();
    output_cleanup ();
    playlist_end ();

    event_queue_cancel_all ();
    hook_cleanup ();
    timer_cleanup ();

    config_save ();
    config_cleanup ();
}

EXPORT void aud_leak_check ()
{
    for (String & path : aud_paths)
        path = String ();

    string_leak_check ();

    if (misc_bytes_allocated)
        AUDWARN ("Bytes allocated at exit: %ld\n", (long) misc_bytes_allocated);
}
