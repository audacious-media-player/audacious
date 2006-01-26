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

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <esd.h>

#include <unistd.h>

#include <libaudacious/util.h>

#include "esdout.h"


static gint fd = 0;
static gpointer buffer;
static gboolean going = FALSE, paused = FALSE, prebuffer, remove_prebuffer;
static gint buffer_size, prebuffer_size, blk_size = 4096;
static gint rd_index = 0, wr_index = 0;
static gint output_time_offset = 0;
static guint64 written = 0, output_bytes = 0;
static gint bps, ebps;
static gint flush;
static gint format, channels, frequency, latency;
static esd_format_t esd_format;
static gint input_bps, input_format, input_frequency, input_channels;
static GThread *buffer_thread;
static gboolean realtime = FALSE;
static void *(*esd_translate) (void *, gint);
static int player_id_unique = 0;

static gint
get_latency(void)
{
    int fd, amount = 0;

#ifndef HAVE_ESD_GET_LATENCY
    esd_server_info_t *info;
#endif

    fd = esd_open_sound(esd_cfg.hostname);

    if (fd == -1)
        return 0;

#ifdef HAVE_ESD_GET_LATENCY
    amount = esd_get_latency(fd);
#else
    info = esd_get_server_info(fd);
    if (info) {
        if (info->format & ESD_STEREO) {
            if (info->format & ESD_BITS16)
                amount = (44100 * (ESD_BUF_SIZE + 64)) / info->rate;
            else
                amount = (44100 * (ESD_BUF_SIZE + 128)) / info->rate;
        }
        else {
            if (info->format & ESD_BITS16)
                amount = (2 * 44100 * (ESD_BUF_SIZE + 128)) / info->rate;
            else
                amount = (2 * 44100 * (ESD_BUF_SIZE + 256)) / info->rate;
        }
        free(info);
    }
    amount += ESD_BUF_SIZE * 2;
#endif
    esd_close(fd);
    return amount;
}

static void *
esd_stou8(void *data, gint length)
{
    int len = length;
    unsigned char *dat = (unsigned char *) data;
    while (len-- > 0)
        *dat++ ^= 0x80;
    return data;
}

static void *
esd_utos16sw(void *data, gint length)
{
    int len = length;
    short *dat = data;
    while (len > 0) {
        *dat = GUINT16_SWAP_LE_BE(*dat) ^ 0x8000;
        dat++;
        len -= 2;
    }
    return data;
}

static void *
esd_utos16(void *data, gint length)
{
    int len = length;
    short *dat = data;
    while (len > 0) {
        *dat ^= 0x8000;
        dat++;
        len -= 2;
    }
    return data;
}

static void *
esd_16sw(void *data, gint length)
{
    int len = length;
    short *dat = data;
    while (len > 0) {
        *dat = GUINT16_SWAP_LE_BE(*dat);
        dat++;
        len -= 2;
    }
    return data;
}

static void
esdout_setup_format(AFormat fmt, gint rate, gint nch)
{
    gboolean swap_sign = FALSE;
    gboolean swap_16 = FALSE;

    format = fmt;
    frequency = rate;
    channels = nch;
    switch (fmt) {
    case FMT_S8:
        swap_sign = TRUE;
    case FMT_U8:
        esd_format = ESD_BITS8;
        break;
    case FMT_U16_LE:
    case FMT_U16_BE:
    case FMT_U16_NE:
        swap_sign = TRUE;
    case FMT_S16_LE:
    case FMT_S16_BE:
    case FMT_S16_NE:
        esd_format = ESD_BITS16;
        break;
    }

#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    if (fmt == FMT_U16_LE || fmt == FMT_S16_LE)
#else
    if (fmt == FMT_U16_BE || fmt == FMT_S16_BE)
#endif
        swap_16 = TRUE;

    esd_translate = (void *(*)()) NULL;
    if (esd_format == ESD_BITS8) {
        if (swap_sign == TRUE)
            esd_translate = esd_stou8;
    }
    else {
        if (swap_sign == TRUE) {
            if (swap_16 == TRUE)
                esd_translate = esd_utos16sw;
            else
                esd_translate = esd_utos16;
        }
        else {
            if (swap_16 == TRUE)
                esd_translate = esd_16sw;
        }
    }

    bps = rate * nch;
    if (esd_format == ESD_BITS16)
        bps *= 2;
    if (nch == 1)
        esd_format |= ESD_MONO;
    else
        esd_format |= ESD_STEREO;
    esd_format |= ESD_STREAM | ESD_PLAY;

    latency = ((get_latency() * frequency) / 44100) * channels;
    if (format != FMT_U8 && format != FMT_S8)
        latency *= 2;
}


gint
esdout_get_written_time(void)
{
    if (!going)
        return 0;
    return (gint) ((written * 1000) / input_bps);
}

gint
esdout_get_output_time(void)
{
    guint64 bytes;

    if (!fd || !going)
        return 0;

    bytes = output_bytes;
    if (!paused)
        bytes -= (bytes < latency ? bytes : latency);

    return output_time_offset + (gint) ((bytes * 1000) / ebps);
}

gint
esdout_used(void)
{
    if (realtime)
        return 0;
    else {
        if (wr_index >= rd_index)
            return wr_index - rd_index;
        return buffer_size - (rd_index - wr_index);
    }
}

gint
esdout_playing(void)
{
    if (!going)
        return FALSE;
    if (!esdout_used())
        return FALSE;

    return TRUE;
}

gint
esdout_free(void)
{
    if (!realtime) {
        if (remove_prebuffer && prebuffer) {
            prebuffer = FALSE;
            remove_prebuffer = FALSE;
        }
        if (prebuffer)
            remove_prebuffer = TRUE;

        if (rd_index > wr_index)
            return (rd_index - wr_index) - 1;
        return (buffer_size - (wr_index - rd_index)) - 1;
    }
    else {
        if (paused)
            return 0;
        else
            return 1000000;
    }
}

