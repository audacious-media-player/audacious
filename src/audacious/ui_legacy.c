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

#include "main.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <time.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#ifdef USE_SAMPLERATE
#  include <samplerate.h>
#endif

#include "platform/smartinclude.h"

#include "configdb.h"
#include "vfs.h"

#include "auddrct.h"
#include "build_stamp.h"
#include "dnd.h"
#include "input.h"
#include "logger.h"
#include "output.h"
#include "playback.h"
#include "playlist.h"
#include "pluginenum.h"
#include "signals.h"
#include "legacy/ui_skin.h"
#include "legacy/ui_equalizer.h"
#include "ui_fileinfo.h"
#include "legacy/ui_hints.h"
#include "legacy/ui_main.h"
#include "legacy/ui_manager.h"
#include "legacy/ui_playlist.h"
#include "ui_preferences.h"
#include "legacy/ui_skinselector.h"
#include "util.h"

#include "libSAD.h"
#ifdef USE_EGGSM
#include "eggsmclient.h"
#include "eggdesktopfile.h"
#endif

#include "icons-stock.h"

#include "ui_new.h"

static void
resume_playback_on_startup(void)
{
    gint i;

    if (!cfg.resume_playback_on_startup ||
        cfg.resume_playback_on_startup_time == -1 ||
        playlist_get_length(playlist_get_active()) <= 0)
        return;

    while (gtk_events_pending()) gtk_main_iteration();

    playback_initiate();

    /* Busy wait; loop is fairly tight to minimize duration of
     * "frozen" GUI. Feel free to tune. --chainsaw */
    for (i = 0; i < 20; i++)
    {
        g_usleep(1000);
        if (!ip_data.playing)
            break;
    }
    playback_seek(cfg.resume_playback_on_startup_time / 1000);
}

static void
run_load_skin_error_dialog(const gchar * skin_path)
{
    const gchar *markup =
        N_("<b><big>Unable to load skin.</big></b>\n"
           "\n"
           "Check that skin at '%s' is usable and default skin is properly "
           "installed at '%s'\n");

    GtkWidget *dialog =
        gtk_message_dialog_new_with_markup(NULL,
                                           GTK_DIALOG_MODAL,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_CLOSE,
                                           _(markup),
                                           skin_path,
                                           BMP_DEFAULT_SKIN_PATH);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

#ifdef GDK_WINDOWING_QUARTZ
static void
set_dock_icon(void)
{
    GdkPixbuf *icon, *pixbuf;
    CGColorSpaceRef colorspace;
    CGDataProviderRef data_provider;
    CGImageRef image;
    gpointer data;
    gint rowstride, pixbuf_width, pixbuf_height;
    gboolean has_alpha;

    icon = gdk_pixbuf_new_from_xpm_data((const gchar **) audacious_player_xpm);
    pixbuf = gdk_pixbuf_scale_simple(icon, 128, 128, GDK_INTERP_BILINEAR);

    data = gdk_pixbuf_get_pixels(pixbuf);
    pixbuf_width = gdk_pixbuf_get_width(pixbuf);
    pixbuf_height = gdk_pixbuf_get_height(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);

    /* create the colourspace for the CGImage. */
    colorspace = CGColorSpaceCreateDeviceRGB();
    data_provider = CGDataProviderCreateWithData(NULL, data,
                                                 pixbuf_height * rowstride,
                                                 NULL);
    image = CGImageCreate(pixbuf_width, pixbuf_height, 8,
                          has_alpha ? 32 : 24, rowstride, colorspace,
                          has_alpha ? kCGImageAlphaLast : 0,
                          data_provider, NULL, FALSE,
                          kCGRenderingIntentDefault);

    /* release the colourspace and data provider, we have what we want. */
    CGDataProviderRelease(data_provider);
    CGColorSpaceRelease(colorspace);

    /* set the dock tile images */
    SetApplicationDockTileImage(image);

#if 0
    /* and release */
    CGImageRelease(image);
    g_object_unref(icon);
    g_object_unref(pixbuf);
#endif
}
#endif

gboolean
_ui_initialize(void)
{
    g_message("GUI and skin setup");
#ifdef GDK_WINDOWING_QUARTZ
    set_dock_icon();
#endif

    gtk_accel_map_load(aud_paths[BMP_PATH_ACCEL_FILE]);

    if (!init_skins(cfg.skin)) {
        run_load_skin_error_dialog(cfg.skin);
        exit(EXIT_FAILURE);
    }

    GDK_THREADS_ENTER();

    /* this needs to be called after all 3 windows are created and
     * input plugins are setup'ed
     * but not if we're running headless --nenolod
     */
    mainwin_setup_menus();

    gint h_vol[2];
    input_get_volume(&h_vol[0], &h_vol[1]);
    hook_call("volume set", h_vol);

    /* FIXME: delayed, because it deals directly with the plugin
     * interface to set menu items */
    create_prefs_window();

    if (cfg.player_visible)
        mainwin_show(TRUE);
    else if (!cfg.playlist_visible && !cfg.equalizer_visible)
    {
        /* all of the windows are hidden... warn user about this */
        mainwin_show_visibility_warning();
    }

    if (cfg.equalizer_visible)
        equalizerwin_show(TRUE);

    if (cfg.playlist_visible)
        playlistwin_show();

    hint_set_always(cfg.always_on_top);

    resume_playback_on_startup();

    g_message("Entering Gtk+ main loop!");
    gtk_main();

    GDK_THREADS_LEAVE();

    return TRUE;
}

static gboolean
_ui_finalize()
{
    gtk_widget_hide(equalizerwin);
    gtk_widget_hide(playlistwin);
    gtk_widget_hide(mainwin);

    gtk_accel_map_save(aud_paths[BMP_PATH_ACCEL_FILE]);
    gtk_main_quit();

    cleanup_skins();

    return TRUE;
}

static Interface legacy_interface = {
    .id = "legacy",
    .desc = N_("Legacy Interface"),
    .init = _ui_initialize,
    .fini = _ui_finalize
};

void
ui_populate_legacy_interface(void)
{
    interface_register(&legacy_interface);
}

