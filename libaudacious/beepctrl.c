/*  Audacious
 *  Copyright (C) 2005-2007  Audacious team
 *
 *  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2003  Haavard Kvaalen
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beepctrl.h"
#include "audacious/controlsocket.h"
#include "libaudacious/configdb.h"

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <grp.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>

/* overrides audacious_get_session_uri(). */
gchar *audacious_session_uri = NULL;
gint  audacious_session_type = 0;

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static gpointer
remote_read_packet(gint fd)
{
    gpointer data = NULL;
    ServerPktHeader pkt_hdr = { 0, 0 };

    if (read(fd, &pkt_hdr, sizeof(ServerPktHeader)) ==
        sizeof(ServerPktHeader))
    {
        if (pkt_hdr.version == XMMS_PROTOCOL_VERSION && 
		pkt_hdr.data_length > 0)
        {
            size_t data_length = pkt_hdr.data_length;
            data = g_malloc0(data_length);
            if ((size_t)read(fd, data, data_length) < data_length)
            {
                g_free(data);
                data = NULL;
            }
        }
    }

    return data;
}

static void
remote_read_ack(gint fd)
{
    gpointer data;

    data = remote_read_packet(fd);
    if (data)
        g_free(data);

}

static void
remote_send_packet(gint fd, guint32 command, gpointer data,
                   guint32 data_length)
{
    ClientPktHeader pkt_hdr;

    memset(&pkt_hdr, '\0', sizeof(ClientPktHeader));

    pkt_hdr.version = XMMS_PROTOCOL_VERSION;
    pkt_hdr.command = command;
    pkt_hdr.data_length = data_length;
    if ((size_t)write(fd, &pkt_hdr, sizeof(ClientPktHeader)) < sizeof(pkt_hdr))
        return;
    if (data_length && data)
        write(fd, data, data_length);
}

static void
remote_send_guint32(gint session, guint32 cmd, guint32 val)
{
    gint fd;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    remote_send_packet(fd, cmd, &val, sizeof(guint32));
    remote_read_ack(fd);
    close(fd);
}

static void
remote_send_boolean(gint session, guint32 cmd, gboolean val)
{
    gint fd;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    remote_send_packet(fd, cmd, &val, sizeof(gboolean));
    remote_read_ack(fd);
    close(fd);
}

static void
remote_send_gfloat(gint session, guint32 cmd, gfloat value)
{
    gint fd;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    remote_send_packet(fd, cmd, &value, sizeof(gfloat));
    remote_read_ack(fd);
    close(fd);
}

static void
remote_send_string(gint session, guint32 cmd, gchar * string)
{
    gint fd;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    remote_send_packet(fd, cmd, string, string ? strlen(string) + 1 : 0);
    remote_read_ack(fd);
    close(fd);
}

static gboolean
remote_cmd(gint session, guint32 cmd)
{
    gint fd;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return FALSE;
    remote_send_packet(fd, cmd, NULL, 0);
    remote_read_ack(fd);
    close(fd);

    return TRUE;
}

static gboolean
remote_get_gboolean(gint session, gint cmd)
{
    gboolean ret = FALSE;
    gpointer data;
    gint fd;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return ret;
    remote_send_packet(fd, cmd, NULL, 0);
    data = remote_read_packet(fd);
    if (data) {
        ret = *((gboolean *) data);
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);

    return ret;
}

static guint32
remote_get_gint(gint session, gint cmd)
{
    gpointer data;
    gint fd, ret = 0;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return ret;
    remote_send_packet(fd, cmd, NULL, 0);
    data = remote_read_packet(fd);
    if (data) {
        ret = *((gint *) data);
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);
    return ret;
}

static gfloat
remote_get_gfloat(gint session, gint cmd)
{
    gpointer data;
    gint fd;
    gfloat ret = 0.0;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return ret;
    remote_send_packet(fd, cmd, NULL, 0);
    data = remote_read_packet(fd);
    if (data) {
        ret = *((gfloat *) data);
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);
    return ret;
}

gchar *
remote_get_string(gint session, gint cmd)
{
    gpointer data;
    gint fd;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return NULL;
    remote_send_packet(fd, cmd, NULL, 0);
    data = remote_read_packet(fd);
    remote_read_ack(fd);
    close(fd);
    return data;
}

gchar *
remote_get_string_pos(gint session, gint cmd, guint32 pos)
{
    gpointer data;
    gint fd;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return NULL;
    remote_send_packet(fd, cmd, &pos, sizeof(guint32));
    data = remote_read_packet(fd);
    remote_read_ack(fd);
    close(fd);
    return data;
}

/**
 * audacious_set_session_uri:
 * @uri: The session URI to set the client API to.
 *
 * Sets the Session URI where Audacious can be reached at.
 **/
