/*********************************************************************
 * 
 *    Copyright (C) 1999-2000, 2001,  Espen Skoglund
 *    Department of Computer Science, University of Tromsø
 * 
 * Filename:      id3_frame.c
 * Description:   Code for handling ID3 frames.
 * Author:        Espen Skoglund <espensk@stud.cs.uit.no>
 * Created at:    Fri Feb  5 23:47:08 1999
 * 
 * $Id: id3_frame.c,v 1.5 2004/07/20 21:47:22 descender Exp $
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *                
 ********************************************************************/
#include "config.h"

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "xmms-id3.h"
#include "id3_header.h"

static void *id3_frame_get_dataptr(struct id3_frame *frame);
static int id3_frame_get_size(struct id3_frame *frame);
static int id3_read_frame_v22(struct id3_tag *id3);


/*
 * Description of all valid ID3v2 frames.
 */
static struct id3_framedesc framedesc[] = {
    {ID3_AENC, "AENC", "Audio encryption"},
    {ID3_APIC, "APIC", "Attached picture"},
    {ID3_ASPI, "ASPI", "Audio seek point index"},   /* v4 only */

    {ID3_COMM, "COMM", "Comments"},
    {ID3_COMR, "COMR", "Commercial frame"},

    {ID3_ENCR, "ENCR", "Encryption method registration"},
    {ID3_EQUA, "EQUA", "Equalization"}, /* v3 only */
    {ID3_EQU2, "EQU2", "Equalization (2)"}, /* v4 only */
    {ID3_ETCO, "ETCO", "Event timing codes"},

    {ID3_GEOB, "GEOB", "General encapsulated object"},
    {ID3_GRID, "GRID", "Group identification registration"},

    {ID3_IPLS, "IPLS", "Involved people list"}, /* v3 only */

    {ID3_LINK, "LINK", "Linked information"},

    {ID3_MCDI, "MCDI", "Music CD identifier"},
    {ID3_MLLT, "MLLT", "MPEG location lookup table"},

    {ID3_OWNE, "OWNE", "Ownership frame"},

    {ID3_PRIV, "PRIV", "Private frame"},
    {ID3_PCNT, "PCNT", "Play counter"},
    {ID3_POPM, "POPM", "Popularimeter"},
    {ID3_POSS, "POSS", "Position synchronisation frame"},

    {ID3_RBUF, "RBUF", "Recommended buffer size"},
    {ID3_RVAD, "RVAD", "Relative volume adjustment"},   /* v3 only */
    {ID3_RVA2, "RVA2", "RVA2 Relative volume adjustment (2)"},  /* v4 only */
    {ID3_RVRB, "RVRB", "Reverb"},

    {ID3_SEEK, "SEEK", "Seek frame"},   /* v4 only */
    {ID3_SIGN, "SIGN", "Signature frame"},  /* v4 only */
    {ID3_SYLT, "SYLT", "Synchronized lyric/text"},
    {ID3_SYTC, "SYTC", "Synchronized tempo codes"},

    {ID3_TALB, "TALB", "Album/Movie/Show title"},
    {ID3_TBPM, "TBPM", "BPM (beats per minute)"},
    {ID3_TCOM, "TCOM", "Composer"},
    {ID3_TCON, "TCON", "Content type"},
    {ID3_TCOP, "TCOP", "Copyright message"},
    {ID3_TDAT, "TDAT", "Date"}, /* v3 only */
    {ID3_TDEN, "TDEN", "Encoding time"},    /* v4 only */
    {ID3_TDLY, "TDLY", "Playlist delay"},
    {ID3_TDOR, "TDOR", "Original release time"},    /* v4 only */
    {ID3_TDRC, "TDRC", "Recording time"},   /* v4 only */
    {ID3_TDRL, "TDRL", "Release time"}, /* v4 only */
    {ID3_TDTG, "TDTG", "Tagging time"}, /* v4 only */

