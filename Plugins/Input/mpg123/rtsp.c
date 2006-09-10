/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2006  Audacious development team.
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
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <nemesi/rtp.h>
#include <nemesi/rtsp.h>
#include <libaudacious/util.h>

#include "mpg123.h"

extern gint mpgdec_bitrate, mpgdec_frequency, mpgdec_stereo;
extern gboolean mpgdec_stereo;

//FIXME that's ugly!
static rtsp_ctrl *ctl;
static rtsp_session *sess;
static rtp_ssrc *ssrc;
static size_t foff;
static rtp_buff conf;
static rtp_frame fr;
static rtp_thread *rtp_th;

int mpgdec_rtsp_open(char *url)
{
//    nms_verbosity_set(0);
    nms_rtsp_hints rtsp_hints = { -1 };
    foff = 0;

    if ( (ctl = rtsp_init(&rtsp_hints))==NULL ) {
        fprintf (stderr, "Cannot init rtsp.\n");
        return 1;
    }

    if ( rtsp_open( ctl, url) )
    {
        fprintf (stderr, "rtsp_open failed.\n");
        // die
        return 1;
    }

    rtsp_wait(ctl);

    sess = ctl->rtsp_queue;

    if (!sess) {
        fprintf (stderr, "No session available.\n");
        return 1;
    }
//look for the first mp3 track
    rtsp_play(ctl,0.0,0.0);
    
    rtp_th = rtsp_get_rtp_th(ctl);    
    
    rtp_fill_buffers(rtp_th);

    for (ssrc = rtp_active_ssrc_queue(rtsp_get_rtp_queue(ctl));
         ssrc; ssrc = rtp_next_active_ssrc(ssrc)) {
        if (ssrc->rtp_sess->announced_fmts->pt == 14) {
            return 0;
        }
    }
    return 1;
}

int mpgdec_rtsp_read(gpointer data, gsize length)
{
    gint ret = 0, len;
    gsize off = 0;
    while (length && !ret && !rtp_fill_buffers(rtp_th)){
        if (!foff)
            ret = rtp_fill_buffer(ssrc, &fr, &conf);
        len = min(length, fr.len - foff);
        memcpy((char *)data + off, fr.data + foff, len);
        length -= len;
        off += len;
        foff = (len+foff)%fr.len;
    }
    return off;
}

void mpgdec_rtsp_close (void)
{
    foff = 0;    
    rtsp_close(ctl);

}

