#include "libaudcore/tuple.h"
#include "audtag.h"
#include "tag_module.h"
#include "util.h"

void tag_init(void) {
    init_tag_modules();
}

/* The tuple's file-related attributes are already set */

Tuple *tag_tuple_read(Tuple *tuple,VFSFile *fd) {
    tag_module_t *mod = find_tag_module(fd);
    g_return_val_if_fail(mod != NULL, NULL);
    AUDDBG("OK\n");
    return mod->populate_tuple_from_file(tuple,fd);
}

gboolean tag_tuple_write_to_file(Tuple *tuple, VFSFile *fd) {
    g_return_val_if_fail((tuple != NULL) && (fd != NULL), FALSE);
    tag_module_t *mod = find_tag_module(fd);
    g_return_val_if_fail(mod != NULL, -1);

    return mod->write_tuple_to_file(tuple, fd);
}
