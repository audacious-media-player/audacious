/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "platform/smartinclude.h"

#include "main.h"

#include <glib/gprintf.h>

#ifdef USE_DBUS
#  include "dbus-service.h"
#  include "audctrl.h"
#endif

#ifdef USE_EGGSM
#include "eggsmclient.h"
#include "eggdesktopfile.h"
#endif

#include "build_stamp.h"
#include "configdb.h"
#include "equalizer.h"
#include "hook.h"
#include "input.h"
#include "logger.h"
#include "output.h"
#include "playback.h"
#include "playlist-new.h"
#include "playlist-utils.h"
#include "pluginenum.h"
#include "signals.h"
#include "util.h"
#include "vfs.h"
#include "chardet.h"

#include "ui_headless.h"
#include "ui_misc.h"

static const gchar *application_name = N_("Audacious");

struct _AudCmdLineOpt
{
    gchar **filenames;
    gint session;
    gboolean play, stop, pause, fwd, rew, play_pause, show_jump_box;
    gboolean enqueue, mainwin, remote, activate;
    gboolean no_log;
    gboolean enqueue_to_temp;
    gboolean version;
    gchar *previous_session_id;
    gchar *interface;
    gboolean macpack;
};
typedef struct _AudCmdLineOpt AudCmdLineOpt;

static AudCmdLineOpt options;

gchar * aud_paths[BMP_PATH_COUNT];

#ifdef USE_DBUS
MprisPlayer *mpris;
#endif

static void print_version(void)
{
    g_printf("%s %s [%s]\n", _(application_name), VERSION, build_stamp);
}

static void aud_make_user_dir(void)
{
    const mode_t mode755 = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    make_directory(aud_paths[BMP_PATH_USER_DIR], mode755);
    make_directory(aud_paths[BMP_PATH_USER_PLUGIN_DIR], mode755);
    make_directory(aud_paths[BMP_PATH_USER_SKIN_DIR], mode755);
    make_directory(aud_paths[BMP_PATH_SKIN_THUMB_DIR], mode755);
    make_directory(aud_paths[BMP_PATH_PLAYLISTS_DIR], mode755);
}

static void aud_free_paths(void)
{
    gint i;

    for (i = 0; i < BMP_PATH_COUNT; i++)
    {
        g_free(aud_paths[i]);
        aud_paths[i] = 0;
    }
}

static void aud_init_paths()
{
    gchar *xdg_config_home;
    gchar *xdg_data_home;
    gchar *xdg_cache_home;

    xdg_config_home = (getenv("XDG_CONFIG_HOME") == NULL ? g_build_filename(g_get_home_dir(), ".config", NULL) : g_strdup(getenv("XDG_CONFIG_HOME")));
    xdg_data_home = (getenv("XDG_DATA_HOME") == NULL ? g_build_filename(g_get_home_dir(), ".local", "share", NULL) : g_strdup(getenv("XDG_DATA_HOME")));
    xdg_cache_home = (getenv("XDG_CACHE_HOME") == NULL ? g_build_filename(g_get_home_dir(), ".cache", NULL) : g_strdup(getenv("XDG_CACHE_HOME")));

    aud_paths[BMP_PATH_USER_DIR] = g_build_filename(xdg_config_home, "audacious", NULL);
    aud_paths[BMP_PATH_USER_SKIN_DIR] = g_build_filename(xdg_data_home, "audacious", "Skins", NULL);
    aud_paths[BMP_PATH_USER_PLUGIN_DIR] = g_build_filename(xdg_data_home, "audacious", "Plugins", NULL);

    aud_paths[BMP_PATH_SKIN_THUMB_DIR] = g_build_filename(xdg_cache_home, "audacious", "thumbs", NULL);

    aud_paths[BMP_PATH_PLAYLISTS_DIR] = g_build_filename(aud_paths[BMP_PATH_USER_DIR], "playlists", NULL);

    aud_paths[BMP_PATH_CONFIG_FILE] = g_build_filename(aud_paths[BMP_PATH_USER_DIR], "config", NULL);
#ifdef HAVE_XSPF_PLAYLIST
    aud_paths[BMP_PATH_PLAYLIST_FILE] = g_build_filename(aud_paths[BMP_PATH_USER_DIR], "playlist.xspf", NULL);
#else
    aud_paths[BMP_PATH_PLAYLIST_FILE] = g_build_filename(aud_paths[BMP_PATH_USER_DIR], "playlist.m3u", NULL);
#endif
    aud_paths[BMP_PATH_ACCEL_FILE] = g_build_filename(aud_paths[BMP_PATH_USER_DIR], "accels", NULL);
    aud_paths[BMP_PATH_LOG_FILE] = g_build_filename(aud_paths[BMP_PATH_USER_DIR], "log", NULL);

    aud_paths[BMP_PATH_GTKRC_FILE] = g_build_filename(aud_paths[BMP_PATH_USER_DIR], "gtkrc", NULL);

    g_free(xdg_config_home);
    g_free(xdg_data_home);
    g_free(xdg_cache_home);

    g_atexit(aud_free_paths);
}

