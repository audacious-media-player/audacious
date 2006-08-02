/*
*
* Author: Giacomo Lozito <james@develia.org>, (C) 2005-2006
*
* MIDI (SMF) parser based on aplaymidi.c from ALSA-utils
* aplaymidi.c is Copyright (c) 2004 Clemens Ladisch <clemens@ladisch.de>
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
* 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307  USA
*
*/


#include "i_midi.h"

#define ERRMSG_MIDITRACK() { g_warning( "%s: invalid MIDI data (offset %#x)" , mf->file_name , mf->file_offset ); return 0; }


/* skip a certain number of bytes */
void i_midi_file_skip_bytes( midifile_t * mf , gint bytes )
{
  while (bytes > 0)
  {
    i_midi_file_read_byte(mf);
    --bytes;
  }
}


/* reads a single byte */
gint i_midi_file_read_byte( midifile_t * mf )
{
  ++mf->file_offset;
  return getc(mf->file_pointer);
}


/* reads a little-endian 32-bit integer */
gint i_midi_file_read_32_le( midifile_t * mf )
{
  gint value;
  value = i_midi_file_read_byte(mf);
  value |= i_midi_file_read_byte(mf) << 8;
  value |= i_midi_file_read_byte(mf) << 16;
  value |= i_midi_file_read_byte(mf) << 24;
  return !feof(mf->file_pointer) ? value : -1;
}


/* reads a 4-character identifier */
gint i_midi_file_read_id( midifile_t * mf )
{
  return i_midi_file_read_32_le(mf);
}


/* reads a fixed-size big-endian number */
gint i_midi_file_read_int( midifile_t * mf , gint bytes )
{
  gint c, value = 0;

  do {
    c = i_midi_file_read_byte(mf);
    if (c == EOF) return -1;
    value = (value << 8) | c;
  } while (--bytes);

  return value;
}


/* reads a variable-length number */
gint i_midi_file_read_var( midifile_t * mf )
{
  gint value, c;

  c = i_midi_file_read_byte(mf);
  value = c & 0x7f;
  if (c & 0x80) {
    c = i_midi_file_read_byte(mf);
    value = (value << 7) | (c & 0x7f);
    if (c & 0x80) {
      c = i_midi_file_read_byte(mf);
      value = (value << 7) | (c & 0x7f);
      if (c & 0x80) {
        c = i_midi_file_read_byte(mf);
        value = (value << 7) | c;
        if (c & 0x80)
          return -1;
      }
    }
  }
  return !feof(mf->file_pointer) ? value : -1;
}


/* allocates a new event */
midievent_t * i_midi_file_new_event(midifile_track_t * track, gint sysex_length)
{
  midievent_t * event;

  event = malloc(sizeof(midievent_t) + sysex_length);
  /* check_mem(event); */

  event->next = NULL;

  /* append at the end of the track's linked list */
  if (track->current_event)
    track->current_event->next = event;
  else
    track->first_event = event;
  track->current_event = event;

  return event;
}


