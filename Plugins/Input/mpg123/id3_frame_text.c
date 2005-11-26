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
    guint8 encoding;

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

    ID3_FRAME_DEFINE_CURSOR(frame);
    ID3_FRAME_READ_OR_RETVAL(encoding, -1);

    return encoding;
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

/* Get size of string in bytes including null. */
gsize id3_string_size(guint8 encoding, const void* text, gsize max_size)
{
    switch ( encoding ) {
    case ID3_ENCODING_ISO_8859_1:
    case ID3_ENCODING_UTF8:
    {
        const guint8* text8 = text;
        while ( (max_size >= sizeof(*text8)) && (*text8 != 0) )
        {
    	   ++text8;
    	   max_size -= sizeof(*text8);
    	}
        
        if (max_size >= sizeof(*text8))
        {
           ++text8;
           max_size -= sizeof(*text8);
        }
        
        return text8 - (guint8*)text;
    }
    case ID3_ENCODING_UTF16:
    case ID3_ENCODING_UTF16BE:
    {
        const guint16* text16 = (guint16*)text;
        while ( (max_size > 0) && (*text16 != 0) )
        {
    	   ++text16;
    	   max_size -= sizeof(*text16);
    	}
    	
    	if (max_size > 0)
    	{
           ++text16;
           max_size -= sizeof(*text16);
        }

        return (guint8*)text16 - (guint8*)text;
    }
    default:
        return 0;
    }
}

/* 
Returns a newly-allocated UTF-8 string in the locale's encoding.
max_size specifies the maximum size of 'text', including terminating nulls.
*/
gchar* id3_string_decode(guint8 encoding, const void* text, gsize max_size)
{
    /* Otherwise, we'll end up passing -1 to functions, eliminating safety benefits. */
    if (max_size <= 0)
       return NULL;

    switch( encoding )
    {
        case ID3_ENCODING_ISO_8859_1:
        {
            return g_locale_to_utf8((const gchar*)text, max_size, NULL, NULL, NULL);
        }
        case ID3_ENCODING_UTF8:
        {
            if (g_utf8_validate((const gchar*)text, max_size, NULL))
               return g_strndup((const gchar*)text, max_size);
            else
               return NULL;
        }
        case ID3_ENCODING_UTF16:
        {
            gsize size_bytes = id3_string_size(encoding, text, max_size);
            gchar* utf8 = g_convert((const gchar*)text, size_bytes, "UTF-8", "UTF-16", NULL, NULL, NULL);
            /* If conversion passes on the BOM as-is, we strip it. */
            if (g_utf8_get_char(utf8) == 0xfeff)
            {
               gchar* new_utf8 = g_strdup(utf8+3);
               g_free(utf8);
               utf8 = new_utf8;
            }
            return utf8;
        }
        case ID3_ENCODING_UTF16BE:
        {
            gsize size_bytes = id3_string_size(encoding, text, max_size);
            return g_convert((const gchar*)text, size_bytes, "UTF-8", "UTF-16BE", NULL, NULL, NULL);
        }
        default:
            return NULL;
    }
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
    guint8 encoding;

    /* Type check */
    if (frame->fr_desc->fd_idstr[0] != 'T')
        return NULL;

    /* If predefined text frame, return description. */
    if (frame->fr_desc->fd_id != ID3_TXXX)
        return frame->fr_desc->fd_description;

    /* Check if frame is compressed */
    if (id3_decompress_frame(frame) == -1)
        return NULL;

    ID3_FRAME_DEFINE_CURSOR(frame);
    ID3_FRAME_READ_OR_RETVAL(encoding, NULL);

    return id3_string_decode(encoding, cursor, length);
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
    guint8 encoding;
    int number = 0;

    /* Check if frame is compressed */
    if (id3_decompress_frame(frame) == -1)
	    return -1;

    ID3_FRAME_DEFINE_CURSOR(frame);
    ID3_FRAME_READ_OR_RETVAL(encoding, number);

    gchar* number_str = id3_string_decode(encoding, cursor, length);
    if (number_str != NULL)
    {
       sscanf(number_str, "%d", &number);
       g_free(number_str);
    }
    
    return number;
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
    strcpy(text, buf);

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
