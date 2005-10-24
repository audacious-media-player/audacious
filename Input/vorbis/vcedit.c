/* This program is licensed under the GNU Library General Public License, version 2,
 * a copy of which is included with this program (LICENCE.LGPL).
 *
 * (c) 2000-2001 Michael Smith <msmith@labyrinth.net.au>
 *
 *
 * Comment editing backend, suitable for use by nice frontend interfaces.
 *
 */
#include "config.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "vcedit.h"
#include <libaudacious/vfs.h>

#define CHUNKSIZE 4096

vcedit_state *
vcedit_new_state(void)
{
    return g_new0(vcedit_state, 1);
}

char *
vcedit_error(vcedit_state * state)
{
    return state->lasterror;
}

vorbis_comment *
vcedit_comments(vcedit_state * state)
{
    return state->vc;
}

static void
vcedit_clear_internals(vcedit_state * state)
{
    if (state->vc) {
        vorbis_comment_clear(state->vc);
        g_free(state->vc);
        state->vc = NULL;
    }
    if (state->os) {
        ogg_stream_clear(state->os);
        g_free(state->os);
        state->os = NULL;
    }
    if (state->oy) {
        ogg_sync_clear(state->oy);
        g_free(state->oy);
        state->oy = NULL;
    }
    if (state->vendor) {
        g_free(state->vendor);
        state->vendor = NULL;
    }
}

void
vcedit_clear(vcedit_state * state)
{
    if (state) {
        vcedit_clear_internals(state);
        g_free(state);
    }
}

/* Next two functions pulled straight from libvorbis, apart from one change
 * - we don't want to overwrite the vendor string.
 */
static void
_v_writestring(oggpack_buffer * o, char *s, int len)
{
    while (len--) {
        oggpack_write(o, *s++, 8);
    }
}

static int
_commentheader_out(vorbis_comment * vc, char *vendor, ogg_packet * op)
{
    oggpack_buffer opb;

    oggpack_writeinit(&opb);

    /* preamble */
    oggpack_write(&opb, 0x03, 8);
    _v_writestring(&opb, "vorbis", 6);

    /* vendor */
    oggpack_write(&opb, strlen(vendor), 32);
    _v_writestring(&opb, vendor, strlen(vendor));

    /* comments */
    oggpack_write(&opb, vc->comments, 32);
    if (vc->comments) {
        int i;
        for (i = 0; i < vc->comments; i++) {
            if (vc->user_comments[i]) {
                oggpack_write(&opb, vc->comment_lengths[i], 32);
                _v_writestring(&opb, vc->user_comments[i],
                               vc->comment_lengths[i]);
            }
            else {
                oggpack_write(&opb, 0, 32);
            }
        }
    }
    oggpack_write(&opb, 1, 1);

    op->packet = _ogg_malloc(oggpack_bytes(&opb));
    memcpy(op->packet, opb.buffer, oggpack_bytes(&opb));

    op->bytes = oggpack_bytes(&opb);
    op->b_o_s = 0;
    op->e_o_s = 0;
    op->granulepos = 0;

    return 0;
}

static int
_blocksize(vcedit_state * s, ogg_packet * p)
{
    int this = vorbis_packet_blocksize(&s->vi, p);
    int ret = (this + s->prevW) / 4;

    if (!s->prevW) {
        s->prevW = this;
        return 0;
    }

    s->prevW = this;
    return ret;
}

static int
_fetch_next_packet(vcedit_state * s, ogg_packet * p, ogg_page * page)
{
    int result;
    char *buffer;
    int bytes;

    result = ogg_stream_packetout(s->os, p);

    if (result > 0)
        return 1;
    else {
        if (s->eosin)
            return 0;
        while (ogg_sync_pageout(s->oy, page) <= 0) {
            buffer = ogg_sync_buffer(s->oy, CHUNKSIZE);
            bytes = s->read(buffer, 1, CHUNKSIZE, s->in);
            ogg_sync_wrote(s->oy, bytes);
            if (bytes == 0)
                return 0;
        }
        if (ogg_page_eos(page))
            s->eosin = 1;
        else if (ogg_page_serialno(page) != s->serial) {
            s->eosin = 1;
            s->extrapage = 1;
            return 0;
        }

        ogg_stream_pagein(s->os, page);
        return _fetch_next_packet(s, p, page);
    }
}


int
vcedit_open(vcedit_state * state, VFSFile * in)
{
    return vcedit_open_callbacks(state, (void *) in,
                                 (vcedit_read_func) vfs_fread,
                                 (vcedit_write_func) vfs_fwrite);
}

