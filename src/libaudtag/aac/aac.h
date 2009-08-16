#ifndef AAC_H
#define	AAC_H
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>
#include "../tag_module.h"


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

static const char* ID3v1GenreList[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
    "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
    "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
    "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
    "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
    "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
    "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
    "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
    "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
    "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
    "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
    "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
    "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
    "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
    "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
    "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
    "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
    "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
    "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
    "Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
    "Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
    "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
    "SynthPop",
};


typedef struct mp4atom
{
    guint32 size;
    gchar* name; //4 bytes
    gchar* body;
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
}StrDataAtom;



gboolean aac_can_handle_file(VFSFile *f);

Tuple *aac_populate_tuple_from_file(Tuple *tuple,VFSFile *f);

gboolean aac_write_tuple_to_file(Tuple* tuple, VFSFile *f);

extern tag_module_t aac;

Atom *ilstAtom;
long ilstFileOffset;
mowgli_list_t *dataAtoms;
#endif

