
#include <glib.h>
#include "libaudcore/tuple.h"
#include "audtag.h"
#include "tag_module.h"

void tag_init(void) {
    init_tag_modules();
}

/* The tuple's file-related attributes are already set */

Tuple *tag_tuple_read(Tuple* tuple) {
    g_return_val_if_fail(tuple != NULL, NULL);

    tag_module_t *mod = find_tag_module(tuple);
    g_return_val_if_fail(mod != NULL, NULL);
    printf("OK\n");
    return mod->populate_tuple_from_file(tuple);

}

gboolean tag_tuple_write_to_file(Tuple *tuple) {
    g_return_val_if_fail(tuple != NULL, FALSE);
    tag_module_t *mod = find_tag_module(tuple);
    g_return_val_if_fail(mod != NULL, -1);

    return mod->write_tuple_to_file(tuple);
}

#if 0 /* FD-based functions may be needed */
gboolean tag_tuple_write_to_fd(tuple, VFSFile *fd) {
    g_return_val_if_fail(fd != NULL, NULL);

    tag_module_t *mod = find_tag_module_fd(fd);
    g_return_val_if_fail(mod != NULL, -1);

    return mod->write_tuple_to_file();
}
#endif