/* reads one complete track from the file */
gint i_midi_file_read_track( midifile_t * mf , midifile_track_t * track ,
                             gint track_end , gint port_count )
{
  gint tick = 0;
  guchar last_cmd = 0;
  guchar port = 0;

  /* the current file position is after the track ID and length */
  while ( mf->file_offset < track_end )
  {
    guchar cmd;
    midievent_t *event;
    gint delta_ticks, len, c;

    delta_ticks = i_midi_file_read_var(mf);
    if ( delta_ticks < 0 )
      break;
    tick += delta_ticks;

    c = i_midi_file_read_byte(mf);
    if (c < 0)
      break;

    if (c & 0x80) {
      /* have command */
      cmd = c;
      if (cmd < 0xf0)
        last_cmd = cmd;
    } else {
      /* running status */
      ungetc(c, mf->file_pointer);
      mf->file_offset--;
      cmd = last_cmd;
      if (!cmd)
        ERRMSG_MIDITRACK();
    }

    switch (cmd >> 4)
    {
      /* maps SMF events to ALSA sequencer events */
      static guchar cmd_type[] = {
        [0x8] = SND_SEQ_EVENT_NOTEOFF,
        [0x9] = SND_SEQ_EVENT_NOTEON,
        [0xa] = SND_SEQ_EVENT_KEYPRESS,
        [0xb] = SND_SEQ_EVENT_CONTROLLER,
        [0xc] = SND_SEQ_EVENT_PGMCHANGE,
        [0xd] = SND_SEQ_EVENT_CHANPRESS,
        [0xe] = SND_SEQ_EVENT_PITCHBEND
      };

      case 0x8: /* channel msg with 2 parameter bytes */
      case 0x9:
      case 0xa:
      case 0xb:
      case 0xe:
      {
        event = i_midi_file_new_event(track, 0);
        event->type = cmd_type[cmd >> 4];
        event->port = port;
        event->tick = tick;
        event->data.d[0] = cmd & 0x0f;
        event->data.d[1] = i_midi_file_read_byte(mf) & 0x7f;
        event->data.d[2] = i_midi_file_read_byte(mf) & 0x7f;
      }
      break;

      case 0xc: /* channel msg with 1 parameter byte */
      case 0xd:
      {
        event = i_midi_file_new_event(track, 0);
        event->type = cmd_type[cmd >> 4];
        event->port = port;
        event->tick = tick;
        event->data.d[0] = cmd & 0x0f;
        event->data.d[1] = i_midi_file_read_byte(mf) & 0x7f;
      }
      break;

      case 0xf:
      {
        switch (cmd)
        {
          case 0xf0: /* sysex */
          case 0xf7: /* continued sysex, or escaped commands */
          {
            len = i_midi_file_read_var(mf);
            if (len < 0)
              ERRMSG_MIDITRACK();
            if (cmd == 0xf0)
              ++len;
            event = i_midi_file_new_event(track, len);
            event->type = SND_SEQ_EVENT_SYSEX;
            event->port = port;
            event->tick = tick;
            event->data.length = len;
            if (cmd == 0xf0) {
              event->sysex[0] = 0xf0;
              c = 1;
            } else {
              c = 0;
            }
            for (; c < len; ++c)
              event->sysex[c] = i_midi_file_read_byte(mf);
          }
          break;

          case 0xff: /* meta event */
          {
            c = i_midi_file_read_byte(mf);
            len = i_midi_file_read_var(mf);
            if (len < 0)
              ERRMSG_MIDITRACK();

            switch (c)
            {
              case 0x21: /* port number */
              {
                if (len < 1)
                  ERRMSG_MIDITRACK();
                port = i_midi_file_read_byte(mf) % port_count;
                i_midi_file_skip_bytes(mf,(len - 1));
              }
              break;

              case 0x2f: /* end of track */
              {
                track->end_tick = tick;
                i_midi_file_skip_bytes(mf,(track_end - mf->file_offset));
                return 1;
              }

              case 0x51: /* tempo */
              {
                if (len < 3)
                  ERRMSG_MIDITRACK();
                if (mf->smpte_timing) {
                  /* SMPTE timing doesn't change */
                  i_midi_file_skip_bytes(mf,len);
                } else {
                  event = i_midi_file_new_event(track, 0);
                  event->type = SND_SEQ_EVENT_TEMPO;
                  event->port = port;
                  event->tick = tick;
                  event->data.tempo = i_midi_file_read_byte(mf) << 16;
                  event->data.tempo |= i_midi_file_read_byte(mf) << 8;
                  event->data.tempo |= i_midi_file_read_byte(mf);
                  i_midi_file_skip_bytes(mf,(len - 3));
                }
              }
              break;

              default: /* ignore all other meta events */
              {
                i_midi_file_skip_bytes(mf,len);
              }
              break;
            }
          }
          break;

          default: /* invalid Fx command */
            ERRMSG_MIDITRACK();
        }
      }
      break;

      default: /* cannot happen */
        ERRMSG_MIDITRACK();
    }
  }
  ERRMSG_MIDITRACK();
}


