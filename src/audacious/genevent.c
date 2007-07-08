/*  Audacious - Cross-platform multimedia platform.
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
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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

#include "dnd.h"
#include "effect.h"
#include "general.h"
#include "hints.h"
#include "input.h"
#include "logger.h"
#include "output.h"
#include "playback.h"
#include "playlist.h"
#include "pluginenum.h"
#include "ui_main.h"
#include "ui_playlist.h"
#include "ui_skinned_textbox.h"
#include "ui_skinned_number.h"
#include "util.h"
#include "visualization.h"
#include "vfs.h"

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
            ui_skinned_number_set_number(mainwin_10min_num, timeleft / 600);
            ui_skinned_number_set_number(mainwin_min_num, (timeleft / 60) % 10);
            ui_skinned_number_set_number(mainwin_10sec_num, (timeleft / 10) % 6);
            ui_skinned_number_set_number(mainwin_sec_num, timeleft % 10);
        }

        if (mainwin_sposition != NULL && !mainwin_sposition->hs_pressed) {
            gchar *time_str;

            time_str = g_strdup_printf("%2.2d", timeleft / 60);
            ui_skinned_textbox_set_text(mainwin_stime_min, time_str);
            g_free(time_str);

            time_str = g_strdup_printf("%2.2d", timeleft % 60);
            ui_skinned_textbox_set_text(mainwin_stime_sec, time_str);
            g_free(time_str);
        }

        playlistwin_set_time(timeleft * 1000, 0, TIMER_ELAPSED);
    }

    if (!cfg.pause_between_songs ||
        g_timer_elapsed(pause_timer, NULL) >= cfg.pause_between_songs_time)
    {
	Playlist *playlist = playlist_get_active();

        GDK_THREADS_ENTER();
        playlist_eof_reached(playlist);
        GDK_THREADS_LEAVE();

        waiting = FALSE;
    }

    return waiting;
}

gint
audcore_generic_events(void)
{
    gint time = 0;

    if (playback_get_playing()) {
        time = playback_get_time();

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

