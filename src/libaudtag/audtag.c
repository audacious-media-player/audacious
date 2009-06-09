
#include <glib.h>
#include "libaudcore/tuple.h"
#include "audtag.h"
#include "tag_module.h"

/* The tuple's file-related attributes are already set */

Tuple *audtag_tuple_read(Tuple* tuple){
	g_return_val_if_fail(tuple != NULL, NULL); 

	tag_module_t *mod = find_tag_module(tuple);
	g_return_val_if_fail(mod != NULL, NULL);

	return mod->populate_tuple_from_file(tuple);
	}

gint audtag_tuple_write_to_file(Tuple *tuple){
	tag_module_t *mod = find_tag_module(tuple);
	g_return_val_if_fail(mod != NULL, -1);

	return mod->write_tuple_to_file(tuple);
	}