/* read a MIDI file in Standard MIDI Format */
/* return values: 0 = error , 1 = ok */
gint i_midi_file_parse_smf( midifile_t * mf , gint port_count )
{
  gint header_len, i;

  /* the curren position is immediately after the "MThd" id */
  header_len = i_midi_file_read_int(mf,4);
  if ( header_len < 6 )
  {
    g_warning( "%s: invalid file format\n" , mf->file_name );
    return 0;
  }

  mf->format = i_midi_file_read_int(mf,2);
  if (( mf->format != 0 ) && ( mf->format != 1 ))
  {
    g_warning( "%s: type %d format is not supported\n" , mf->file_name , mf->format);
    return 0;
  }

  mf->num_tracks = i_midi_file_read_int(mf,2);
  if (( mf->num_tracks < 1 ) || ( mf->num_tracks > 1000 ))
  {
    g_warning( "%s: invalid number of tracks (%d)\n" , mf->file_name , mf->num_tracks );
    mf->num_tracks = 0;
    return 0;
  }

  mf->tracks = calloc( mf->num_tracks , sizeof(midifile_track_t) );
  if ( !mf->tracks )
  {
    g_warning( "out of memory\n" );
    mf->num_tracks = 0;
    return 0;
  }

  mf->time_division = i_midi_file_read_int(mf,2);
  if ( mf->time_division < 0 )
  {
    g_warning( "%s: invalid file format\n" , mf->file_name );
    return 0;
  }

  mf->smpte_timing = !!(mf->time_division & 0x8000);

  /* read tracks */
  for ( i = 0 ; i < mf->num_tracks ; ++i )
  {
    gint len;

    /* search for MTrk chunk */
    for (;;)
    {
      gint id = i_midi_file_read_id(mf);
      len = i_midi_file_read_int(mf,4);
      if ( feof(mf->file_pointer) )
      {
        g_warning( "%s: unexpected end of file\n" , mf->file_name );
        return 0;
      }
      if (( len < 0 ) || ( len >= 0x10000000))
      {
        g_warning( "%s: invalid chunk length %d\n" , mf->file_name , len );
        return 0;
      }
      if ( id == MAKE_ID('M', 'T', 'r', 'k') )
        break;
      i_midi_file_skip_bytes(mf,len);
    }

    if ( !i_midi_file_read_track( mf , &mf->tracks[i] , mf->file_offset + len , port_count ) )
      return 0;
  }

  /* calculate the max_tick for the entire file */
  mf->max_tick = 0;
  for ( i = 0 ; i < mf->num_tracks ; ++i )
  {
    if ( mf->tracks[i].end_tick > mf->max_tick )
      mf->max_tick = mf->tracks[i].end_tick;
  }

  /* ok, success */
  return 1;
}


/* read a MIDI file enclosed in RIFF format */
/* return values: 0 = error , 1 = ok */
gint i_midi_file_parse_riff( midifile_t * mf )
{
  /* skip file length (4 bytes) */
  i_midi_file_skip_bytes(mf,4);

  /* check file type ("RMID" = RIFF MIDI) */
  if ( i_midi_file_read_id(mf) != MAKE_ID('R', 'M', 'I', 'D') )
    return 0;

  /* search for "data" chunk */
  for (;;)
  {
    gint id = i_midi_file_read_id(mf);
    gint len = i_midi_file_read_32_le(mf);

    if ( feof(mf->file_pointer) )
      return 0;

    if ( id == MAKE_ID('d', 'a', 't', 'a') )
      break;

    if (len < 0)
      return 0;

    i_midi_file_skip_bytes(mf,((len + 1) & ~1));
  }

  /* the "data" chunk must contain data in SMF format */
  if ( i_midi_file_read_id(mf) != MAKE_ID('M', 'T', 'h', 'd') )
    return 0;

  /* ok, success */
  return 1;
}


/* midifile init */
void i_midi_init( midifile_t * mf )
{
  mf->file_pointer = NULL;
  mf->file_name = NULL;
  mf->file_offset = 0;
  mf->num_tracks = 0;
  mf->tracks = NULL;
  mf->max_tick = 0;
  mf->smpte_timing = 0;
  mf->format = 0;
  mf->time_division = 0;
  mf->ppq = 0;
  mf->current_tempo = 0;
  mf->playing_tick = 0;
  mf->avg_microsec_per_tick = 0;
  mf->length = 0;
  mf->skip_offset = 0;
  return;
}


void i_midi_free( midifile_t * mf )
{
  if ( mf->tracks )
  {
    gint i;
    /* free event list for each track */
    for ( i = 0 ; i < mf->num_tracks ; ++i )
    {
      midievent_t * event = mf->tracks[i].first_event;
      midievent_t * event_tmp = NULL;
      while( event )
      {
        event_tmp = event;
        event = event->next;
        free( event_tmp );
      }
    }
    /* free track array */
    free( mf->tracks );
    mf->tracks = NULL;
  }
}


