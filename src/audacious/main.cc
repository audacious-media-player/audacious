/*
 * main.c
 * Copyright 2007-2013 William Pitcock and John Lindgren
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define AUD_GLIB_INTEGRATION
#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/mainloop.h>
#include <libaudcore/playlist.h>
#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>

#ifdef USE_DBUS
#include "aud-dbus.h"
#endif

#include "main.h"
#include "util.h"

static struct {
    int help, version;
    int play, pause, play_pause, stop, fwd, rew;
    int enqueue, enqueue_to_temp;
    int mainwin, show_jump_box;
    int headless, quit_after_play;
    int verbose;
    int qt;
} options;

static bool initted = false;
static Index<PlaylistAddItem> filenames;

static const struct {
    const char * long_arg;
    char short_arg;
    int * value;
    const char * desc;
} arg_map[] = {
    {"help", 'h', & options.help, N_("Show command-line help")},
    {"version", 'v', & options.version, N_("Show version")},
    {"play", 'p', & options.play, N_("Start playback")},
    {"pause", 'u', & options.pause, N_("Pause playback")},
    {"play-pause", 't', & options.play_pause, N_("Pause if playing, play otherwise")},
    {"stop", 's', & options.stop, N_("Stop playback")},
    {"rew", 'r', & options.rew, N_("Skip to previous song")},
    {"fwd", 'f', & options.fwd, N_("Skip to next song")},
    {"enqueue", 'e', & options.enqueue, N_("Add files to the playlist")},
    {"enqueue-to-temp", 'E', & options.enqueue_to_temp, N_("Add files to a temporary playlist")},
    {"show-main-window", 'm', & options.mainwin, N_("Display the main window")},
    {"show-jump-box", 'j', & options.show_jump_box, N_("Display the jump-to-song window")},
    {"headless", 'H', & options.headless, N_("Start without a graphical interface")},
    {"quit-after-play", 'q', & options.quit_after_play, N_("Quit on playback stop")},
    {"verbose", 'V', & options.verbose, N_("Print debugging messages (may be used twice)")},
#if defined(USE_QT) && defined(USE_GTK)
    {"qt", 'Q', & options.qt, N_("Run in Qt mode")},
#endif
};

static bool parse_options (int argc, char * * argv)
{
    CharPtr cur (g_get_current_dir ());

#ifdef _WIN32
    Index<String> args = get_argv_utf8 ();

    for (int n = 1; n < args.len (); n ++)
    {
        const char * arg = args[n];
#else
    for (int n = 1; n < argc; n ++)
    {
        const char * arg = argv[n];
#endif

        if (arg[0] != '-')  /* filename */
        {
            String uri;

            if (strstr (arg, "://"))
                uri = String (arg);
            else if (g_path_is_absolute (arg))
                uri = String (filename_to_uri (arg));
            else
                uri = String (filename_to_uri (filename_build ({cur, arg})));

            filenames.append (uri);
        }
        else if (! arg[1])  /* "-" (standard input) */
        {
            filenames.append (String ("stdin://"));
        }
        else if (arg[1] >= '1' && arg[1] <= '9')  /* instance number */
        {
            aud_set_instance (arg[1] - '0');
        }
        else if (arg[1] == '-')  /* long option */
        {
            bool found = false;

            for (auto & arg_info : arg_map)
            {
                if (! strcmp (arg + 2, arg_info.long_arg))
                {
                    (* arg_info.value) ++;
                    found = true;
                    break;
                }
            }

            if (! found)
            {
                fprintf (stderr, _("Unknown option: %s\n"), arg);
                return false;
            }
        }
        else  /* short form */
        {
            for (int c = 1; arg[c]; c ++)
            {
                bool found = false;

                for (auto & arg_info : arg_map)
                {
                    if (arg[c] == arg_info.short_arg)
                    {
                        (* arg_info.value) ++;
                        found = true;
                        break;
                    }
                }

                if (! found)
                {
                    fprintf (stderr, _("Unknown option: -%c\n"), arg[c]);
                    return false;
                }
            }
        }
    }

    aud_set_headless_mode (options.headless);

    if (options.verbose >= 2)
        audlog::set_stderr_level (audlog::Debug);
    else if (options.verbose)
        audlog::set_stderr_level (audlog::Info);

    if (options.qt)
        aud_set_mainloop_type (MainloopType::Qt);

    return true;
}

static void print_help ()
{
    static const char pad[21] = "                    ";

    fprintf (stderr, "%s", _("Usage: audacious [OPTION] ... [FILE] ...\n\n"));
    fprintf (stderr, "  -1, -2, -3, etc.          %s\n", _("Select instance to run/control"));

    for (auto & arg_info : arg_map)
        fprintf (stderr, "  -%c, --%s%.*s%s\n", arg_info.short_arg,
         arg_info.long_arg, (int) (20 - strlen (arg_info.long_arg)), pad,
         _(arg_info.desc));

    fprintf (stderr, "\n");
}

