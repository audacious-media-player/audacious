/*********************************************************************
 * 
 *    Copyright (C) 1998, 1999, 2002,  Espen Skoglund
 *    Department of Computer Science, University of Tromsø
 * 
 * Filename:      id3.h
 * Description:   Include file for accessing the ID3 library.
 * Author:        Espen Skoglund <espensk@stud.cs.uit.no>
 * Created at:    Thu Nov  5 15:55:10 1998
 *                
 * $Id: xmms-id3.h,v 1.1 2004/07/20 21:47:22 descender Exp $
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *                
 ********************************************************************/
#ifndef ID3_H
#define ID3_H

#include <glib.h>
#include <libaudacious/vfs.h>

/*
 * Option flags to id3_open_*().
 */
#define ID3_OPENF_NONE		0x0000
#define ID3_OPENF_NOCHK		0x0001
#define ID3_OPENF_CREATE	0x0002


/*
 * The size of the read buffer used by file operations.
 */
#define ID3_FD_BUFSIZE	8192


/*
 * Structure describing the ID3 tag.
 */
struct id3_tag {
    int id3_type;               /* Memory or file desriptor */
    int id3_oflags;             /* Flags from open call */
    int id3_flags;              /* Flags from tag header */
    int id3_altered;            /* Set when tag has been altered */
    int id3_newtag;             /* Set if this is a new tag */

    int id3_version;            /* Major ID3 version number */
    int id3_revision;           /* ID3 revision number */

    int id3_tagsize;            /* Total size of ID3 tag */
    int id3_pos;                /* Current position within tag */

    char *id3_error_msg;        /* Last error message */

    char id3_buffer[256];       /* Used for various strings */

    union {
        /*
         * Memory specific fields.
         */
        struct {
            void *id3_ptr;
        } me;

        /*
         * File desc. specific fields.
         */
        struct {
            int id3_fd;
            void *id3_buf;
        } fd;

        /*
         * File ptr. specific fields.
         */
        struct {
            VFSFile *id3_fp;
            void *id3_buf;
        } fp;
    } s;

    /*
     * Functions for doing operations within ID3 tag.
     */
    int (*id3_seek) (struct id3_tag *, int);
    void *(*id3_read) (struct id3_tag *, void *, int);

    /*
     * Linked list of ID3 frames.
     */
    GList *id3_frame;
};

#define ID3_TYPE_NONE	0
#define ID3_TYPE_MEM	1
#define ID3_TYPE_FD	2
#define ID3_TYPE_FP	3


/*
 * Structure describing an ID3 frame.
 */
struct id3_frame {
    struct id3_tag *fr_owner;
    struct id3_framedesc *fr_desc;
    int fr_flags;
    unsigned char fr_encryption;
    unsigned char fr_grouping;
    unsigned char fr_altered;

    void *fr_data;              /* Pointer to frame data, excluding headers */
    int fr_size;                /* Size of uncompressed frame */

    void *fr_raw_data;          /* Frame data */
    int fr_raw_size;            /* Frame size */

    void *fr_data_z;            /* The decompressed compressed frame */
    int fr_size_z;              /* Size of decompressed compressed frame */
};


/*
 * Structure describing an ID3 frame type.
 */
struct id3_framedesc {
    guint32 fd_id;
    char fd_idstr[4];
    char *fd_description;
};


/*
 * Text encodings.
 */
#define ID3_ENCODING_ISO_8859_1	0x00
#define ID3_ENCODING_UTF16	0x01
#define ID3_ENCODING_UTF16BE	0x02
#define ID3_ENCODING_UTF8	0x03



/*
 * ID3 frame id numbers.
 */
#define ID3_FRAME_ID(a,b,c,d)   ((a << 24) | (b << 16) | (c << 8) | d)