    {ID3_TENC, "TENC", "Encoded by"},
    {ID3_TEXT, "TEXT", "Lyricist/Text writer"},
    {ID3_TFLT, "TFLT", "File type"},
    {ID3_TIME, "TIME", "Time"}, /* v3 only */
    {ID3_TIPL, "TIPL", "Involved people list"}, /* v4 only */
    {ID3_TIT1, "TIT1", "Content group description"},
    {ID3_TIT2, "TIT2", "Title/songname/content description"},
    {ID3_TIT3, "TIT3", "Subtitle/Description refinement"},
    {ID3_TKEY, "TKEY", "Initial key"},
    {ID3_TLAN, "TLAN", "Language(s)"},
    {ID3_TLEN, "TLEN", "Length"},
    {ID3_TMCL, "TMCL", "Musician credits list"},    /* v4 only */
    {ID3_TMOO, "TMOO", "Mood"}, /* v4 only */
    {ID3_TMED, "TMED", "Media type"},
    {ID3_TOAL, "TOAL", "Original album/movie/show title"},
    {ID3_TOFN, "TOFN", "Original filename"},
    {ID3_TOLY, "TOLY", "Original lyricist(s)/text writer(s)"},
    {ID3_TOPE, "TOPE", "Original artist(s)/performer(s)"},
    {ID3_TORY, "TORY", "Original release year"},    /* v3 only */
    {ID3_TOWN, "TOWN", "File owner/licensee"},
    {ID3_TPE1, "TPE1", "Lead performer(s)/Soloist(s)"},
    {ID3_TPE2, "TPE2", "Band/orchestra/accompaniment"},
    {ID3_TPE3, "TPE3", "Conductor/performer refinement"},
    {ID3_TPE4, "TPE4", "Interpreted, remixed, or otherwise modified by"},
    {ID3_TPOS, "TPOS", "Part of a set"},
    {ID3_TPRO, "TPRO", "Produced notice"},  /* v4 only */
    {ID3_TPUB, "TPUB", "Publisher"},
    {ID3_TRCK, "TRCK", "Track number/Position in set"},
    {ID3_TRDA, "TRDA", "Recording dates"},  /* v3 only */
    {ID3_TRSN, "TRSN", "Internet radio station name"},
    {ID3_TRSO, "TRSO", "Internet radio station owner"},
    {ID3_TSIZ, "TSIZ", "Size"}, /* v3 only */
    {ID3_TSOA, "TSOA", "Album sort order"}, /* v4 only */
    {ID3_TSOP, "TSOP", "Performer sort order"}, /* v4 only */
    {ID3_TSOT, "TSOT", "Title sort order"}, /* v4 only */

    {ID3_TSRC, "TSRC", "ISRC (international standard recording code)"},
    {ID3_TSSE, "TSSE", "Software/Hardware and settings used for encoding"},
    {ID3_TSST, "TSST", "Set subtitle"}, /* v4 only */
    {ID3_TYER, "TYER", "Year"}, /* v3 only */
    {ID3_TXXX, "TXXX", "User defined text information frame"},

    {ID3_UFID, "UFID", "Unique file identifier"},
    {ID3_USER, "USER", "Terms of use"},
    {ID3_USLT, "USLT", "Unsychronized lyric/text transcription"},

    {ID3_WCOM, "WCOM", "Commercial information"},
    {ID3_WCOP, "WCOP", "Copyright/Legal information"},
    {ID3_WOAF, "WOAF", "Official audio file webpage"},
    {ID3_WOAR, "WOAR", "Official artist/performer webpage"},
    {ID3_WOAS, "WOAS", "Official audio source webpage"},
    {ID3_WORS, "WORS", "Official internet radio station homepage"},
    {ID3_WPAY, "WPAY", "Payment"},
    {ID3_WPUB, "WPUB", "Publishers official webpage"},
    {ID3_WXXX, "WXXX", "User defined URL link frame"},
};

struct id3_framedesc22 {
    guint32 fd_v22, fd_v24;
};

static struct id3_framedesc22 framedesc22[] = {
    {ID3_BUF, ID3_RBUF},        /* Recommended buffer size */

    {ID3_CNT, ID3_PCNT},        /* Play counter */
    {ID3_COM, ID3_COMM},        /* Comments */
    {ID3_CRA, ID3_AENC},        /* Audio encryption */
    {ID3_CRM, 0},               /* Encrypted meta frame */

    {ID3_ETC, ID3_ETCO},        /* Event timing codes */
    /* Could be converted to EQU2 */
    {ID3_EQU, 0},               /* Equalization */

    {ID3_GEO, ID3_GEOB},        /* General encapsulated object */

    /* Would need conversion to TIPL */
    {ID3_IPL, 0},               /* Involved people list */

    /* This is so fragile it's not worth trying to save */
    {ID3_LNK, 0},               /* Linked information */

    {ID3_MCI, ID3_MCDI},        /* Music CD Identifier */
    {ID3_MLL, ID3_MLLT},        /* MPEG location lookup table */

