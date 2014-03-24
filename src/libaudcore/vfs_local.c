/*
 * vfs_local.c
 * Copyright 2009-2014 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include "vfs_local.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "audstrings.h"

#ifdef _WIN32
#define fseeko fseeko64
#define ftello ftello64
#endif

typedef enum {
    OP_NONE,
    OP_READ,
    OP_WRITE
} LocalOp;

typedef struct {
    char * path;
    FILE * stream;
    int64_t cached_size;
    LocalOp last_op;
} LocalFile;

static void * local_fopen (const char * uri, const char * mode)
{
    char * path = uri_to_filename (uri);
    g_return_val_if_fail (path, NULL);

    const char * suffix = "";

#ifdef _WIN32
    if (! strchr (mode, 'b'))  /* binary mode (Windows) */
        suffix = "b";
#else
    if (! strchr (mode, 'e'))  /* close on exec (POSIX) */
        suffix = "e";
#endif

    SCONCAT2 (mode2, mode, suffix);

    FILE * stream = g_fopen (path, mode2);

    if (! stream)
    {
        perror (path);
        str_unref (path);
        return NULL;
    }

    LocalFile * local = g_slice_new (LocalFile);

    local->path = path;
    local->stream = stream;
    local->cached_size = -1;
    local->last_op = OP_NONE;

    return local;
}

static int local_fclose (VFSFile * file)
{
    LocalFile * local = vfs_get_handle (file);

    int result = fclose (local->stream);
    if (result < 0)
        perror (local->path);

    str_unref (local->path);
    g_slice_free (LocalFile, local);

    return result;
}

static int64_t local_fread (void * ptr, int64_t size, int64_t nitems, VFSFile * file)
{
    LocalFile * local = vfs_get_handle (file);

    if (local->last_op == OP_WRITE)
    {
        if (fseeko (local->stream, 0, SEEK_CUR) < 0)  /* flush buffers */
        {
            perror (local->path);
            return 0;
        }
    }

    local->last_op = OP_READ;

    clearerr (local->stream);

    int64_t result = fread (ptr, size, nitems, local->stream);
    if (result < nitems && ferror (local->stream))
        perror (local->path);

    return result;
}

static int64_t local_fwrite (const void * ptr, int64_t size, int64_t nitems, VFSFile * file)
{
    LocalFile * local = vfs_get_handle (file);

    if (local->last_op == OP_READ)
    {
        if (fseeko (local->stream, 0, SEEK_CUR) < 0)  /* flush buffers */
        {
            perror (local->path);
            return 0;
        }
    }

    local->last_op = OP_WRITE;
    local->cached_size = -1;

    clearerr (local->stream);

    int64_t result = fwrite (ptr, size, nitems, local->stream);
    if (result < nitems && ferror (local->stream))
        perror (local->path);

    return result;
}

static int local_fseek (VFSFile * file, int64_t offset, int whence)
{
    LocalFile * local = vfs_get_handle (file);

    int result = fseeko (local->stream, offset, whence);
    if (result < 0)
        perror (local->path);

    if (result == 0)
        local->last_op = OP_NONE;

    return result;
}

static int64_t local_ftell (VFSFile * file)
{
    LocalFile * local = vfs_get_handle (file);
    return ftello (local->stream);
}

static bool_t local_feof (VFSFile * file)
{
    LocalFile * local = vfs_get_handle (file);
    return feof (local->stream);
}

static int local_ftruncate (VFSFile * file, int64_t length)
{
    LocalFile * local = vfs_get_handle (file);

    if (local->last_op != OP_NONE)
    {
        if (fseeko (local->stream, 0, SEEK_CUR) < 0)  /* flush buffers */
        {
            perror (local->path);
            return 0;
        }
    }

    int result = ftruncate (fileno (local->stream), length);
    if (result < 0)
        perror (local->path);

    if (result == 0)
    {
        local->last_op = OP_NONE;
        local->cached_size = length;
    }

    return result;
}

static int64_t local_fsize (VFSFile * file)
{
    LocalFile * local = vfs_get_handle (file);

    if (local->cached_size >= 0)
        return local->cached_size;

    int64_t saved_pos = ftello (local->stream);

    if (local_fseek (file, 0, SEEK_END) < 0)
        return -1;

    int64_t length = ftello (local->stream);

    if (local_fseek (file, saved_pos, SEEK_SET) < 0)
        return -1;

    return length;
}

VFSConstructor vfs_local_vtable = {
    .vfs_fopen_impl = local_fopen,
    .vfs_fclose_impl = local_fclose,
    .vfs_fread_impl = local_fread,
    .vfs_fwrite_impl = local_fwrite,
    .vfs_fseek_impl = local_fseek,
    .vfs_ftell_impl = local_ftell,
    .vfs_feof_impl = local_feof,
    .vfs_ftruncate_impl = local_ftruncate,
    .vfs_fsize_impl = local_fsize
};
