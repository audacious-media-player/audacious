/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2010  Audacious development team.
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

#include <errno.h>
#include <limits.h>
 
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gtk/gtk.h>

#include "main.h"

#include <glib/gprintf.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudtag/audtag.h>

#ifdef USE_DBUS
#  include "dbus-service.h"
#  include "audctrl.h"
#endif

#ifdef USE_EGGSM
#include "eggsmclient.h"
#include "eggdesktopfile.h"
#endif

#include "audconfig.h"
#include "chardet.h"
#include "configdb.h"
#include "debug.h"
#include "drct.h"
#include "equalizer.h"
#include "glib-compat.h"
#include "i18n.h"
#include "interface.h"
#include "misc.h"
#include "playback.h"
#include "playlist.h"
#include "plugins.h"
#include "signals.h"
#include "util.h"

#define AUTOSAVE_INTERVAL 300 /* seconds */

static const gchar *application_name = N_("Audacious");

struct _AudCmdLineOpt
{
    gchar **filenames;
    gint session;
    gboolean play, stop, pause, fwd, rew, play_pause, show_jump_box;
    gboolean enqueue, mainwin, remote, activate;
    gboolean enqueue_to_temp;
    gboolean version;
    gchar *previous_session_id;
};
typedef struct _AudCmdLineOpt AudCmdLineOpt;

static AudCmdLineOpt options;

static gchar * aud_paths[AUD_PATH_COUNT];

#ifdef USE_DBUS
MprisPlayer *mpris;
MprisTrackList *mpris_tracklist;
#endif

static void print_version(void)
{
    g_printf("%s %s (%s)\n", _(application_name), VERSION, BUILDSTAMP);
}

static void aud_make_user_dir(void)
{
#ifdef S_IRGRP
    const mode_t mode755 = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
#else
    const mode_t mode755 = S_IRWXU;
#endif

    make_directory(aud_paths[AUD_PATH_USER_DIR], mode755);
    make_directory(aud_paths[AUD_PATH_USER_PLUGIN_DIR], mode755);
    make_directory(aud_paths[AUD_PATH_PLAYLISTS_DIR], mode755);
}

static void aud_free_paths(void)
{
    gint i;

    for (i = 0; i < AUD_PATH_COUNT; i++)
    {
        g_free(aud_paths[i]);
        aud_paths[i] = 0;
    }
}

static void normalize_path (gchar * path)
{
#ifdef _WIN32
    string_replace_char (path, '/', '\\');
#endif
    gint len = strlen (path);
#ifdef _WIN32
    if (len > 3 && path[len - 1] == '\\') /* leave "C:\" */
#else
    if (len > 1 && path[len - 1] == '/') /* leave leading "/" */
#endif
        path[len - 1] = 0;
}

static gchar * last_path_element (gchar * path)
{
    gchar * slash = strrchr (path, G_DIR_SEPARATOR);
    return (slash && slash[1]) ? slash + 1 : NULL;
}

static void strip_path_element (gchar * path, gchar * elem)
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

static void relocate_path (gchar * * pathp, const gchar * old, const gchar * new)
{
    gchar * path = * pathp;
    gint len = strlen (old);

#ifdef _WIN32
    if (strncasecmp (path, old, len))
#else
    if (strncmp (path, old, len))
#endif
    {
        fprintf (stderr, "Failed to relocate a data path.  Falling back to "
         "compile-time path: %s\n", path);
        return;
    }

    * pathp = g_strconcat (new, path + len, NULL);
    g_free (path);
}

