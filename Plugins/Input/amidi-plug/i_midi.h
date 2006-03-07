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
* 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
*/

#ifndef _I_MIDI_H
#define _I_MIDI_H 1

#include "i_common.h"
#include <alsa/asoundlib.h>

#define MAKE_ID(c1, c2, c3, c4) ((c1) | ((c2) << 8) | ((c3) << 16) | ((c4) << 24))

/* MIDI event we care of */
#define MIDI_EVENT_NOTEOFF	1
#define MIDI_EVENT_NOTEON	2
#define MIDI_EVENT_KEYPRESS	3
#define MIDI_EVENT_CONTROLLER	4
#define MIDI_EVENT_PGMCHANGE	5
#define MIDI_EVENT_CHANPRESS	6
#define MIDI_EVENT_PITCHBEND	7
#define MIDI_EVENT_SYSEX	8
#define MIDI_EVENT_TEMPO	9


struct midievent_stru {
  struct midievent_stru * next;		/* linked list */
  guchar type;				/* SND_SEQ_EVENT_xxx */
  guchar port;				/* port index */
  guint tick;
  union {
    guchar d[3];			/* channel and data bytes */
    gint tempo;
    guint length;			/* length of sysex data */
  } data;
  guchar sysex[0];
};

typedef struct midievent_stru midievent_t;

typedef struct
{
  midievent_t * first_event;	/* list of all events in this track */
  gint end_tick;			/* length of this track */
  midievent_t * current_event;	/* used while loading and playing */
}
midifile_track_t;

typedef struct
{
  FILE * file_pointer;
  gchar * file_name;
  gint file_offset;

  gint num_tracks;
  midifile_track_t *tracks;

  gushort format;
  guint max_tick;
  gint smpte_timing;

  gint time_division;
  gint ppq;
  gint current_tempo;

  gint playing_tick;
  gint avg_microsec_per_tick;
  gint length;

  gint skip_offset;
}
midifile_t;

extern midifile_t midifile;

midievent_t * i_midi_file_new_event( midifile_track_t * , gint );
void i_midi_file_skip_bytes( midifile_t * , gint );
gint i_midi_file_read_byte( midifile_t * );
gint i_midi_file_read_32_le( midifile_t * );
gint i_midi_file_read_id( midifile_t * );
gint i_midi_file_read_int( midifile_t * , gint );
gint i_midi_file_read_var( midifile_t * );
gint i_midi_file_read_track( midifile_t * , midifile_track_t * , gint , gint );
gint i_midi_file_parse_riff( midifile_t * );
gint i_midi_file_parse_smf( midifile_t * , gint );
void i_midi_init( midifile_t * );
void i_midi_free( midifile_t * );
gint i_midi_setget_tempo( midifile_t * );
void i_midi_setget_length( midifile_t * );
void i_midi_get_bpm( midifile_t * , gint * , gint * );
/* helper function */
gint i_midi_parse_from_filename( gchar * , midifile_t * );

#endif /* !_I_MIDI_H */