static GOptionEntry cmd_entries[] = {
    {"rew", 'r', 0, G_OPTION_ARG_NONE, &options.rew, N_("Skip backwards in playlist"), NULL},
    {"play", 'p', 0, G_OPTION_ARG_NONE, &options.play, N_("Start playing current playlist"), NULL},
    {"pause", 'u', 0, G_OPTION_ARG_NONE, &options.pause, N_("Pause current song"), NULL},
    {"stop", 's', 0, G_OPTION_ARG_NONE, &options.stop, N_("Stop current song"), NULL},
    {"play-pause", 't', 0, G_OPTION_ARG_NONE, &options.play_pause, N_("Pause if playing, play otherwise"), NULL},
    {"fwd", 'f', 0, G_OPTION_ARG_NONE, &options.fwd, N_("Skip forward in playlist"), NULL},
    {"show-jump-box", 'j', 0, G_OPTION_ARG_NONE, &options.show_jump_box, N_("Display Jump to File dialog"), NULL},
    {"enqueue", 'e', 0, G_OPTION_ARG_NONE, &options.enqueue, N_("Don't clear the playlist"), NULL},
    {"enqueue-to-temp", 'E', 0, G_OPTION_ARG_NONE, &options.enqueue_to_temp, N_("Add new files to a temporary playlist"), NULL},
    {"show-main-window", 'm', 0, G_OPTION_ARG_NONE, &options.mainwin, N_("Display the main window"), NULL},
    {"activate", 'a', 0, G_OPTION_ARG_NONE, &options.activate, N_("Display all open Audacious windows"), NULL},
    {"no-log", 'N', 0, G_OPTION_ARG_NONE, &options.no_log, N_("Print all errors and warnings to stdout"), NULL},
    {"version", 'v', 0, G_OPTION_ARG_NONE, &options.version, N_("Show version"), NULL},
    {"interface", 'i', 0, G_OPTION_ARG_STRING, &options.interface, N_("Interface to use (use -i list to view available interfaces)"), NULL},
#ifdef GDK_WINDOWING_QUARTZ
    {"macpack", 'n', 0, G_OPTION_ARG_NONE, &options.macpack, N_("Used in macpacking"), NULL},   /* Make this hidden */
#endif
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &options.filenames, N_("FILE..."), NULL},
    {NULL},
};

static void parse_cmd_line_options(gint * argc, gchar *** argv)
{
    GOptionContext *context;
    GError *error = NULL;

    memset(&options, '\0', sizeof(AudCmdLineOpt));
    options.session = -1;

    /* If audacious2 is run with no arguments, bring it to the top. This is
       handy when the user forgets that Audacious is already running. */
    if (*argc == 1)
        options.mainwin = 1;

    context = g_option_context_new(_("- play multimedia files"));
    g_option_context_add_main_entries(context, cmd_entries, PACKAGE_NAME);
    g_option_context_add_group(context, gtk_get_option_group(FALSE));
#ifdef USE_EGGSM
    g_option_context_add_group(context, egg_sm_client_get_option_group());
#endif

    if (!g_option_context_parse(context, argc, argv, &error))
        /* checking for MacOS X -psn_0_* errors */
        if (error->message && !g_strrstr(error->message, "-psn_0_"))
        {
            g_printerr(_("%s: %s\nTry `%s --help' for more information.\n"), (*argv)[0], error->message, (*argv)[0]);
            exit(EXIT_FAILURE);
        }

    g_free(context);
}

