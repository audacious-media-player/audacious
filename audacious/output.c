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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "output.h"
#include "iir.h"
#include "main.h"
#include "input.h"

#include "playlist.h"
#include "libaudacious/util.h"

OutputPluginData op_data = {
    NULL,
    NULL
};

OutputPlugin *
get_current_output_plugin(void)
{
    return op_data.current_output_plugin;
}

void
set_current_output_plugin(gint i)
{
#if 0
    gint time;
    gint pos;
    gboolean playing;
#endif

    GList *node = g_list_nth(op_data.output_list, i);
    if (!node) {
        op_data.current_output_plugin = NULL;
        return;
    }

    op_data.current_output_plugin = node->data;


#if 0
    playing = bmp_playback_get_playing();
    if (playing) {

        /* FIXME: we do all on our own here */
        
        guint min = 0, sec = 0, params, time, pos;
        gchar timestr[10];
        
        bmp_playback_pause();
        pos = get_playlist_position();
        time = bmp_playback_get_time() / 1000;
        g_snprintf(timestr, sizeof(timestr), "%u:%2.2u",
                   time / 60, time % 60);
        
        params = sscanf(timestr, "%u:%u", &min, &sec);
        if (params == 2)
            time = (min * 60) + sec;
        else if (params == 1)
            time = min;
        else
            return;
        
        bmp_playback_stop();
        playlist_set_position(pos);
        bmp_playback_play_file(playlist_get_filename(pos));
        
        while (!bmp_playback_get_playing())
            g_message("waiting...");

        if (playlist_get_current_length() > -1 &&
            time <= (playlist_get_current_length() / 1000)) {
            /* Some time for things to cool down and heat up */
            g_usleep(1000000);
            bmp_playback_seek(time);
        }
    }
#endif
}

GList *
get_output_list(void)
{
    return op_data.output_list;
}

void
output_about(gint i)
{
    OutputPlugin *out = g_list_nth(op_data.output_list, i)->data;
    if (out && out->about)
        out->about();
}

void
output_configure(gint i)
{
    OutputPlugin *out = g_list_nth(op_data.output_list, i)->data;
    if (out && out->configure)
        out->configure();
}

void
output_get_volume(gint * l, gint * r)
{
    *l = *r = -1;

    if (!op_data.current_output_plugin)
        return;

    if (!op_data.current_output_plugin->get_volume)
        return;

    op_data.current_output_plugin->get_volume(l, r);
}

void
output_set_volume(gint l, gint r)
{
    if (!op_data.current_output_plugin)
        return;

    if (!op_data.current_output_plugin->set_volume)
        return;

    op_data.current_output_plugin->set_volume(l, r);
}

void
output_set_eq(gboolean active, gfloat pre, gfloat * bands)
{
    int i;
    preamp[0] = 1.0 + 0.0932471 * pre + 0.00279033 * pre * pre;
    preamp[1] = 1.0 + 0.0932471 * pre + 0.00279033 * pre * pre;

    for (i = 0; i < 10; ++i)
    {
	set_gain(i, 0, 0.03 * bands[i] + 0.000999999 * bands[i] * bands[i]);
	set_gain(i, 1, 0.03 * bands[i] + 0.000999999 * bands[i] * bands[i]);
    }
}

/* this should be in BYTES, NOT gint16s */
static void
byteswap(size_t size,
         gint16 * buf)
{
    gint16 *it;
    size &= ~1;                  /* must be multiple of 2  */
    for (it = buf; it < buf + size / 2; ++it)
        *(guint16 *) it = GUINT16_SWAP_LE_BE(*(guint16 *) it);
}

static void
output_to_plugin(gint time,
                 AFormat format,
                 gint n_channels,
                 gint length,
                 gpointer sample,
                 int *going)
{
    OutputPlugin *op = get_current_output_plugin();

    /* do vis plugin(s) */
    input_add_vis_pcm(time, format, n_channels, length, sample);

    while (op->buffer_free() < length) { /* wait output buf          */
        if (going && !*going)            /*   thread stopped?        */
            return;                      /*     so finish            */

        g_usleep(10000);                 /*   else sleep for retry   */
    }                                    

    op->write_audio(sample, length);     /* do output                */
}



/* called by input plugin when data is ready */
void
produce_audio(gint time,        /* position             */
              AFormat fmt,      /* output format        */
              gint nch,         /* channels             */
              gint length,      /* length of sample     */
              gpointer ptr,     /* data                 */
              int *going        /* 0 when time to stop  */
              )
{
#ifndef XMMS_EQ

    static int init = 0;
    int swapped = 0;
    int myorder = G_BYTE_ORDER == G_LITTLE_ENDIAN ? FMT_S16_LE : FMT_S16_BE;
    int caneq = (fmt == FMT_S16_NE || fmt == myorder);

    if (!caneq && cfg.equalizer_active) {    /* wrong byte order         */
        byteswap(length, ptr);               /*  so convert              */
        ++swapped;
        ++caneq;
    }                                        /*  can eq now, mark swapd  */
    else if (caneq && !cfg.equalizer_active) /* right order but no eq    */
        caneq = 0;                           /*  so don't eq             */

    if (caneq) {                /* if eq enab               */
        if (!init) {            /*  if first run            */
            init_iir();         /*   then init eq           */
            ++init;
        }

        iir(&ptr, length, nch);

        if (swapped)               /* if was swapped          */
            byteswap(length, ptr); /*  swap back for output   */
    }                           

#endif

    output_to_plugin(time, fmt, nch, length, ptr, going);
}
