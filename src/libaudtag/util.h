#include <glib-2.0/glib.h>
#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"



time_t unix_time(guint64 win_time);

guint16 get_year(guint64 win_time);

void printTuple(Tuple *tuple);