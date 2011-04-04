/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2011  Audacious development team.
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

#include <gtk/gtk.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudtag/audtag.h>

#include "config.h"

#ifdef USE_DBUS
#include "audctrl.h"
#include "dbus-service.h"
#endif

#ifdef USE_EGGSM
#include "eggdesktopfile.h"
#include "eggsmclient.h"
#endif

#include "audconfig.h"
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
#include "util.h"

/* chardet.c */
void chardet_init (void);

/* mpris-signals.c */
void mpris_signals_init (void);
void mpris_signals_cleanup (void);

/* signals.c */
void signals_init (void);

/* smclient.c */
void smclient_init (void);

#define AUTOSAVE_INTERVAL 300 /* seconds */

static struct {
    gchar **filenames;
    gint session;
    gboolean play, stop, pause, fwd, rew, play_pause, show_jump_box;
    gboolean enqueue, mainwin, remote, activate;
    gboolean enqueue_to_temp;
    gboolean version;
    gchar *previous_session_id;
} options;

static gchar * aud_paths[AUD_PATH_COUNT];

static void make_dirs(void)
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

static void relocate_paths (void)
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

static void init_paths (void)
{
    relocate_paths ();

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

static void parse_options (gint * argc, gchar *** argv)
{
    GOptionContext *context;
    GError *error = NULL;

    memset (& options, 0, sizeof options);
    options.session = -1;

    context = g_option_context_new(_("- play multimedia files"));
    g_option_context_add_main_entries(context, cmd_entries, PACKAGE_NAME);
    g_option_context_add_group(context, gtk_get_option_group(FALSE));
#ifdef USE_EGGSM
    g_option_context_add_group(context, egg_sm_client_get_option_group());
#endif

    if (!g_option_context_parse(context, argc, argv, &error))
    {
        fprintf (stderr,
         _("%s: %s\nTry `%s --help' for more information.\n"), (* argv)[0],
         error->message, (* argv)[0]);
        exit (EXIT_FAILURE);
    }

    g_option_context_free (context);
}

static gboolean get_lock (void)
{
    gchar path[PATH_MAX];
    snprintf (path, sizeof path, "%s" G_DIR_SEPARATOR_S "lock",
     aud_paths[AUD_PATH_USER_DIR]);

    int handle = open (path, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

    if (handle < 0)
    {
        if (errno != EEXIST)
            fprintf (stderr, "Cannot create %s: %s.\n", path, strerror (errno));
        return FALSE;
    }

    close (handle);
    return TRUE;
}

static void release_lock (void)
{
    gchar path[PATH_MAX];
    snprintf (path, sizeof path, "%s" G_DIR_SEPARATOR_S "lock",
     aud_paths[AUD_PATH_USER_DIR]);

    unlink (path);
}

static GList * convert_filenames (void)
{
    if (! options.filenames)
        return NULL;

    gchar * * f = options.filenames;
    GList * list = NULL;
    gchar * cur = g_get_current_dir ();

    for (gint i = 0; f[i]; i ++)
    {
        gchar * uri;

        if (strstr (f[i], "://"))
            uri = g_strdup (f[i]);
        else if (g_path_is_absolute (f[i]))
            uri = filename_to_uri (f[i]);
        else
        {
            gchar * tmp = g_build_filename (cur, f[i], NULL);
            uri = filename_to_uri (tmp);
            g_free (tmp);
        }

        list = g_list_prepend (list, uri);
    }

    g_free (cur);
    return g_list_reverse (list);
}

static void do_remote (void)
{
#ifdef USE_DBUS
    DBusGProxy * session = audacious_get_dbus_proxy ();

    if (session && audacious_remote_is_running (session))
    {
        GList * list = convert_filenames ();

        if (list)
        {
            if (options.enqueue_to_temp)
                audacious_remote_playlist_open_list_to_temp (session, list);
            else if (options.enqueue)
                audacious_remote_playlist_add (session, list);
            else
                audacious_remote_playlist_open_list (session, list);

            g_list_foreach (list, (GFunc) g_free, NULL);
            g_list_free (list);
        }

        if (options.play)
            audacious_remote_play (session);
        if (options.pause)
            audacious_remote_pause (session);
        if (options.play_pause)
            audacious_remote_play_pause (session);
        if (options.stop)
            audacious_remote_stop (session);
        if (options.rew)
            audacious_remote_playlist_prev (session);
        if (options.fwd)
            audacious_remote_playlist_next (session);
        if (options.show_jump_box)
            audacious_remote_show_jtf_box (session);
        if (options.activate)
            audacious_remote_activate (session);
        if (options.mainwin)
            audacious_remote_main_win_toggle (session, TRUE);

        exit (EXIT_SUCCESS);
    }
#endif

    GtkWidget * dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_WARNING,
     GTK_BUTTONS_OK_CANCEL, _("Audacious seems to be already running but is "
     "not responding.  You can start another instance of the program, but "
     "please be warned that this can cause data loss.  If Audacious is not "
     "running, you can safely ignore this message.  Press OK to start "
     "Audacious or Cancel to quit."));

    g_signal_connect (dialog, "destroy", (GCallback) gtk_widget_destroyed,
     & dialog);

    if (gtk_dialog_run ((GtkDialog *) dialog) != GTK_RESPONSE_OK)
        exit (EXIT_FAILURE);

    if (dialog)
        gtk_widget_destroy (dialog);
}

static void do_commands (void)
{
    GList * list = convert_filenames ();

    if (list)
    {
        if (options.enqueue_to_temp)
        {
            drct_pl_open_temp_list (list);
            cfg.resume_state = 0;
        }
        else if (options.enqueue)
            drct_pl_add_list (list, -1);
        else
        {
            drct_pl_open_list (list);
            cfg.resume_state = 0;
        }

        g_list_foreach (list, (GFunc) g_free, NULL);
        g_list_free (list);
    }

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

static void init_one (gint * p_argc, gchar * * * p_argv)
{
    init_paths ();
    make_dirs ();

    bindtextdomain (PACKAGE_NAME, aud_paths[AUD_PATH_LOCALE_DIR]);
    bind_textdomain_codeset (PACKAGE_NAME, "UTF-8");
    bindtextdomain (PACKAGE_NAME "-plugins", aud_paths[AUD_PATH_LOCALE_DIR]);
    bind_textdomain_codeset (PACKAGE_NAME "-plugins", "UTF-8");
    textdomain (PACKAGE_NAME);

    mowgli_init ();
    chardet_init ();

    g_thread_init (NULL);
    gdk_threads_init ();
    gdk_threads_enter ();

    gtk_rc_add_default_file (aud_paths[AUD_PATH_GTKRC_FILE]);
    gtk_init (p_argc, p_argv);

#ifdef USE_EGGSM
    egg_sm_client_set_mode (EGG_SM_CLIENT_MODE_NORMAL);
    egg_set_desktop_file (aud_paths[AUD_PATH_DESKTOP_FILE]);
#endif
}

static void init_two (void)
{
    hook_init ();
    tag_init ();

    aud_config_load ();
    tag_set_verbose (cfg.verbose);
    vfs_set_verbose (cfg.verbose);

    eq_init ();
    register_interface_hooks ();

#ifdef HAVE_SIGWAIT
    signals_init ();
#endif
#ifdef USE_EGGSM
    smclient_init ();
#endif

    AUDDBG ("Loading lowlevel plugins.\n");
    start_plugins_one ();

    playlist_init ();
    load_playlists ();

#ifdef USE_DBUS
    init_dbus ();
#endif

    do_commands ();

    AUDDBG ("Loading highlevel plugins.\n");
    start_plugins_two ();

    mpris_signals_init ();
}

static void shut_down (void)
{
    mpris_signals_cleanup ();

    AUDDBG ("Capturing state.\n");
    aud_config_save ();
    save_playlists ();

    AUDDBG ("Unloading highlevel plugins.\n");
    stop_plugins_two ();

    AUDDBG ("Stopping playback.\n");
    if (playback_get_playing ())
        playback_stop ();

    playlist_end ();

    AUDDBG ("Unloading lowlevel plugins.\n");
    stop_plugins_one ();

    AUDDBG ("Saving configuration.\n");
    cfg_db_flush ();

    gdk_threads_leave ();
}

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
    init_one (& argc, & argv);
    parse_options (& argc, & argv);

    if (options.version)
    {
        printf ("%s %s (%s)\n", _("Audacious"), VERSION, BUILDSTAMP);
        return EXIT_SUCCESS;
    }

    if (! get_lock ())
        do_remote (); /* may exit */

    AUDDBG ("No remote session; starting up.\n");
    init_two ();

    AUDDBG ("Startup complete.\n");
    g_timeout_add_seconds (AUTOSAVE_INTERVAL, autosave_cb, NULL);
    hook_associate ("quit", (HookFunction) gtk_main_quit, NULL);

    gtk_main ();

    shut_down ();
    release_lock ();
    return EXIT_SUCCESS;
}
