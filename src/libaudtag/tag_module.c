#include "libaudcore/tuple.h"

#include "tag_module.h"

#include "wma/module.h"

/* yeah, this sucks, but couldn't find a better way for adding modules */
void init_tag_modules(void)
{
	 tag_modules = g_list_prepend(tag_modules, &wma);

}

tag_module_t *find_tag_module(Tuple* tuple)
{
	GList * l;
	for (l = tag_modules; l; l = l->next)	
	{
		tag_module_t *mod = l->data;
		if ( mod->can_handle(tuple))
			return mod;
	}
	return NULL;
}


