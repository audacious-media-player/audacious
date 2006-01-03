/*  BMP - Cross-platform multimedia player
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

#include "controlsocket.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "main.h"
#include "equalizer.h"
#include "mainwin.h"
#include "input.h"
#include "libaudcore/playback.h"
#include "playlist.h"
#include "ui_playlist.h"
#include "prefswin.h"
#include "skin.h"
#include "libaudacious/beepctrl.h"


#define CTRLSOCKET_BACKLOG        100
#define CTRLSOCKET_TIMEOUT        100000


static gint session_id = 0;

static gint ctrl_fd = 0;
static gchar *socket_name = NULL;

static gpointer ctrlsocket_func(gpointer);
static GThread *ctrlsocket_thread;

static GList *packet_list = NULL;
static GMutex *packet_list_mutex = NULL;

static gboolean started = FALSE;
static gboolean going = TRUE;
static GCond *start_cond = NULL;
static GMutex *status_mutex = NULL;


static void
ctrlsocket_start_thread(void)
{
    start_cond = g_cond_new();
    status_mutex = g_mutex_new();
    packet_list_mutex = g_mutex_new();

    ctrlsocket_thread = g_thread_create(ctrlsocket_func, NULL, TRUE, NULL);
}

gboolean
ctrlsocket_setup(void)
{
    struct sockaddr_un saddr;
    gint i;
    gint fd;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        g_critical("ctrlsocket_setup(): Failed to open socket: %s",
                   strerror(errno));
        return FALSE;
    }

    for (i = 0;; i++) {
        saddr.sun_family = AF_UNIX;
        g_snprintf(saddr.sun_path, sizeof(saddr.sun_path),
                   "%s/%s_%s.%d", g_get_tmp_dir(),
                   CTRLSOCKET_NAME, g_get_user_name(), i);

        if (xmms_remote_is_running(i)) {
            if (cfg.allow_multiple_instances)
                continue;
            break;
        }

        if ((unlink(saddr.sun_path) == -1) && errno != ENOENT) {
            g_critical
                ("ctrlsocket_setup(): Failed to unlink %s (Error: %s)",
                 saddr.sun_path, strerror(errno));
            break;
        }

        if (bind(fd, (struct sockaddr *) &saddr, sizeof(saddr)) == -1) {
            g_critical
                ("ctrlsocket_setup(): Failed to assign %s to a socket (Error: %s)",
                 saddr.sun_path, strerror(errno));
            break;
        }

        listen(fd, CTRLSOCKET_BACKLOG);

        socket_name = g_strdup(saddr.sun_path);
        ctrl_fd = fd;
        session_id = i;
        going = TRUE;

        ctrlsocket_start_thread();

        return TRUE;
    }

    close(fd);

    return FALSE;
}

gint
ctrlsocket_get_session_id(void)
{
    return session_id;
}

void
ctrlsocket_cleanup(void)
{
    if (ctrl_fd) {
        g_mutex_lock(status_mutex);
        going = FALSE;
        g_cond_signal(start_cond);
        g_mutex_unlock(status_mutex);

        /* wait for ctrlsocket_thread to terminate */
        g_thread_join(ctrlsocket_thread);

        /* close and remove socket */
        close(ctrl_fd);
        ctrl_fd = 0;
        unlink(socket_name);
        g_free(socket_name);

        g_cond_free(start_cond);
        g_mutex_free(status_mutex);
        g_mutex_free(packet_list_mutex);
    }
}

void
ctrlsocket_start(void)
{
    /* tell control socket thread to go 'live' i.e. start handling
     * packets  */
    g_mutex_lock(status_mutex);
    started = TRUE;
    g_cond_signal(start_cond);
    g_mutex_unlock(status_mutex);
}

