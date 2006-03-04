/*********************************************************************
 * 
 *    Copyright (C) 1999-2000,  Espen Skoglund
 *    Department of Computer Science, University of Tromsø
 * 
 * Filename:      id3_tag.c
 * Description:   Code for handling ID3 tags.
 * Author:        Espen Skoglund <espensk@stud.cs.uit.no>
 * Created at:    Tue Feb  9 21:13:19 1999
 *                
 * $Id: id3_tag.c,v 1.6 2004/07/20 21:47:22 descender Exp $
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

#include "xmms-id3.h"
#include "id3_header.h"


/*
 * Function id3_init_tag (id3)
 *
 *    Initialize an empty ID3 tag.
 *
 */
void
id3_init_tag(struct id3_tag *id3)
{
    /*
     * Initialize header.
     */
    id3->id3_version = 3;
    id3->id3_revision = 0;
    id3->id3_flags = ID3_THFLAG_USYNC | ID3_THFLAG_EXP;
    id3->id3_tagsize = 0;

    id3->id3_altered = 1;
    id3->id3_newtag = 1;
    id3->id3_pos = 0;

    /*
     * Initialize frames.
     */
    id3->id3_frame = NULL;
}


/*
 * Function id3_read_tag (id3)
 *
 *    Read the ID3 tag from the input stream.  The start of the tag
 *    must be positioned in the next tag in the stream.  Return 0 upon
 *    success, or -1 if an error occured.
 *
 */
int
id3_read_tag(struct id3_tag *id3)
{
    char *buf;
    gint  cyc = 0;	/* deal with malformed tags gracefully through iteration pacing */

    /*
     * We know that the tag will be at least this big.
     *
     * tag header + "ID3"
     */
    id3->id3_tagsize = ID3_TAGHDR_SIZE + 3;

    if (!(id3->id3_oflags & ID3_OPENF_NOCHK)) {
        /*
         * Check if we have a valid ID3 tag.
         */
        char *id = id3->id3_read(id3, NULL, 3);
        if (id == NULL)
            return -1;

        if (id[0] != 'I' || id[1] != 'D' || id[2] != '3') {
            /*
             * ID3 tag was not detected.
             */
            id3->id3_seek(id3, -3);
            return -1;
        }
    }

    /*
     * Read ID3 tag-header.
     */
    buf = id3->id3_read(id3, NULL, ID3_TAGHDR_SIZE);
    if (buf == NULL)
        return -1;

    id3->id3_version = buf[0];
    id3->id3_revision = buf[1];
    id3->id3_flags = buf[2];
    id3->id3_tagsize = ID3_GET_SIZE28(buf[3], buf[4], buf[5], buf[6]);
    id3->id3_newtag = 0;
    id3->id3_pos = 0;

    if (id3->id3_version < 2 || id3->id3_version > 4)
        return -1;

    /*
     * Parse extended header.
     */
    if (id3->id3_flags & ID3_THFLAG_EXT) {
        buf = id3->id3_read(id3, NULL, ID3_EXTHDR_SIZE);
        if (buf == NULL)
            return -1;
    }

    /*
     * Parse frames.
     */
    while (id3->id3_pos < id3->id3_tagsize) {
        if (id3_read_frame(id3) == -1)
            return -1;

	/* 100 iterations, this ID3 is definately hosed */
	if (++cyc == 100)
            return -1;
    }

    return 0;
}