void
audacious_set_session_uri(gchar *uri)
{
    audacious_session_uri = uri;
}

/**
 * audacious_get_session_uri:
 * @session: Legacy XMMS session id (usually 0).
 *
 * Attempts to determine what the Session URI may be.
 *
 * Return value: A session URI.
 **/
gchar *
audacious_get_session_uri(gint session)
{
    ConfigDb *db;
    gchar *value = NULL;

    if (audacious_session_uri != NULL)
    {
        return g_strdup(audacious_session_uri);
    }

    if (audacious_session_type != AUDACIOUS_TYPE_UNIX)
    {
        db = bmp_cfg_db_open();

        bmp_cfg_db_get_string(db, NULL, "listen_uri_base", &value);

        bmp_cfg_db_close(db);
    }

    if (value == NULL)
        return g_strdup_printf("unix://localhost/%s/%s_%s.%d", g_get_tmp_dir(),
		CTRLSOCKET_NAME, g_get_user_name(), session);

    audacious_session_uri = value;

    return value;
}

/**
 * audacious_set_session_type:
 * @type: The type to set the session type to.
 *
 * Sets the type of session used by the audacious server.
 **/
void
audacious_set_session_type(gint type)
{
   audacious_session_type = type;
}

/**
 * audacious_determine_session_type:
 * @session: Legacy XMMS session id (usually 0).
 *
 * Attempts to determine what the session type may be.
 **/
gint
audacious_determine_session_type(gint session)
{
    gchar *uri = NULL;

    if (audacious_session_type != 0)
    {
        return audacious_session_type;
    }

    uri = audacious_get_session_uri(session);

    if (!g_strncasecmp(uri, "tcp://", 6))
        audacious_session_type = AUDACIOUS_TYPE_TCP;
    else
        audacious_session_type = AUDACIOUS_TYPE_UNIX;

    if (audacious_session_type == 0)
        audacious_session_type = AUDACIOUS_TYPE_UNIX;

    /* memory leak! */
    g_free(uri);

    return audacious_session_type;
}

/* tcp://192.168.100.1:5900/zyzychynxi389xvmfewqaxznvnw */

/**
 * audacious_decode_tcp_uri:
 * @session: The legacy XMMS session id (usually 0).
 * @in: A TCP:// Session URI to decode.
 * @host: Pointer to a host buffer.
 * @port: Pointer to the TCP port.
 * @key: Pointer to a security key buffer.
 *
 * Decodes a tcp:// session URI.
 **/
void
audacious_decode_tcp_uri(gint session, gchar *in, gchar **host, gint *port, gchar **key)
{
    static gchar *workbuf, *keybuf;
    gint iport;
    gchar *tmp = g_strdup(in);

    /* split out the host/port and key */
    workbuf = tmp;
    workbuf += 6;

    keybuf = strchr(workbuf, '/');
    *keybuf++ = '\0';

    *key = g_strdup(keybuf);

    if (strchr(workbuf, ':') == NULL)
    {
        *host = g_strdup(workbuf);
        *port = 37370 + session;
    }
    else
    {
        gchar *hostbuf = NULL;
        sscanf(workbuf, "%s:%d", hostbuf, &iport);

        *port = iport + session;
    }

    g_free(tmp);
}

/* unix://localhost/tmp/audacious_nenolod.0 */

/**
 * audacious_decode_unix_uri:
 * @session: The legacy XMMS session id (usually 0).
 * @in: A UNIX:// Session URI to decode.
 * @key: Pointer to a UNIX path buffer.
 *
 * Decodes a unix:// session URI.
 **/
void
audacious_decode_unix_uri(gint session, gchar *in, gchar **key)
{
    static gchar *workbuf, *keybuf;
    gchar *tmp = g_strdup(in);

    /* split out the host/port and key */
    workbuf = tmp;
    workbuf += 7;

    keybuf = strchr(workbuf, '/');
    *keybuf++ = '\0';

    *key = g_strdup(keybuf);

    g_free(tmp);
}

/**
 * xmms_connect_to_session:
 * @session: Legacy XMMS-style session identifier.
 *
 * Connects to an audacious server.
 *
 * Return value: an FD on success, otherwise -1.
 **/
