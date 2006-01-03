/*  Audacious - Cross-platform multimedia platform.
 *  Copyright (C) 2005  Audacious development team.
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "libaudacious/configdb.h"
#include "libaudacious/beepctrl.h"
#include "libaudacious/util.h"
#include "libaudacious/vfs.h"

#include "controlsocket.h"
#include "dnd.h"
#include "dock.h"
#include "effect.h"
#include "equalizer.h"
#include "general.h"
#include "hints.h"
#include "input.h"
#include "logger.h"
#include "mainwin.h"
#include "output.h"
#include "libaudcore/playback.h"
#include "playlist.h"
#include "ui_playlist.h"
#include "pluginenum.h"
#include "prefswin.h"
#include "skin.h"
#include "skinwin.h"
#include "util.h"
#include "visualization.h"

gboolean ev_waiting = FALSE;

static gboolean
idle_func_change_song(gboolean waiting)
{
    static GTimer *pause_timer = NULL;

    if (!pause_timer)
        pause_timer = g_timer_new();

    if (cfg.pause_between_songs) {
        gint timeleft;

        if (!waiting) {
            g_timer_start(pause_timer);
            waiting = TRUE;
        }

        timeleft = cfg.pause_between_songs_time -
            (gint) g_timer_elapsed(pause_timer, NULL);

        if (mainwin_10min_num != NULL) {
            number_set_number(mainwin_10min_num, timeleft / 600);
            number_set_number(mainwin_min_num, (timeleft / 60) % 10);
            number_set_number(mainwin_10sec_num, (timeleft / 10) % 6);
            number_set_number(mainwin_sec_num, timeleft % 10);
        }

        if (mainwin_sposition != NULL && !mainwin_sposition->hs_pressed) {
            gchar time_str[5];

            g_snprintf(time_str, sizeof(time_str), "%2.2d", timeleft / 60);
            textbox_set_text(mainwin_stime_min, time_str);

            g_snprintf(time_str, sizeof(time_str), "%2.2d", timeleft % 60);
            textbox_set_text(mainwin_stime_sec, time_str);
        }

        playlistwin_set_time(timeleft * 1000, 0, TIMER_ELAPSED);
    }

    if (!cfg.pause_between_songs ||
        g_timer_elapsed(pause_timer, NULL) >= cfg.pause_between_songs_time) {

        GDK_THREADS_ENTER();
        playlist_eof_reached();
        GDK_THREADS_LEAVE();

        waiting = FALSE;
    }

    return waiting;
}

gint
audcore_generic_events(void)
{
    gint time = 0;

    ctrlsocket_check();

    if (bmp_playback_get_playing()) {
        time = bmp_playback_get_time();

        switch (time) {
        case -1:
            /* no song playing */
            ev_waiting = idle_func_change_song(ev_waiting);
            break;

        default:
            ev_waiting = FALSE;
        }
    }

    return time;
}