    /* Would need to convert header for APIC */
    {ID3_PIC, 0},               /* Attached picture */
    {ID3_POP, ID3_POPM},        /* Popularimeter */

    {ID3_REV, ID3_RVRB},        /* Reverb */
    /* Could be converted to RVA2 */
    {ID3_RVA, 0},               /* Relative volume adjustment */

    {ID3_SLT, ID3_SYLT},        /* Synchronized lyric/text */
    {ID3_STC, ID3_SYTC},        /* Synced tempo codes */

    {ID3_TAL, ID3_TALB},        /* Album/Movie/Show title */
    {ID3_TBP, ID3_TBPM},        /* BPM (Beats Per Minute) */
    {ID3_TCM, ID3_TCOM},        /* Composer */
    {ID3_TCO, ID3_TCON},        /* Content type */
    {ID3_TCR, ID3_TCOP},        /* Copyright message */
    /* This could be incorporated into TDRC */
    {ID3_TDA, 0},               /* Date */
    {ID3_TDY, ID3_TDLY},        /* Playlist delay */
    {ID3_TEN, ID3_TENC},        /* Encoded by */
    {ID3_TFT, ID3_TFLT},        /* File type */
    /* This could be incorporated into TDRC */
    {ID3_TIM, 0},               /* Time */
    {ID3_TKE, ID3_TKEY},        /* Initial key */
    {ID3_TLA, ID3_TLAN},        /* Language(s) */
    {ID3_TLE, ID3_TLEN},        /* Length */
    {ID3_TMT, ID3_TMED},        /* Media type */
    {ID3_TOA, ID3_TOPE},        /* Original artist(s)/performer(s) */
    {ID3_TOF, ID3_TOFN},        /* Original filename */
    {ID3_TOL, ID3_TOLY},        /* Original Lyricist(s)/text writer(s) */
    /*
     * The docs says that original release year should be in
     * milliseconds!  Hopefully that is a typo.
     */
    {ID3_TOR, ID3_TDOR},        /* Original release year */
    {ID3_TOT, ID3_TOAL},        /* Original album/Movie/Show title */
    {ID3_TP1, ID3_TPE1},        /* Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group */
    {ID3_TP2, ID3_TPE2},        /* Band/Orchestra/Accompaniment */
    {ID3_TP3, ID3_TPE3},        /* Conductor/Performer refinement */
    {ID3_TP4, ID3_TPE4},        /* Interpreted, remixed, or otherwise modified by */
    {ID3_TPA, ID3_TPOS},        /* Part of a set */
    {ID3_TPB, ID3_TPUB},        /* Publisher */
    {ID3_TRC, ID3_TSRC},        /* ISRC (International Standard Recording Code) */
    {ID3_TRD, 0},               /* Recording dates */
    {ID3_TRK, ID3_TRCK},        /* Track number/Position in set */
    {ID3_TSI, 0},               /* Size */
    {ID3_TSS, ID3_TSSE},        /* Software/hardware and settings used for encoding */
    {ID3_TT1, ID3_TIT1},        /* Content group description */
    {ID3_TT2, ID3_TIT2},        /* Title/Songname/Content description */
    {ID3_TT3, ID3_TIT3},        /* Subtitle/Description refinement */
    {ID3_TXT, ID3_TEXT},        /* Lyricist/text writer */
    {ID3_TXX, ID3_TXXX},        /* User defined text information frame */
    {ID3_TYE, ID3_TDRC},        /* Year */

    {ID3_UFI, ID3_UFID},        /* Unique file identifier */
    {ID3_ULT, ID3_USLT},        /* Unsychronized lyric/text transcription */

    {ID3_WAF, ID3_WOAF},        /* Official audio file webpage */
    {ID3_WAR, ID3_WOAR},        /* Official artist/performer webpage */
    {ID3_WAS, ID3_WOAS},        /* Official audio source webpage */
    {ID3_WCM, ID3_WCOM},        /* Commercial information */
    {ID3_WCP, ID3_WCOP},        /* Copyright/Legal information */
    {ID3_WPB, ID3_WPUB},        /* Publishers official webpage */
    {ID3_WXX, ID3_WXXX},        /* User defined URL link frame */
};

/*
 * These need to be signed, because otherwise this code will get wonky.
 * BTW, I hate this code and I hope it dies someday.
 *   --nenolod
 */
