/*
 * audtag.c
 * Copyright 2009-2011 Paula Stanciu and John Lindgren
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>

#include "audtag.h"
#include "tag_module.h"
#include "util.h"

bool tag_verbose = false;

EXPORT void tag_set_verbose (bool verbose)
{
    tag_verbose = verbose;
}

/* The tuple's file-related attributes are already set */

EXPORT bool tag_tuple_read (Tuple & tuple, VFSFile * handle)
{
    tag_module_t * module = find_tag_module (handle, TAG_TYPE_NONE);

    if (! module || ! module->read_tag)
    {
        AUDDBG ("read_tag() not supported for %s\n", vfs_get_filename (handle));
        return false;
    }

    return module->read_tag (tuple, handle);
}

EXPORT bool tag_image_read (VFSFile * handle, void * * data, int64_t * size)
{
    tag_module_t * module = find_tag_module (handle, TAG_TYPE_NONE);

    if (! module || ! module->read_image)
    {
        AUDDBG ("read_image() not supported for %s\n", vfs_get_filename (handle));
        return false;
    }

    return module->read_image (handle, data, size);
}

EXPORT bool tag_tuple_write (const Tuple & tuple, VFSFile * handle, int new_type)
{
    tag_module_t * module = find_tag_module (handle, new_type);

    if (! module || ! module->write_tag)
    {
        AUDDBG ("write_tag() not supported for %s\n", vfs_get_filename (handle));
        return false;
    }

    return module->write_tag (tuple, handle);
}

EXPORT bool tag_update_stream_metadata (Tuple & tuple, VFSFile * handle)
{
    bool updated = false;
    int value;

    String old = tuple.get_str (FIELD_TITLE);
    String val = vfs_get_metadata (handle, "track-name");

    if (val && (! old || strcmp (old, val)))
    {
        tuple.set_str (FIELD_TITLE, val);
        updated = true;
    }

    old = tuple.get_str (FIELD_ARTIST);
    val = vfs_get_metadata (handle, "stream-name");

    if (val && (! old || strcmp (old, val)))
    {
        tuple.set_str (FIELD_ARTIST, val);
        updated = true;
    }

    val = vfs_get_metadata (handle, "content-bitrate");
    value = val ? atoi (val) / 1000 : 0;

    if (value && value != tuple.get_int (FIELD_BITRATE))
    {
        tuple.set_int (FIELD_BITRATE, value);
        updated = true;
    }

    return updated;
}