gint
xmms_connect_to_session(gint session)
{
    gint fd;
    gint type = audacious_determine_session_type(session);
    gchar *uri = audacious_get_session_uri(session);

    if (type == AUDACIOUS_TYPE_UNIX)
    {
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1)
        {
            uid_t stored_uid, euid;
            struct sockaddr_un saddr;
	    gchar *path;

            saddr.sun_family = AF_UNIX;
            stored_uid = getuid();
            euid = geteuid();
            setuid(euid);
	    
	    audacious_decode_unix_uri(session, uri, &path);

	    g_strlcpy(saddr.sun_path, path, 108);
            g_free(path);
            setreuid(stored_uid, euid);

	    g_free(uri);

            if (connect(fd, (struct sockaddr *) &saddr, sizeof(saddr)) != -1)
                return fd;
        }
    }
    else
    {
        if ((fd = socket(AF_INET, SOCK_STREAM, 0)) != -1)
        {
	    struct hostent *hp;
	    struct sockaddr_in saddr;
	    gchar *host, *key;
	    gint port;

	    audacious_decode_tcp_uri(session, uri, &host, &port, &key);

            /* resolve it */
            if ((hp = gethostbyname(host)) == NULL)
            {
                 close(fd);
                 return -1;
            }

	    memset(&saddr, '\0', sizeof(saddr));
            saddr.sin_family = AF_INET;
            saddr.sin_port = htons(port);
            memcpy(&saddr.sin_addr, hp->h_addr, hp->h_length);

            g_free(host);
            g_free(key);

	    g_free(uri);

            if (connect(fd, (struct sockaddr *) &saddr, sizeof(saddr)) != -1)
                return fd;
        }
    }

    close(fd);
    return -1;
}

/**
 * xmms_remote_playlist:
 * @session: Legacy XMMS-style session identifier.
 * @list: A list of URIs to play.
 * @num: Number of URIs to play.
 * @enqueue: Whether or not the new playlist should be added on, or replace the current playlist.
 *
 * Sends a playlist to audacious.
 **/
void
xmms_remote_playlist(gint session, gchar ** list, gint num, gboolean enqueue)
{
    gint fd, i;
    gchar *data, *ptr;
    gint data_length;
    guint32 len;

    g_return_if_fail(list != NULL);
    g_return_if_fail(num > 0);

    if (!enqueue)
        xmms_remote_playlist_clear(session);

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;

    for (i = 0, data_length = 0; i < num; i++)
        data_length += (((strlen(list[i]) + 1) + 3) / 4) * 4 + 4;
    if (data_length) {
        data_length += 4;
        data = g_malloc(data_length);
        for (i = 0, ptr = data; i < num; i++) {
            len = strlen(list[i]) + 1;
            *((guint32 *) ptr) = len;
            ptr += 4;
            memcpy(ptr, list[i], len);
            ptr += ((len + 3) / 4) * 4;
        }
        *((guint32 *) ptr) = 0;
        remote_send_packet(fd, CMD_PLAYLIST_ADD, data, data_length);
        remote_read_ack(fd);
        close(fd);
        g_free(data);
    }

    if (!enqueue)
        xmms_remote_play(session);
}

/**
 * xmms_remote_get_version:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious for it's protocol version.
 *
 * Return value: The protocol version used by Audacious.
 **/
gint
xmms_remote_get_version(gint session)
{
    return remote_get_gint(session, CMD_GET_VERSION);
}

/**
 * xmms_remote_play_files:
 * @session: Legacy XMMS-style session identifier.
 * @list: A GList of URIs to play.
 *
 * Sends a list of URIs to Audacious to play.
 **/
void
xmms_remote_play_files(gint session, GList * list)
{
    g_return_if_fail(list != NULL);

    xmms_remote_playlist_clear(session);
    xmms_remote_playlist_add(session, list);
    xmms_remote_play(session);
}

/**
 * xmms_remote_playlist_add:
 * @session: Legacy XMMS-style session identifier.
 * @list: A GList of URIs to add to the playlist.
 *
 * Sends a list of URIs to Audacious to add to the playlist.
 **/
void
xmms_remote_playlist_add(gint session, GList * list)
{
    gchar **str_list;
    GList *node;
    gint i, num;

    g_return_if_fail(list != NULL);

    num = g_list_length(list);
    str_list = g_malloc0(num * sizeof(gchar *));
    for (i = 0, node = list; i < num && node; i++, node = g_list_next(node))
        str_list[i] = node->data;

    xmms_remote_playlist(session, str_list, num, TRUE);
    g_free(str_list);
}

/**
 * xmms_remote_playlist_delete:
 * @session: Legacy XMMS-style session identifier.
 * @pos: The playlist position to delete.
 *
 * Deletes a playlist entry.
 **/
void
xmms_remote_playlist_delete(gint session, gint pos)
{
    remote_send_guint32(session, CMD_PLAYLIST_DELETE, pos);
}

/**
 * xmms_remote_play:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to begin playback.
 **/
void
xmms_remote_play(gint session)
{
    remote_cmd(session, CMD_PLAY);
}

/**
 * xmms_remote_pause:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to pause.
 **/
void
xmms_remote_pause(gint session)
{
    remote_cmd(session, CMD_PAUSE);
}

/**
 * xmms_remote_stop:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to stop.
 **/
void
xmms_remote_stop(gint session)
{
    remote_cmd(session, CMD_STOP);
}

