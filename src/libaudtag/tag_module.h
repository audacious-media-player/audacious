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

/* Interface of the tagging library */

#ifndef TAG_MODULE_H
#define TAG_MODULE_H

G_BEGIN_DECLS

#include <glib.h>
#include <mowgli.h>
#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"

mowgli_list_t tag_modules;
int number_of_modules;
typedef Tuple* pTuple;

typedef struct _module {
    gchar *name;
    gint type; /* set to TAG_TYPE_NONE if the module cannot create new tags */
    gboolean(*can_handle_file) (VFSFile *fd);
    gboolean (* read_tag) (Tuple * tuple, VFSFile * handle);
    gboolean (* read_image) (VFSFile * handle, void * * data, gint * size);
    gboolean (* write_tag) (const Tuple * tuple, VFSFile * handle);

    mowgli_node_t node;
} tag_module_t;

/* this function must be modified when including new modules */
void init_tag_modules(void);

tag_module_t * find_tag_module (VFSFile * handle, gint new_type);

G_END_DECLS
#endif /* TAG_MODULE_H */
