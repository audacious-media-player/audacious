#ifndef APE_H

#define APE_H

#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"

typedef struct  apeheader
{
    gchar *preamble;  //65 bits
    guint32 version;
    guint32 tagSize;
    guint32 itemCount;
    guint32 flags;
    guint64 reserved;
}APEv2Header;


gboolean ape_can_handle_file(VFSFile *f);

Tuple *ape_populate_tuple_from_file(Tuple *tuple,VFSFile *f);

gboolean ape_write_tuple_to_file(Tuple* tuple, VFSFile *f);

extern tag_module_t ape;

#endif