/**
 * xmms_remote_play_pause:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to either play or pause.
 **/
void
xmms_remote_play_pause(gint session)
{
    remote_cmd(session, CMD_PLAY_PAUSE);
}

/**
 * xmms_remote_is_playing:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about whether it is playing or not.
 *
 * Return value: TRUE if playing, FALSE otherwise.
 **/
gboolean
xmms_remote_is_playing(gint session)
{
    return remote_get_gboolean(session, CMD_IS_PLAYING);
}

/**
 * xmms_remote_is_paused:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about whether it is paused or not.
 *
 * Return value: TRUE if playing, FALSE otherwise.
 **/
gboolean
xmms_remote_is_paused(gint session)
{
    return remote_get_gboolean(session, CMD_IS_PAUSED);
}

/**
 * xmms_remote_get_playlist_pos:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about the current playlist position.
 *
 * Return value: The current playlist position.
 **/
gint
xmms_remote_get_playlist_pos(gint session)
{
    return remote_get_gint(session, CMD_GET_PLAYLIST_POS);
}

/**
 * xmms_remote_set_playlist_pos:
 * @session: Legacy XMMS-style session identifier.
 * @pos: Playlist position to jump to.
 *
 * Tells audacious to jump to a different playlist position.
 **/
void
xmms_remote_set_playlist_pos(gint session, gint pos)
{
    remote_send_guint32(session, CMD_SET_PLAYLIST_POS, pos);
}

/**
 * xmms_remote_get_playlist_length:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about the current playlist length.
 *
 * Return value: The amount of entries in the playlist.
 **/
gint
xmms_remote_get_playlist_length(gint session)
{
    return remote_get_gint(session, CMD_GET_PLAYLIST_LENGTH);
}

/**
 * xmms_remote_playlist_clear:
 * @session: Legacy XMMS-style session identifier.
 *
 * Clears the playlist.
 **/
void
xmms_remote_playlist_clear(gint session)
{
    remote_cmd(session, CMD_PLAYLIST_CLEAR);
}

/**
 * xmms_remote_get_output_time:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about the current output position.
 *
 * Return value: The current output position.
 **/
gint
xmms_remote_get_output_time(gint session)
{
    return remote_get_gint(session, CMD_GET_OUTPUT_TIME);
}

/**
 * xmms_remote_jump_to_time:
 * @session: Legacy XMMS-style session identifier.
 * @pos: The time (in milliseconds) to jump to.
 *
 * Tells audacious to seek to a new time position.
 **/
void
xmms_remote_jump_to_time(gint session, gint pos)
{
    remote_send_guint32(session, CMD_JUMP_TO_TIME, pos);
}

/**
 * xmms_remote_get_volume:
 * @session: Legacy XMMS-style session identifier.
 * @vl: Pointer to integer containing the left channel's volume.
 * @vr: Pointer to integer containing the right channel's volume.
 *
 * Queries audacious about the current volume.
 **/
void
xmms_remote_get_volume(gint session, gint * vl, gint * vr)
{
    gint fd;
    gpointer data;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;

    remote_send_packet(fd, CMD_GET_VOLUME, NULL, 0);
    data = remote_read_packet(fd);
    if (data) {
        *vl = ((guint32 *) data)[0];
        *vr = ((guint32 *) data)[1];
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);
}

/**
 * xmms_remote_get_main_volume:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about the current volume.
 *
 * Return value: The current volume.
 **/
gint
xmms_remote_get_main_volume(gint session)
{
    gint vl, vr;

    xmms_remote_get_volume(session, &vl, &vr);

    return (vl > vr) ? vl : vr;
}

/**
 * xmms_remote_get_balance:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about the current balance.
 *
 * Return value: The current balance.
 **/
gint
xmms_remote_get_balance(gint session)
{
    return remote_get_gint(session, CMD_GET_BALANCE);
}

/**
 * xmms_remote_set_volume:
 * @session: Legacy XMMS-style session identifier.
 * @vl: The volume for the left channel.
 * @vr: The volume for the right channel.
 *
 * Sets the volume for the left and right channels in Audacious.
 **/
void
xmms_remote_set_volume(gint session, gint vl, gint vr)
{
    gint fd;
    guint32 v[2];

    if (vl < 0)
        vl = 0;
    if (vl > 100)
        vl = 100;
    if (vr < 0)
        vr = 0;
    if (vr > 100)
        vr = 100;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    v[0] = vl;
    v[1] = vr;
    remote_send_packet(fd, CMD_SET_VOLUME, v, 2 * sizeof(guint32));
    remote_read_ack(fd);
    close(fd);
}

/**
 * xmms_remote_set_main_volume:
 * @session: Legacy XMMS-style session identifier.
 * @v: The volume to set.
 *
 * Sets the volume in Audacious.
 **/
