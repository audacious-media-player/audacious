#include <glib.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "util.h"
#include "tag_module.h"
#include "wma/module.h"
#include "id3/id3.h"
#include "ape/ape.h"
#include "aac/aac.h"

void init_tag_modules(void)
{
    mowgli_node_add(&wma, mowgli_node_create(), &tag_modules);
    mowgli_node_add(&id3, mowgli_node_create(), &tag_modules);
    mowgli_node_add(&ape, mowgli_node_create(), &tag_modules);
    mowgli_node_add(&aac, mowgli_node_create(), &tag_modules);
}

tag_module_t *find_tag_module(VFSFile * fd)
{
   mowgli_node_t *mod, *tmod;
   MOWGLI_LIST_FOREACH_SAFE(mod, tmod, tag_modules.head)
   {
       vfs_fseek(fd,0,SEEK_SET);
     if (((tag_module_t*)(mod->data))->can_handle_file(fd))
       return (tag_module_t*)(mod->data);
   }

    AUDDBG("no module found\n");
    return NULL;
}
