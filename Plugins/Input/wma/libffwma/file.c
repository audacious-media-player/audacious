/*
 * Buffered file io for ffmpeg system
 * Copyright (c) 2001 Fabrice Bellard
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
#include "cutils.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "libaudacious/vfs.h"
#include "mms.h"

/* standard file protocol */

static int file_open(URLContext *h, const char *filename, int flags)
{
    VFSFile *file;

    strstart(filename, "file:", &filename);

    if (flags & URL_WRONLY) {
	file = vfs_fopen(filename, "wb");
    } else {
	file = vfs_fopen(filename, "rb");
    }
    
    if (file == NULL)
        return -ENOENT;
    h->priv_data = file;
    return 0;
}

static int file_read(URLContext *h, unsigned char *buf, int size)
{
    VFSFile *file;
    file = h->priv_data;
    return vfs_fread(buf, 1, size, file);
}

static int file_write(URLContext *h, unsigned char *buf, int size)
{
    VFSFile *file;
    file = h->priv_data;
    return vfs_fwrite(buf, 1, size, file);
}

/* XXX: use llseek */
static offset_t file_seek(URLContext *h, offset_t pos, int whence)
{
    int result = 0;
    VFSFile *file;
    file = h->priv_data;
    result = vfs_fseek(file, pos, whence);
    if (result == 0)
	result = vfs_ftell(file);
    else
        result = -1;
    return result;
}

static int file_close(URLContext *h)
{
    VFSFile *file;
    file = h->priv_data;
    return vfs_fclose(file);
}

URLProtocol file_protocol = {
    "file",
    file_open,
    file_read,
    file_write,
    file_seek,
    file_close,
    NULL
};

/* pipe protocol */

static int pipe_open(URLContext *h, const char *filename, int flags)
{
    int fd;

    if (flags & URL_WRONLY) {
        fd = 1;
    } else {
        fd = 0;
    }
    h->priv_data = (void *)(long)fd;
    return 0;
}

static int pipe_read(URLContext *h, unsigned char *buf, int size)
{
    int fd = (int)(long)h->priv_data;
    return read(fd, buf, size);
}

static int pipe_write(URLContext *h, unsigned char *buf, int size)
{
    int fd = (int)(long)h->priv_data;
    return write(fd, buf, size);
}

static int pipe_close(URLContext *h)
{
    return 0;
}

URLProtocol pipe_protocol = {
    "pipe",
    pipe_open,
    pipe_read,
    pipe_write,
    NULL,
    pipe_close,
    NULL
};

/* new mms protocol stuff --nenolod */

static int _mms_open(URLContext *h, const char *filename, int flags)
{
    mms_t *mms = mms_connect(NULL, NULL, filename, 1);

    if (mms == NULL)
	return -ENOENT;

    h->priv_data = (void *)mms;
    return 0;
}

static int _mms_read(URLContext *h, unsigned char *buf, int size)
{
    mms_t *mms = (mms_t *) h->priv_data;
    return mms_read(NULL, mms, (char*)buf, size);
}

static int _mms_close(URLContext *h)
{
    mms_t *mms = (mms_t *) h->priv_data;
    mms_close(mms);
    return 0;
}

URLProtocol mms_protocol = {
    "mms",
    _mms_open,
    _mms_read,
    NULL,
    NULL,
    _mms_close,
    NULL
};
