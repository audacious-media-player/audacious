#include <glib.h>
#include <glib/gstdio.h>

#include "id3.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"



tag_module_t id3 = {id3_can_handle_file, id3_populate_tuple_from_file, id3_write_tuple_to_file};

gboolean id3_can_handle_file(VFSFile *f)
{
    return FALSE;
}

Tuple *id3_populate_tuple_from_file(VFSFile *f)
{
    return NULL;
}

gboolean id3_write_tuple_to_file(Tuple* tuple, VFSFile *f)
{
    return FALSE;
}