#define ID3_AENC	ID3_FRAME_ID('A','E','N','C')
#define ID3_APIC	ID3_FRAME_ID('A','P','I','C')
#define ID3_ASPI	ID3_FRAME_ID('A','S','P','I')
#define ID3_COMM	ID3_FRAME_ID('C','O','M','M')
#define ID3_COMR	ID3_FRAME_ID('C','O','M','R')
#define ID3_ENCR	ID3_FRAME_ID('E','N','C','R')
#define ID3_EQUA	ID3_FRAME_ID('E','Q','U','A')
#define ID3_EQU2	ID3_FRAME_ID('E','Q','U','2')
#define ID3_ETCO	ID3_FRAME_ID('E','T','C','O')
#define ID3_GEOB	ID3_FRAME_ID('G','E','O','B')
#define ID3_GRID	ID3_FRAME_ID('G','R','I','D')
#define ID3_IPLS	ID3_FRAME_ID('I','P','L','S')
#define ID3_LINK	ID3_FRAME_ID('L','I','N','K')
#define ID3_MCDI	ID3_FRAME_ID('M','C','D','I')
#define ID3_MLLT	ID3_FRAME_ID('M','L','L','T')
#define ID3_OWNE	ID3_FRAME_ID('O','W','N','E')
#define ID3_PRIV	ID3_FRAME_ID('P','R','I','V')
#define ID3_PCNT	ID3_FRAME_ID('P','C','N','T')
#define ID3_POPM	ID3_FRAME_ID('P','O','P','M')
#define ID3_POSS	ID3_FRAME_ID('P','O','S','S')
#define ID3_RBUF	ID3_FRAME_ID('R','B','U','F')
#define ID3_RVAD	ID3_FRAME_ID('R','V','A','D')
#define ID3_RVA2	ID3_FRAME_ID('R','V','A','2')
#define ID3_RVRB	ID3_FRAME_ID('R','V','R','B')
#define ID3_SEEK	ID3_FRAME_ID('S','E','E','K')
#define ID3_SIGN	ID3_FRAME_ID('S','I','G','N')
#define ID3_SYLT	ID3_FRAME_ID('S','Y','L','T')
#define ID3_SYTC	ID3_FRAME_ID('S','Y','T','C')
#define ID3_TALB	ID3_FRAME_ID('T','A','L','B')
#define ID3_TBPM	ID3_FRAME_ID('T','B','P','M')
#define ID3_TCOM	ID3_FRAME_ID('T','C','O','M')
#define ID3_TCON	ID3_FRAME_ID('T','C','O','N')
#define ID3_TCOP	ID3_FRAME_ID('T','C','O','P')
#define ID3_TDAT	ID3_FRAME_ID('T','D','A','T')
#define ID3_TDEN	ID3_FRAME_ID('T','D','E','N')
#define ID3_TDLY	ID3_FRAME_ID('T','D','L','Y')
#define ID3_TDOR	ID3_FRAME_ID('T','D','O','R')
#define ID3_TDRC	ID3_FRAME_ID('T','D','R','C')
#define ID3_TDRL	ID3_FRAME_ID('T','D','R','L')
#define ID3_TDTG	ID3_FRAME_ID('T','D','T','G')
#define ID3_TENC	ID3_FRAME_ID('T','E','N','C')
#define ID3_TEXT	ID3_FRAME_ID('T','E','X','T')
#define ID3_TFLT	ID3_FRAME_ID('T','F','L','T')
#define ID3_TIME	ID3_FRAME_ID('T','I','M','E')
#define ID3_TIPL	ID3_FRAME_ID('T','I','P','L')
#define ID3_TIT1	ID3_FRAME_ID('T','I','T','1')
#define ID3_TIT2	ID3_FRAME_ID('T','I','T','2')
#define ID3_TIT3	ID3_FRAME_ID('T','I','T','3')
#define ID3_TKEY	ID3_FRAME_ID('T','K','E','Y')
#define ID3_TLAN	ID3_FRAME_ID('T','L','A','N')
#define ID3_TLEN	ID3_FRAME_ID('T','L','E','N')
#define ID3_TMCL	ID3_FRAME_ID('T','M','C','L')
#define ID3_TMED	ID3_FRAME_ID('T','M','E','D')
#define ID3_TMOO	ID3_FRAME_ID('T','M','O','O')
#define ID3_TOAL	ID3_FRAME_ID('T','O','A','L')
#define ID3_TOFN	ID3_FRAME_ID('T','O','F','N')
#define ID3_TOLY	ID3_FRAME_ID('T','O','L','Y')
#define ID3_TOPE	ID3_FRAME_ID('T','O','P','E')
#define ID3_TORY	ID3_FRAME_ID('T','O','R','Y')
#define ID3_TOWN	ID3_FRAME_ID('T','O','W','N')
#define ID3_TPE1	ID3_FRAME_ID('T','P','E','1')
#define ID3_TPE2	ID3_FRAME_ID('T','P','E','2')
#define ID3_TPE3	ID3_FRAME_ID('T','P','E','3')
#define ID3_TPE4	ID3_FRAME_ID('T','P','E','4')
#define ID3_TPOS	ID3_FRAME_ID('T','P','O','S')
#define ID3_TPRO	ID3_FRAME_ID('T','P','R','O')
#define ID3_TPUB	ID3_FRAME_ID('T','P','U','B')
#define ID3_TRCK	ID3_FRAME_ID('T','R','C','K')
#define ID3_TRDA	ID3_FRAME_ID('T','R','D','A')
#define ID3_TRSN	ID3_FRAME_ID('T','R','S','N')
#define ID3_TRSO	ID3_FRAME_ID('T','R','S','O')
#define ID3_TSIZ	ID3_FRAME_ID('T','S','I','Z')
#define ID3_TSOA	ID3_FRAME_ID('T','S','O','A')
#define ID3_TSOP	ID3_FRAME_ID('T','S','O','P')
#define ID3_TSOT	ID3_FRAME_ID('T','S','O','T')
#define ID3_TSRC	ID3_FRAME_ID('T','S','R','C')
#define ID3_TSSE	ID3_FRAME_ID('T','S','S','E')
#define ID3_TSST	ID3_FRAME_ID('T','S','S','T')
#define ID3_TYER	ID3_FRAME_ID('T','Y','E','R')
#define ID3_TXXX	ID3_FRAME_ID('T','X','X','X')
#define ID3_UFID	ID3_FRAME_ID('U','F','I','D')
#define ID3_USER	ID3_FRAME_ID('U','S','E','R')
#define ID3_USLT	ID3_FRAME_ID('U','S','L','T')
#define ID3_WCOM	ID3_FRAME_ID('W','C','O','M')
#define ID3_WCOP	ID3_FRAME_ID('W','C','O','P')
#define ID3_WOAF	ID3_FRAME_ID('W','O','A','F')
#define ID3_WOAR	ID3_FRAME_ID('W','O','A','R')
#define ID3_WOAS	ID3_FRAME_ID('W','O','A','S')
#define ID3_WORS	ID3_FRAME_ID('W','O','R','S')
#define ID3_WPAY	ID3_FRAME_ID('W','P','A','Y')
#define ID3_WPUB	ID3_FRAME_ID('W','P','U','B')
#define ID3_WXXX	ID3_FRAME_ID('W','X','X','X')

