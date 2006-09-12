/*
*
* Author: Giacomo Lozito <james@develia.org>, (C) 2005-2006
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*
*/

#ifndef _I_MIDIEVENT_H
#define _I_MIDIEVENT_H 1

struct midievent_stru {
  struct midievent_stru * next;		/* linked list */
  guchar type;				/* SND_SEQ_EVENT_xxx */
  guchar port;				/* port index */
  guint tick;
  guint tick_real;			/* tick with custom offset */
  union {
    guchar d[3];			/* channel and data bytes */
    gint tempo;
    guint length;			/* length of sysex data */
    gchar * metat;			/* meta-event text */
  } data;
  guchar sysex[0];
};

typedef struct midievent_stru midievent_t;

#endif /* !_I_MIDIEVENT_H */
