/*********************************************************************
 * 
 *    Copyright (C) 1998, 1999,  Espen Skoglund
 *    Department of Computer Science, University of Tromsø
 * 
 * Filename:      id3_header.h
 * Description:   Definitions for various ID3 headers.
 * Author:        Espen Skoglund <espensk@stud.cs.uit.no>
 * Created at:    Thu Nov  5 15:55:10 1998
 *                
 * $Id: id3_header.h,v 1.4 2004/04/13 23:53:01 descender Exp $
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
#ifndef ID3_HEADER_H
#define ID3_HEADER_H

#include <stdio.h>

/*
 * Layout for the ID3 tag header.
 */
#if 0
struct id3_taghdr {
    guint8 th_version;
    guint8 th_revision;
    guint8 th_flags;
    guint32 th_size;
};
#endif

/* Header size excluding "ID3" */
#define ID3_TAGHDR_SIZE 7       /* Size on disk */

#define ID3_THFLAG_USYNC	0x80000000
#define ID3_THFLAG_EXT		0x40000000
#define ID3_THFLAG_EXP		0x20000000

#if 0
#define ID3_SET_SIZE28(size)		\
    ( ((size << 3) & 0x7f000000) |	\
      ((size << 2) & 0x007f0000) |	\
      ((size << 1) & 0x00007f00) |	\
      ((size     ) & 0x0000007f) )

#define ID3_GET_SIZE28(size)		\
    ( ((size & 0x7f000000) >> 3) |	\
      ((size & 0x007f0000) >> 2) |	\
      ((size & 0x00007f00) >> 1) |	\
      ((size & 0x0000007f)     ) )
#endif

#define ID3_SET_SIZE28(size, a, b, c, d)	\
do {						\
	a = (size >> (24 + 3)) & 0x7f;		\
	b = (size >> (16 + 2)) & 0x7f;		\
	c = (size >> ( 8 + 1)) & 0x7f;		\
	d = size & 0x7f;			\
} while (0)

#define ID3_GET_SIZE28(a, b, c, d)		\
(((a & 0x7f) << (24 - 3)) |			\
 ((b & 0x7f) << (16 - 2)) |			\
 ((c & 0x7f) << ( 8 - 1)) |			\
 ((d & 0x7f)))



/*
 * Layout for the extended header.
 */
#if 0
struct id3_exthdr {
    guint32 eh_size;
    guint16 eh_flags;
    guint32 eh_padsize;
};
#endif

#define ID3_EXTHDR_SIZE 10

#define ID3_EHFLAG_CRC		0x80000000



/*
 * Layout for the frame header.
 */
#if 0
struct id3_framehdr {
    guint32 fh_id;
    guint32 fh_size;
    guint16 fh_flags;
};
#endif

#define ID3_FRAMEHDR_SIZE 10


#define ID3_FHFLAG_TAGALT	0x8000
#define ID3_FHFLAG_FILEALT	0x4000
#define ID3_FHFLAG_RO		0x2000
#define ID3_FHFLAG_COMPRESS	0x0080
#define ID3_FHFLAG_ENCRYPT	0x0040
#define ID3_FHFLAG_GROUP	0x0020


typedef enum {
    ID3_UNI_LATIN = 0x007f,
    ID3_UNI_LATIN_1 = 0x00ff,

    ID3_UNI_SUPPORTED = 0x00ff,
    ID3_UNI_UNSUPPORTED = 0xffff,
} id3_unicode_blocks;

#define DEBUG_ID3
#ifdef DEBUG_ID3
#define id3_error(id3, error)		\
  (void) ( id3->id3_error_msg = error,	\
           printf( "Error %s, line %d: %s\n", __FILE__, __LINE__, error ) )


#else
#define id3_error(id3, error)		\
  (void) ( id3->id3_error_msg = error )

#endif

/*
 * Version 2.2.0 
 */

/*
 * Layout for the frame header.
 */
#if 0
struct id3_framehdr {
    char fh_id[3];
    guint8 fh_size[3];
};
#endif

#define ID3_FRAMEHDR_SIZE_22 6

#endif                          /* ID3_HEADER_H */
