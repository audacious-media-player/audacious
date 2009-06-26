/* External Interface of the tagging library */

#ifndef AUDTAG_H
#define AUDTAG_H

G_BEGIN_DECLS

#include <glib.h>
#include "libaudcore/tuple.h"
#include <mowgli.h> 

void audtag_init(void);
void audtag_terminate(void);

Tuple *audtag_tuple_read(Tuple* tuple);

gint audtag_tuple_write_to_file(Tuple *tuple);

G_END_DECLS
#endif /* AUDTAG_H */