void
xmms_remote_set_main_volume(gint session, gint v)
{
    gint b, vl, vr;

    b = xmms_remote_get_balance(session);

    if (b < 0) {
        vl = v;
        vr = (v * (100 - abs(b))) / 100;
    }
    else if (b > 0) {
        vl = (v * (100 - b)) / 100;
        vr = v;
    }
    else
        vl = vr = v;
    xmms_remote_set_volume(session, vl, vr);
}

/**
 * xmms_remote_set_balance:
 * @session: Legacy XMMS-style session identifier.
 * @b: The balance to set.
 *
 * Sets the balance in Audacious.
 **/
void
xmms_remote_set_balance(gint session, gint b)
{
    gint v, vl, vr;

    if (b < -100)
        b = -100;
    if (b > 100)
        b = 100;

    v = xmms_remote_get_main_volume(session);

    if (b < 0) {
        vl = v;
        vr = (v * (100 - abs(b))) / 100;
    }
    else if (b > 0) {
        vl = (v * (100 - b)) / 100;
        vr = v;
    }
    else
        vl = vr = v;
    xmms_remote_set_volume(session, vl, vr);
}

/**
 * xmms_remote_get_skin:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries Audacious about it's skin.
 *
 * Return value: A path to the currently selected skin.
 **/
gchar *
xmms_remote_get_skin(gint session)
{
    return remote_get_string(session, CMD_GET_SKIN);
}

/**
 * xmms_remote_set_skin:
 * @session: Legacy XMMS-style session identifier.
 * @skinfile: Path to a skinfile to use with Audacious.
 *
 * Tells audacious to start using the skinfile provided.
 **/
void
xmms_remote_set_skin(gint session, gchar * skinfile)
{
    remote_send_string(session, CMD_SET_SKIN, skinfile);
}

/**
 * xmms_remote_get_playlist_file:
 * @session: Legacy XMMS-style session identifier.
 * @pos: The playlist position to query for.
 *
 * Queries Audacious about a playlist entry's file.
 *
 * Return value: A path to the file in the playlist at %pos position.
 **/
gchar *
xmms_remote_get_playlist_file(gint session, gint pos)
{
    return remote_get_string_pos(session, CMD_GET_PLAYLIST_FILE, pos);
}

/**
 * xmms_remote_get_playlist_title:
 * @session: Legacy XMMS-style session identifier.
 * @pos: The playlist position to query for.
 *
 * Queries Audacious about a playlist entry's title.
 *
 * Return value: The title for the entry in the playlist at %pos position.
 **/
gchar *
xmms_remote_get_playlist_title(gint session, gint pos)
{
    return remote_get_string_pos(session, CMD_GET_PLAYLIST_TITLE, pos);
}

/**
 * xmms_remote_get_playlist_time:
 * @session: Legacy XMMS-style session identifier.
 * @pos: The playlist position to query for.
 *
 * Queries Audacious about a playlist entry's length.
 *
 * Return value: The length of the entry in the playlist at %pos position.
 **/
gint
xmms_remote_get_playlist_time(gint session, gint pos)
{
    gpointer data;
    gint fd, ret = 0;
    guint32 p = pos;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return ret;
    remote_send_packet(fd, CMD_GET_PLAYLIST_TIME, &p, sizeof(guint32));
    data = remote_read_packet(fd);
    if (data) {
        ret = *((gint *) data);
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);
    return ret;
}

/**
 * xmms_remote_get_info:
 * @session: Legacy XMMS-style session identifier.
 * @rate: Pointer to an integer containing the bitrate.
 * @freq: Pointer to an integer containing the frequency.
 * @nch: Pointer to an integer containing the number of channels.
 *
 * Queries Audacious about the current audio format.
 **/
void
xmms_remote_get_info(gint session, gint * rate, gint * freq, gint * nch)
{
    gint fd;
    gpointer data;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    remote_send_packet(fd, CMD_GET_INFO, NULL, 0);
    data = remote_read_packet(fd);
    if (data) {
        *rate = ((guint32 *) data)[0];
        *freq = ((guint32 *) data)[1];
        *nch = ((guint32 *) data)[2];
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);
}

/**
 * xmms_remote_get_eq_data:
 * @session: Legacy XMMS-style session identifier.
 *
 * Not implemented, present for compatibility with libxmms API.
 **/
void
xmms_remote_get_eq_data(gint session)
{
    /* Obsolete */
}

/**
 * xmms_remote_set_eq_data:
 * @session: Legacy XMMS-style session identifier.
 *
 * Not implemented, present for compatibility with libxmms API.
 **/
void
xmms_remote_set_eq_data(gint session)
{
    /* Obsolete */
}

/**
 * xmms_remote_pl_win_toggle:
 * @session: Legacy XMMS-style session identifier.
 * @show: Whether or not to show the playlist window.
 *
 * Toggles the playlist window's visibility.
 **/
