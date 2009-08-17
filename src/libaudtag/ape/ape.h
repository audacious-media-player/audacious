#ifndef APE_H

#define APE_H

#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"

enum {
    APE_ALBUM = 0,
    APE_TITLE,
    APE_COPYRIGHT,
    APE_ARTIST,
    APE_TRACKNR,
    APE_YEAR,
    APE_GENRE,
    APE_COMMENT,
    APE_ITEMS_NO
};



typedef struct  apeheader
{
    gchar *preamble;  //64 bits
    guint32 version;
    guint32 tagSize;
    guint32 itemCount;
    guint32 flags;
    guint64 reserved;
}APEv2Header;

typedef struct tagitem
{
    guint32 size;
    guint32 flags;
    gchar* key; //null terminated
    gchar* value;
}APETagItem;

gboolean ape_can_handle_file(VFSFile *f);

Tuple *ape_populate_tuple_from_file(Tuple *tuple,VFSFile *f);

gboolean ape_write_tuple_to_file(Tuple* tuple, VFSFile *f);

extern tag_module_t ape;
mowgli_dictionary_t *tagItems;
mowgli_list_t *tagKeys;
guint32 headerPosition ;

#endif