/* queue set tempo */
gint i_midi_setget_tempo( midifile_t * mf )
{
  gint smpte_timing , i = 0;
  gint time_division = mf->time_division;

  /* interpret and set tempo */
  smpte_timing = !!(time_division & 0x8000);
  if (!smpte_timing)
  {
    /* time_division is ticks per quarter */
    mf->current_tempo = 500000;
    mf->ppq = time_division;
  }
  else
  {
    /* upper byte is negative frames per second */
    i = 0x80 - ((time_division >> 8) & 0x7f);
    /* lower byte is ticks per frame */
    time_division &= 0xff;
    /* now pretend that we have quarter-note based timing */
    switch (i)
    {
      case 24:
        mf->current_tempo = 500000;
        mf->ppq = 12 * time_division;
        break;
      case 25:
        mf->current_tempo = 400000;
        mf->ppq = 10 * time_division;
        break;
      case 29: /* 30 drop-frame */
        mf->current_tempo = 100000000;
        mf->ppq = 2997 * time_division;
        break;
      case 30:
        mf->current_tempo = 500000;
        mf->ppq = 15 * time_division;
        break;
      default:
        g_warning("Invalid number of SMPTE frames per second (%d)\n", i);
        return 0;
    }
  }
  DEBUGMSG( "MIDI tempo set -> time division: %i\n" , midifile.time_division );
  DEBUGMSG( "MIDI tempo set -> tempo: %i\n" , midifile.current_tempo );
  DEBUGMSG( "MIDI tempo set -> ppq: %i\n" , midifile.ppq );
  return 1;
}


/* this will set the midi length in microseconds
   COMMENT: this will also reset current position in each track! */
void i_midi_setget_length( midifile_t * mf )
{
  gint length_microsec = 0, last_tick = 0, i = 0;
  /* get the first microsec_per_tick ratio */
  gint microsec_per_tick = (gint)(mf->current_tempo / mf->ppq);

  /* initialize current position in each track */
  for (i = 0; i < mf->num_tracks; ++i)
    mf->tracks[i].current_event = mf->tracks[i].first_event;

  /* search for tempo events in each track; in fact, since the program
     currently supports type 0 and type 1 MIDI files, we should find
     tempo events only in one track */
  DEBUGMSG( "LENGTH calc: starting calc loop\n" );
  for (;;)
  {
    midievent_t * event = NULL;
    midifile_track_t * event_track = NULL;
    gint i, min_tick = mf->max_tick + 1;

    /* search next event */
    for ( i = 0 ; i < mf->num_tracks ; ++i )
    {
      midifile_track_t * track = &mf->tracks[i];
      midievent_t * e2 = track->current_event;
      if (e2 && e2->tick < min_tick)
      {
        min_tick = e2->tick;
        event = e2;
        event_track = track;
      }
    }

    if (!event)
    {
      /* calculate the remaining length */
      length_microsec += ( microsec_per_tick * ( mf->max_tick - last_tick ) );
      break; /* end of song reached */
    }

    /* advance pointer to next event */
    event_track->current_event = event->next;

    /* check if this is a tempo event */
    if ( event->type == SND_SEQ_EVENT_TEMPO )
    {
      DEBUGMSG( "LENGTH calc: tempo event (%i) encountered during calc on tick %i\n" ,
                event->data.tempo , event->tick );
      /* increment length_microsec with the amount of microsec before tempo change */
      length_microsec += ( microsec_per_tick * ( event->tick - last_tick ) );
      /* now update last_tick and the microsec_per_tick ratio */
      last_tick = event->tick;
      microsec_per_tick = (gint)(event->data.tempo / mf->ppq);
    }
  }

  /* IMPORTANT
     this couple of important values is set by i_midi_set_length */
  mf->length = length_microsec;
  mf->avg_microsec_per_tick = (gint)(length_microsec / mf->max_tick);

  return;
}


/* this will get the weighted average bpm of the midi file;
   if the file has a variable bpm, 'bpm' is set to -1;
   COMMENT: this will also reset current position in each track! */