void
xmms_remote_pl_win_toggle(gint session, gboolean show)
{
    remote_send_boolean(session, CMD_PL_WIN_TOGGLE, show);
}

/**
 * xmms_remote_eq_win_toggle:
 * @session: Legacy XMMS-style session identifier.
 * @show: Whether or not to show the equalizer window.
 *
 * Toggles the equalizer window's visibility.
 **/
void
xmms_remote_eq_win_toggle(gint session, gboolean show)
{
    remote_send_boolean(session, CMD_EQ_WIN_TOGGLE, show);
}

/**
 * xmms_remote_main_win_toggle:
 * @session: Legacy XMMS-style session identifier.
 * @show: Whether or not to show the main window.
 *
 * Toggles the main window's visibility.
 **/
void
xmms_remote_main_win_toggle(gint session, gboolean show)
{
    remote_send_boolean(session, CMD_MAIN_WIN_TOGGLE, show);
}

/**
 * xmms_remote_is_main_win:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries Audacious about the main window's visibility.
 *
 * Return value: TRUE if visible, FALSE otherwise.
 **/
gboolean
xmms_remote_is_main_win(gint session)
{
    return remote_get_gboolean(session, CMD_IS_MAIN_WIN);
}

/**
 * xmms_remote_is_pl_win:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries Audacious about the playlist window's visibility.
 *
 * Return value: TRUE if visible, FALSE otherwise.
 **/
gboolean
xmms_remote_is_pl_win(gint session)
{
    return remote_get_gboolean(session, CMD_IS_PL_WIN);
}

/**
 * xmms_remote_is_eq_win:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries Audacious about the equalizer window's visibility.
 *
 * Return value: TRUE if visible, FALSE otherwise.
 **/
gboolean
xmms_remote_is_eq_win(gint session)
{
    return remote_get_gboolean(session, CMD_IS_EQ_WIN);
}

/**
 * xmms_remote_show_prefs_box:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to show the preferences pane.
 **/
void
xmms_remote_show_prefs_box(gint session)
{
    remote_cmd(session, CMD_SHOW_PREFS_BOX);
}

/**
 * xmms_remote_show_jtf_box:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to show the Jump-to-File pane.
 **/
void
xmms_remote_show_jtf_box(gint session)
{
    remote_cmd(session, CMD_SHOW_JTF_BOX);
}

/**
 * xmms_remote_toggle_aot:
 * @session: Legacy XMMS-style session identifier.
 * @ontop: Whether or not Audacious should be always-on-top.
 *
 * Tells audacious to toggle the always-on-top feature.
 **/
void
xmms_remote_toggle_aot(gint session, gboolean ontop)
{
    remote_send_boolean(session, CMD_TOGGLE_AOT, ontop);
}

/**
 * xmms_remote_show_about_box:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to show the about pane.
 **/
void
xmms_remote_show_about_box(gint session)
{
    remote_cmd(session, CMD_SHOW_ABOUT_BOX);
}

/**
 * xmms_remote_eject:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to display the open files pane.
 **/
void
xmms_remote_eject(gint session)
{
    remote_cmd(session, CMD_EJECT);
}

/**
 * xmms_remote_playlist_prev:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to move backwards in the playlist.
 **/
void
xmms_remote_playlist_prev(gint session)
{
    remote_cmd(session, CMD_PLAYLIST_PREV);
}

/**
 * xmms_remote_playlist_next:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to move forward in the playlist.
 **/
void
xmms_remote_playlist_next(gint session)
{
    remote_cmd(session, CMD_PLAYLIST_NEXT);
}

/**
 * xmms_remote_playlist_add_url_string:
 * @session: Legacy XMMS-style session identifier.
 * @string: The URI to add.
 *
 * Tells audacious to add an URI to the playlist.
 **/
void
xmms_remote_playlist_add_url_string(gint session, gchar * string)
{
    g_return_if_fail(string != NULL);
    remote_send_string(session, CMD_PLAYLIST_ADD_URL_STRING, string);
}

/**
 * xmms_remote_playlist_ins_url_string:
 * @session: Legacy XMMS-style session identifier.
 * @string: The URI to add.
 * @pos: The position to add the URI at.
 *
 * Tells audacious to add an URI to the playlist at a specific position.
 **/
void
xmms_remote_playlist_ins_url_string(gint session, gchar * string, gint pos)
{
    gint fd, size;
    gchar *packet;

    g_return_if_fail(string != NULL);

    size = strlen(string) + 1 + sizeof(gint);

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;

    packet = g_malloc0(size);
    *((gint *) packet) = pos;
    strcpy(packet + sizeof(gint), string);
    remote_send_packet(fd, CMD_PLAYLIST_INS_URL_STRING, packet, size);
    remote_read_ack(fd);
    close(fd);
    g_free(packet);
}

