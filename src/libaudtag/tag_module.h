/*
 * tag_module.h
 * Copyright 2009-2011 Paula Stanciu, Tony Vroon, and John Lindgren
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

#ifndef TAG_MODULE_H
#define TAG_MODULE_H

#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"

typedef Tuple* pTuple;

typedef struct _module {
    char *name;
    int type; /* set to TAG_TYPE_NONE if the module cannot create new tags */
    bool_t(*can_handle_file) (VFSFile *fd);
    bool_t (* read_tag) (Tuple * tuple, VFSFile * handle);
    bool_t (* read_image) (VFSFile * handle, void * * data, int64_t * size);
    bool_t (* write_tag) (const Tuple * tuple, VFSFile * handle);
} tag_module_t;

tag_module_t * find_tag_module (VFSFile * handle, int new_type);

#endif /* TAG_MODULE_H */
