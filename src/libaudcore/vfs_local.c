/*
 * vfs_local.c
 * Copyright 2009-2013 John Lindgren
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
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "audstrings.h"

#define INT_TO_POINTER(x) ((void *) (ptrdiff_t) (x))
#define POINTER_TO_INT(x) ((int) (ptrdiff_t) (x))

#define local_error(...) do { \
    fprintf (stderr, __VA_ARGS__); \
    fputc ('\n', stderr); \
} while (0)

static void * local_fopen (const char * uri, const char * mode)
{
    bool_t update;
    mode_t mode_flag;
    int handle;

    update = (strchr (mode, '+') != NULL);

    switch (mode[0])
    {
      case 'r':
        mode_flag = update ? O_RDWR : O_RDONLY;
        break;
      case 'w':
        mode_flag = (update ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC;
        break;
      case 'a':
        mode_flag = (update ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
        break;
      default:
        return NULL;
    }

#ifdef O_CLOEXEC
    mode_flag |= O_CLOEXEC;
#endif
#ifdef _WIN32
    mode_flag |= O_BINARY;
#endif

    char * filename = uri_to_filename (uri);
    if (! filename)
        return NULL;

    if (mode_flag & O_CREAT)
#ifdef S_IRGRP
        handle = open (filename, mode_flag, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#else
        handle = open (filename, mode_flag, S_IRUSR | S_IWUSR);
#endif
    else
        handle = open (filename, mode_flag);

    if (handle < 0)
    {
        local_error ("Cannot open %s: %s.", filename, strerror (errno));
        str_unref (filename);
        return NULL;
    }

    str_unref (filename);
    return INT_TO_POINTER (handle);
}

static int local_fclose (VFSFile * file)
{
    int handle = POINTER_TO_INT (vfs_get_handle (file));
    int result = 0;

    if (close (handle) < 0)
    {
        local_error ("close failed: %s.", strerror (errno));
        result = -1;
    }

    return result;
}

static int64_t local_fread (void * ptr, int64_t size, int64_t nitems, VFSFile * file)
{
    int handle = POINTER_TO_INT (vfs_get_handle (file));
    int64_t goal = size * nitems;
    int64_t total = 0;

    while (total < goal)
    {
        int64_t readed = read (handle, (char *) ptr + total, goal - total);

        if (readed < 0)
        {
            local_error ("read failed: %s.", strerror (errno));
            break;
        }

        if (! readed)
            break;

        total += readed;
    }

    return (size > 0) ? total / size : 0;
}

static int64_t local_fwrite (const void * ptr, int64_t size, int64_t nitems,
 VFSFile * file)
{
    int handle = POINTER_TO_INT (vfs_get_handle (file));
    int64_t goal = size * nitems;
    int64_t total = 0;

    while (total < goal)
    {
        int64_t written = write (handle, (char *) ptr + total, goal - total);

        if (written < 0)
        {
            local_error ("write failed: %s.", strerror (errno));
            break;
        }

        total += written;
    }

    return (size > 0) ? total / size : 0;
}

static int local_fseek (VFSFile * file, int64_t offset, int whence)
{
    int handle = POINTER_TO_INT (vfs_get_handle (file));

    if (lseek (handle, offset, whence) < 0)
    {
        local_error ("lseek failed: %s.", strerror (errno));
        return -1;
    }

    return 0;
}

static int64_t local_ftell (VFSFile * file)
{
    int handle = POINTER_TO_INT (vfs_get_handle (file));
    int64_t result = lseek (handle, 0, SEEK_CUR);

    if (result < 0)
        local_error ("lseek failed: %s.", strerror (errno));

    return result;
}

static int local_getc (VFSFile * file)
{
    unsigned char c;

    return (local_fread (& c, 1, 1, file) == 1) ? c : -1;
}

static int local_ungetc (int c, VFSFile * file)
{
    return (! local_fseek (file, -1, SEEK_CUR)) ? c : -1;
}

static bool_t local_feof (VFSFile * file)
{
    int test = local_getc (file);

    if (test < 0)
        return TRUE;

    local_ungetc (test, file);
    return FALSE;
}

static int local_ftruncate (VFSFile * file, int64_t length)
{
    int handle = POINTER_TO_INT (vfs_get_handle (file));

    int result = ftruncate (handle, length);

    if (result < 0)
        local_error ("ftruncate failed: %s.", strerror (errno));

    return result;
}

static int64_t local_fsize (VFSFile * file)
{
    int64_t position, length;

    position = local_ftell (file);

    if (position < 0)
        return -1;

    local_fseek (file, 0, SEEK_END);
    length = local_ftell (file);

    if (length < 0)
        return -1;

    local_fseek (file, position, SEEK_SET);

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
