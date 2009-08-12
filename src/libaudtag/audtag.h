/* External Interface of the tagging library */

#ifndef AUDTAG_H
#define AUDTAG_H

G_BEGIN_DECLS

#include <glib.h>
#include <mowgli.h> 
#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"

void tag_init(void);
void tag_terminate(void);

Tuple *tag_tuple_read(Tuple *tuple, VFSFile *fd);

gboolean tag_tuple_write_to_file(Tuple *tuple, VFSFile *fd);

G_END_DECLS
#endif /* AUDTAG_H */


