/*********************************************************************
 * 
 *    Copyright (C) 1999, 2001, 2002,  Espen Skoglund
 *    Department of Computer Science, University of Tromsø
 * 
 * Filename:      id3_frame_text.c
 * Description:   Code for handling ID3 text frames.
 * Author:        Espen Skoglund <espensk@stud.cs.uit.no>
 * Created at:    Fri Feb  5 23:50:33 1999
 *                
 * $Id: id3_frame_text.c,v 1.7 2004/08/21 13:04:47 descender Exp $
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "xmms-id3.h"
#include "id3_header.h"


char *
id3_utf16_to_ascii(void *utf16)
{
    char ascii[256];
    char *uc = (char *) utf16 + 2;
    size_t i;

    for (i = 0; *uc != 0 && i < sizeof(ascii); i++, uc += 2)
        ascii[i] = *uc;

    ascii[i] = 0;
    return g_strdup(ascii);
}


/*
 * Function id3_get_encoding (frame)
 *
 *    Return text encoding for frame, or -1 if frame does not have any
 *    text encoding.
 *
 */
gint8
id3_get_encoding(struct id3_frame * frame)
{
    /* Type check */
    if (!id3_frame_is_text(frame) &&
        frame->fr_desc->fd_id != ID3_WXXX &&
        frame->fr_desc->fd_id != ID3_IPLS &&
        frame->fr_desc->fd_id != ID3_USLT &&
        frame->fr_desc->fd_id != ID3_SYLT &&
        frame->fr_desc->fd_id != ID3_COMM &&
        frame->fr_desc->fd_id != ID3_APIC &&
        frame->fr_desc->fd_id != ID3_GEOB &&
        frame->fr_desc->fd_id != ID3_USER &&
        frame->fr_desc->fd_id != ID3_OWNE &&
        frame->fr_desc->fd_id != ID3_COMR)
        return -1;

    /* Check if frame is compressed */
    if (id3_decompress_frame(frame) == -1)
        return -1;

    return *(gint8 *) frame->fr_data;
}


/*
 * Function id3_set_encoding (frame, encoding)
 *
 *    Set text encoding for frame.  Return 0 upon success, or -1 if an
 *    error occured. 
 *
 */
int
id3_set_encoding(struct id3_frame *frame, gint8 encoding)
{
    /* Type check */
    if (frame->fr_desc->fd_idstr[0] != 'T' &&
        frame->fr_desc->fd_id != ID3_WXXX &&
        frame->fr_desc->fd_id != ID3_IPLS &&
        frame->fr_desc->fd_id != ID3_USLT &&
        frame->fr_desc->fd_id != ID3_SYLT &&
        frame->fr_desc->fd_id != ID3_COMM &&
        frame->fr_desc->fd_id != ID3_APIC &&
        frame->fr_desc->fd_id != ID3_GEOB &&
        frame->fr_desc->fd_id != ID3_USER &&
        frame->fr_desc->fd_id != ID3_OWNE &&
        frame->fr_desc->fd_id != ID3_COMR)
        return -1;

    /* Check if frame is compressed */
    if (id3_decompress_frame(frame) == -1)
        return -1;

    /* Changing the encoding of frames is not supported yet */
    if (*(gint8 *) frame->fr_data != encoding)
        return -1;

    /* Set encoding */
    *(gint8 *) frame->fr_data = encoding;
    return 0;
}


/*
 * Function id3_get_text (frame)
 *
 *    Return string contents of frame.
 *
 */
char *
id3_get_text(struct id3_frame *frame)
{
    /* Type check */
    if (frame->fr_desc->fd_idstr[0] != 'T' && frame->fr_desc->fd_id != ID3_COMM)
        return NULL;

    /* Check if frame is compressed */
    if (id3_decompress_frame(frame) == -1)
        return NULL;

    if (frame->fr_desc->fd_id == ID3_TXXX || frame->fr_desc->fd_id == ID3_COMM) {
        /*
         * This is a user defined text frame.  Skip the description.
         */
        switch (*(guint8 *) frame->fr_data) {
        case ID3_ENCODING_ISO_8859_1:
            {
                char *text = (char *) frame->fr_data + 1;

                while (*text != 0)
                    text++;

                return g_strdup(++text);
            }
        case ID3_ENCODING_UTF16:
            {
                char *text16 = (char *) frame->fr_data + 1;

                while (*text16 != 0 || *(text16 + 1) != 0)
                    text16 += 2;

                return id3_utf16_to_ascii(text16 + 2);
            }
        default:
            return NULL;
        }
    }

    if (*(guint8 *) frame->fr_data == ID3_ENCODING_ISO_8859_1)
        return g_strdup((char *) frame->fr_data + 1);
    else
        return id3_utf16_to_ascii(((char *) frame->fr_data + 1));
}


