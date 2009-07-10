/* Interface of the tagging library */

#ifndef TAG_MODULE_H
#define TAG_MODULE_H

G_BEGIN_DECLS

#include <glib.h>
#include "libaudcore/tuple.h"

mowgli_dictionary_t *tag_modules;
int number_of_modules;
typedef Tuple* pTuple;
typedef struct _module
{
	gboolean (*can_handle) (Tuple *tuple);
	pTuple (*populate_tuple_from_file)(Tuple* tuple);
	gboolean (*write_tuple_to_file) (Tuple* tuple);
} tag_module_t;

/* this function must be modified when including new modules */
void init_tag_modules(void);

tag_module_t *find_tag_module(Tuple* tuple);

G_END_DECLS
#endif /* TAG_MODULE_H */


