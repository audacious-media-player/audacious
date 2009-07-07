#ifndef WMA_H

#define WMA_H

#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"




typedef struct headerObj
{
    guint64 size;
    guint32 objectsNr;
    //8+8 byte reserved
}HeaderObject;


typedef struct contentField
{
	guint16 size;
	gunichar2 *strValue;
}ContentField;

gboolean wma_can_handle_file(const gchar *file_path);

Tuple *wma_populate_tuple_from_file(Tuple* tuple);

gboolean wma_write_tuple_to_file (Tuple* tuple);


#endif
