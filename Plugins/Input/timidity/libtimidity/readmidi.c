/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timidity.h"
#include "timidity_internal.h"
#include "options.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"

/* Computes how many (fractional) samples one MIDI delta-time unit contains */
static void compute_sample_increment(MidSong *song, sint32 tempo,
				     sint32 divisions)
{
  double a;
  a = (double) (tempo) * (double) (song->rate) * (65536.0/1000000.0) /
    (double)(divisions);

  song->sample_correction = (sint32)(a) & 0xFFFF;
  song->sample_increment = (sint32)(a) >> 16;

  DEBUG_MSG("Samples per delta-t: %d (correction %d)\n",
	  song->sample_increment, song->sample_correction);
}

/* Read variable-length number (7 bits per byte, MSB first) */
static sint32 getvl(MidIStream *stream)
{
  sint32 l=0;
  uint8 c;
  for (;;)
    {
      mid_istream_read(stream, &c, 1, 1);
      l += (c & 0x7f);
      if (!(c & 0x80)) return l;
      l<<=7;
    }
}

/* Print a string from the file, followed by a newline. Any non-ASCII
   or unprintable characters will be converted to periods. */
static int read_meta_data(MidIStream *stream, sint32 len, uint8 type, MidSong *song)
{
  char *s=safe_malloc(len+1);
  MidSongMetaId id;
  static char *label[] = {
    "Text event: ", "Text: ", "Copyright: ", "Track name: ",
    "Instrument: ", "Lyric: ", "Marker: ", "Cue point: "};

  if (len != (sint32) mid_istream_read(stream, s, 1, len))
    {
      free(s);
      return -1;
    }
  s[len]='\0';
  while (len--)
    {
      if (((unsigned char)s[len])<32)
	s[len]='.';
    }
  DEBUG_MSG("%s%s\n", label[(type > 7) ? 0 : type], s);

  switch (type)
  {
    case 1: id = MID_SONG_TEXT; break;
    case 2: id = MID_SONG_COPYRIGHT; break;
    default: free(s); s = NULL;
  }
  if (s)
    {
      if (song->meta_data[id])
	free(song->meta_data[id]);
      song->meta_data[id] = s;
    }
	
  return 0;
}

#define MIDIEVENT(at,t,ch,pa,pb) \
  new=safe_malloc(sizeof(MidEventList)); \
  new->event.time=at; new->event.type=t; new->event.channel=ch; \
  new->event.a=pa; new->event.b=pb; new->next=0;\
  return new;

#define MAGIC_EOT ((MidEventList *)(-1))

/* Read a MIDI event, returning a freshly allocated element that can
   be linked to the event list */