#ifdef USE_DBUS
static void do_remote ()
{
    GDBusConnection * bus = nullptr;
    ObjAudacious * obj = nullptr;
    GError * error = nullptr;

#if ! GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init ();
#endif

    /* check whether the selected instance is running */
    if (dbus_server_init () != StartupType::Client)
        return;

    if (! (bus = g_bus_get_sync (G_BUS_TYPE_SESSION, nullptr, & error)) ||
     ! (obj = obj_audacious_proxy_new_sync (bus, (GDBusProxyFlags) 0,
     dbus_server_name (), "/org/atheme/audacious", nullptr, & error)))
    {
        AUDERR ("D-Bus error: %s\n", error->message);
        g_error_free (error);
        return;
    }

    AUDINFO ("Connected to remote session.\n");

    /* if no command line options, then present running instance */
    if (! (filenames.len () || options.play || options.pause ||
     options.play_pause || options.stop || options.rew || options.fwd ||
     options.show_jump_box || options.mainwin))
        options.mainwin = true;

    if (filenames.len ())
    {
        Index<const char *> list;

        for (auto & item : filenames)
            list.append (item.filename);

        list.append (nullptr);

        if (options.enqueue_to_temp)
            obj_audacious_call_open_list_to_temp_sync (obj, list.begin (), nullptr, nullptr);
        else if (options.enqueue)
            obj_audacious_call_add_list_sync (obj, list.begin (), nullptr, nullptr);
        else
            obj_audacious_call_open_list_sync (obj, list.begin (), nullptr, nullptr);
    }

    if (options.play)
        obj_audacious_call_play_sync (obj, nullptr, nullptr);
    if (options.pause)
        obj_audacious_call_pause_sync (obj, nullptr, nullptr);
    if (options.play_pause)
        obj_audacious_call_play_pause_sync (obj, nullptr, nullptr);
    if (options.stop)
        obj_audacious_call_stop_sync (obj, nullptr, nullptr);
    if (options.rew)
        obj_audacious_call_reverse_sync (obj, nullptr, nullptr);
    if (options.fwd)
        obj_audacious_call_advance_sync (obj, nullptr, nullptr);
    if (options.show_jump_box)
        obj_audacious_call_show_jtf_box_sync (obj, true, nullptr, nullptr);
    if (options.mainwin)
        obj_audacious_call_show_main_win_sync (obj, true, nullptr, nullptr);

    const char * startup_id = getenv ("DESKTOP_STARTUP_ID");
    if (startup_id)
        obj_audacious_call_startup_notify_sync (obj, startup_id, nullptr, nullptr);

    g_object_unref (obj);

    exit (EXIT_SUCCESS);
}
#endif

static void do_commands ()
{
    bool resume = aud_get_bool (nullptr, "resume_playback_on_startup");

    if (filenames.len ())
    {
        if (options.enqueue_to_temp)
        {
            aud_drct_pl_open_temp_list (std::move (filenames));
            resume = false;
        }
        else if (options.enqueue)
            aud_drct_pl_add_list (std::move (filenames), -1);
        else
        {
            aud_drct_pl_open_list (std::move (filenames));
            resume = false;
        }
    }

    if (resume)
        aud_resume ();

    if (options.play || options.play_pause)
    {
        if (! aud_drct_get_playing ())
            aud_drct_play ();
        else if (aud_drct_get_paused ())
            aud_drct_pause ();
    }
}

static void do_commands_at_idle (void *)
{
    if (options.show_jump_box && ! options.headless)
        aud_ui_show_jump_to_song ();
    if (options.mainwin && ! options.headless)
        aud_ui_show (true);
}

static void main_cleanup ()
{
    if (initted)
    {
        /* Somebody was naughty and called exit() instead of aud_quit().
         * aud_cleanup() has not been called yet, so there's no point in running
         * leak checks.  Note that it's not safe to call aud_cleanup() from the
         * exit handler, since we don't know what context we're in (we could be
         * deep inside the call tree of some plugin, for example). */
        AUDWARN ("exit() called unexpectedly; skipping normal cleanup.\n");
        return;
    }

    filenames.clear ();
    aud_leak_check ();
}

static bool check_should_quit ()
{
    return options.quit_after_play && ! aud_drct_get_playing () &&
     ! Playlist::add_in_progress_any ();
}

static void maybe_quit ()
{
    if (check_should_quit ())
        aud_quit ();
}

int main (int argc, char * * argv)
{
    atexit (main_cleanup);

#ifdef _WIN32
    SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
#endif
#ifdef HAVE_SIGWAIT
    signals_init_one ();
#endif

    aud_init_i18n ();

    if (! parse_options (argc, argv))
    {
        print_help ();
        return EXIT_FAILURE;
    }

    if (options.help)
    {
        print_help ();
        return EXIT_SUCCESS;
    }

    if (options.version)
    {
        printf ("%s %s (%s)\n", _("Audacious"), VERSION, BUILDSTAMP);
        return EXIT_SUCCESS;
    }

#if USE_DBUS
    do_remote (); /* may exit */
#endif

    AUDINFO ("No remote session; starting up.\n");

#ifdef HAVE_SIGWAIT
    signals_init_two ();
#endif

    initted = true;
    aud_init ();

    do_commands ();

    if (! check_should_quit ())
    {
        QueuedFunc at_idle_func;
        at_idle_func.queue (do_commands_at_idle, nullptr);

        hook_associate ("playback stop", (HookFunction) maybe_quit, nullptr);
        hook_associate ("playlist add complete", (HookFunction) maybe_quit, nullptr);
        hook_associate ("quit", (HookFunction) aud_quit, nullptr);

        aud_run ();

        hook_dissociate ("playback stop", (HookFunction) maybe_quit);
        hook_dissociate ("playlist add complete", (HookFunction) maybe_quit);
        hook_dissociate ("quit", (HookFunction) aud_quit);
    }

#ifdef USE_DBUS
    dbus_server_cleanup ();
#endif

    aud_cleanup ();
    initted = false;

    return EXIT_SUCCESS;
}