static void find_data_paths (void)
{
    /* Start with the paths hard coded at compile time. */
    aud_paths[AUD_PATH_BIN_DIR] = g_strdup (HARDCODE_BINDIR);
    aud_paths[AUD_PATH_DATA_DIR] = g_strdup (HARDCODE_DATADIR);
    aud_paths[AUD_PATH_PLUGIN_DIR] = g_strdup (HARDCODE_PLUGINDIR);
    aud_paths[AUD_PATH_LOCALE_DIR] = g_strdup (HARDCODE_LOCALEDIR);
    aud_paths[AUD_PATH_DESKTOP_FILE] = g_strdup (HARDCODE_DESKTOPFILE);
    aud_paths[AUD_PATH_ICON_FILE] = g_strdup (HARDCODE_ICONFILE);
    normalize_path (aud_paths[AUD_PATH_BIN_DIR]);
    normalize_path (aud_paths[AUD_PATH_DATA_DIR]);
    normalize_path (aud_paths[AUD_PATH_PLUGIN_DIR]);
    normalize_path (aud_paths[AUD_PATH_LOCALE_DIR]);
    normalize_path (aud_paths[AUD_PATH_DESKTOP_FILE]);
    normalize_path (aud_paths[AUD_PATH_ICON_FILE]);

    /* Compare the compile-time path to the executable and the actual path to
     * see if we have been moved. */
    gchar * old = g_strdup (aud_paths[AUD_PATH_BIN_DIR]);
    gchar * new = get_path_to_self ();
    if (! new)
    {
ERR:
        g_free (old);
        g_free (new);
        return;
    }
    normalize_path (new);

    /* Strip the name of the executable file, leaving the path. */
    gchar * base = last_path_element (new);
    if (! base)
        goto ERR;
    strip_path_element (new, base);

    /* Strip innermost folder names from both paths as long as they match.  This
     * leaves a compile-time prefix and a run-time one to replace it with. */
    gchar * a, * b;
    while ((a = last_path_element (old)) && (b = last_path_element (new)) &&
#ifdef _WIN32
     ! strcasecmp (a, b))
#else
     ! strcmp (a, b))
#endif
    {
        strip_path_element (old, a);
        strip_path_element (new, b);
    }

    /* Do the replacements. */
    relocate_path (& aud_paths[AUD_PATH_BIN_DIR], old, new);
    relocate_path (& aud_paths[AUD_PATH_DATA_DIR], old, new);
    relocate_path (& aud_paths[AUD_PATH_PLUGIN_DIR], old, new);
    relocate_path (& aud_paths[AUD_PATH_LOCALE_DIR], old, new);
    relocate_path (& aud_paths[AUD_PATH_DESKTOP_FILE], old, new);
    relocate_path (& aud_paths[AUD_PATH_ICON_FILE], old, new);
    
    g_free (old);
    g_free (new);
}

static void aud_init_paths (void)
{
    find_data_paths ();

    const gchar * xdg_config_home = g_get_user_config_dir ();
    const gchar * xdg_data_home = g_get_user_data_dir ();

#ifdef _WIN32
    /* Some libraries (libmcs) and plugins (filewriter) use these variables,
     * which are generally not set on Windows. */
    g_setenv ("HOME", g_get_home_dir (), TRUE);
    g_setenv ("XDG_CONFIG_HOME", xdg_config_home, TRUE);
    g_setenv ("XDG_DATA_HOME", xdg_data_home, TRUE);
    g_setenv ("XDG_CACHE_HOME", g_get_user_cache_dir (), TRUE);
#endif

    aud_paths[AUD_PATH_USER_DIR] = g_build_filename(xdg_config_home, "audacious", NULL);
    aud_paths[AUD_PATH_USER_PLUGIN_DIR] = g_build_filename(xdg_data_home, "audacious", "Plugins", NULL);
    aud_paths[AUD_PATH_PLAYLISTS_DIR] = g_build_filename(aud_paths[AUD_PATH_USER_DIR], "playlists", NULL);
    aud_paths[AUD_PATH_PLAYLIST_FILE] = g_build_filename(aud_paths[AUD_PATH_USER_DIR], "playlist.xspf", NULL);
    aud_paths[AUD_PATH_GTKRC_FILE] = g_build_filename(aud_paths[AUD_PATH_USER_DIR], "gtkrc", NULL);

    for (gint i = 0; i < AUD_PATH_COUNT; i ++)
        AUDDBG ("Data path: %s\n", aud_paths[i]);

    g_atexit(aud_free_paths);
}

