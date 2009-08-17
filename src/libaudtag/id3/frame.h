#ifndef AUD_ID3_FRAME
#define AUD_ID3_FRAME

#include <glib-2.0/glib.h>

enum {
    ID3_ALBUM = 0,
    ID3_TITLE,
    ID3_COMPOSER,
    ID3_COPYRIGHT,
    ID3_DATE,
    ID3_TIME,
    ID3_LENGTH,
    ID3_ARTIST,
    ID3_TRACKNR,
    ID3_YEAR,
    ID3_GENRE,
    ID3_COMMENT,
    ID3_TAGS_NO
};

char * id3_frames[] = {"TALB","TIT2","TCOM", "TCOP", "TDAT", "TIME", "TLEN", "TPE1", "TRCK", "TYER","TCON", "COMM"};

#endif
