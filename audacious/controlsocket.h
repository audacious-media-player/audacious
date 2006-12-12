/*  BMP - Cross-platform multimedia player
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

#ifndef CONTROLSOCKET_H
#define CONTROLSOCKET_H

#include <glib.h>

#define XMMS_PROTOCOL_VERSION	     1

#define CTRLSOCKET_NAME              "audacious"
#define CTRLSOCKET_IO_TIMEOUT_USEC   100000

enum {
    CMD_GET_VERSION, CMD_PLAYLIST_ADD, CMD_PLAY, CMD_PAUSE, CMD_STOP,
    CMD_IS_PLAYING, CMD_IS_PAUSED, CMD_GET_PLAYLIST_POS,
    CMD_SET_PLAYLIST_POS, CMD_GET_PLAYLIST_LENGTH, CMD_PLAYLIST_CLEAR,
    CMD_GET_OUTPUT_TIME, CMD_JUMP_TO_TIME, CMD_GET_VOLUME,
    CMD_SET_VOLUME, CMD_GET_SKIN, CMD_SET_SKIN, CMD_GET_PLAYLIST_FILE,
    CMD_GET_PLAYLIST_TITLE, CMD_GET_PLAYLIST_TIME, CMD_GET_INFO,
    CMD_GET_EQ_DATA, CMD_SET_EQ_DATA, CMD_PL_WIN_TOGGLE,
    CMD_EQ_WIN_TOGGLE, CMD_SHOW_PREFS_BOX, CMD_TOGGLE_AOT,
    CMD_SHOW_ABOUT_BOX, CMD_EJECT, CMD_PLAYLIST_PREV, CMD_PLAYLIST_NEXT,
    CMD_PING, CMD_GET_BALANCE, CMD_TOGGLE_REPEAT, CMD_TOGGLE_SHUFFLE,
    CMD_MAIN_WIN_TOGGLE, CMD_PLAYLIST_ADD_URL_STRING,
    CMD_IS_EQ_WIN, CMD_IS_PL_WIN, CMD_IS_MAIN_WIN, CMD_PLAYLIST_DELETE,
    CMD_IS_REPEAT, CMD_IS_SHUFFLE,
    CMD_GET_EQ, CMD_GET_EQ_PREAMP, CMD_GET_EQ_BAND,
    CMD_SET_EQ, CMD_SET_EQ_PREAMP, CMD_SET_EQ_BAND,
    CMD_QUIT, CMD_PLAYLIST_INS_URL_STRING, CMD_PLAYLIST_INS, CMD_PLAY_PAUSE,
    CMD_PLAYQUEUE_ADD, CMD_GET_PLAYQUEUE_LENGTH, CMD_PLAYQUEUE_REMOVE,
    CMD_TOGGLE_ADVANCE, CMD_IS_ADVANCE,
    CMD_ACTIVATE, CMD_SHOW_JTF_BOX,
    CMD_PLAYQUEUE_CLEAR, CMD_PLAYQUEUE_IS_QUEUED,
    CMD_PLAYQUEUE_GET_POS, CMD_PLAYQUEUE_GET_QPOS
};


typedef struct {
    guint16 version;
    guint16 command;
    guint32 data_length;
} ClientPktHeader;

typedef struct {
    guint16 version;
    guint32 data_length;
} ServerPktHeader;

typedef struct {
    ClientPktHeader hdr;
    gpointer data;
    gint fd;
} PacketNode;


gboolean ctrlsocket_setup(void);
gboolean ctrlsocket_setup_unix(void);
gboolean ctrlsocket_setup_tcp(void);
void ctrlsocket_start(void);
void ctrlsocket_check(void);
void ctrlsocket_cleanup(void);
gint ctrlsocket_get_session_id(void);

#endif
