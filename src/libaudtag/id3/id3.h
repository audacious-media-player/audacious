#ifndef ID3_H

#define ID3_H

#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"
gboolean id3_can_handle_file(VFSFile *f);

Tuple *id3_populate_tuple_from_file(VFSFile *f);

gboolean id3_write_tuple_to_file(Tuple* tuple, VFSFile *f);

extern tag_module_t id3;

#endif