static MidEventList *read_midi_event(MidIStream *stream, MidSong *song)
{
  static uint8 laststatus, lastchan;
  static uint8 nrpn=0, rpn_msb[16], rpn_lsb[16]; /* one per channel */
  uint8 me, type, a,b,c;
  sint32 len;
  MidEventList *new;

  for (;;)
    {
      song->at += getvl(stream);
      if (mid_istream_read(stream, &me, 1, 1) != 1)
	{
	  DEBUG_MSG("read_midi_event: mid_istream_read() failure\n");
	  return 0;
	}
      
      if(me==0xF0 || me == 0xF7) /* SysEx event */
	{
	  len=getvl(stream);
	  mid_istream_skip(stream, len);
	}
      else if(me==0xFF) /* Meta event */
	{
	  mid_istream_read(stream, &type, 1, 1);
	  len=getvl(stream);
	  if (type>0 && type<16)
	    {
	      read_meta_data(stream, len, type, song);
	    }
	  else
	    switch(type)
	      {
	      case 0x2F: /* End of Track */
		return MAGIC_EOT;

	      case 0x51: /* Tempo */
		mid_istream_read(stream, &a, 1, 1);
		mid_istream_read(stream, &b, 1, 1);
		mid_istream_read(stream, &c, 1, 1);
		MIDIEVENT(song->at, ME_TEMPO, c, a, b);
		
	      default:
		DEBUG_MSG("(Meta event type 0x%02x, length %d)\n", type, len);
		mid_istream_skip(stream, len);
		break;
	      }
	}
      else
	{
	  a=me;
	  if (a & 0x80) /* status byte */
	    {
	      lastchan=a & 0x0F;
	      laststatus=(a>>4) & 0x07;
	      mid_istream_read(stream, &a, 1, 1);
	      a &= 0x7F;
	    }
	  switch(laststatus)
	    {
	    case 0: /* Note off */
	      mid_istream_read(stream, &b, 1, 1);
	      b &= 0x7F;
	      MIDIEVENT(song->at, ME_NOTEOFF, lastchan, a,b);

	    case 1: /* Note on */
	      mid_istream_read(stream, &b, 1, 1);
	      b &= 0x7F;
	      MIDIEVENT(song->at, ME_NOTEON, lastchan, a,b);

	    case 2: /* Key Pressure */
	      mid_istream_read(stream, &b, 1, 1);
	      b &= 0x7F;
	      MIDIEVENT(song->at, ME_KEYPRESSURE, lastchan, a, b);

	    case 3: /* Control change */
	      mid_istream_read(stream, &b, 1, 1);
	      b &= 0x7F;
	      {
		int control=255;
		switch(a)
		  {
		  case 7: control=ME_MAINVOLUME; break;
		  case 10: control=ME_PAN; break;
		  case 11: control=ME_EXPRESSION; break;
		  case 64: control=ME_SUSTAIN; break;
		  case 120: control=ME_ALL_SOUNDS_OFF; break;
		  case 121: control=ME_RESET_CONTROLLERS; break;
		  case 123: control=ME_ALL_NOTES_OFF; break;

		    /* These should be the SCC-1 tone bank switch
		       commands. I don't know why there are two, or
		       why the latter only allows switching to bank 0.
		       Also, some MIDI files use 0 as some sort of
		       continuous controller. This will cause lots of
		       warnings about undefined tone banks. */
		  case 0: control=ME_TONE_BANK; break;
		  case 32: 
		    if (b!=0)
		      DEBUG_MSG("(Strange: tone bank change 0x20%02x)\n", b);
		    else
		      control=ME_TONE_BANK;
		    break;

		  case 100: nrpn=0; rpn_msb[lastchan]=b; break;
		  case 101: nrpn=0; rpn_lsb[lastchan]=b; break;
		  case 99: nrpn=1; rpn_msb[lastchan]=b; break;
		  case 98: nrpn=1; rpn_lsb[lastchan]=b; break;
		    
		  case 6:
		    if (nrpn)
		      {
			DEBUG_MSG("(Data entry (MSB) for NRPN %02x,%02x: %d)\n",
				rpn_msb[lastchan], rpn_lsb[lastchan], b);
			break;
		      }
		    
		    switch((rpn_msb[lastchan]<<8) | rpn_lsb[lastchan])
		      {
		      case 0x0000: /* Pitch bend sensitivity */
			control=ME_PITCH_SENS;
			break;

		      case 0x7F7F: /* RPN reset */
			/* reset pitch bend sensitivity to 2 */
			MIDIEVENT(song->at, ME_PITCH_SENS, lastchan, 2, 0);

		      default:
			DEBUG_MSG("(Data entry (MSB) for RPN %02x,%02x: %d)\n",
				rpn_msb[lastchan], rpn_lsb[lastchan], b);
			break;
		      }
		    break;
		    
		  default:
		    DEBUG_MSG("(Control %d: %d)\n", a, b);
		    break;
		  }
		if (control != 255)
		  { 
		    MIDIEVENT(song->at, control, lastchan, b, 0); 
		  }
	      }
	      break;

	    case 4: /* Program change */
	      a &= 0x7f;
	      MIDIEVENT(song->at, ME_PROGRAM, lastchan, a, 0);

	    case 5: /* Channel pressure - NOT IMPLEMENTED */
	      break;

	    case 6: /* Pitch wheel */
	      mid_istream_read(stream, &b, 1, 1);
	      b &= 0x7F;
	      MIDIEVENT(song->at, ME_PITCHWHEEL, lastchan, a, b);

	    default: 
	      DEBUG_MSG("*** Can't happen: status 0x%02X, channel 0x%02X\n",
		      laststatus, lastchan);
	      break;
	    }
	}
    }
  
  return new;
}

#undef MIDIEVENT

/* Read a midi track into the linked list, either merging with any previous
   tracks or appending to them. */