/*
 * Function id3_get_text_desc (frame)
 *
 *    Get description part of a text frame.
 *
 */
char *
id3_get_text_desc(struct id3_frame *frame)
{
    /* Type check */
    if (frame->fr_desc->fd_idstr[0] != 'T')
        return NULL;

    /* If predefined text frame, return description. */
    if (frame->fr_desc->fd_id != ID3_TXXX)
        return frame->fr_desc->fd_description;

    /* Check if frame is compressed */
    if (id3_decompress_frame(frame) == -1)
        return NULL;

    if (*(guint8 *) frame->fr_data == ID3_ENCODING_ISO_8859_1)
        return g_strdup((char *) frame->fr_data + 1);
    else
        return id3_utf16_to_ascii((char *) frame->fr_data + 1);
}


/*
 * Function id3_get_text_number (frame)
 *
 *    Return string contents of frame translated to a positive
 *    integer, or -1 if an error occured.
 *
 */
int
id3_get_text_number(struct id3_frame *frame)
{
    int number = 0;

    /* Check if frame is compressed */
    if (id3_decompress_frame(frame) == -1)
        return -1;

    /*
     * Generate integer according to encoding.
     */
    switch (*(guint8 *) frame->fr_data) {
    case ID3_ENCODING_ISO_8859_1:
        {
            char *text = ((char *) frame->fr_data) + 1;

            while (*text >= '0' && *text <= '9') {
                number *= 10;
                number += *text - '0';
                text++;
            }

            return number;
        }
    case ID3_ENCODING_UTF16:
        {
            char *text = ((char *) frame->fr_data) + 3;

/*  	if (*(gint16 *) frame->fr_data == 0xfeff) */
/*  	    text++; */

            while (*text >= '0' && *text <= '9') {
                number *= 10;
                number += *text - '0';
                text++;
            }

            return number;
        }

    default:
        return -1;
    }
}


/*
 * Function id3_set_text (frame, text)
 *
 *    Set text for the indicated frame (only ISO-8859-1 is currently
 *    supported).  Return 0 upon success, or -1 if an error occured.
 *
 */
int
id3_set_text(struct id3_frame *frame, char *text)
{
    /* Type check */
    if (frame->fr_desc->fd_idstr[0] != 'T')
        return -1;

    /*
     * Release memory occupied by previous data.
     */
    id3_frame_clear_data(frame);

    /*
     * Allocate memory for new data.
     */
    frame->fr_raw_size = strlen(text) + 1;
    frame->fr_raw_data = g_malloc(frame->fr_raw_size + 1);

    /*
     * Copy contents.
     */
    *(gint8 *) frame->fr_raw_data = ID3_ENCODING_ISO_8859_1;
    memcpy((char *) frame->fr_raw_data + 1, text, frame->fr_raw_size);

    frame->fr_altered = 1;
    frame->fr_owner->id3_altered = 1;

    frame->fr_data = frame->fr_raw_data;
    frame->fr_size = frame->fr_raw_size;

    return 0;
}


/*
 * Function id3_set_text_number (frame, number)
 *
 *    Set number for the indicated frame (only ISO-8859-1 is currently
 *    supported).  Return 0 upon success, or -1 if an error occured.
 *
 */
int
id3_set_text_number(struct id3_frame *frame, int number)
{
    char buf[64];
    int pos;
    char *text;

    /* Type check */
    if (frame->fr_desc->fd_idstr[0] != 'T')
        return -1;

    /*
     * Release memory occupied by previous data.
     */
    id3_frame_clear_data(frame);

    /*
     * Create a string with a reversed number.
     */
    pos = 0;
    while (number > 0 && pos < 64) {
        buf[pos++] = (number % 10) + '0';
        number /= 10;
    }
    if (pos == 64)
        return -1;
    if (pos == 0)
        buf[pos++] = '0';

    /*
     * Allocate memory for new data.
     */
    frame->fr_raw_size = pos + 1;
    frame->fr_raw_data = g_malloc(frame->fr_raw_size + 1);

    /*
     * Insert contents.
     */
    *(gint8 *) frame->fr_raw_data = ID3_ENCODING_ISO_8859_1;
    text = (char *) frame->fr_raw_data + 1;
    while (--pos >= 0)
        *text++ = buf[pos];
    *text = '\0';

    frame->fr_altered = 1;
    frame->fr_owner->id3_altered = 1;

    frame->fr_data = frame->fr_raw_data;
    frame->fr_size = frame->fr_raw_size;

    return 0;
}

gboolean
id3_frame_is_text(struct id3_frame * frame)
{
    if (frame && frame->fr_desc &&
        (frame->fr_desc->fd_idstr[0] == 'T' ||
         frame->fr_desc->fd_idstr[0] == 'W'))
        return TRUE;
    return FALSE;
}
