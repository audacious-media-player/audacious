#ifndef TAG_WMA_MODULE_H

#define TAG_WMA_MODULE_H

#include <libaudcore/vfs.h>

gboolean wma_can_handle(VFSFile *fd);

Tuple *wma_populate_tuple_from_file(VFSFile *fd);

gboolean wma_write_tuple_to_file(Tuple* tuple, VFSFile *fd);

extern tag_module_t wma;
#endif