/*
 * Version 2.2.0 
 */

#define ID3_FRAME_ID_22(a, b, c)   ((a << 16) | (b << 8) | c)

#define ID3_BUF ID3_FRAME_ID_22('B', 'U', 'F')
#define ID3_CNT ID3_FRAME_ID_22('C', 'N', 'T')
#define ID3_COM ID3_FRAME_ID_22('C', 'O', 'M')
#define ID3_CRA ID3_FRAME_ID_22('C', 'R', 'A')
#define ID3_CRM ID3_FRAME_ID_22('C', 'R', 'M')
#define ID3_ETC ID3_FRAME_ID_22('E', 'T', 'C')
#define ID3_EQU ID3_FRAME_ID_22('E', 'Q', 'U')
#define ID3_GEO ID3_FRAME_ID_22('G', 'E', 'O')
#define ID3_IPL ID3_FRAME_ID_22('I', 'P', 'L')
#define ID3_LNK ID3_FRAME_ID_22('L', 'N', 'K')
#define ID3_MCI ID3_FRAME_ID_22('M', 'C', 'I')
#define ID3_MLL ID3_FRAME_ID_22('M', 'L', 'L')
#define ID3_PIC ID3_FRAME_ID_22('P', 'I', 'C')
#define ID3_POP ID3_FRAME_ID_22('P', 'O', 'P')
#define ID3_REV ID3_FRAME_ID_22('R', 'E', 'V')
#define ID3_RVA ID3_FRAME_ID_22('R', 'V', 'A')
#define ID3_SLT ID3_FRAME_ID_22('S', 'L', 'T')
#define ID3_STC ID3_FRAME_ID_22('S', 'T', 'C')
#define ID3_TAL ID3_FRAME_ID_22('T', 'A', 'L')
#define ID3_TBP ID3_FRAME_ID_22('T', 'B', 'P')
#define ID3_TCM ID3_FRAME_ID_22('T', 'C', 'M')
#define ID3_TCO ID3_FRAME_ID_22('T', 'C', 'O')
#define ID3_TCR ID3_FRAME_ID_22('T', 'C', 'R')
#define ID3_TDA ID3_FRAME_ID_22('T', 'D', 'A')
#define ID3_TDY ID3_FRAME_ID_22('T', 'D', 'Y')
#define ID3_TEN ID3_FRAME_ID_22('T', 'E', 'N')
#define ID3_TFT ID3_FRAME_ID_22('T', 'F', 'T')
#define ID3_TIM ID3_FRAME_ID_22('T', 'I', 'M')
#define ID3_TKE ID3_FRAME_ID_22('T', 'K', 'E')
#define ID3_TLA ID3_FRAME_ID_22('T', 'L', 'A')
#define ID3_TLE ID3_FRAME_ID_22('T', 'L', 'E')
#define ID3_TMT ID3_FRAME_ID_22('T', 'M', 'T')
#define ID3_TOA ID3_FRAME_ID_22('T', 'O', 'A')
#define ID3_TOF ID3_FRAME_ID_22('T', 'O', 'F')
#define ID3_TOL ID3_FRAME_ID_22('T', 'O', 'L')
#define ID3_TOR ID3_FRAME_ID_22('T', 'O', 'R')
#define ID3_TOT ID3_FRAME_ID_22('T', 'O', 'T')
#define ID3_TP1 ID3_FRAME_ID_22('T', 'P', '1')
#define ID3_TP2 ID3_FRAME_ID_22('T', 'P', '2')
#define ID3_TP3 ID3_FRAME_ID_22('T', 'P', '3')
#define ID3_TP4 ID3_FRAME_ID_22('T', 'P', '4')
#define ID3_TPA ID3_FRAME_ID_22('T', 'P', 'A')
#define ID3_TPB ID3_FRAME_ID_22('T', 'P', 'B')
#define ID3_TRC ID3_FRAME_ID_22('T', 'R', 'C')
#define ID3_TRD ID3_FRAME_ID_22('T', 'R', 'D')
#define ID3_TRK ID3_FRAME_ID_22('T', 'R', 'K')
#define ID3_TSI ID3_FRAME_ID_22('T', 'S', 'I')
#define ID3_TSS ID3_FRAME_ID_22('T', 'S', 'S')
#define ID3_TT1 ID3_FRAME_ID_22('T', 'T', '1')
#define ID3_TT2 ID3_FRAME_ID_22('T', 'T', '2')
#define ID3_TT3 ID3_FRAME_ID_22('T', 'T', '3')
#define ID3_TXT ID3_FRAME_ID_22('T', 'X', 'T')
#define ID3_TXX ID3_FRAME_ID_22('T', 'X', 'X')
#define ID3_TYE ID3_FRAME_ID_22('T', 'Y', 'E')
#define ID3_UFI ID3_FRAME_ID_22('U', 'F', 'I')
#define ID3_ULT ID3_FRAME_ID_22('U', 'L', 'T')
#define ID3_WAF ID3_FRAME_ID_22('W', 'A', 'F')
#define ID3_WAR ID3_FRAME_ID_22('W', 'A', 'R')
#define ID3_WAS ID3_FRAME_ID_22('W', 'A', 'S')
#define ID3_WCM ID3_FRAME_ID_22('W', 'C', 'M')
#define ID3_WCP ID3_FRAME_ID_22('W', 'C', 'P')
#define ID3_WPB ID3_FRAME_ID_22('W', 'P', 'B')
#define ID3_WXX ID3_FRAME_ID_22('W', 'X', 'X')