static struct id3_framedesc *
find_frame_description(guint32 id)
{
    int i;
    for (i = 0; i < (int) sizeof(framedesc) / (int) sizeof(struct id3_framedesc); i++)
        if (framedesc[i].fd_id == id)
            return &framedesc[i];
    return NULL;
}


/*
 * Function id3_read_frame (id3)
 *
 *    Read next frame from the indicated ID3 tag.  Return 0 upon
 *    success, or -1 if an error occured.
 *
 */
int
id3_read_frame(struct id3_tag *id3)
{
    struct id3_frame *frame;
    guint32 id;
    char *buf;

    if (id3->id3_version == 2)
        return id3_read_frame_v22(id3);

    /*
     * Read frame header.
     */
    buf = id3->id3_read(id3, NULL, ID3_FRAMEHDR_SIZE);
    if (buf == NULL)
        return -1;

    /*
     * If we encounter an invalid frame id, we assume that there is
     * some padding in the header.  We just skip the rest of the ID3
     * tag.
     */
    if (!((buf[0] >= '0' && buf[0] <= '9')
          || (buf[0] >= 'A' && buf[0] <= 'Z'))) {
        id3->id3_seek(id3, id3->id3_tagsize - id3->id3_pos);
        return 0;
    }
    id = ID3_FRAME_ID(buf[0], buf[1], buf[2], buf[3]);

    /*
     * Allocate frame.
     */
    frame = g_malloc0(sizeof(struct id3_frame));

    frame->fr_owner = id3;
    /* FIXME v2.4.0 */
    frame->fr_raw_size = buf[4] << 24 | buf[5] << 16 | buf[6] << 8 | buf[7];
    if (frame->fr_raw_size < 0 || frame->fr_raw_size > 1000000) {
        g_free(frame);
        return -1;
    }
    frame->fr_flags = buf[8] << 8 | buf[9];

    /*
     * Determine the type of the frame.
     */

    frame->fr_desc = find_frame_description(id);

    /*
     * Check if frame had a valid id.
     */
    if (frame->fr_desc == NULL) {
        /*
         * No. Ignore the frame.
         */
        if (id3->id3_seek(id3, frame->fr_raw_size) < 0) {
            g_free(frame);
            return -1;
        }
        return 0;
    }

    /*
     * Initialize frame.
     */

    /*
     * We allocate 2 extra bytes.  This simplifies retrieval of
     * text strings.
     */
    frame->fr_raw_data = g_malloc0(frame->fr_raw_size + 2);
    if (id3->id3_read(id3, frame->fr_raw_data, frame->fr_raw_size) == NULL) {
        g_free(frame->fr_raw_data);
        g_free(frame);
        return -1;
    }

    /*
     * Insert frame into linked list.
     */
    id3->id3_frame = g_list_append(id3->id3_frame, frame);

    /*
     * Check if frame is compressed using zlib.
     */
    if (frame->fr_flags & ID3_FHFLAG_COMPRESS)
        return 0;

    frame->fr_data = id3_frame_get_dataptr(frame);
    frame->fr_size = id3_frame_get_size(frame);

    return 0;
}


/*
 * Function id3_get_frame (id3, type, num)
 *
 *    Search in the list of frames for the ID3-tag, and return a frame
 *    of the indicated type.  If tag contains several frames of the
 *    indicated type, the third argument tells which of the frames to
 *    return.
 *
 */
struct id3_frame *
id3_get_frame(struct id3_tag *id3, guint32 type, int num)
{
    GList *node;

    for (node = id3->id3_frame; node != NULL; node = node->next) {
        struct id3_frame *fr = node->data;
        if (fr->fr_desc && fr->fr_desc->fd_id == type) {
            if (--num <= 0)
                return fr;
        }
    }
    return NULL;
}

/*
 * Function decompress_frame(frame)
 *
 *    Uncompress the indicated frame.  Return 0 upon success, or -1 if
 *    an error occured.
 *
 */