static gint
write_all(gint fd, gconstpointer buf, size_t count)
{
    size_t left = count;
    GTimer *timer;
    gulong usec;
    gint written;

    timer = g_timer_new();

    do {
        if ((written = write(fd, buf, left)) < 0) {
            count = -1;
            break;
        }
        left -= written;
        buf = (gchar *) buf + written;
        g_timer_elapsed(timer, &usec);
    }
    while (left > 0 && usec <= CTRLSOCKET_IO_TIMEOUT_USEC);

    g_timer_destroy(timer);
    return count - left;
}

static void
ctrl_write_packet(gint fd, gpointer data, gint length)
{
    ServerPktHeader pkthdr;

    pkthdr.version = XMMS_PROTOCOL_VERSION;
    pkthdr.data_length = length;
    if (write_all(fd, &pkthdr, sizeof(ServerPktHeader)) < sizeof(pkthdr))
        return;
    if (data && length > 0)
        write_all(fd, data, length);
}

static void
ctrl_write_gint(gint fd, gint val)
{
    ctrl_write_packet(fd, &val, sizeof(gint));
}

static void
ctrl_write_gfloat(gint fd, gfloat val)
{
    ctrl_write_packet(fd, &val, sizeof(gfloat));
}

static void
ctrl_write_gboolean(gint fd, gboolean bool)
{
    ctrl_write_packet(fd, &bool, sizeof(gboolean));
}

static void
ctrl_write_string(gint fd, gchar * string)
{
    ctrl_write_packet(fd, string, string ? strlen(string) + 1 : 0);
}

static void
ctrl_ack_packet(PacketNode * pkt)
{
    ctrl_write_packet(pkt->fd, NULL, 0);
    close(pkt->fd);
    if (pkt->data)
        g_free(pkt->data);
    g_free(pkt);
}

static gint
read_all(gint fd, gpointer buf, size_t count)
{
    size_t left = count;
    GTimer *timer;
    gulong usec;
    gint r;

    timer = g_timer_new();

    do {
        if ((r = read(fd, buf, left)) < 0) {
            count = -1;
            break;
        }
        left -= r;
        buf = (gchar *) buf + r;
        g_timer_elapsed(timer, &usec);
    }
    while (left > 0 && usec <= CTRLSOCKET_IO_TIMEOUT_USEC);

    g_timer_destroy(timer);
    return count - left;
}

static gboolean
ctrlsocket_is_going(void)
{
    gboolean result;

    g_mutex_lock(status_mutex);
    result = going;
    g_mutex_unlock(status_mutex);

    return result;
}

