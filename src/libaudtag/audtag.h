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

void tag_init(void);
void tag_terminate(void);

Tuple *tag_tuple_read(Tuple *tuple, VFSFile *fd);

gboolean tag_tuple_write_to_file(Tuple *tuple, VFSFile *fd);

G_END_DECLS
#endif /* AUDTAG_H */


