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

#include <glib.h>

#include <libaudcore/tuple.h>

#include "audtag.h"
#include "config.h"
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

    if (module == NULL)
        return FALSE;

    return module->read_tag (tuple, handle);
}

EXPORT bool_t tag_image_read (VFSFile * handle, void * * data, int64_t * size)
{
    tag_module_t * module = find_tag_module (handle, TAG_TYPE_NONE);

    if (module == NULL || module->read_image == NULL)
        return FALSE;

    return module->read_image (handle, data, size);
}

EXPORT bool_t tag_tuple_write (const Tuple * tuple, VFSFile * handle, int new_type)
{
    tag_module_t * module = find_tag_module (handle, new_type);

    if (module == NULL)
        return FALSE;

    return module->write_tag (tuple, handle);
}

/* deprecated */
EXPORT bool_t tag_tuple_write_to_file (Tuple * tuple, VFSFile * handle)
{
    return tag_tuple_write (tuple, handle, TAG_TYPE_NONE);
}
