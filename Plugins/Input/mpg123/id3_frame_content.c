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
#include <string.h>

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
     gchar *text, *text_it;
     guint8 encoding;

     /* Type check */
     if (frame->fr_desc->fd_id != ID3_TCON)
         return NULL;

     /* Check if frame is compressed */
     if (id3_decompress_frame(frame) == -1)
         return NULL;

     ID3_FRAME_DEFINE_CURSOR(frame);
     ID3_FRAME_READ_OR_RETVAL(encoding, NULL);

     text = text_it = id3_string_decode(encoding, cursor, length);

     if (text == NULL)
        return NULL;

     /*
      * Expand ID3v1 genre numbers.
      */
     while ((text_it = strstr(text_it, "(")) != NULL)
     {
         gchar* replace = NULL;
         gchar* ref_start = text_it + 1;

         if (*ref_start == ')')
         {
            /* False alarm */
            ++text_it;
            continue;
         }

         gsize ref_size = strstr(ref_start, ")") - ref_start;

         if (strncmp(ref_start, "RX", ref_size) == 0)
         {
            replace = _("Remix");
         }
         else if (strncmp(ref_start, "CR", ref_size) == 0)
         {
            replace = _("Cover");
         }
         else
         {
             /* Maybe an ID3v1 genre? */
             int genre_number;
             gchar* genre_number_str = g_strndup(ref_start, ref_size);
             if (sscanf(genre_number_str, "%d", &genre_number) > 0)
             {
                 /* Boundary check */
                 if (genre_number >= sizeof(mpg123_id3_genres) / sizeof(char *))
                     continue;

                 replace = gettext(mpg123_id3_genres[genre_number]);
             }
         }

         if (replace != NULL)
         {
             /* Amazingly hairy code to replace a part of the original genre string
                with 'replace'. */
             gchar* copy = g_malloc(strlen(text) - ref_size + strlen(replace) + 1);
             gsize pos = 0;
             gsize copy_size;

             /* Copy the part before the replaced part */
             copy_size = ref_start - text;
             memcpy(copy + pos, text, copy_size);
             pos += copy_size;
             /* Copy the replacement instead of the original reference */
             copy_size = strlen(replace);
             memcpy(copy + pos, replace, copy_size);
             pos += copy_size;
             /* Copy the rest, including the null */
             memcpy(copy + pos, ref_start + ref_size, strlen(ref_start + ref_size)+1);

             /* Put into original variables */
             gsize offset = text_it - text;
             g_free(text);
             text = copy;
             text_it = text + offset;
         }

         ++text_it;
     } 

     return text;

}
