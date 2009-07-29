#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"
#include "../tag_module.h"
#include "../util.h"
#include "module.h"
#include "wma.h"

gboolean can_handle(VFSFile *fd) {
    return wma_can_handle_file(fd);
}

Tuple *populate_tuple_from_file(VFSFile *fd) {
    return wma_populate_tuple_from_file(fd);
}

gboolean write_tuple_to_file(Tuple *tuple, VFSFile* fd) {
    return wma_write_tuple_to_file(tuple, fd);
}

tag_module_t wma = {can_handle, populate_tuple_from_file, write_tuple_to_file};