static int
decompress_frame(struct id3_frame *frame)
{
#ifdef HAVE_LIBZ
    z_stream z;
    int r;

    /*
     * Fetch the size of the decompressed data.
     */
    frame->fr_size_z = g_ntohl(*((guint32 *) frame->fr_raw_data));
    if (frame->fr_size_z < 0 || frame->fr_size_z > 1000000)
        return -1;

    /*
     * Allocate memory to hold uncompressed frame.
     */
    frame->fr_data_z = g_malloc(frame->fr_size_z +
                                (id3_frame_is_text(frame) ? 2 : 0));

    /*
     * Initialize zlib.
     */
    z.next_in = id3_frame_get_dataptr(frame);
    z.avail_in = id3_frame_get_size(frame);
    z.zalloc = NULL;
    z.zfree = NULL;
    z.opaque = NULL;

    r = inflateInit(&z);
    switch (r) {
    case Z_OK:
        break;
    case Z_MEM_ERROR:
        id3_error(frame->fr_owner, "zlib - no memory");
        goto Error_init;
    case Z_VERSION_ERROR:
        id3_error(frame->fr_owner, "zlib - invalid version");
        goto Error_init;
    default:
        id3_error(frame->fr_owner, "zlib - unknown error");
        goto Error_init;
    }

    /*
     * Decompress frame.
     */
    z.next_out = frame->fr_data_z;
    z.avail_out = frame->fr_size_z;
    r = inflate(&z, Z_SYNC_FLUSH);
    switch (r) {
    case Z_STREAM_END:
        break;
    case Z_OK:
        if (z.avail_in == 0)
            /*
             * This should not be possible with a correct stream.
             * We will be nice however, and try to go on.
             */
            break;
        id3_error(frame->fr_owner, "zlib - buffer exhausted");
        goto Error_inflate;
    default:
        id3_error(frame->fr_owner, "zlib - unknown error");
        goto Error_inflate;
    }

    r = inflateEnd(&z);
    if (r != Z_OK)
        id3_error(frame->fr_owner, "zlib - inflateEnd error");

    /*
     * Null-terminate text frames.
     */
    if (id3_frame_is_text(frame)) {
        ((char *) frame->fr_data_z)[frame->fr_size_z] = 0;
        ((char *) frame->fr_data_z)[frame->fr_size_z + 1] = 0;
    }
    frame->fr_data = frame->fr_data_z;
    frame->fr_size = frame->fr_size_z + (id3_frame_is_text(frame) ? 2 : 0);

    return 0;

    /*
     * Cleanup code.
     */
  Error_inflate:
    r = inflateEnd(&z);
  Error_init:
    g_free(frame->fr_data_z);
    frame->fr_data_z = NULL;
#endif
    return -1;
}

/*
 * Function id3_decompress_frame(frame)
 *
 *    Check if frame is compressed, and uncompress if necessary.
 *    Return 0 upon success, or -1 if an error occured.
 *
 */
int
id3_decompress_frame(struct id3_frame *frame)
{
    if (!(frame->fr_flags & ID3_FHFLAG_COMPRESS))
        /* Frame not compressed */
        return 0;
    if (frame->fr_data_z)
        /* Frame already decompressed */
        return 0;
    /* Do decompression */
    return decompress_frame(frame);
}


/*
 * Function id3_delete_frame (frame)
 *
 *    Remove frame from ID3 tag and release memory ocupied by it.
 *
 */
int
id3_delete_frame(struct id3_frame *frame)
{
    GList *list = frame->fr_owner->id3_frame;
    int ret;

    /*
     * Search for frame in list.
     */

    if (g_list_find(list, frame) != NULL) {
        /*
         * Frame does not exist in frame list.
         */
        ret = -1;

    }
    else {
        /*
         * Remove frame from frame list.
         */
        list = g_list_remove(list, frame);
        frame->fr_owner->id3_altered = 1;
        ret = 0;
    }

    /*
     * Release memory occupied by frame.
     */
    if (frame->fr_raw_data)
        g_free(frame->fr_raw_data);
    if (frame->fr_data_z)
        g_free(frame->fr_data_z);
    g_free(frame);

    return ret;
}


/*
 * Function id3_add_frame (id3, type)
 *
 *    Add a new frame to the ID3 tag.  Return a pointer to the new
 *    frame, or NULL if an error occured.
 *
 */
struct id3_frame *
id3_add_frame(struct id3_tag *id3, guint32 type)
{
    struct id3_frame *frame;
    int i;

    /*
     * Allocate frame.
     */
    frame = g_malloc0(sizeof(struct id3_frame));

    /*
     * Initialize frame
     */
    frame->fr_owner = id3;

    /*
     * Try finding the correct frame descriptor.
     */
    for (i = 0; i < (int) sizeof(framedesc) / (int) sizeof(struct id3_framedesc); i++) {
        if (framedesc[i].fd_id == type) {
            frame->fr_desc = &framedesc[i];
            break;
        }
    }

    /*
     * Insert frame into linked list.
     */
    id3->id3_frame = g_list_append(id3->id3_frame, frame);
    id3->id3_altered = 1;

    return frame;
}


