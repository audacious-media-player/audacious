/*
 * Buffered I/O for ffmpeg system
 * Copyright (c) 2000,2001 Fabrice Bellard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "avformat.h"
#include "avio.h"
#include <stdarg.h>

#define IO_BUFFER_SIZE 32768

int init_put_byte(ByteIOContext *s,
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  void (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*seek)(void *opaque, offset_t offset, int whence))
{
    s->buffer = buffer;
    s->buffer_size = buffer_size;
    s->buf_ptr = buffer;
    s->write_flag = write_flag;
    if (!s->write_flag) 
        s->buf_end = buffer;
    else
        s->buf_end = buffer + buffer_size;
    s->opaque = opaque;
    s->write_packet = write_packet;
    s->read_packet = read_packet;
    s->seek = seek;
    s->pos = 0;
    s->must_flush = 0;
    s->eof_reached = 0;
    s->is_streamed = 0;
    s->max_packet_size = 0;
    return 0;
}
                  
offset_t url_fseek(ByteIOContext *s, offset_t offset, int whence)
{
    offset_t offset1;

    if (whence != SEEK_CUR && whence != SEEK_SET)
        return -EINVAL;
   
    {
        if (whence == SEEK_CUR) {
            offset1 = s->pos - (s->buf_end - s->buffer) + (s->buf_ptr - s->buffer);
            if (offset == 0)
                return offset1;
            offset += offset1;
        }
        offset1 = offset - (s->pos - (s->buf_end - s->buffer));
        if (offset1 >= 0 && offset1 <= (s->buf_end - s->buffer)) {
            /* can do the seek inside the buffer */
            s->buf_ptr = s->buffer + offset1;
        } else {
            if (!s->seek)
                return -EPIPE;
            s->buf_ptr = s->buffer;
            s->buf_end = s->buffer;
            s->seek(s->opaque, offset, SEEK_SET);
            s->pos = offset;
        }
        s->eof_reached = 0;
    }
    return offset;
}

void url_fskip(ByteIOContext *s, offset_t offset)
{
    url_fseek(s, offset, SEEK_CUR);
}

offset_t url_ftell(ByteIOContext *s)
{
    return url_fseek(s, 0, SEEK_CUR);
}

int url_feof(ByteIOContext *s)
{
    return s->eof_reached;
}

/* Input stream */

static void fill_buffer(ByteIOContext *s)
{
    int len;

    /* no need to do anything if EOF already reached */
    if (s->eof_reached)
        return;
    len = s->read_packet(s->opaque, s->buffer, s->buffer_size);
    if (len <= 0) {
        /* do not modify buffer if EOF reached so that a seek back can
           be done without rereading data */
        s->eof_reached = 1;
    } else {
        s->pos += len;
        s->buf_ptr = s->buffer;
        s->buf_end = s->buffer + len;
    }
}

/* NOTE: return 0 if EOF, so you cannot use it if EOF handling is
   necessary */
/* XXX: put an inline version */
int get_byte(ByteIOContext *s)
{
    if (s->buf_ptr < s->buf_end) {
        return *s->buf_ptr++;
    } else {
        fill_buffer(s);
        if (s->buf_ptr < s->buf_end)
            return *s->buf_ptr++;
        else
            return 0;
    }
}

/* NOTE: return URL_EOF (-1) if EOF */
int url_fgetc(ByteIOContext *s)
{
    if (s->buf_ptr < s->buf_end) {
        return *s->buf_ptr++;
    } else {
        fill_buffer(s);
        if (s->buf_ptr < s->buf_end)
            return *s->buf_ptr++;
        else
            return URL_EOF;
    }
}

int get_buffer(ByteIOContext *s, unsigned char *buf, int size)
{
    int len, size1;

    size1 = size;
    while (size > 0) {
        len = s->buf_end - s->buf_ptr;
        if (len > size)
            len = size;
        if (len == 0) {
            fill_buffer(s);
            len = s->buf_end - s->buf_ptr;
            if (len == 0)
                break;
        } else {
            memcpy(buf, s->buf_ptr, len);
            buf += len;
            s->buf_ptr += len;
            size -= len;
        }
    }
    return size1 - size;
}

unsigned int get_le16(ByteIOContext *s)
{
    unsigned int val;
    val = get_byte(s);
    val |= get_byte(s) << 8;
    return val;
}

unsigned int get_le32(ByteIOContext *s)
{
    unsigned int val;
    val = get_byte(s);
    val |= get_byte(s) << 8;
    val |= get_byte(s) << 16;
    val |= get_byte(s) << 24;
    return val;
}