static int read_track(MidIStream *stream, MidSong *song, int append)
{
  MidEventList *meep;
  MidEventList *next, *new;
  sint32 len;
  char tmp[4];

  meep = song->evlist;
  if (append && meep)
    {
      /* find the last event in the list */
      for (; meep->next; meep=meep->next)
	;
      song->at = meep->event.time;
    }
  else
    song->at=0;

  /* Check the formalities */
  
  if (mid_istream_read(stream, tmp, 1, 4) != 4 || mid_istream_read(stream, &len, 4, 1) != 1)
    {
      DEBUG_MSG("Can't read track header.\n");
      return -1;
    }
  len=SWAPBE32(len);
  if (memcmp(tmp, "MTrk", 4))
    {
      DEBUG_MSG("Corrupt MIDI file.\n");
      return -2;
    }

  for (;;)
    {
      if (!(new=read_midi_event(stream, song))) /* Some kind of error  */
	return -2;

      if (new==MAGIC_EOT) /* End-of-track Hack. */
	{
	  return 0;
	}

      next=meep->next;
      while (next && (next->event.time < new->event.time))
	{
	  meep=next;
	  next=meep->next;
	}
	  
      new->next=next;
      meep->next=new;

      song->event_count++; /* Count the event. (About one?) */
      meep=new;
    }
}

/* Free the linked event list from memory. */
static void free_midi_list(MidSong *song)
{
  MidEventList *meep, *next;
  if (!(meep = song->evlist)) return;
  while (meep)
    {
      next=meep->next;
      free(meep);
      meep=next;
    }
  song->evlist=0;
}

/* Allocate an array of MidiEvents and fill it from the linked list of
   events, marking used instruments for loading. Convert event times to
   samples: handle tempo changes. Strip unnecessary events from the list.
   Free the linked list. */
static MidEvent *groom_list(MidSong *song, sint32 divisions,sint32 *eventsp,
			     sint32 *samplesp)
{
  MidEvent *groomed_list, *lp;
  MidEventList *meep;
  sint32 i, our_event_count, tempo, skip_this_event, new_value;
  sint32 sample_cum, samples_to_do, at, st, dt, counting_time;

  int current_bank[16], current_set[16], current_program[16]; 
  /* Or should each bank have its own current program? */

  for (i=0; i<16; i++)
    {
      current_bank[i]=0;
      current_set[i]=0;
      current_program[i]=song->default_program;
    }

  tempo=500000;
  compute_sample_increment(song, tempo, divisions);

  /* This may allocate a bit more than we need */
  groomed_list=lp=safe_malloc(sizeof(MidEvent) * (song->event_count+1));
  meep=song->evlist;

  our_event_count=0;
  st=at=sample_cum=0;
  counting_time=2; /* We strip any silence before the first NOTE ON. */

  for (i = 0; i < song->event_count; i++)
    {
      skip_this_event=0;

      if (meep->event.type==ME_TEMPO)
	{
	  tempo=
	    meep->event.channel + meep->event.b * 256 + meep->event.a * 65536;
	  compute_sample_increment(song, tempo, divisions);
	  skip_this_event=1;
	}
      else switch (meep->event.type)
	{
	case ME_PROGRAM:
	  if (ISDRUMCHANNEL(song, meep->event.channel))
	    {
	      if (song->drumset[meep->event.a]) /* Is this a defined drumset? */
		new_value=meep->event.a;
	      else
		{
		  DEBUG_MSG("Drum set %d is undefined\n", meep->event.a);
		  new_value=meep->event.a=0;
		}
	      if (current_set[meep->event.channel] != new_value)
		current_set[meep->event.channel]=new_value;
	      else 
		skip_this_event=1;
	    }
	  else
	    {
	      new_value=meep->event.a;
	      if ((current_program[meep->event.channel] != SPECIAL_PROGRAM)
		  && (current_program[meep->event.channel] != new_value))
		current_program[meep->event.channel] = new_value;
	      else
		skip_this_event=1;
	    }
	  break;

	case ME_NOTEON:
	  if (counting_time)
	    counting_time=1;
	  if (ISDRUMCHANNEL(song, meep->event.channel))
	    {
	      /* Mark this instrument to be loaded */
	      if (!(song->drumset[current_set[meep->event.channel]]
		    ->instrument[meep->event.a]))
		song->drumset[current_set[meep->event.channel]]
		  ->instrument[meep->event.a] = MAGIC_LOAD_INSTRUMENT;
	    }
	  else
	    {
	      if (current_program[meep->event.channel]==SPECIAL_PROGRAM)
		break;
	      /* Mark this instrument to be loaded */
	      if (!(song->tonebank[current_bank[meep->event.channel]]
		    ->instrument[current_program[meep->event.channel]]))
		song->tonebank[current_bank[meep->event.channel]]
		  ->instrument[current_program[meep->event.channel]] =
		    MAGIC_LOAD_INSTRUMENT;
	    }
	  break;

	case ME_TONE_BANK:
	  if (ISDRUMCHANNEL(song, meep->event.channel))
	    {
	      skip_this_event=1;
	      break;
	    }
	  if (song->tonebank[meep->event.a]) /* Is this a defined tone bank? */
	    new_value=meep->event.a;
	  else 
	    {
	      DEBUG_MSG("Tone bank %d is undefined\n", meep->event.a);
	      new_value=meep->event.a=0;
	    }
	  if (current_bank[meep->event.channel]!=new_value)
	    current_bank[meep->event.channel]=new_value;
	  else
	    skip_this_event=1;
	  break;
	}

      /* Recompute time in samples*/
      if ((dt=meep->event.time - at) && !counting_time)
	{
	  samples_to_do = song->sample_increment * dt;
	  sample_cum += song->sample_correction * dt;
	  if (sample_cum & 0xFFFF0000)
	    {
	      samples_to_do += ((sample_cum >> 16) & 0xFFFF);
	      sample_cum &= 0x0000FFFF;
	    }
	  st += samples_to_do;
	}
      else if (counting_time==1) counting_time=0;
      if (!skip_this_event)
	{
	  /* Add the event to the list */
	  *lp=meep->event;
	  lp->time=st;
	  lp++;
	  our_event_count++;
	}
      at=meep->event.time;
      meep=meep->next;
    }
  /* Add an End-of-Track event */
  lp->time=st;
  lp->type=ME_EOT;
  our_event_count++;
  free_midi_list(song);

  *eventsp=our_event_count;
  *samplesp=st;
  return groomed_list;
}

