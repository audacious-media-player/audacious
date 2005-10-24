/*********************************************************************
 * 
 *    Copyright (C) 1999, 2001, 2002,
 *    Department of Computer Science, University of Tromsø
 * 
 * Filename:      id3_frame_url.c
 * Description:   Code for handling ID3 URL frames.
 * Author:        Espen Skoglund <espensk@stud.cs.uit.no>
 * Created at:    Tue Feb  9 21:10:45 1999
 *                
 * $Id: id3_frame_url.c,v 1.6 2004/07/20 21:47:22 descender Exp $
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

#include "xmms-id3.h"
#include "id3_header.h"



/*
 * Function id3_get_url (frame)
 *
 *    Return URL of frame.
 *
 */
char *
id3_get_url(struct id3_frame *frame)
{
    /* Type check */
    if (frame->fr_desc->fd_idstr[0] != 'W')
        return NULL;

    /* Check if frame is compressed */
    if (id3_decompress_frame(frame) == -1)
        return NULL;

    if (frame->fr_desc->fd_id == ID3_WXXX) {
        /*
         * This is a user defined link frame.  Skip the description.
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
                gint16 *text16 = (gint16 *) ((glong) frame->fr_data + 1);

                while (*text16 != 0)
                    text16++;

                return g_strdup((char *) (++text16));
            }
        default:
            return NULL;
        }
    }

    return g_strdup((char *) frame->fr_data);
}


/*
 * Function id3_get_url_desc (frame)
 *
 *    Get description of a URL.
 *
 */
char *
id3_get_url_desc(struct id3_frame *frame)
{
    /* Type check */
    if (frame->fr_desc->fd_idstr[0] != 'W')
        return NULL;

    /* If predefined link frame, return description. */
    if (frame->fr_desc->fd_id != ID3_WXXX)
        return frame->fr_desc->fd_description;

    /* Check if frame is compressed */
    if (id3_decompress_frame(frame) == -1)
        return NULL;

    if (*(guint8 *) frame->fr_data == ID3_ENCODING_ISO_8859_1)
        return g_strdup((char *) frame->fr_data + 1);
    else
        return id3_utf16_to_ascii((gint16 *) ((glong) frame->fr_data + 1));
}
