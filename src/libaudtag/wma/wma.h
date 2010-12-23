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

/* TAG plugin API */
gboolean wma_can_handle_file(VFSFile *f);
gboolean wma_read_tag (Tuple * tuple, VFSFile * handle);
gboolean wma_write_tag (Tuple * tuple, VFSFile * handle);

#endif /* _WMA_H */