/**
 * xmms_remote_is_running:
 * @session: Legacy XMMS-style session identifier.
 *
 * Checks to see if an Audacious server is running.
 *
 * Return value: TRUE if yes, otherwise FALSE.
 **/
gboolean
xmms_remote_is_running(gint session)
{
    return remote_cmd(session, CMD_PING);
}

/**
 * xmms_remote_toggle_repeat:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to toggle the repeat feature.
 **/
void
xmms_remote_toggle_repeat(gint session)
{
    remote_cmd(session, CMD_TOGGLE_REPEAT);
}

/**
 * xmms_remote_toggle_shuffle:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to toggle the shuffle feature.
 **/
void
xmms_remote_toggle_shuffle(gint session)
{
    remote_cmd(session, CMD_TOGGLE_SHUFFLE);
}

/**
 * xmms_remote_toggle_advance:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to toggle the no-playlist-advance feature.
 **/
void
xmms_remote_toggle_advance(int session)
{
    remote_cmd(session, CMD_TOGGLE_ADVANCE);
}

/**
 * xmms_remote_is_repeat:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about whether or not the repeat feature is active.
 *
 * Return value: TRUE if yes, otherwise FALSE.
 **/
gboolean
xmms_remote_is_repeat(gint session)
{
    return remote_get_gboolean(session, CMD_IS_REPEAT);
}

/**
 * xmms_remote_is_shuffle:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about whether or not the shuffle feature is active.
 *
 * Return value: TRUE if yes, otherwise FALSE.
 **/
gboolean
xmms_remote_is_shuffle(gint session)
{
    return remote_get_gboolean(session, CMD_IS_SHUFFLE);
}

/**
 * xmms_remote_is_advance:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about whether or not the no-playlist-advance feature is active.
 *
 * Return value: TRUE if yes, otherwise FALSE.
 **/
gboolean
xmms_remote_is_advance(gint session)
{
    return remote_get_gboolean(session, CMD_IS_ADVANCE);
}

/**
 * xmms_remote_playqueue_add:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to add a playlist entry to the playqueue.
 **/
void
xmms_remote_playqueue_add(gint session, gint pos)
{
    remote_send_guint32(session, CMD_PLAYQUEUE_ADD, pos);
}

/**
 * xmms_remote_playqueue_remove:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to remove a playlist entry from the playqueue.
 **/
void
xmms_remote_playqueue_remove(gint session, gint pos)
{
    remote_send_guint32(session, CMD_PLAYQUEUE_REMOVE, pos);
}

/**
 * xmms_remote_playqueue_clear:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to clear the playqueue.
 **/
void
xmms_remote_playqueue_clear(gint session)
{
    remote_cmd(session, CMD_PLAYQUEUE_CLEAR);
}

/**
 * xmms_remote_get_playqueue_length:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about the playqueue's length.
 *
 * Return value: The number of entries in the playqueue.
 **/
gint
xmms_remote_get_playqueue_length(gint session)
{
    return remote_get_gint(session, CMD_GET_PLAYQUEUE_LENGTH);
}

/**
 * xmms_remote_playqueue_is_queued:
 * @session: Legacy XMMS-style session identifier.
 * @pos: Position to check queue for.
 *
 * Queries audacious about whether or not a playlist entry is in the playqueue.
 *
 * Return value: TRUE if yes, FALSE otherwise.
 **/
gboolean
xmms_remote_playqueue_is_queued(gint session, gint pos)
{
    gpointer data;
    gint fd, ret = 0;
    guint32 p = pos;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return ret;
    remote_send_packet(fd, CMD_PLAYQUEUE_IS_QUEUED, &p, sizeof(guint32));
    data = remote_read_packet(fd);
    if (data) {
        ret = *((gint *) data);
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);
    return ret;
}

/**
 * xmms_remote_get_playqueue_position:
 * @session: Legacy XMMS-style session identifier.
 * @pos: Position to check queue for.
 *
 * Queries audacious about what the playqueue position is for a playlist entry.
 *
 * Return value: TRUE if yes, FALSE otherwise.
 **/
gint
xmms_remote_get_playqueue_position(gint session, gint pos)
{
    gpointer data;
    gint fd, ret = 0;
    guint32 p = pos;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return ret;
    remote_send_packet(fd, CMD_PLAYQUEUE_GET_POS, &p, sizeof(guint32));
    data = remote_read_packet(fd);
    if (data) {
        ret = *((gint *) data);
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);
    return ret;
}

/**
 * xmms_remote_get_playqueue_queue_position:
 * @session: Legacy XMMS-style session identifier.
 * @pos: Position to check queue for.
 *
 * Queries audacious about what the playlist position is for a playqueue entry.
 *
 * Return value: TRUE if yes, FALSE otherwise.
 **/