static void handle_cmd_line_filenames(gboolean is_running)
{
    gint i;
    gchar *working, **filenames = options.filenames;
    GList * fns = NULL;
#ifdef USE_DBUS
    DBusGProxy *session = audacious_get_dbus_proxy();
#endif

    if (filenames == NULL)
        return;

    working = g_get_current_dir();
    for (i = 0; filenames[i] != NULL; i++)
    {
        gchar *uri;
        if (strstr(filenames[i], "://"))
            uri = g_strdup(filenames[i]);
        else if (g_path_is_absolute(filenames[i]))
            uri = g_filename_to_uri(filenames[i], NULL, NULL);
        else
        {
            gchar *absolute = g_build_filename(working, filenames[i], NULL);
            uri = g_filename_to_uri(absolute, NULL, NULL);
            g_free(absolute);
        }
        fns = g_list_prepend(fns, uri);
    }
    fns = g_list_reverse(fns);
    g_free(working);

#ifdef USE_DBUS
    if (is_running)
    {
        if (options.enqueue_to_temp)
            audacious_remote_playlist_open_list_to_temp (session, fns);
        else if (options.enqueue)
            audacious_remote_playlist_add (session, fns);
        else
            audacious_remote_playlist_open_list (session, fns);
    }
    else                        /* !is_running */
#endif
    {
        if (options.enqueue_to_temp)
        {
            drct_pl_temp_open_list (fns);
            cfg.resume_state = 0;
        }
        else if (options.enqueue)
            drct_pl_add (fns);
        else
        {
            drct_pl_open_list (fns);
            cfg.resume_state = 0;
        }
    }                           /* !is_running */

    g_list_foreach(fns, (GFunc) g_free, NULL);
    g_list_free(fns);
}

static void handle_cmd_line_options_first(void)
{
#ifdef USE_DBUS
    DBusGProxy *session;
#endif

    if (options.version)
    {
        print_version();
        exit(EXIT_SUCCESS);
    }

#ifdef USE_DBUS
    session = audacious_get_dbus_proxy();
    if (audacious_remote_is_running(session))
    {
        handle_cmd_line_filenames(TRUE);
        if (options.rew)
            audacious_remote_playlist_prev(session);
        if (options.play)
            audacious_remote_play(session);
        if (options.pause)
            audacious_remote_pause(session);
        if (options.stop)
            audacious_remote_stop(session);
        if (options.fwd)
            audacious_remote_playlist_next(session);
        if (options.play_pause)
            audacious_remote_play_pause(session);
        if (options.show_jump_box)
            audacious_remote_show_jtf_box(session);
        if (options.mainwin)
            audacious_remote_main_win_toggle(session, 1);
        if (options.activate)
            audacious_remote_activate(session);
        exit(EXIT_SUCCESS);
    }
#endif

    if (options.interface == NULL)
    {
        mcs_handle_t *db = cfg_db_open();
        cfg_db_get_string(db, NULL, "interface", &options.interface);
        if (options.interface == NULL)
            options.interface = g_strdup("skinned");
        cfg_db_close(db);
    }
}

static void handle_cmd_line_options(void)
{
    handle_cmd_line_filenames(FALSE);

    if (cfg.resume_playback_on_startup && cfg.resume_state > 0)
    {
        playback_initiate ();

        if (cfg.resume_state == 2)
            playback_pause ();

        playback_seek (cfg.resume_playback_on_startup_time);
    }

    if (options.play || options.play_pause)
    {
        if (! playback_get_playing ())
            playback_initiate ();
        else if (playback_get_paused ())
            playback_pause ();
    }

    if (options.show_jump_box)
        drct_jtf_show();
    if (options.mainwin)
        drct_main_win_toggle(TRUE);
    if (options.activate)
        drct_activate();
}

static void aud_setup_logger(void)
{
    if (!aud_logger_start(aud_paths[BMP_PATH_LOG_FILE]))
        return;

    g_atexit(aud_logger_stop);
}

static gboolean load_extra_playlist(const gchar * path, const gchar * basename, gpointer def)
{
    gchar * filename = g_filename_to_uri (path, NULL, NULL);
    gint playlist = playlist_count();

    playlist_insert(playlist);
    playlist_insert_playlist(playlist, 0, filename);

    g_free (filename);

    return FALSE;               /* keep loading other playlists */
}

static void playlist_system_init()
{
    gchar * filename = g_filename_to_uri (aud_paths[BMP_PATH_PLAYLIST_FILE],
     NULL, NULL);

    playlist_init();

    if (vfs_file_test (filename, G_FILE_TEST_EXISTS))
        playlist_insert_playlist (0, 0, filename);

    g_free (filename);

    /* Load extra playlists */
    if (!dir_foreach(aud_paths[BMP_PATH_PLAYLISTS_DIR], load_extra_playlist, NULL, NULL))
        g_warning("Could not load extra playlists\n");

    playlist_load_state ();
    playlist_set_shuffle(cfg.shuffle);
}

void aud_quit(void)
{
    Interface *i = interface_get(options.interface);

    g_message("Saving configuration");
    aud_config_save();
    playlist_save_state ();

    if (playback_get_playing ())
        playback_stop ();

    g_message("Shutting down user interface subsystem");
    interface_destroy(i);

    output_cleanup ();

    g_message("Plugin subsystem shutdown");
    plugin_system_cleanup();

    g_message("Playlist cleanup");
    playlist_end();

    g_message("Shutdown finished, bye.");
    exit(EXIT_SUCCESS);
}

