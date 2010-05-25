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

#ifndef _WMA_H

#define _WMA_H

#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"
#include "wma_fmt.h"

#ifndef TAG_WMA_MODULE_H
/* static functions */

/* reads the whole structure, containing the data */
static GenericHeader *read_generic_header(VFSFile *f, gboolean read_data);

static HeaderObj *read_top_header_object(VFSFile *f);

static gboolean write_top_header_object(VFSFile *f, HeaderObj *h);

static ContentDescriptor **read_descriptors(VFSFile *f, guint count, Tuple*t, gboolean populate_tuple);

static ContentDescrObj *read_content_descr_obj(VFSFile *f);

static ExtContentDescrObj *read_ext_content_descr_obj(VFSFile * f, Tuple *t, gboolean populate_tuple);

static long ftell_object_by_guid(VFSFile *f, GUID *g);
static long ftell_object_by_str(VFSFile *f, gchar *s);
#endif

/* TAG plugin API */
gboolean wma_can_handle_file(VFSFile *f);
gboolean wma_read_tag (Tuple * tuple, VFSFile * handle);
gboolean wma_write_tag (Tuple * tuple, VFSFile * handle);

#endif /* _WMA_H */
