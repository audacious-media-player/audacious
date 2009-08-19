#ifndef _WMA_H

#define _WMA_H

#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"
#include "wma_fmt.h"

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

/* interface functions*/
gboolean wma_can_handle_file(VFSFile *f);

Tuple *wma_populate_tuple_from_file(Tuple *t, VFSFile *f);

gboolean wma_write_tuple_to_file(Tuple *t, VFSFile *f);

#endif /* _WMA_H */

