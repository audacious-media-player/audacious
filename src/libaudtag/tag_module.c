#include "libaudcore/tuple.h"

#include "tag_module.h"

#include "wma/module.h"

void init_tag_modules(void)
{
    tag_modules = mowgli_dictionary_create(strcasecmp);
    mowgli_dictionary_add(tag_modules,"wma", &wma);

}

tag_module_t *find_tag_module(Tuple* tuple)
{
    mowgli_dictionary_iteration_state_t *state = NULL;
    mowgli_dictionary_foreach_start(tag_modules, state);

    while(1)
    {
        tag_module_t *mod = (tag_module_t*)mowgli_dictionary_foreach_cur(tag_modules, state);    
        if(mod == NULL)
            return NULL;
        if(mod->can_handle(tuple))
            return mod;
        mowgli_dictionary_foreach_next(tag_modules, state);

    }
}