const gchar * get_path (gint id)
{
    g_return_val_if_fail (id >= 0 && id < AUD_PATH_COUNT, NULL);
    return aud_paths[id];
}

static GOptionEntry cmd_entries[] = {
    {"rew", 'r', 0, G_OPTION_ARG_NONE, &options.rew, N_("Skip backwards in playlist"), NULL},
    {"play", 'p', 0, G_OPTION_ARG_NONE, &options.play, N_("Start playing current playlist"), NULL},
    {"pause", 'u', 0, G_OPTION_ARG_NONE, &options.pause, N_("Pause current song"), NULL},
    {"stop", 's', 0, G_OPTION_ARG_NONE, &options.stop, N_("Stop current song"), NULL},
    {"play-pause", 't', 0, G_OPTION_ARG_NONE, &options.play_pause, N_("Pause if playing, play otherwise"), NULL},
    {"fwd", 'f', 0, G_OPTION_ARG_NONE, &options.fwd, N_("Skip forward in playlist"), NULL},
    {"show-jump-box", 'j', 0, G_OPTION_ARG_NONE, &options.show_jump_box, N_("Display Jump to File dialog"), NULL},
    {"enqueue", 'e', 0, G_OPTION_ARG_NONE, &options.enqueue, N_("Add files to the playlist"), NULL},
    {"enqueue-to-temp", 'E', 0, G_OPTION_ARG_NONE, &options.enqueue_to_temp, N_("Add new files to a temporary playlist"), NULL},
    {"show-main-window", 'm', 0, G_OPTION_ARG_NONE, &options.mainwin, N_("Display the main window"), NULL},
    {"activate", 'a', 0, G_OPTION_ARG_NONE, &options.activate, N_("Display all open Audacious windows"), NULL},
    {"version", 'v', 0, G_OPTION_ARG_NONE, &options.version, N_("Show version"), NULL},
    {"verbose", 'V', 0, G_OPTION_ARG_NONE, &cfg.verbose, N_("Print debugging messages"), NULL},
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &options.filenames, N_("FILE..."), NULL},
    {NULL},
};

static void parse_cmd_line_options(gint * argc, gchar *** argv)
{
    GOptionContext *context;
    GError *error = NULL;

    memset(&options, '\0', sizeof(AudCmdLineOpt));
    options.session = -1;

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

    g_option_context_free (context);
}

#ifndef USE_DBUS
static void set_lock_file (gboolean lock)
{
    gchar path[PATH_MAX];
    snprintf (path, sizeof path, "%s" G_DIR_SEPARATOR_S "lock",
     aud_paths[AUD_PATH_USER_DIR]);
     
    if (lock)
    {
        int handle = open (path, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
        if (handle < 0 && errno == EEXIST)
        {
            GtkWidget * win = gtk_message_dialog_new (NULL, 0,
             GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
             _("Running multiple instances of Audacious can lead to corrupted "
             "configuration files.  If Audacious is not already running, or if "
             "you want to run another instance anyway, please delete the file "
             "%s and run Audacious again.  Audacious will now close."), path);
            g_signal_connect (win, "response", (GCallback) gtk_widget_destroy,
             NULL);
            g_signal_connect (win, "destroy", (GCallback) gtk_main_quit, NULL);
            gtk_widget_show (win);
            gtk_main ();
            exit (EXIT_FAILURE);
        }
        close (handle);
    }
    else
        unlink (path);
}
#endif

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
        gchar * uri;

        if (strstr (filenames[i], "://"))
            uri = g_strdup (filenames[i]);
        else if (g_path_is_absolute (filenames[i]))
            uri = filename_to_uri (filenames[i]);
        else
        {
            gchar * absolute = g_build_filename (working, filenames[i], NULL);
            uri = filename_to_uri (absolute);
            g_free (absolute);
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
            drct_pl_open_temp_list (fns);
            cfg.resume_state = 0;
        }
        else if (options.enqueue)
            drct_pl_add_list (fns, -1);
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
}