/*
 * Prototypes.
 */

/* From id3.c */
struct id3_tag *id3_open_mem(void *, int);
struct id3_tag *id3_open_fd(int, int);
struct id3_tag *id3_open_fp(VFSFile *, int);
int id3_set_output(struct id3_tag *, char *);
int id3_close(struct id3_tag *);
int id3_tell(struct id3_tag *);
int id3_alter_file(struct id3_tag *);
int id3_write_tag(struct id3_tag *, int);

/* From id3_frame.c */
int id3_read_frame(struct id3_tag *id3);
struct id3_frame *id3_get_frame(struct id3_tag *, guint32, int);
int id3_delete_frame(struct id3_frame *frame);
struct id3_frame *id3_add_frame(struct id3_tag *, guint32);
int id3_decompress_frame(struct id3_frame *);
void id3_destroy_frames(struct id3_tag *id);
void id3_frame_clear_data(struct id3_frame *frame);

/* From id3_frame_text.c */
char *id3_utf16_to_ascii(void *);
gint8 id3_get_encoding(struct id3_frame *);
int id3_set_encoding(struct id3_frame *, gint8);
char *id3_get_text(struct id3_frame *);
char *id3_get_text_desc(struct id3_frame *);
int id3_get_text_number(struct id3_frame *);
int id3_set_text(struct id3_frame *, char *);
int id3_set_text_number(struct id3_frame *, int);
gboolean id3_frame_is_text(struct id3_frame *frame);

/* From id3_frame_content.c */
char *id3_get_content(struct id3_frame *);

/* From id3_frame_url.c */
char *id3_get_url(struct id3_frame *);
char *id3_get_url_desc(struct id3_frame *);

/* From id3_tag.c */
void id3_init_tag(struct id3_tag *id3);
int id3_read_tag(struct id3_tag *id3);

#endif                          /* ID3_H */
