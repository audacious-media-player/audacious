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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/playlist.h>
#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>

#ifdef USE_DBUS
#include "aud-dbus.h"
#endif

#include "main.h"
#include "util.h"

static struct {
    bool_t help, version;
    bool_t play, pause, play_pause, stop, fwd, rew;
    bool_t enqueue, enqueue_to_temp;
    bool_t mainwin, show_jump_box;
    bool_t headless, quit_after_play;
    bool_t verbose;
} options;

static Index<PlaylistAddItem> filenames;

static const struct {
    const char * long_arg;
    char short_arg;
    bool_t * value;
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
    {"verbose", 'V', & options.verbose, N_("Print debugging messages")},
};

static bool_t parse_options (int argc, char * * argv)
{
    char * cur = g_get_current_dir ();
    bool_t success = TRUE;

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

            if (uri)
                filenames.append ({uri});
        }
        else if (arg[1] == '-')  /* long option */
        {
            unsigned i;

            for (i = 0; i < ARRAY_LEN (arg_map); i ++)
            {
                if (! strcmp (arg + 2, arg_map[i].long_arg))
                {
                    * arg_map[i].value = TRUE;
                    break;
                }
            }

            if (i == ARRAY_LEN (arg_map))
            {
                fprintf (stderr, _("Unknown option: %s\n"), arg);
                success = FALSE;
                goto OUT;
            }
        }
        else  /* short form */
        {
            for (int c = 1; arg[c]; c ++)
            {
                unsigned i;

                for (i = 0; i < ARRAY_LEN (arg_map); i ++)
                {
                    if (arg[c] == arg_map[i].short_arg)
                    {
                        * arg_map[i].value = TRUE;
                        break;
                    }
                }

                if (i == ARRAY_LEN (arg_map))
                {
                    fprintf (stderr, _("Unknown option: -%c\n"), arg[c]);
                    success = FALSE;
                    goto OUT;
                }
            }
        }
    }

    aud_set_headless_mode (options.headless);
    aud_set_verbose_mode (options.verbose);

OUT:
    g_free (cur);
    return success;
}

static void print_help (void)
{
    static const char pad[21] = "                    ";

    fprintf (stderr, _("Usage: audacious [OPTION] ... [FILE] ...\n\n"));

    for (unsigned i = 0; i < ARRAY_LEN (arg_map); i ++)
        fprintf (stderr, "  -%c, --%s%.*s%s\n", arg_map[i].short_arg,
         arg_map[i].long_arg, (int) (20 - strlen (arg_map[i].long_arg)), pad,
         _(arg_map[i].desc));

    fprintf (stderr, "\n");
}

#ifdef USE_DBUS
static void do_remote (void)
{
    GDBusConnection * bus = NULL;
    ObjAudacious * obj = NULL;
    GError * error = NULL;
    char * version = NULL;

    if (! (bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, & error)))
        goto ERR;

    if (! (obj = obj_audacious_proxy_new_sync (bus, (GDBusProxyFlags) 0,
     "org.atheme.audacious", "/org/atheme/audacious", NULL, & error)))
        goto ERR;

    /* check whether remote is running */
    obj_audacious_call_version_sync (obj, & version, NULL, NULL);

    if (! version)
        goto DONE;

    AUDDBG ("Connected to remote version %s.\n", version);

    /* if no command line options, then present running instance */
    if (! (filenames.len () || options.play || options.pause ||
     options.play_pause || options.stop || options.rew || options.fwd ||
     options.show_jump_box || options.mainwin))
        options.mainwin = TRUE;

    if (filenames.len ())
    {
        int n_filenames = filenames.len ();
        const char * * list = g_new (const char *, n_filenames + 1);

        for (int i = 0; i < n_filenames; i ++)
            list[i] = filenames[i].filename;

        list[n_filenames] = NULL;

        if (options.enqueue_to_temp)
            obj_audacious_call_open_list_to_temp_sync (obj, list, NULL, NULL);
        else if (options.enqueue)
            obj_audacious_call_add_list_sync (obj, list, NULL, NULL);
        else
            obj_audacious_call_open_list_sync (obj, list, NULL, NULL);

        g_free (list);
    }

    if (options.play)
        obj_audacious_call_play_sync (obj, NULL, NULL);
    if (options.pause)
        obj_audacious_call_pause_sync (obj, NULL, NULL);
    if (options.play_pause)
        obj_audacious_call_play_pause_sync (obj, NULL, NULL);
    if (options.stop)
        obj_audacious_call_stop_sync (obj, NULL, NULL);
    if (options.rew)
        obj_audacious_call_reverse_sync (obj, NULL, NULL);
    if (options.fwd)
        obj_audacious_call_advance_sync (obj, NULL, NULL);
    if (options.show_jump_box)
        obj_audacious_call_show_jtf_box_sync (obj, TRUE, NULL, NULL);
    if (options.mainwin)
        obj_audacious_call_show_main_win_sync (obj, TRUE, NULL, NULL);

    g_free (version);
    g_object_unref (obj);

    exit (EXIT_SUCCESS);

ERR:
    fprintf (stderr, "D-Bus error: %s\n", error->message);
    g_error_free (error);

DONE:
    if (obj)
        g_object_unref (obj);

    return;
}
#endif

static void do_commands (void)
{
    bool_t resume = aud_get_bool (NULL, "resume_playback_on_startup");

    if (filenames.len ())
    {
        if (options.enqueue_to_temp)
        {
            aud_drct_pl_open_temp_list (std::move (filenames));
            resume = FALSE;
        }
        else if (options.enqueue)
            aud_drct_pl_add_list (std::move (filenames), -1);
        else
        {
            aud_drct_pl_open_list (std::move (filenames));
            resume = FALSE;
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

    if (options.show_jump_box && ! options.headless)
        aud_ui_show_jump_to_song ();
    if (options.mainwin && ! options.headless)
        aud_ui_show (TRUE);
}

static void main_cleanup (void)
{
    aud_cleanup_paths ();

    filenames.clear ();

    strpool_shutdown ();
}

static bool_t check_should_quit (void)
{
    return options.quit_after_play && ! aud_drct_get_playing () &&
     ! aud_playlist_add_in_progress (-1);
}

static void maybe_quit (void)
{
    if (check_should_quit ())
        aud_quit ();
}

int main (int argc, char * * argv)
{
    atexit (main_cleanup);

#ifdef HAVE_SIGWAIT
    signals_init_one ();
#endif

    aud_init_paths ();
    aud_init_i18n ();

#if ! GLIB_CHECK_VERSION (2, 36, 0)
    g_type_init ();
#endif

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

    AUDDBG ("No remote session; starting up.\n");

#ifdef HAVE_SIGWAIT
    signals_init_two ();
#endif

    aud_init ();

    do_commands ();

#ifdef USE_DBUS
    dbus_server_init ();
#endif

    if (check_should_quit ())
        goto QUIT;

    hook_associate ("playback stop", (HookFunction) maybe_quit, NULL);
    hook_associate ("playlist add complete", (HookFunction) maybe_quit, NULL);
    hook_associate ("quit", (HookFunction) aud_quit, NULL);

    aud_run ();

    hook_dissociate ("playback stop", (HookFunction) maybe_quit);
    hook_dissociate ("playlist add complete", (HookFunction) maybe_quit);
    hook_dissociate ("quit", (HookFunction) aud_quit);

QUIT:
#ifdef USE_DBUS
    dbus_server_cleanup ();
#endif

    aud_cleanup ();

    return EXIT_SUCCESS;
}
