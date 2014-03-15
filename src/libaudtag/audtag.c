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

#include <libaudcore/tuple.h>

#include "audtag.h"
#include "tag_module.h"
#include "util.h"

bool_t tag_verbose = FALSE;

EXPORT void tag_set_verbose (bool_t verbose)
{
    tag_verbose = verbose;
}

/* The tuple's file-related attributes are already set */

EXPORT bool_t tag_tuple_read (Tuple * tuple, VFSFile * handle)
{
    tag_module_t * module = find_tag_module (handle, TAG_TYPE_NONE);

    if (! module || ! module->read_tag)
    {
        TAGDBG ("read_tag() not supported for %s\n", vfs_get_filename (handle));
        return FALSE;
    }

    return module->read_tag (tuple, handle);
}

EXPORT bool_t tag_image_read (VFSFile * handle, void * * data, int64_t * size)
{
    tag_module_t * module = find_tag_module (handle, TAG_TYPE_NONE);

    if (! module || ! module->read_image)
    {
        TAGDBG ("read_image() not supported for %s\n", vfs_get_filename (handle));
        return FALSE;
    }

    return module->read_image (handle, data, size);
}

EXPORT bool_t tag_tuple_write (const Tuple * tuple, VFSFile * handle, int new_type)
{
    tag_module_t * module = find_tag_module (handle, new_type);

    if (! module || ! module->write_tag)
    {
        TAGDBG ("write_tag() not supported for %s\n", vfs_get_filename (handle));
        return FALSE;
    }

    return module->write_tag (tuple, handle);
}

EXPORT bool_t tag_update_stream_metadata (Tuple * tuple, VFSFile * handle)
{
    bool_t updated = FALSE;
    char * old, * new;
    int value;

    old = tuple_get_str (tuple, FIELD_TITLE);
    new = vfs_get_metadata (handle, "track-name");

    if (new && (! old || strcmp (old, new)))
    {
        tuple_set_str (tuple, FIELD_TITLE, new);
        updated = TRUE;
    }

    str_unref (old);
    str_unref (new);

    old = tuple_get_str (tuple, FIELD_ARTIST);
    new = vfs_get_metadata (handle, "stream-name");

    if (new && (! old || strcmp (old, new)))
    {
        tuple_set_str (tuple, FIELD_ARTIST, new);
        updated = TRUE;
    }

    str_unref (old);
    str_unref (new);

    new = vfs_get_metadata (handle, "content-bitrate");
    value = new ? atoi (new) / 1000 : 0;

    if (value && value != tuple_get_int (tuple, FIELD_BITRATE))
    {
        tuple_set_int (tuple, FIELD_BITRATE, value);
        updated = TRUE;
    }

    str_unref (new);

    return updated;
}

/* deprecated */
EXPORT bool_t tag_tuple_write_to_file (Tuple * tuple, VFSFile * handle)
{
    return tag_tuple_write (tuple, handle, TAG_TYPE_NONE);
}