int
vcedit_open_callbacks(vcedit_state * state, void *in,
                      vcedit_read_func read_func,
                      vcedit_write_func write_func)
{
    char *buffer;
    int bytes, i;
    ogg_packet *header;
    ogg_packet header_main;
    ogg_packet header_comments;
    ogg_packet header_codebooks;
    ogg_page og;

    state->in = in;
    state->read = read_func;
    state->write = write_func;

    state->oy = g_new(ogg_sync_state, 1);
    ogg_sync_init(state->oy);

    buffer = ogg_sync_buffer(state->oy, CHUNKSIZE);

    bytes = state->read(buffer, 1, CHUNKSIZE, state->in);

    ogg_sync_wrote(state->oy, bytes);

    if (ogg_sync_pageout(state->oy, &og) != 1) {
        if (bytes < CHUNKSIZE)
            state->lasterror = "Input truncated or empty.";
        else
            state->lasterror = "Input is not an Ogg bitstream.";
        goto err;
    }

    state->serial = ogg_page_serialno(&og);

    state->os = g_new(ogg_stream_state, 1);
    ogg_stream_init(state->os, state->serial);

    vorbis_info_init(&state->vi);

    state->vc = g_new(vorbis_comment, 1);
    vorbis_comment_init(state->vc);

    if (ogg_stream_pagein(state->os, &og) < 0) {
        state->lasterror = "Error reading first page of Ogg bitstream.";
        goto err;
    }

    if (ogg_stream_packetout(state->os, &header_main) != 1) {
        state->lasterror = "Error reading initial header packet.";
        goto err;
    }

    if (vorbis_synthesis_headerin(&state->vi, state->vc, &header_main) < 0) {
        state->lasterror = "Ogg bitstream does not contain vorbis data.";
        goto err;
    }

    state->mainlen = header_main.bytes;
    state->mainbuf = g_malloc(state->mainlen);
    memcpy(state->mainbuf, header_main.packet, header_main.bytes);

    i = 0;
    header = &header_comments;
    while (i < 2) {
        while (i < 2) {
            int result = ogg_sync_pageout(state->oy, &og);
            if (result == 0)
                break;          /* Too little data so far */
            else if (result == 1) {
                ogg_stream_pagein(state->os, &og);
                while (i < 2) {
                    result = ogg_stream_packetout(state->os, header);
                    if (result == 0)
                        break;
                    if (result == -1) {
                        state->lasterror = "Corrupt secondary header.";
                        goto err;
                    }
                    vorbis_synthesis_headerin(&state->vi, state->vc, header);
                    if (i == 1) {
                        state->booklen = header->bytes;
                        state->bookbuf = g_malloc(state->booklen);
                        memcpy(state->bookbuf, header->packet, header->bytes);
                    }
                    i++;
                    header = &header_codebooks;
                }
            }
        }

        buffer = ogg_sync_buffer(state->oy, CHUNKSIZE);
        bytes = state->read(buffer, 1, CHUNKSIZE, state->in);
        if (bytes == 0 && i < 2) {
            state->lasterror = "EOF before end of vorbis headers.";
            goto err;
        }
        ogg_sync_wrote(state->oy, bytes);
    }

    /* Copy the vendor tag */
    state->vendor = g_malloc(strlen(state->vc->vendor) + 1);
    strcpy(state->vendor, state->vc->vendor);

    /* Headers are done! */
    return 0;

  err:
    vcedit_clear_internals(state);
    return -1;
}

#if 0
static void
dump_state(vcedit_state * state)
{
}
#endif