/*
 * Destroy all frames  in an id3 tag, and free all data
 */
void
id3_destroy_frames(struct id3_tag *id)
{
    GList *node;

    for (node = id->id3_frame; node != NULL; node = node->next) {
        struct id3_frame *frame = node->data;
        /*
         * Release memory occupied by frame.
         */
        if (frame->fr_raw_data)
            g_free(frame->fr_raw_data);
        if (frame->fr_data_z)
            g_free(frame->fr_data_z);
        g_free(frame);
    }
    g_list_free(id->id3_frame);
    id->id3_frame = NULL;
}

static int
id3_frame_extra_headers(struct id3_frame *frame)
{
    int retv = 0;
    /*
     * If frame is encrypted, we have four extra bytes in the
     * header.
     */
    if (frame->fr_flags & ID3_FHFLAG_COMPRESS)
        retv += 4;
    /*
     * If frame is encrypted, we have one extra byte in the
     * header.
     */
    if (frame->fr_flags & ID3_FHFLAG_ENCRYPT)
        retv += 1;

    /*
     * If frame has grouping identity, we have one extra byte in
     * the header.
     */
    if (frame->fr_flags & ID3_FHFLAG_GROUP)
        retv += 1;

    return retv;
}

static void *
id3_frame_get_dataptr(struct id3_frame *frame)
{
    char *ptr = frame->fr_raw_data;

    ptr += id3_frame_extra_headers(frame);

    return ptr;
}

static int
id3_frame_get_size(struct id3_frame *frame)
{
    return frame->fr_raw_size - id3_frame_extra_headers(frame);
}

void
id3_frame_clear_data(struct id3_frame *frame)
{
    if (frame->fr_raw_data)
        g_free(frame->fr_raw_data);
    if (frame->fr_data_z)
        g_free(frame->fr_data_z);
    frame->fr_raw_data = NULL;
    frame->fr_raw_size = 0;
    frame->fr_data = NULL;
    frame->fr_size = 0;
    frame->fr_data_z = NULL;
    frame->fr_size_z = 0;
}

static guint32
find_v24_id(guint32 v22)
{
    int i;
    for (i = 0; i < (int) sizeof(framedesc22) / (int) sizeof(struct id3_framedesc22); i++)
        if (framedesc22[i].fd_v22 == v22)
            return framedesc22[i].fd_v24;

    return 0;
}

static int
id3_read_frame_v22(struct id3_tag *id3)
{
    struct id3_frame *frame;
    guint32 id, idv24;
    char *buf;
    int size;

    /*
     * Read frame header.
     */
    buf = id3->id3_read(id3, NULL, ID3_FRAMEHDR_SIZE_22);
    if (buf == NULL)
        return -1;

    /*
     * If we encounter an invalid frame id, we assume that there
     * is some.  We just skip the rest of the ID3 tag.
     */
    if (!((buf[0] >= '0' && buf[0] <= '9')
          || (buf[0] >= 'A' && buf[0] <= 'Z'))) {
        id3->id3_seek(id3, id3->id3_tagsize - id3->id3_pos);
        return 0;
    }

    id = ID3_FRAME_ID_22(buf[0], buf[1], buf[2]);
    size = buf[3] << 16 | buf[4] << 8 | buf[5];

    if ((idv24 = find_v24_id(id)) == 0) {
        if (id3->id3_seek(id3, size) < 0)
            return -1;
        return 0;
    }

    /*
     * Allocate frame.
     */
    frame = g_malloc0(sizeof(struct id3_frame));

    frame->fr_owner = id3;
    frame->fr_raw_size = size;
    if (frame->fr_raw_size < 0 || frame->fr_raw_size > 1000000) {
        g_free(frame);
        return -1;
    }

    /*
     * Initialize frame.
     */
    frame->fr_desc = find_frame_description(idv24);

    /*
     * We allocate 2 extra bytes.  This simplifies retrieval of
     * text strings.
     */
    frame->fr_raw_data = g_malloc0(frame->fr_raw_size + 2);
    if (id3->id3_read(id3, frame->fr_raw_data, frame->fr_raw_size) == NULL) {
        g_free(frame->fr_raw_data);
        g_free(frame);
        return -1;
    }

    /*
     * Insert frame into linked list.
     */
    id3->id3_frame = g_list_append(id3->id3_frame, frame);

    frame->fr_data = frame->fr_raw_data;
    frame->fr_size = frame->fr_raw_size;

    return 0;
}
