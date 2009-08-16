/* Interface of the tagging library */

#ifndef TAG_MODULE_H
#define TAG_MODULE_H

G_BEGIN_DECLS

#include <glib.h>
#include <mowgli.h>
#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"
        
mowgli_list_t tag_modules;
int number_of_modules;
typedef Tuple* pTuple;

typedef struct _module {
    gboolean(*can_handle_file) (VFSFile *fd);
    pTuple(*populate_tuple_from_file)(Tuple *tuple, VFSFile* fd);
    gboolean(*write_tuple_to_file) (Tuple * tuple, VFSFile *fd);
} tag_module_t;

/* this function must be modified when including new modules */
void init_tag_modules(void);

tag_module_t *find_tag_module(VFSFile *fd);

G_END_DECLS
#endif /* TAG_MODULE_H */
