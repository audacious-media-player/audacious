#ifndef AAC_H
#define	AAC_H
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"

#define FTYP    "ftyp"
#define MOOV    "moov"
#define ILST    "ilst"
#define UDTA    "udta"
#define META    "meta"
#define ILST    "ilst"
#define FREE    "free"

enum {
    MP4_ALBUM = 0,
    MP4_TITLE,
    MP4_COPYRIGHT,
    MP4_ARTIST,
    MP4_ARTIST2,
    MP4_TRACKNR,
    MP4_YEAR,
    MP4_GENRE,
    MP4_COMMENT,
    MP4_ITEMS_NO
};

typedef struct mp4atom
{
    guint32 size;
    gchar* name; //4 bytes
    gchar* body;
    int type;
}Atom;


typedef struct strdataatom
{
    guint32 atomsize;
    gchar* name;
    guint32 datasize;
    gchar* dataname;
    guint32 vflag;
    guint32 nullData;
    gchar*  data;
    int type;
}StrDataAtom;



gboolean aac_can_handle_file(VFSFile *f);

Tuple *aac_populate_tuple_from_file(Tuple *tuple,VFSFile *f);

gboolean aac_write_tuple_to_file(Tuple* tuple, VFSFile *f);

extern tag_module_t aac;

Atom *ilstAtom;
guint64 ilstFileOffset;
guint32 newilstSize ;
mowgli_list_t *dataAtoms;
mowgli_dictionary_t *ilstAtoms;

#endif