void i_midi_get_bpm( midifile_t * mf , gint * bpm , gint * wavg_bpm )
{
  gint i = 0 , last_tick = 0;
  guint weighted_avg_tempo = 0;
  gboolean is_monotempo = TRUE;
  gint last_tempo = mf->current_tempo;

  /* initialize current position in each track */
  for ( i = 0 ; i < mf->num_tracks ; ++i )
    mf->tracks[i].current_event = mf->tracks[i].first_event;

  /* search for tempo events in each track; in fact, since the program
     currently supports type 0 and type 1 MIDI files, we should find
     tempo events only in one track */
  DEBUGMSG( "BPM calc: starting calc loop\n" );
  for (;;)
  {
    midievent_t * event = NULL;
    midifile_track_t * event_track = NULL;
    gint i, min_tick = mf->max_tick + 1;

    /* search next event */
    for ( i = 0 ; i < mf->num_tracks ; ++i )
    {
      midifile_track_t * track = &mf->tracks[i];
      midievent_t * e2 = track->current_event;
      if (e2 && e2->tick < min_tick)
      {
        min_tick = e2->tick;
        event = e2;
        event_track = track;
      }
    }

    if (!event)
    {
      /* calculate the remaining length */
      weighted_avg_tempo += (guint)( last_tempo * ((gfloat)( mf->max_tick - last_tick ) / (gfloat)mf->max_tick ) );
      break; /* end of song reached */
    }

    /* advance pointer to next event */
    event_track->current_event = event->next;

    /* check if this is a tempo event */
    if ( event->type == SND_SEQ_EVENT_TEMPO )
    {
      /* check if this is a tempo change (real change, tempo should be
         different) in the midi file (and it shouldn't be at tick 0); */
      if (( is_monotempo ) && ( event->tick > 0 ) && ( event->data.tempo != last_tempo ))
        is_monotempo = FALSE;

      DEBUGMSG( "BPM calc: tempo event (%i) encountered during calc on tick %i\n" ,
                event->data.tempo , event->tick );
      /* add the previous tempo change multiplied for its weight (the tick interval for the tempo )  */
      weighted_avg_tempo += (guint)( last_tempo * ((gfloat)( event->tick - last_tick ) / (gfloat)mf->max_tick ) );
      /* now update last_tick and the microsec_per_tick ratio */
      last_tick = event->tick;
      last_tempo = event->data.tempo;
    }
  }

  DEBUGMSG( "BPM calc: weighted average tempo: %i\n" , weighted_avg_tempo );

  *wavg_bpm = (gint)( 60000000 / weighted_avg_tempo );

  DEBUGMSG( "BPM calc: weighted average bpm: %i\n" , *wavg_bpm );

  if ( is_monotempo )
    *bpm = *wavg_bpm; /* the song has fixed bpm */
  else
    *bpm = -1; /* the song has variable bpm */

  return;
}


/* helper function that parses a midi file; returns 1 on success, 0 otherwise */
gint i_midi_parse_from_filename( gchar * filename , midifile_t * mf )
{
  i_midi_init( mf );
  DEBUGMSG( "PARSE_FROM_FILENAME requested, opening file: %s\n" , filename );
  mf->file_pointer = fopen( filename , "rb" );
  if (!mf->file_pointer)
  {
    g_warning( "Cannot open %s\n" , filename );
    return 0;
  }
  mf->file_name = filename;

  switch( i_midi_file_read_id( mf ) )
  {
    case MAKE_ID('R', 'I', 'F', 'F'):
    {
      DEBUGMSG( "PARSE_FROM_FILENAME requested, RIFF chunk found, processing...\n" );
      /* read riff chunk */
      if ( !i_midi_file_parse_riff( mf ) )
        WARNANDBREAK( "%s: invalid file format (riff parser)\n" , filename );

      /* if that was read correctly, go ahead and read smf data */
    }

    case MAKE_ID('M', 'T', 'h', 'd'):
    {
      DEBUGMSG( "PARSE_FROM_FILENAME requested, MThd chunk found, processing...\n" );
      /* we don't care about port count here, pass 1 */
      if ( !i_midi_file_parse_smf( mf , 1 ) )
        WARNANDBREAK( "%s: invalid file format (smf parser)\n" , filename );

      if ( mf->time_division < 1 )
        WARNANDBREAK( "%s: invalid time division (%i)\n" , filename , mf->time_division );

      /* fill mf->ppq and mf->tempo using time_division */
      if ( !i_midi_setget_tempo( mf ) )
        WARNANDBREAK( "%s: invalid values while setting ppq and tempo\n" , filename );

      /* fill mf->length, keeping in count tempo-changes */
      i_midi_setget_length( mf );

      /* ok, mf has been filled with information; successfully return */
      fclose( mf->file_pointer );
      return 1;
    }

    default:
    {
      g_warning( "%s is not a Standard MIDI File\n" , filename );
      break;
    }
  }

  /* something failed */
  fclose( mf->file_pointer );
  return 0;
}
