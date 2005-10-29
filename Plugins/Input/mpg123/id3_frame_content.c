/*********************************************************************
 * 
 *    Copyright (C) 1999, 2002,  Espen Skoglund
 *    Department of Computer Science, University of Tromsø
 * 
 * Filename:      id3_frame_content.c
 * Description:   Code for handling ID3 content frames.
 * Author:        Espen Skoglund <espensk@stud.cs.uit.no>
 * Created at:    Mon Feb  8 17:13:46 1999
 *                
 * $Id: id3_frame_content.c,v 1.7 2004/07/20 21:47:22 descender Exp $
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

#include <glib.h>
#include <glib/gi18n.h>

#include "xmms-id3.h"

#include "mpg123.h"


/*
 * Function id3_get_content (frame)
 *
 *    Expand content type string of frame and return it.  Return NULL
 *    upon error.
 *
 */
char *
id3_get_content(struct id3_frame *frame)
{
    char *text, *text_beg, *ptr;
    char buffer[256];
    int spc = sizeof(buffer) - 1;

    /* Type check */
    if (frame->fr_desc->fd_id != ID3_TCON)
        return NULL;

    /* Check if frame is compressed */
    if (id3_decompress_frame(frame) == -1)
        return NULL;

    if (*(guint8 *) frame->fr_data == ID3_ENCODING_ISO_8859_1)
        text_beg = text = g_strdup((char *) frame->fr_data + 1);
    else
        text_beg = text = id3_utf16_to_ascii((char *) frame->fr_data + 1);

    /*
     * If content is just plain text, return it.
     */
    if (text[0] != '(') {
        return text;
    }

    /*
     * Expand ID3v1 genre numbers.
     */
    ptr = buffer;
    while (text[0] == '(' && text[1] != '(' && spc > 0) {
        const char *genre;
        int num = 0;

        if (text[1] == 'R' && text[2] == 'X') {
            text += 4;
            genre = _(" (Remix)");
            if (ptr == buffer)
                genre++;

        }
        else if (text[1] == 'C' && text[2] == 'R') {
            text += 4;
            genre = _(" (Cover)");
            if (ptr == buffer)
                genre++;

        }
        else {
            /* Get ID3v1 genre number */
            text++;
            while (*text != ')') {
                num *= 10;
                num += *text++ - '0';
            }
            text++;

            /* Boundary check */
            if (num >= sizeof(mpg123_id3_genres) / sizeof(char *))
                continue;

            genre = gettext(mpg123_id3_genres[num]);

            if (ptr != buffer && spc-- > 0)
                *ptr++ = '/';
        }

        /* Expand string into buffer */
        while (*genre != '\0' && spc > 0) {
            *ptr++ = *genre++;
            spc--;
        }
    }

    /*
     * Add plaintext refinement.
     */
    if (*text == '(')
        text++;
    if (*text != '\0' && ptr != buffer && spc-- > 0)
        *ptr++ = ' ';
    while (*text != '\0' && spc > 0) {
        *ptr++ = *text++;
        spc--;
    }
    *ptr = '\0';

    g_free(text_beg);

    /*
     * Return the expanded content string.
     */
    return g_strdup(buffer);
}