MidEvent *read_midi_file(MidIStream *stream, MidSong *song, sint32 *count, sint32 *sp)
{
  sint32 len, divisions;
  sint16 format, tracks, divisions_tmp;
  int i;
  char tmp[4];

  song->event_count=0;
  song->at=0;
  song->evlist=0;

  if (mid_istream_read(stream, tmp, 1, 4) != 4 || mid_istream_read(stream, &len, 4, 1) != 1)
    {
      DEBUG_MSG("Not a MIDI file!\n");
      return 0;
    }
  len=SWAPBE32(len);
  if (memcmp(tmp, "MThd", 4) || len < 6)
    {
      DEBUG_MSG("Not a MIDI file!\n");
      return 0;
    }

  mid_istream_read(stream, &format, 2, 1);
  mid_istream_read(stream, &tracks, 2, 1);
  mid_istream_read(stream, &divisions_tmp, 2, 1);
  format=SWAPBE16(format);
  tracks=SWAPBE16(tracks);
  divisions_tmp=SWAPBE16(divisions_tmp);

  if (divisions_tmp<0)
    {
      /* SMPTE time -- totally untested. Got a MIDI file that uses this? */
      divisions=
	(sint32)(-(divisions_tmp/256)) * (sint32)(divisions_tmp & 0xFF);
    }
  else divisions=(sint32)(divisions_tmp);

  if (len > 6)
    {
      DEBUG_MSG("MIDI file header size %u bytes", len);
      mid_istream_skip(stream, len-6); /* skip the excess */
    }
  if (format<0 || format >2)
    {
      DEBUG_MSG("Unknown MIDI file format %d\n", format);
      return 0;
    }
  DEBUG_MSG("Format: %d  Tracks: %d  Divisions: %d\n",
	  format, tracks, divisions);

  /* Put a do-nothing event first in the list for easier processing */
  song->evlist=safe_malloc(sizeof(MidEventList));
  song->evlist->event.time=0;
  song->evlist->event.type=ME_NONE;
  song->evlist->next=0;
  song->event_count++;

  switch(format)
    {
    case 0:
      if (read_track(stream, song, 0))
	{
	  free_midi_list(song);
	  return 0;
	}
      break;

    case 1:
      for (i=0; i<tracks; i++)
	if (read_track(stream, song, 0))
	  {
	    free_midi_list(song);
	    return 0;
	  }
      break;

    case 2: /* We simply play the tracks sequentially */
      for (i=0; i<tracks; i++)
	if (read_track(stream, song, 1))
	  {
	    free_midi_list(song);
	    return 0;
	  }
      break;
    }
  return groom_list(song, divisions, count, sp);
}