static gpointer
ctrlsocket_func(gpointer arg)
{
    fd_set set;
    struct timeval tv;
    struct sockaddr_un saddr;
    gint fd, b, i;
    gint info[3];
    gint32 v[2];
    PacketNode *pkt;
    socklen_t len;
    gfloat fval[11];

    g_mutex_lock(status_mutex);
    while (!started && going)
        g_cond_wait(start_cond, status_mutex);
    g_mutex_unlock(status_mutex);

    while (ctrlsocket_is_going()) {
        FD_ZERO(&set);
        FD_SET(ctrl_fd, &set);
        tv.tv_sec = 0;
        tv.tv_usec = CTRLSOCKET_TIMEOUT;
        len = sizeof(saddr);
        if (select(ctrl_fd + 1, &set, NULL, NULL, &tv) <= 0)
            continue;
        if ((fd = accept(ctrl_fd, (struct sockaddr *) &saddr, &len)) == -1)
            continue;

        pkt = g_new0(PacketNode, 1);
        if (read_all(fd, &pkt->hdr, sizeof(ClientPktHeader))
            < sizeof(ClientPktHeader)) {
            g_free(pkt);
            continue;
        }

        if (pkt->hdr.data_length) {
            size_t data_length = pkt->hdr.data_length;
            pkt->data = g_malloc0(data_length);
            if (read_all(fd, pkt->data, data_length) < data_length) {
                g_free(pkt->data);
                g_free(pkt);
                g_warning("ctrlsocket_func(): Incomplete data packet dropped");
                continue;
            }
        }

        pkt->fd = fd;
        switch (pkt->hdr.command) {
        case CMD_GET_VERSION:
            ctrl_write_gint(pkt->fd, 0x09a3);
            ctrl_ack_packet(pkt);
            break;
        case CMD_IS_PLAYING:
            ctrl_write_gboolean(pkt->fd, bmp_playback_get_playing());
            ctrl_ack_packet(pkt);
            break;
        case CMD_IS_PAUSED:
            ctrl_write_gboolean(pkt->fd, bmp_playback_get_paused());
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_PLAYLIST_POS:
            ctrl_write_gint(pkt->fd, playlist_get_position());
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_PLAYLIST_LENGTH:
            ctrl_write_gint(pkt->fd, playlist_get_length());
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_PLAYQUEUE_LENGTH:
            ctrl_write_gint(pkt->fd, playlist_queue_get_length());
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_OUTPUT_TIME:
            if (bmp_playback_get_playing())
                ctrl_write_gint(pkt->fd, bmp_playback_get_time());
            else
                ctrl_write_gint(pkt->fd, 0);
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_VOLUME:
            input_get_volume(&v[0], &v[1]);
            ctrl_write_packet(pkt->fd, v, sizeof(v));
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_BALANCE:
            input_get_volume(&v[0], &v[1]);
            if (v[0] < 0 || v[1] < 0)
                b = 0;
            else if (v[0] > v[1])
                b = -100 + ((v[1] * 100) / v[0]);
            else if (v[1] > v[0])
                b = 100 - ((v[0] * 100) / v[1]);
            else
                b = 0;
            ctrl_write_gint(pkt->fd, b);
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_SKIN:
            ctrl_write_string(pkt->fd, bmp_active_skin->path);
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_PLAYLIST_FILE:
            if (pkt->data) {
                gchar *filename;
                filename = playlist_get_filename(*((guint32 *) pkt->data));
                ctrl_write_string(pkt->fd, filename);
                g_free(filename);
            }
            else
                ctrl_write_string(pkt->fd, NULL);
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_PLAYLIST_TITLE:
            if (pkt->data) {
                gchar *title;
                title = playlist_get_songtitle(*((guint32 *) pkt->data));
                ctrl_write_string(pkt->fd, title);
                g_free(title);
            }
            else
                ctrl_write_string(pkt->fd, NULL);
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_PLAYLIST_TIME:
            if (pkt->data)
                ctrl_write_gint(pkt->fd,
                                playlist_get_songtime(*
                                                      ((guint32 *) pkt->
                                                       data)));
            else
                ctrl_write_gint(pkt->fd, -1);

            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_INFO:
            playback_get_sample_params(&info[0], &info[1], &info[2]);
            ctrl_write_packet(pkt->fd, info, 3 * sizeof(gint));
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_EQ_DATA:
        case CMD_SET_EQ_DATA:
            /* obsolete */
            ctrl_ack_packet(pkt);
            break;
        case CMD_PING:
            ctrl_ack_packet(pkt);
            break;
        case CMD_PLAYLIST_ADD:
            if (pkt->data) {
                guint32 *dataptr = pkt->data;
                while ((len = *dataptr) > 0) {
                    gchar *filename;

                    dataptr++;
                    filename = g_malloc0(len);
                    memcpy(filename, dataptr, len);

                    GDK_THREADS_ENTER();
                    playlist_add_url(filename);
                    GDK_THREADS_LEAVE();

                    g_free(filename);
                    dataptr += (len + 3) / 4;
                }
            }
            ctrl_ack_packet(pkt);
            break;
        case CMD_PLAYLIST_ADD_URL_STRING:
            GDK_THREADS_ENTER();
            playlist_add_url(pkt->data);
            GDK_THREADS_LEAVE();

            ctrl_ack_packet(pkt);
            break;
        case CMD_PLAYLIST_INS_URL_STRING:
            if (pkt->data) {
                gint pos = *(gint *) pkt->data;
                gchar *ptr = pkt->data;
                ptr += sizeof(gint);
                playlist_ins_url(ptr, pos);
            }
            ctrl_ack_packet(pkt);
            break;
        case CMD_PLAYLIST_DELETE:
            GDK_THREADS_ENTER();
            playlist_delete_index(*((guint32 *) pkt->data));
            GDK_THREADS_LEAVE();
            ctrl_ack_packet(pkt);
            break;
        case CMD_PLAYLIST_CLEAR:
            GDK_THREADS_ENTER();
            playlist_clear();
            mainwin_clear_song_info();
            mainwin_set_info_text();
            GDK_THREADS_LEAVE();
            ctrl_ack_packet(pkt);
            break;
        case CMD_IS_MAIN_WIN:
            ctrl_write_gboolean(pkt->fd, cfg.player_visible);
            ctrl_ack_packet(pkt);
            break;
        case CMD_IS_PL_WIN:
            ctrl_write_gboolean(pkt->fd, cfg.playlist_visible);
            ctrl_ack_packet(pkt);
            break;
        case CMD_IS_EQ_WIN:
            ctrl_write_gboolean(pkt->fd, cfg.equalizer_visible);
            ctrl_ack_packet(pkt);
            break;
        case CMD_IS_REPEAT:
            ctrl_write_gboolean(pkt->fd, cfg.repeat);
            ctrl_ack_packet(pkt);
            break;
        case CMD_IS_SHUFFLE:
            ctrl_write_gboolean(pkt->fd, cfg.shuffle);
            ctrl_ack_packet(pkt);
            break;
        case CMD_IS_ADVANCE:
            ctrl_write_gboolean(pkt->fd, !cfg.no_playlist_advance);
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_EQ:
            fval[0] = equalizerwin_get_preamp();
            for (i = 0; i < 10; i++)
                fval[i + 1] = equalizerwin_get_band(i);
            ctrl_write_packet(pkt->fd, fval, 11 * sizeof(gfloat));
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_EQ_PREAMP:
            ctrl_write_gfloat(pkt->fd, equalizerwin_get_preamp());
            ctrl_ack_packet(pkt);
            break;
        case CMD_GET_EQ_BAND:
            i = *((guint32 *) pkt->data);
            ctrl_write_gfloat(pkt->fd, equalizerwin_get_band(i));
            ctrl_ack_packet(pkt);
            break;
        default:
            g_mutex_lock(packet_list_mutex);
            packet_list = g_list_append(packet_list, pkt);
            ctrl_write_packet(pkt->fd, NULL, 0);
            close(pkt->fd);
            g_mutex_unlock(packet_list_mutex);
            break;
        }
    }
    g_thread_exit(NULL);

    /* Used to suppress GCC warnings. Sometimes you'd wish C has
       native threading support :p */
    return NULL;
}