static void handle_cmd_line_options(void)
{
    handle_cmd_line_filenames(FALSE);

    if (cfg.resume_playback_on_startup && cfg.resume_state > 0)
        playback_play (cfg.resume_playback_on_startup_time, cfg.resume_state ==
         2);

    if (options.play || options.play_pause)
    {
        if (! playback_get_playing ())
            playback_play (0, FALSE);
        else if (playback_get_paused ())
            playback_pause ();
    }

    if (options.show_jump_box)
        interface_show_jump_to_track ();
    if (options.mainwin)
        interface_toggle_visibility ();
}

void aud_quit (void)
{
    AUDDBG ("Ending main loop.\n");
    gtk_main_quit ();
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

static gboolean autosave_cb (void * unused)
{
    AUDDBG ("Saving configuration.\n");
    aud_config_save ();
    cfg_db_flush ();
    save_playlists ();
    return TRUE;
}

gint main(gint argc, gchar ** argv)
{
    g_thread_init (NULL);
    gdk_threads_init ();
    gdk_threads_enter ();

    aud_init_paths ();
    aud_make_user_dir ();

    mowgli_init();
    chardet_init();
    tag_init();

    hook_init();
    hook_associate ("quit", (HookFunction) gtk_main_quit, NULL);

    /* Setup l10n early so we can print localized error messages */
    bindtextdomain (PACKAGE_NAME, aud_paths[AUD_PATH_LOCALE_DIR]);
    bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
    bindtextdomain (PACKAGE_NAME "-plugins", aud_paths[AUD_PATH_LOCALE_DIR]);
    bind_textdomain_codeset(PACKAGE_NAME "-plugins", "UTF-8");
    textdomain(PACKAGE_NAME);

#if !defined(_WIN32) && defined(USE_EGGSM)
    egg_set_desktop_file (aud_paths[AUD_PATH_DESKTOP_FILE]);
#endif

    gtk_rc_add_default_file(aud_paths[AUD_PATH_GTKRC_FILE]);

    parse_cmd_line_options(&argc, &argv);
    tag_set_verbose (cfg.verbose);

    if (!gtk_init_check(&argc, &argv))
    {                           /* XXX */
        /* GTK check failed, and no arguments passed to indicate
           that user is intending to only remote control a running
           session */
        g_printerr(_("%s: Unable to open display, exiting.\n"), argv[0]);
        exit(EXIT_FAILURE);
    }

#ifndef USE_DBUS
    AUDDBG ("Locking configuration folder.\n");
    set_lock_file (TRUE);
#endif

    AUDDBG ("Loading configuration.\n");
    aud_config_load();
    atexit (aud_config_free);

    signal_handlers_init();
    handle_cmd_line_options_first();

    AUDDBG ("Initializing core.\n");
    playlist_init ();
    eq_init ();

    AUDDBG ("Loading plugins, stage one.\n");
    start_plugins_one ();

    AUDDBG ("Loading saved state.\n");
    load_playlists ();

#ifdef USE_DBUS
    AUDDBG ("Initializing D-Bus.\n");
    init_dbus();
    init_playback_hooks();
#endif

    handle_cmd_line_options();
    register_interface_hooks();

    AUDDBG ("Loading plugins, stage two.\n");
    start_plugins_two ();

    AUDDBG ("Startup complete.\n");

    g_timeout_add_seconds (AUTOSAVE_INTERVAL, autosave_cb, NULL);
    gtk_main ();

    AUDDBG ("Capturing state.\n");
    aud_config_save ();
    save_playlists ();

    AUDDBG ("Stopping playback.\n");
    if (playback_get_playing ())
        playback_stop ();

    AUDDBG ("Unloading plugins.\n");
    stop_plugins ();

    AUDDBG ("Saving configuration.\n");
    cfg_db_flush ();

    AUDDBG ("Shutting down core.\n");
    playlist_end ();

#ifndef USE_DBUS
    AUDDBG ("Unlocking configuration folder.\n");
    set_lock_file (FALSE);
#endif

    gdk_threads_leave ();
    return EXIT_SUCCESS;
}