uint64_t get_le64(ByteIOContext *s)
{
    uint64_t val;
    val = (uint64_t)get_le32(s);
    val |= (uint64_t)get_le32(s) << 32;
    return val;
}

unsigned int get_be16(ByteIOContext *s)
{
    unsigned int val;
    val = get_byte(s) << 8;
    val |= get_byte(s);
    return val;
}

unsigned int get_be32(ByteIOContext *s)
{
    unsigned int val;
    val = get_byte(s) << 24;
    val |= get_byte(s) << 16;
    val |= get_byte(s) << 8;
    val |= get_byte(s);
    return val;
}

double get_be64_double(ByteIOContext *s)
{
    union {
        double d;
        uint64_t ull;
    } u;

    u.ull = get_be64(s);
    return u.d;
}

char *get_strz(ByteIOContext *s, char *buf, int maxlen)
{
    int i = 0;
    char c;

    while ((c = get_byte(s))) {
        if (i < maxlen-1)
            buf[i++] = c;
    }
    
    buf[i] = 0; /* Ensure null terminated, but may be truncated */

    return buf;
}

uint64_t get_be64(ByteIOContext *s)
{
    uint64_t val;
    val = (uint64_t)get_be32(s) << 32;
    val |= (uint64_t)get_be32(s);
    return val;
}

/* link with avio functions */

#define	url_write_packet NULL

static int url_read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    URLContext *h = opaque;
    return url_read(h, buf, buf_size);
}

static int url_seek_packet(void *opaque, int64_t offset, int whence)
{
    URLContext *h = opaque;
    url_seek(h, offset, whence);
    return 0;
}

int url_fdopen(ByteIOContext *s, URLContext *h)
{
    uint8_t *buffer;
    int buffer_size, max_packet_size;

    
    max_packet_size = url_get_max_packet_size(h);
    if (max_packet_size) {
        buffer_size = max_packet_size; /* no need to bufferize more than one packet */
    } else {
        buffer_size = IO_BUFFER_SIZE;
    }
    buffer = malloc(buffer_size);
    if (!buffer)
        return -ENOMEM;

    if (init_put_byte(s, buffer, buffer_size, 
                      (h->flags & URL_WRONLY) != 0, h,
                      url_read_packet, url_write_packet, url_seek_packet) < 0) {
        free(buffer);
        return -EIO;
    }
    s->is_streamed = h->is_streamed;
    s->max_packet_size = max_packet_size;
    return 0;
}

/* XXX: must be called before any I/O */
int url_setbufsize(ByteIOContext *s, int buf_size)
{
    uint8_t *buffer;
    buffer = malloc(buf_size);
    if (!buffer)
        return -ENOMEM;

    free(s->buffer);
    s->buffer = buffer;
    s->buffer_size = buf_size;
    s->buf_ptr = buffer;
    if (!s->write_flag) 
        s->buf_end = buffer;
    else
        s->buf_end = buffer + buf_size;
    return 0;
}

/* NOTE: when opened as read/write, the buffers are only used for
   reading */
int url_fopen(ByteIOContext *s, const char *filename, int flags)
{
    URLContext *h;
    int err;

    err = url_open(&h, filename, flags);
    if (err < 0)
        return err;
    err = url_fdopen(s, h);
    if (err < 0) {
        url_close(h);
        return err;
    }
    return 0;
}

int url_fclose(ByteIOContext *s)
{
    URLContext *h = s->opaque;
    
    free(s->buffer);
    memset(s, 0, sizeof(ByteIOContext));
    return url_close(h);
}

URLContext *url_fileno(ByteIOContext *s)
{
    return s->opaque;
}

/* note: unlike fgets, the EOL character is not returned and a whole
   line is parsed. return NULL if first char read was EOF */
char *url_fgets(ByteIOContext *s, char *buf, int buf_size)
{
    int c;
    char *q;

    c = url_fgetc(s);
    if (c == EOF)
        return NULL;
    q = buf;
    for(;;) {
        if (c == EOF || c == '\n')
            break;
        if ((q - buf) < buf_size - 1)
            *q++ = c;
        c = url_fgetc(s);
    }
    if (buf_size > 0)
        *q = '\0';
    return buf;
}

/* 
 * Return the maximum packet size associated to packetized buffered file
 * handle. If the file is not packetized (stream like http or file on
 * disk), then 0 is returned.
 * 
 * @param h buffered file handle
 * @return maximum packet size in bytes
 */
int url_fget_max_packet_size(ByteIOContext *s)
{
    return s->max_packet_size;
}