static int print_interface_info(mowgli_dictionary_elem_t * delem, void *privdata)
{
    Interface *i = (Interface *) delem->data;
    g_print("  %-15s - %s\n", i->id, i->desc);
    return 0;
}

static void quit_cb(void *hook_data, void *user_data)
{
    aud_quit();
}

#ifdef USE_DBUS
static void mpris_status_cb1(gpointer hook_data, gpointer user_data)
{
    mpris_emit_status_change(mpris, GPOINTER_TO_INT(user_data));
}

static void mpris_status_cb2(gpointer hook_data, gpointer user_data)
{
    mpris_emit_status_change(mpris, -1);
}

void init_playback_hooks(void)
{
    hook_associate("playback begin", mpris_status_cb1, GINT_TO_POINTER(MPRIS_STATUS_PLAY));
    hook_associate("playback pause", mpris_status_cb1, GINT_TO_POINTER(MPRIS_STATUS_PAUSE));
    hook_associate("playback unpause", mpris_status_cb1, GINT_TO_POINTER(MPRIS_STATUS_PLAY));
    hook_associate("playback stop", mpris_status_cb1, GINT_TO_POINTER(MPRIS_STATUS_STOP));

    hook_associate("playback shuffle", mpris_status_cb2, NULL);
    hook_associate("playback repeat", mpris_status_cb2, NULL);
    hook_associate("playback no playlist advance", mpris_status_cb2, NULL);
}
#endif

gint main(gint argc, gchar ** argv)
{
    Interface *i;

    /* glib-2.13.0 requires g_thread_init() to be called before all
       other GLib functions */
    g_thread_init(NULL);
    if (!g_thread_supported())
    {
        g_printerr(_("Sorry, threads aren't supported on your platform.\n"));
        exit(EXIT_FAILURE);
    }

    gdk_threads_init();
    mowgli_init();
    chardet_init();

    hook_init();
    hook_associate("quit", quit_cb, 0);

    /* Setup l10n early so we can print localized error messages */
    gtk_set_locale();
    bindtextdomain(PACKAGE_NAME, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
    bindtextdomain(PACKAGE_NAME "-plugins", LOCALEDIR);
    bind_textdomain_codeset(PACKAGE_NAME "-plugins", "UTF-8");
    textdomain(PACKAGE_NAME);

#if !defined(_WIN32) && defined(USE_EGGSM)
    egg_set_desktop_file(AUDACIOUS_DESKTOP_FILE);
#endif
    aud_init_paths();
    aud_make_user_dir();

    gtk_rc_add_default_file(aud_paths[BMP_PATH_GTKRC_FILE]);

    parse_cmd_line_options(&argc, &argv);

    if (options.no_log == FALSE)
        aud_setup_logger();

    g_message("Initializing Gtk+");
    if (!gtk_init_check(&argc, &argv))
    {                           /* XXX */
        /* GTK check failed, and no arguments passed to indicate
           that user is intending to only remote control a running
           session */
        g_printerr(_("%s: Unable to open display, exiting.\n"), argv[0]);
        exit(EXIT_FAILURE);
    }

    g_message("Loading configuration");
    aud_config_load();

    g_message("Initializing signal handlers");
    signal_handlers_init();

    g_message("Handling commandline options, part #1");
    handle_cmd_line_options_first();

    output_init ();

#ifdef USE_DBUS
    g_message("Initializing D-Bus");
    init_dbus();
    init_playback_hooks();
#endif

    g_message("Initializing plugin subsystems...");
    plugin_system_init();

    g_message("Populating included interfaces");
    ui_populate_headless_interface();

    /* Check if user wants to list available interfaces */
    if (!g_ascii_strcasecmp(options.interface, "list"))
    {
        g_print(_("Available interfaces:\n\n"));
        interface_foreach(print_interface_info, NULL);
        g_print("\n");
        plugin_system_cleanup();
        exit(EXIT_SUCCESS);
    }

    playlist_system_init();
    init_equalizer ();

    g_message("Handling commandline options, part #2");
    handle_cmd_line_options();

    g_message("Registering interface hooks");
    register_interface_hooks();

#ifndef NOT_ALPHA_RELEASE
    g_message("Displaying unsupported version warning.");
    ui_display_unsupported_version_warning();
#endif

    g_message("Selecting interface %s", options.interface);
    i = interface_get(options.interface);

    if (i != NULL)
    {
        g_message("Running interface %s@%p", options.interface, (void *) i);
        interface_run(i);
    }
    else
    {
        g_print("%s: unable to launch selected interface %s\n", argv[0], options.interface);
        return EXIT_FAILURE;
    }

    aud_quit();
    return EXIT_SUCCESS;
}
