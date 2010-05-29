/*
 * Copyright 2009 Paula Stanciu
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

/* External Interface of the tagging library */

#ifndef AUDTAG_H
#define AUDTAG_H

G_BEGIN_DECLS

#include <glib.h>
#include <mowgli.h>
#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"
#include "config.h"

enum
{
    TAG_TYPE_NONE = 0,
    TAG_TYPE_APE,
};

void tag_init(void);
void tag_terminate(void);

gboolean tag_tuple_read (Tuple * tuple, VFSFile *fd);
gboolean tag_image_read (VFSFile * handle, void * * data, gint * size);

/* new_type specifies the type of tag (see the TAG_TYPE_* enum) that should be
 * written if the file does not have any existing tag. */
gboolean tag_tuple_write (Tuple * tuple, VFSFile * handle, gint new_type);

/* deprecated, use tag_tuple_write */
gboolean tag_tuple_write_to_file (Tuple * tuple, VFSFile * handle);

G_END_DECLS
#endif /* AUDTAG_H */
