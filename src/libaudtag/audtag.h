/* External Interface of the tagging library */

#ifndef AUDTAG_H
#define AUDTAG_H

G_BEGIN_DECLS

#include <glib.h>
#include "libaudcore/tuple.h"
#include <mowgli.h> 

void tag_init(void);
void tag_terminate(void);

Tuple *tag_tuple_read(Tuple* tuple);

gint tag_tuple_write_to_file(Tuple *tuple);

G_END_DECLS
#endif /* AUDTAG_H */