static void
esdout_write_audio(gpointer data, gint length)
{
    AFormat new_format;
    gint new_frequency, new_channels;
    EffectPlugin *ep;

    new_format = input_format;
    new_frequency = input_frequency;
    new_channels = input_channels;

    ep = get_current_effect_plugin();
    if (effects_enabled() && ep && ep->query_format) {
        ep->query_format(&new_format, &new_frequency, &new_channels);
    }

    if (new_format != format || new_frequency != frequency
        || new_channels != channels) {
        output_time_offset += (gint) ((output_bytes * 1000) / ebps);
        output_bytes = 0;
        esdout_setup_format(new_format, new_frequency, new_channels);
        frequency = new_frequency;
        channels = new_channels;
        esd_close(fd);
        esdout_set_audio_params();
    }
    if (effects_enabled() && ep && ep->mod_samples)
        length =
            ep->mod_samples(&data, length, input_format, input_frequency,
                            input_channels);
    if (esd_translate)
        output_bytes += write(fd, esd_translate(data, length), length);
    else
        output_bytes += write(fd, data, length);
}


void
esdout_write(gpointer ptr, gint length)
{
    gint cnt, off = 0;

    if (!realtime) {
        remove_prebuffer = FALSE;

        written += length;
        while (length > 0) {
            cnt = MIN(length, buffer_size - wr_index);
            memcpy((gchar *) buffer + wr_index, (gchar *) ptr + off, cnt);
            wr_index = (wr_index + cnt) % buffer_size;
            length -= cnt;
            off += cnt;

        }
    }
    else {
        if (paused)
            return;
        esdout_write_audio(ptr, length);
        written += length;

    }

}

void
esdout_close(void)
{
    if (!going)
        return;

    going = 0;

    if (!realtime)
        g_thread_join(buffer_thread);
    else
        esd_close(fd);

    wr_index = 0;
    rd_index = 0;
    g_free(esd_cfg.playername);
    esd_cfg.playername = NULL;
}

void
esdout_flush(gint time)
{
    if (!realtime) {
        flush = time;
        while (flush != -1)
            g_usleep(10000);
    }
    else {
        output_time_offset = time;
        written = (guint64) (time / 10) * (guint64) (input_bps / 100);
        output_bytes = 0;
    }
}

void
esdout_pause(short p)
{
    paused = p;
}

gpointer
esdout_loop(gpointer arg)
{
    gint length, cnt;


    while (going) {
        if (esdout_used() > prebuffer_size)
            prebuffer = FALSE;
        if (esdout_used() > 0 && !paused && !prebuffer) {
            length = MIN(blk_size, esdout_used());
            while (length > 0) {
                cnt = MIN(length, buffer_size - rd_index);
                esdout_write_audio((gchar *) buffer + rd_index, cnt);
                rd_index = (rd_index + cnt) % buffer_size;
                length -= cnt;
            }
        }
        else
            g_usleep(10000);

        if (flush != -1) {
            output_time_offset = flush;
            written = (guint64) (flush / 10) * (guint64) (input_bps / 100);
            rd_index = wr_index = output_bytes = 0;
            flush = -1;
            prebuffer = TRUE;
        }

    }

    esd_close(fd);
    g_free(buffer);
    return NULL;
}

void
esdout_set_audio_params(void)
{
    fd = esd_play_stream(esd_format, frequency,
                         esd_cfg.hostname, esd_cfg.playername);
    /* Set the stream's mixer */
    if (fd != -1)
        esdout_mixer_init();
    ebps = frequency * channels;
    if (format == FMT_U16_BE || format == FMT_U16_LE ||
        format == FMT_S16_BE || format == FMT_S16_LE ||
        format == FMT_S16_NE || format == FMT_U16_NE)
        ebps *= 2;
}

gint
esdout_open(AFormat fmt, gint rate, gint nch)
{
    esdout_setup_format(fmt, rate, nch);

    input_format = format;
    input_channels = channels;
    input_frequency = frequency;
    input_bps = bps;

    realtime = xmms_check_realtime_priority();

    if (!realtime) {
        buffer_size = (esd_cfg.buffer_size * input_bps) / 1000;
        if (buffer_size < 8192)
            buffer_size = 8192;
        prebuffer_size = (buffer_size * esd_cfg.prebuffer) / 100;
        if (buffer_size - prebuffer_size < 4096)
            prebuffer_size = buffer_size - 4096;

        buffer = g_malloc0(buffer_size);
    }
    flush = -1;
    prebuffer = 1;
    wr_index = rd_index = output_time_offset = written = output_bytes = 0;
    paused = FALSE;
    remove_prebuffer = FALSE;

    esd_cfg.playername = g_strdup_printf("xmms - plugin (%d) [%d]", getpid(), player_id_unique++);

    if (esd_cfg.hostname)
        g_free(esd_cfg.hostname);
    if (esd_cfg.use_remote)
        esd_cfg.hostname =
            g_strdup_printf("%s:%d", esd_cfg.server, esd_cfg.port);
    else
        esd_cfg.hostname = NULL;

    esdout_set_audio_params();
    if (fd == -1) {
        g_free(esd_cfg.playername);
        esd_cfg.playername = NULL;
        g_free(buffer);
        return 0;
    }
    going = 1;

    if (!realtime)
        buffer_thread = g_thread_create(esdout_loop, NULL, TRUE, NULL);
    return 1;
}

void
esdout_tell(AFormat * fmt, gint * rate, gint * nch)
{
	(*fmt) = format;
	(*rate) = frequency;
	(*nch) = channels;
}
