#include <glib.h>
#include <glib/gstdio.h>

#include "ape.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"

tag_module_t ape = {ape_can_handle_file, ape_populate_tuple_from_file, ape_write_tuple_to_file};

/* reading fucntions */

APEv2Header *readAPEHeader(VFSFile *fd)
{
    APEv2Header *header = g_new0(APEv2Header,1);
    header->preamble = read_ASCII(fd,8);
    return NULL;
}

gboolean ape_can_handle_file(VFSFile *f)
{
    return FALSE;
}

Tuple *ape_populate_tuple_from_file(Tuple *tuple,VFSFile *f)
{
    return NULL;
}

gboolean ape_write_tuple_to_file(Tuple* tuple, VFSFile *f)
{
    return FALSE;
}