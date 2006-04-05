/*
 * Shorten container management.
 *   Copyright (c) 2006 William Pitcock <nenolod -at- nenolod.net>
 *   Copyright (c) 2006 Thomas Cort <tcort -at- cs.ubishops.ca>
 *
 * RAW encoder and decoder
 *   Copyright (c) 2001 Fabrice Bellard.
 *   Copyright (c) 2005 Alex Beregszaszi.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "avformat.h"
#include "utils.h"

#define RAW_PACKET_SIZE 1024

static int raw_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    int ret, size;
    //    AVStream *st = s->streams[0];

    size= RAW_PACKET_SIZE;

    ret= av_get_packet(&s->pb, pkt, size);

    pkt->stream_index = 0;
    if (ret <= 0) {
        return AVERROR_IO;
    }
    /* note: we need to modify the packet size here to handle the last
       packet */
    pkt->size = ret;
    return ret;
}

static int raw_read_close(AVFormatContext *s)
{   
    return 0;
}

static int shorten_read_header(AVFormatContext *s,
                               AVFormatParameters *ap)
{
    AVStream *st;

    st = av_new_stream(s, 0);
    if (!st)
        return AVERROR_NOMEM;
    st->codec.codec_type = CODEC_TYPE_AUDIO;
    st->codec.codec_id = CODEC_ID_SHORTEN;
    st->need_parsing = 1;
    /* the parameters will be extracted from the compressed bitstream */
    return 0;
}

AVInputFormat shorten_iformat = {
    "shn",
    "raw shorten",
    0,
    NULL,
    shorten_read_header,
    raw_read_packet,
    raw_read_close,
    .extensions = "shn",
};

int raw_init(void)
{
    av_register_input_format(&shorten_iformat);
    return 0;
}