int
vcedit_write(vcedit_state * state, void *out)
{

    ogg_stream_state streamout;
    ogg_packet header_main;
    ogg_packet header_comments;
    ogg_packet header_codebooks;

    ogg_page ogout, ogin;
    ogg_packet op;
    ogg_int64_t granpos = 0;
    int result;
    char *buffer;
    int bytes;
    int needflush = 0, needout = 0;

    state->eosin = 0;
    state->extrapage = 0;

    header_main.bytes = state->mainlen;
    header_main.packet = state->mainbuf;
    header_main.b_o_s = 1;
    header_main.e_o_s = 0;
    header_main.granulepos = 0;

    header_codebooks.bytes = state->booklen;
    header_codebooks.packet = state->bookbuf;
    header_codebooks.b_o_s = 0;
    header_codebooks.e_o_s = 0;
    header_codebooks.granulepos = 0;

    ogg_stream_init(&streamout, state->serial);

    _commentheader_out(state->vc, state->vendor, &header_comments);

    ogg_stream_packetin(&streamout, &header_main);
    ogg_stream_packetin(&streamout, &header_comments);
    ogg_stream_packetin(&streamout, &header_codebooks);

    while ((result = ogg_stream_flush(&streamout, &ogout))) {
        if (state->write(ogout.header, 1, ogout.header_len, out) !=
            (size_t) ogout.header_len)
            goto cleanup;
        if (state->write(ogout.body, 1, ogout.body_len, out) !=
            (size_t) ogout.body_len)
            goto cleanup;
    }

    while (_fetch_next_packet(state, &op, &ogin)) {
        int size;
        size = _blocksize(state, &op);
        granpos += size;

        if (needflush) {
            if (ogg_stream_flush(&streamout, &ogout)) {
                if (state->write(ogout.header, 1, ogout.header_len,
                                 out) != (size_t) ogout.header_len)
                    goto cleanup;
                if (state->write(ogout.body, 1, ogout.body_len,
                                 out) != (size_t) ogout.body_len)
                    goto cleanup;
            }
        }
        else if (needout) {
            if (ogg_stream_pageout(&streamout, &ogout)) {
                if (state->write(ogout.header, 1, ogout.header_len,
                                 out) != (size_t) ogout.header_len)
                    goto cleanup;
                if (state->write(ogout.body, 1, ogout.body_len,
                                 out) != (size_t) ogout.body_len)
                    goto cleanup;
            }
        }

        needflush = needout = 0;

        if (op.granulepos == -1) {
            op.granulepos = granpos;
            ogg_stream_packetin(&streamout, &op);
        }
        else {                  /* granulepos is set, validly. Use it, and force a flush to 
                                   account for shortened blocks (vcut) when appropriate */
            if (granpos > op.granulepos) {
                granpos = op.granulepos;
                ogg_stream_packetin(&streamout, &op);
                needflush = 1;
            }
            else {
                ogg_stream_packetin(&streamout, &op);
                needout = 1;
            }
        }
    }

    streamout.e_o_s = 1;
    while (ogg_stream_flush(&streamout, &ogout)) {
        if (state->write(ogout.header, 1, ogout.header_len,
                         out) != (size_t) ogout.header_len)
            goto cleanup;
        if (state->write(ogout.body, 1, ogout.body_len,
                         out) != (size_t) ogout.body_len)
            goto cleanup;
    }

    /* FIXME: freeing this here probably leaks memory */
    /* Done with this, now */
    vorbis_info_clear(&state->vi);

    if (state->extrapage) {
        if (state->write(ogin.header, 1, ogin.header_len,
                         out) != (size_t) ogin.header_len)
            goto cleanup;
        if (state->write(ogin.body, 1, ogin.body_len, out) !=
            (size_t) ogin.body_len)
            goto cleanup;
    }

    state->eosin = 0;           /* clear it, because not all paths to here do */
    while (!state->eosin) {     /* We reached eos, not eof */
        /* We copy the rest of the stream (other logical streams)
         * through, a page at a time. */
        while (1) {
            result = ogg_sync_pageout(state->oy, &ogout);
            if (result == 0)
                break;
            if (result < 0)
                state->lasterror = "Corrupt or missing data, continuing...";
            else {
                /* Don't bother going through the rest, we can just 
                 * write the page out now */
                if (state->write(ogout.header, 1, ogout.header_len,
                                 out) != (size_t) ogout.header_len)
                    goto cleanup;
                if (state->write(ogout.body, 1, ogout.body_len, out) !=
                    (size_t) ogout.body_len)
                    goto cleanup;
            }
        }
        buffer = ogg_sync_buffer(state->oy, CHUNKSIZE);
        bytes = state->read(buffer, 1, CHUNKSIZE, state->in);
        ogg_sync_wrote(state->oy, bytes);
        if (bytes == 0) {
            state->eosin = 1;
            break;
        }
    }


  cleanup:
    ogg_stream_clear(&streamout);
    ogg_packet_clear(&header_comments);

    g_free(state->mainbuf);
    g_free(state->bookbuf);

    vcedit_clear_internals(state);
    if (!state->eosin) {
        state->lasterror =
            "Error writing stream to output. "
            "Output stream may be corrupted or truncated.";
        return -1;
    }

    return 0;
}