gint
xmms_remote_get_playqueue_queue_position(gint session, gint pos)
{
    gpointer data;
    gint fd, ret = 0;
    guint32 p = pos;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return ret;
    remote_send_packet(fd, CMD_PLAYQUEUE_GET_QPOS, &p, sizeof(guint32));
    data = remote_read_packet(fd);
    if (data) {
        ret = *((gint *) data);
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);
    return ret;
}

/**
 * xmms_remote_get_eq:
 * @session: Legacy XMMS-style session identifier.
 * @preamp: Pointer to value for preamp setting.
 * @bands: Pointer to array of band settings.
 *
 * Queries audacious about the equalizer settings.
 **/
void
xmms_remote_get_eq(gint session, gfloat * preamp, gfloat ** bands)
{
    gint fd;
    gpointer data;

    if (preamp)
        *preamp = 0.0;

    if (bands)
        *bands = NULL;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    remote_send_packet(fd, CMD_GET_EQ, NULL, 0);
    data = remote_read_packet(fd);
    if (data) {
            if (preamp)
                *preamp = *((gfloat *) data);
            if (bands)
                *bands =
                    (gfloat *) g_memdup((gfloat *) data + 1,
                                        10 * sizeof(gfloat));
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);
}

/**
 * xmms_remote_get_eq_preamp:
 * @session: Legacy XMMS-style session identifier.
 *
 * Queries audacious about the equalizer preamp's setting.
 *
 * Return value: The equalizer preamp's setting.
 **/
gfloat
xmms_remote_get_eq_preamp(gint session)
{
    return remote_get_gfloat(session, CMD_GET_EQ_PREAMP);
}

/**
 * xmms_remote_get_eq_band:
 * @session: Legacy XMMS-style session identifier.
 * @band: Which band to lookup the value for.
 *
 * Queries audacious about an equalizer band's value.
 *
 * Return value: The equalizer band's value.
 **/
gfloat
xmms_remote_get_eq_band(gint session, gint band)
{
    gint fd;
    gpointer data;
    gfloat val = 0.0;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return val;
    remote_send_packet(fd, CMD_GET_EQ_BAND, &band, sizeof(band));
    data = remote_read_packet(fd);
    if (data) {
        val = *((gfloat *) data);
        g_free(data);
    }
    remote_read_ack(fd);
    close(fd);
    return val;
}

/**
 * xmms_remote_set_eq:
 * @session: Legacy XMMS-style session identifier.
 * @preamp: Value for preamp setting.
 * @bands: Array of band settings.
 *
 * Tells audacious to set the equalizer up using the provided values.
 **/
void
xmms_remote_set_eq(gint session, gfloat preamp, gfloat * bands)
{
    gint fd, i;
    gfloat data[11];

    g_return_if_fail(bands != NULL);

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    data[0] = preamp;
    for (i = 0; i < 10; i++)
        data[i + 1] = bands[i];
    remote_send_packet(fd, CMD_SET_EQ, data, sizeof(data));
    remote_read_ack(fd);
    close(fd);
}

/**
 * xmms_remote_set_eq_preamp:
 * @session: Legacy XMMS-style session identifier.
 * @preamp: Value for preamp setting.
 *
 * Tells audacious to set the equalizer's preamp setting.
 **/
void
xmms_remote_set_eq_preamp(gint session, gfloat preamp)
{
    remote_send_gfloat(session, CMD_SET_EQ_PREAMP, preamp);
}

/**
 * xmms_remote_set_eq_band:
 * @session: Legacy XMMS-style session identifier.
 * @band: The band to set the value for.
 * @value: The value to set that band to.
 *
 * Tells audacious to set an equalizer band's setting.
 **/
void
xmms_remote_set_eq_band(gint session, gint band, gfloat value)
{
    gint fd;
    gchar data[sizeof(gint) + sizeof(gfloat)];

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    *((gint *) data) = band;
    *((gfloat *) (data + sizeof(gint))) = value;
    remote_send_packet(fd, CMD_SET_EQ_BAND, data, sizeof(data));
    remote_read_ack(fd);
    close(fd);
}

/**
 * xmms_remote_quit:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to quit.
 **/
void
xmms_remote_quit(gint session)
{
    gint fd;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    remote_send_packet(fd, CMD_QUIT, NULL, 0);
    remote_read_ack(fd);
    close(fd);
}

/**
 * xmms_remote_activate:
 * @session: Legacy XMMS-style session identifier.
 *
 * Tells audacious to display the main window and become the selected window.
 **/
void
xmms_remote_activate(gint session)
{
    gint fd;

    if ((fd = xmms_connect_to_session(session)) == -1)
        return;
    remote_send_packet(fd, CMD_ACTIVATE, NULL, 0);
    remote_read_ack(fd);
    close(fd);
}