void
ctrlsocket_check(void)
{
    GList *pkt_list, *next;
    PacketNode *pkt;
    gpointer data;
    guint32 v[2], i, num;
    gboolean tbool;
    gfloat *fval, f;

    g_mutex_lock(packet_list_mutex);
    for (pkt_list = packet_list; pkt_list; pkt_list = next) {
        pkt = pkt_list->data;
        data = pkt->data;

        switch (pkt->hdr.command) {
        case CMD_PLAY:
            if (bmp_playback_get_paused())
                bmp_playback_pause();
            else if (playlist_get_length())
                bmp_playback_initiate();
            else
                mainwin_eject_pushed();
            break;
        case CMD_PAUSE:
            bmp_playback_pause();
            break;
        case CMD_STOP:
            bmp_playback_stop();
            mainwin_clear_song_info();
            break;
        case CMD_PLAY_PAUSE:
            if (bmp_playback_get_playing())
                bmp_playback_pause();
            else
                bmp_playback_initiate();
            break;
        case CMD_PLAYQUEUE_ADD:
            num = *((guint32 *) data);
            if (num < playlist_get_length())
                playlist_queue_position(num);
            break;
        case CMD_PLAYQUEUE_REMOVE:
            num = *((guint32 *) data);
            if (num < playlist_get_length())
                playlist_queue_remove(num);
            break;
        case CMD_SET_PLAYLIST_POS:
            num = *((guint32 *) data);
            if (num < playlist_get_length())
                playlist_set_position(num);
            break;
        case CMD_JUMP_TO_TIME:
            num = *((guint32 *) data);
            if (playlist_get_current_length() > 0 &&
                num < playlist_get_current_length())
                bmp_playback_seek(num / 1000);
            break;
        case CMD_SET_VOLUME:
            v[0] = ((guint32 *) data)[0];
            v[1] = ((guint32 *) data)[1];
            for (i = 0; i < 2; i++) {
                if (v[i] > 100)
                    v[i] = 100;
            }
            input_set_volume(v[0], v[1]);
            break;
        case CMD_SET_SKIN:
            bmp_active_skin_load(data);
            break;
        case CMD_PL_WIN_TOGGLE:
            tbool = *((gboolean *) data);
            if (tbool)
                playlistwin_show();
            else
                playlistwin_hide();
            break;
        case CMD_EQ_WIN_TOGGLE:
            tbool = *((gboolean *) data);
            equalizerwin_show(!!tbool);
            break;
        case CMD_SHOW_PREFS_BOX:
            show_prefs_window();
            break;
        case CMD_TOGGLE_AOT:
            tbool = *((gboolean *) data);
            mainwin_set_always_on_top(tbool);
            break;
        case CMD_SHOW_ABOUT_BOX:
            break;
        case CMD_EJECT:
            mainwin_eject_pushed();
            break;
        case CMD_PLAYLIST_PREV:
            playlist_prev();
            break;
        case CMD_PLAYLIST_NEXT:
            playlist_next();
            break;
        case CMD_TOGGLE_REPEAT:
            mainwin_repeat_pushed(!cfg.repeat);
            break;
        case CMD_TOGGLE_SHUFFLE:
            mainwin_shuffle_pushed(!cfg.shuffle);
            break;
        case CMD_TOGGLE_ADVANCE:
            /* FIXME: to be implemented */
            break;
        case CMD_MAIN_WIN_TOGGLE:
            tbool = *((gboolean *) data);
            mainwin_show(!!tbool);
            break;
        case CMD_SET_EQ:
            if (pkt->hdr.data_length >= 11 * sizeof(gfloat)) {
                fval = (gfloat *) data;
                equalizerwin_set_preamp(fval[0]);
                for (i = 0; i < 10; i++)
                    equalizerwin_set_band(i, fval[i + 1]);
            }
            break;
        case CMD_SET_EQ_PREAMP:
            f = *((gfloat *) data);
            equalizerwin_set_preamp(f);
            break;
        case CMD_SET_EQ_BAND:
            if (pkt->hdr.data_length >= sizeof(gint) + sizeof(gfloat)) {
                i = *((gint *) data);
                f = *((gfloat *) ((gchar *) data + sizeof(gint)));
                equalizerwin_set_band(i, f);
            }
            break;
        case CMD_QUIT:
            /*
             * We unlock the packet_list_mutex to
             * avoid that cleanup_ctrlsocket() can
             * deadlock, mainwin_quit_cb() will
             * never return anyway, so this will
             * work ok.
             */
            g_mutex_unlock(packet_list_mutex);
            mainwin_quit_cb();
            break;
	case CMD_ACTIVATE:
	    gtk_window_present(GTK_WINDOW(mainwin));
	    break;
        default:
            g_message("Unknown socket command received");
            break;
        }
        next = g_list_next(pkt_list);
        packet_list = g_list_remove_link(packet_list, pkt_list);
        g_list_free_1(pkt_list);
        if (pkt->data)
            g_free(pkt->data);
        g_free(pkt);
    }
    g_mutex_unlock(packet_list_mutex);
}
