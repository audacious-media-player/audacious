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

#include "amidi-plug.h"


InputPlugin *get_iplugin_info(void)
{
  amidiplug_ip.description = g_strdup_printf(_("AMIDI-Plug %s (MIDI Player)"), AMIDIPLUG_VERSION);
  return &amidiplug_ip;
}


static gint amidiplug_is_our_file( gchar * filename )
{
#ifdef MIDIFILE_PROBE_MAGICBYTES
    VFSFile * fp;
    gchar magic_bytes[4];

    fp = vfs_fopen( filename , "rb" );

    if (fp == NULL)
	return FALSE;

    vfs_fread( magic_bytes , 1 , 4 , fp );

    if ( !strncmp( magic_bytes , "MThd" , 4 ) )
    {
      vfs_fclose( fp );
      DEBUGMSG( "MIDI found, %s is a standard midi file\n" , filename );
      return TRUE;
    }

    if ( !strncmp( magic_bytes , "RIFF" , 4 ) )
    {
      /* skip the four bytes after RIFF,
         then read the next four */
      vfs_fseek( fp , 4 , SEEK_CUR );
      vfs_fread( magic_bytes , 1 , 4 , fp );
      if ( !strncmp( magic_bytes , "RMID" , 4 ) )
      {
        vfs_fclose( fp );
        DEBUGMSG( "MIDI found, %s is a riff midi file\n" , filename );
        return TRUE;
      }
    }
    vfs_fclose( fp );
#else
    gchar * ext = strrchr( filename, '.' );
    /* check the filename extension */
    if ( ( ext ) &&
         (( !strcasecmp(ext,".mid") ) || ( !strcasecmp(ext,".midi") ) ||
          ( !strcasecmp(ext,".rmi") ) || ( !strcasecmp(ext,".rmid") )) )
      return TRUE;
#endif
  return FALSE;
}


static void amidiplug_init( void )
{
  g_log_set_handler(NULL , G_LOG_LEVEL_WARNING , g_log_default_handler , NULL);
  DEBUGMSG( "init, read configuration\n" );
  /* read configuration */
  i_configure_cfg_read();
  amidiplug_playing_status = AMIDIPLUG_STOP;
}


static void amidiplug_cleanup( void )
{
  g_free( amidiplug_ip.description );
}


static void amidiplug_configure( void )
{
  if ( amidiplug_gui_prefs.config_win )
  {
    return;
  }
  else
  {
    GSList * wports = NULL;
    GSList * scards = NULL;
    /* get an updated list of writable ALSA MIDI ports and ALSA-enabled sound cards*/
    DEBUGMSG( "get an updated list of writable ALSA MIDI ports\n" );
    wports = i_seq_port_get_list();
    DEBUGMSG( "get an updated list of ALSA-enabled sound cards\n" );
    scards = i_seq_card_get_list();
    /* display them in our nice config dialog */
    DEBUGMSG( "opening config window\n" );
    i_configure_gui( wports , scards );
    /* free the cards list and the ports list */
    i_seq_card_free_list( scards );
    i_seq_port_free_list( wports );
  }
}


static void amidiplug_aboutbox( void )
{
  i_about_gui();
}


static void amidiplug_file_info_box( gchar * filename )
{
  i_fileinfo_gui( filename );
}


static void amidiplug_stop( void )
{
  DEBUGMSG( "STOP request at tick: %i\n" , midifile.playing_tick );
  pthread_mutex_lock( &amidiplug_playing_mutex );
  if (( amidiplug_playing_status == AMIDIPLUG_PLAY ) ||
      ( amidiplug_playing_status == AMIDIPLUG_STOP ))
  {
    amidiplug_playing_status = AMIDIPLUG_STOP;
    pthread_mutex_unlock( &amidiplug_playing_mutex );
    pthread_join( amidiplug_play_thread , NULL );
    DEBUGMSG( "STOP activated (play thread joined)\n" );
  }
  else if ( amidiplug_playing_status == AMIDIPLUG_PAUSE )
  {
    amidiplug_playing_status = AMIDIPLUG_STOP;
    DEBUGMSG( "STOP activated (from PAUSE to STOP)\n" );
    pthread_mutex_unlock( &amidiplug_playing_mutex );
  }
  else /* AMIDIPLUG_ERR */
  {
    DEBUGMSG( "STOP activated (in error handling, ok)\n" );
    pthread_mutex_unlock( &amidiplug_playing_mutex );
  }
  /* kill the sequencer (while it has been already killed if we come from
     pause, it's safe to do anyway since it checks for multiple calls */
  i_seq_off();
  /* free midi data (same as above) */
  i_midi_free( &midifile );
}


static void amidiplug_pause( gshort paused )
{
  if ( paused )
  {
    DEBUGMSG( "PAUSE request at tick: %i\n" , midifile.playing_tick );
    pthread_mutex_lock( &amidiplug_playing_mutex );
    /* this cond is used to avoid race conditions */
    while ( amidiplug_playing_status != AMIDIPLUG_PLAY )
      pthread_cond_wait( &amidiplug_playing_cond , &amidiplug_playing_mutex );
    amidiplug_playing_status = AMIDIPLUG_PAUSE;
    pthread_mutex_unlock( &amidiplug_playing_mutex );

    pthread_join( amidiplug_play_thread , NULL );
    DEBUGMSG( "PAUSE activated (play thread joined)\n" , midifile.playing_tick );

    /* kill the sequencer */
    i_seq_off();
  }
  else
  {
    DEBUGMSG( "PAUSE deactivated, returning to tick %i\n" , midifile.playing_tick );
    /* revive the sequencer */
    i_seq_on( 0 , NULL );
    /* re-set initial tempo */
    i_midi_setget_tempo( &midifile );
    i_seq_queue_set_tempo( midifile.current_tempo , midifile.ppq );
    /* get back to the previous state */
    amidiplug_skipto( midifile.playing_tick );

    pthread_mutex_lock( &amidiplug_playing_mutex );
    /* play play play! */
    DEBUGMSG( "PAUSE deactivated, starting play thread again\n" );
    pthread_create(&amidiplug_play_thread, NULL, amidiplug_play_loop, NULL);
    /* this cond is used to avoid race conditions */
    while ( amidiplug_playing_status != AMIDIPLUG_PLAY )
      pthread_cond_wait( &amidiplug_playing_cond , &amidiplug_playing_mutex );
    pthread_mutex_unlock( &amidiplug_playing_mutex );
  }
}


static void amidiplug_seek( gint time )
{
  DEBUGMSG( "SEEK requested (time %i), pausing song...\n" , time );
  pthread_mutex_lock( &amidiplug_playing_mutex );
  /* this cond is used to avoid race conditions */
  while ( amidiplug_playing_status != AMIDIPLUG_PLAY )
      pthread_cond_wait( &amidiplug_playing_cond , &amidiplug_playing_mutex );
  amidiplug_playing_status = AMIDIPLUG_PAUSE;
  pthread_mutex_unlock( &amidiplug_playing_mutex );

  pthread_join( amidiplug_play_thread , NULL );
  DEBUGMSG( "SEEK requested (time %i), song paused\n" , time );
  /* kill the sequencer */
  i_seq_off();
  /* revive the sequencer */
  i_seq_on( 0 , NULL );
  /* re-set initial tempo */
  i_midi_setget_tempo( &midifile );
  i_seq_queue_set_tempo( midifile.current_tempo , midifile.ppq );
  /* get back to the previous state */
  DEBUGMSG( "SEEK requested (time %i), moving to tick %i of %i\n" ,
            time , (gint)((time * 1000000) / midifile.avg_microsec_per_tick) , midifile.max_tick );
  midifile.playing_tick = (gint)((time * 1000000) / midifile.avg_microsec_per_tick);
  amidiplug_skipto( midifile.playing_tick );
  /* play play play! */
  DEBUGMSG( "SEEK done, starting play thread again\n" );
  pthread_create(&amidiplug_play_thread, NULL, amidiplug_play_loop, NULL);
}


static gint amidiplug_get_time( void )
{
  gint pt;
  pthread_mutex_lock( &amidiplug_playing_mutex );
  if (( amidiplug_playing_status == AMIDIPLUG_PLAY ) ||
      ( amidiplug_playing_status == AMIDIPLUG_PAUSE ))
  {
    pthread_mutex_unlock( &amidiplug_playing_mutex );
    pthread_mutex_lock(&amidiplug_gettime_mutex);
    pt = midifile.playing_tick;
    pthread_mutex_unlock(&amidiplug_gettime_mutex);
    return (gint)((pt * midifile.avg_microsec_per_tick) / 1000);
  }
  else if ( amidiplug_playing_status == AMIDIPLUG_STOP )
  {
    pthread_mutex_unlock( &amidiplug_playing_mutex );
    DEBUGMSG( "GETTIME on stopped song, returning -1\n" , time );
    return -1;
  }
  else /* AMIDIPLUG_ERR */
  {
    pthread_mutex_unlock( &amidiplug_playing_mutex );
    DEBUGMSG( "GETTIME on halted song (an error occurred?), returning -1 and stopping the player\n" , time );
    xmms_remote_stop(0);
    return -1;
  }
}


static void amidiplug_get_volume( gint * l , gint * r )
{
  gchar mixer_card[10];
  snprintf( mixer_card , 8 , "hw:%i" , amidiplug_cfg.mixer_card_id );
  mixer_card[9] = '\0';
  /* get volume */
  i_seq_mixer_get_volume( l , r , mixer_card , amidiplug_cfg.mixer_control_name ,
                          amidiplug_cfg.mixer_control_id );
}


static void amidiplug_set_volume( gint  l , gint  r )
{
  gchar mixer_card[10];
  snprintf( mixer_card , 8 , "hw:%i" , amidiplug_cfg.mixer_card_id );
  mixer_card[9] = '\0';
  /* set volume */
  i_seq_mixer_set_volume( l , r , mixer_card , amidiplug_cfg.mixer_control_name ,
                          amidiplug_cfg.mixer_control_id );
}


static void amidiplug_get_song_info( gchar * filename , gchar ** title , gint * length )
{
  /* song title, get it from the filename */
  *title = G_PATH_GET_BASENAME(filename);

  /* sure, it's possible to calculate the length of a MIDI file anytime,
     but the file must be entirely parsed to calculate it; this could lead
     to performance loss, for now it's safer to calculate the length only
     right before playing the file */
  *length = -1;
  return;
}


static void amidiplug_play( gchar * filename )
{
  gint port_count = 0;
  DEBUGMSG( "PLAY requested, midifile init\n" );
  /* midifile init */
  i_midi_init( &midifile );

  /* get the number of selected alsa ports */
  port_count = i_util_str_count( amidiplug_cfg.seq_writable_ports , ':' );
  if ( port_count < 1 )
  {
    g_warning( "No ALSA ports selected\n" );
    amidiplug_playing_status = AMIDIPLUG_ERR;
    return;
  }

  DEBUGMSG( "PLAY requested, opening file: %s\n" , filename );
  midifile.file_pointer = fopen( filename , "rb" );
  if (!midifile.file_pointer)
  {
    g_warning( "Cannot open %s\n" , filename );
    amidiplug_playing_status = AMIDIPLUG_ERR;
    return;
  }
  midifile.file_name = filename;

  switch( i_midi_file_read_id( &midifile ) )
  {
    case MAKE_ID('R', 'I', 'F', 'F'):
    {
      DEBUGMSG( "PLAY requested, RIFF chunk found, processing...\n" );
      /* read riff chunk */
      if ( !i_midi_file_parse_riff( &midifile ) )
        WARNANDBREAKANDPLAYERR( "%s: invalid file format (riff parser)\n" , filename );

      /* if that was read correctly, go ahead and read smf data */
    }

    case MAKE_ID('M', 'T', 'h', 'd'):
    {
      DEBUGMSG( "PLAY requested, MThd chunk found, processing...\n" );
      if ( !i_midi_file_parse_smf( &midifile , port_count ) )
        WARNANDBREAKANDPLAYERR( "%s: invalid file format (smf parser)\n" , filename );

      if ( midifile.time_division < 1 )
        WARNANDBREAKANDPLAYERR( "%s: invalid time division (%i)\n" , filename , midifile.time_division );

      DEBUGMSG( "PLAY requested, setting ppq and tempo...\n" );
      /* fill midifile.ppq and midifile.tempo using time_division */
      if ( !i_midi_setget_tempo( &midifile ) )
        WARNANDBREAKANDPLAYERR( "%s: invalid values while setting ppq and tempo\n" , filename );

      DEBUGMSG( "PLAY requested, sequencer init\n" );
      /* sequencer on */
      if ( !i_seq_on( 1 , amidiplug_cfg.seq_writable_ports ) )
        WARNANDBREAKANDPLAYERR( "%s: ALSA problem, play aborted\n" , filename );

      DEBUGMSG( "PLAY requested, setting sequencer queue tempo...\n" );
      /* set sequencer queue tempo using ppq and tempo (call only after i_midi_setget_tempo) */
      if ( !i_seq_queue_set_tempo( midifile.current_tempo , midifile.ppq ) )
      {
        i_seq_off(); /* kill the sequencer */
        WARNANDBREAKANDPLAYERR( "%s: ALSA queue problem, play aborted\n" , filename );
      }

      /* fill midifile.length, keeping in count tempo-changes */
      i_midi_setget_length( &midifile );
      DEBUGMSG( "PLAY requested, song length calculated: %i msec\n" , (gint)(midifile.length / 1000) );

      /* our length is in microseconds, but the player wants milliseconds */
      amidiplug_ip.set_info( G_PATH_GET_BASENAME(filename) , (gint)(midifile.length / 1000) , -1 , -1 , -1 );

      /* play play play! */
      DEBUGMSG( "PLAY requested, starting play thread\n" );
      amidiplug_playing_status = AMIDIPLUG_PLAY;
      pthread_create(&amidiplug_play_thread, NULL, amidiplug_play_loop, NULL);
      break;
    }

    default:
    {
      amidiplug_playing_status = AMIDIPLUG_ERR;
      g_warning( "%s is not a Standard MIDI File\n" , filename );
      break;
    }
  }

  fclose( midifile.file_pointer );
  return;
}



void * amidiplug_play_loop( void * arg )
{
  snd_seq_event_t ev;
  gint i, p, c;
  gboolean rewind = FALSE;

  pthread_mutex_lock( &amidiplug_playing_mutex );
  if ( amidiplug_playing_status != AMIDIPLUG_PAUSE )
  {
    DEBUGMSG( "PLAY thread, rewind tracks to their first event\n" );
    rewind = TRUE;
  }
  else
  {
    DEBUGMSG( "PLAY thread, do not rewind tracks to their first event (coming from a PAUSE status)\n" );
    amidiplug_playing_status = AMIDIPLUG_PLAY;
    pthread_cond_signal( &amidiplug_playing_cond );
  }
  pthread_mutex_unlock( &amidiplug_playing_mutex );

  if ( rewind )
  {
    /* initialize current position in each track */
    for (i = 0; i < midifile.num_tracks; ++i)
      midifile.tracks[i].current_event = midifile.tracks[i].first_event;
    snd_seq_start_queue(sc.seq, sc.queue, NULL);
  }

  /* common settings for all our events */
  snd_seq_ev_clear(&ev);
  ev.queue = sc.queue;
  ev.source.port = 0;
  ev.flags = SND_SEQ_TIME_STAMP_TICK;

  DEBUGMSG( "PLAY thread, start the play loop\n" );
  for (;;)
  {
    midievent_t * event = NULL;
    midifile_track_t * event_track = NULL;
    gint i, min_tick = midifile.max_tick + 1;

    /* search next event */
    for (i = 0; i < midifile.num_tracks; ++i)
    {
      midifile_track_t * track = &midifile.tracks[i];
      midievent_t * e2 = track->current_event;
      if (( e2 ) && ( e2->tick < min_tick ))
      {
        min_tick = e2->tick;
        event = e2;
        event_track = track;
      }
    }

    /* check if the song has been stopped */
    pthread_mutex_lock( &amidiplug_playing_mutex );
    if ( amidiplug_playing_status != AMIDIPLUG_PLAY )
    {
      DEBUGMSG( "PLAY thread, PAUSE or STOP requested, exiting from play loop\n" );
      event = NULL;
    }
    pthread_mutex_unlock( &amidiplug_playing_mutex );

    if (!event)
      break; /* end of song reached */

    /* advance pointer to next event */
    event_track->current_event = event->next;

    /* output the event */
    ev.type = event->type;
    ev.time.tick = event->tick - midifile.skip_offset;
    ev.dest = sc.dest_port[event->port];

    switch (ev.type)
    {
      case SND_SEQ_EVENT_NOTEON:
      case SND_SEQ_EVENT_NOTEOFF:
      case SND_SEQ_EVENT_KEYPRESS:
      {
        snd_seq_ev_set_fixed(&ev);
        ev.data.note.channel = event->data.d[0];
        ev.data.note.note = event->data.d[1];
        ev.data.note.velocity = event->data.d[2];
        break;
      }
      case SND_SEQ_EVENT_CONTROLLER:
      {
        snd_seq_ev_set_fixed(&ev);
        ev.data.control.channel = event->data.d[0];
        ev.data.control.param = event->data.d[1];
        ev.data.control.value = event->data.d[2];
        break;
      }
      case SND_SEQ_EVENT_PGMCHANGE:
      case SND_SEQ_EVENT_CHANPRESS:
      {
        snd_seq_ev_set_fixed(&ev);
        ev.data.control.channel = event->data.d[0];
        ev.data.control.value = event->data.d[1];
        break;
      }
      case SND_SEQ_EVENT_PITCHBEND:
      {
        snd_seq_ev_set_fixed(&ev);
        ev.data.control.channel = event->data.d[0];
        ev.data.control.value = ((event->data.d[1]) | ((event->data.d[2]) << 7)) - 0x2000;
        break;
      }
      case SND_SEQ_EVENT_SYSEX:
      {
        snd_seq_ev_set_variable(&ev, event->data.length, event->sysex);
        break;
      }
      case SND_SEQ_EVENT_TEMPO:
      {
        snd_seq_ev_set_fixed(&ev);
        ev.dest.client = SND_SEQ_CLIENT_SYSTEM;
        ev.dest.port = SND_SEQ_PORT_SYSTEM_TIMER;
        ev.data.queue.queue = sc.queue;
        ev.data.queue.param.value = event->data.tempo;
        DEBUGMSG( "PLAY thread, processing tempo event with value %i on tick %i\n" ,
                  event->data.tempo , event->tick );
        pthread_mutex_lock(&amidiplug_gettime_mutex);
        midifile.current_tempo = event->data.tempo;
        pthread_mutex_unlock(&amidiplug_gettime_mutex);
        break;
      }
      default:
      {
        DEBUGMSG( "PLAY thread, encountered invalid event type %i\n" , ev.type );
        break;
      }
    }

    pthread_mutex_lock(&amidiplug_gettime_mutex);
    midifile.playing_tick = event->tick;
    pthread_mutex_unlock(&amidiplug_gettime_mutex);

    snd_seq_event_output(sc.seq, &ev);
    snd_seq_drain_output(sc.seq);
    snd_seq_sync_output_queue(sc.seq);
  }

  /* time to shutdown playback! */
  /* send "ALL SOUNDS OFF" to all channels on all ports */
  ev.type = SND_SEQ_EVENT_CONTROLLER;
  ev.time.tick = 0;
  snd_seq_ev_set_fixed(&ev);
  ev.data.control.param = MIDI_CTL_ALL_SOUNDS_OFF;
  ev.data.control.value = 0;
  for ( p = 0 ; p < sc.dest_port_num ; p++)
  {
    ev.queue = sc.queue;
    ev.dest = sc.dest_port[p];

    for ( c = 0 ; c < 16 ; c++ )
    {
      ev.data.control.channel = c;
      snd_seq_event_output(sc.seq, &ev);
      snd_seq_drain_output(sc.seq);
    }
  }

  /* schedule queue stop at end of song */
  snd_seq_ev_clear(&ev);
  ev.queue = sc.queue;
  ev.source.port = 0;
  ev.flags = SND_SEQ_TIME_STAMP_TICK;

  snd_seq_ev_set_fixed(&ev);
  ev.type = SND_SEQ_EVENT_STOP;
  ev.time.tick = midifile.max_tick - midifile.skip_offset;
  ev.dest.client = SND_SEQ_CLIENT_SYSTEM;
  ev.dest.port = SND_SEQ_PORT_SYSTEM_TIMER;
  ev.data.queue.queue = sc.queue;
  snd_seq_event_output(sc.seq, &ev);
  snd_seq_drain_output(sc.seq);
  /* snd_seq_sync_output_queue(sc.seq); */

  pthread_mutex_lock( &amidiplug_playing_mutex );
  if ( amidiplug_playing_status != AMIDIPLUG_PAUSE )
  {
    amidiplug_playing_status = AMIDIPLUG_STOP;
    DEBUGMSG( "PLAY thread, song stopped/ended\n" );
    
  }
  pthread_mutex_unlock( &amidiplug_playing_mutex );

  pthread_exit(NULL);
}


/* amidigplug_skipto: re-do all events that influence the playing of our
   midi file; re-do them using a time-tick of 0, so they are processed
   istantaneously and proceed this way until the playing_tick is reached;
   also obtain the correct skip_offset from the playing_tick */
void amidiplug_skipto( gint playing_tick )
{
  snd_seq_event_t ev;
  gint i;

  /* this check is always made, for safety*/
  if ( playing_tick >= midifile.max_tick )
    playing_tick = midifile.max_tick - 1;

  /* initialize current position in each track */
  for (i = 0; i < midifile.num_tracks; ++i)
    midifile.tracks[i].current_event = midifile.tracks[i].first_event;

  /* common settings for all our events */
  snd_seq_ev_clear(&ev);
  ev.queue = sc.queue;
  ev.source.port = 0;
  ev.flags = SND_SEQ_TIME_STAMP_TICK;

  snd_seq_start_queue(sc.seq, sc.queue, NULL);

  DEBUGMSG( "SKIPTO request, starting skipto loop\n" );
  for (;;)
  {
    midievent_t * event = NULL;
    midifile_track_t * event_track = NULL;
    gint i, min_tick = midifile.max_tick + 1;

    /* search next event */
    for (i = 0; i < midifile.num_tracks; ++i)
    {
      midifile_track_t * track = &midifile.tracks[i];
      midievent_t * e2 = track->current_event;
      if (( e2 ) && ( e2->tick < min_tick ))
      {
        min_tick = e2->tick;
        event = e2;
        event_track = track;
      }
    }

    /* unlikely here... unless very strange MIDI files are played :) */
    if (!event)
    {
      DEBUGMSG( "SKIPTO request, reached the last event but not the requested tick (!)\n" );
      break; /* end of song reached */
    }

    /* reached the requested tick, job done */
    if ( event->tick >= playing_tick )
    {
      DEBUGMSG( "SKIPTO request, reached the requested tick, exiting from skipto loop\n" );
      break;
    }

    /* advance pointer to next event */
    event_track->current_event = event->next;

    /* output the event */
    ev.type = event->type;
    ev.time.tick = event->tick;
    ev.dest = sc.dest_port[event->port];

    switch (ev.type)
    {
      /* do nothing for these
      case SND_SEQ_EVENT_NOTEON:
      case SND_SEQ_EVENT_NOTEOFF:
      case SND_SEQ_EVENT_KEYPRESS:
      {
        break;
      } */
      case SND_SEQ_EVENT_CONTROLLER:
      {
        snd_seq_ev_set_fixed(&ev);
        ev.data.control.channel = event->data.d[0];
        ev.data.control.param = event->data.d[1];
        ev.data.control.value = event->data.d[2];
        ev.time.tick = 0;
        snd_seq_event_output(sc.seq, &ev);
        snd_seq_drain_output(sc.seq);
        snd_seq_sync_output_queue(sc.seq);
        break;
      }
      case SND_SEQ_EVENT_PGMCHANGE:
      case SND_SEQ_EVENT_CHANPRESS:
      {
        snd_seq_ev_set_fixed(&ev);
        ev.data.control.channel = event->data.d[0];
        ev.data.control.value = event->data.d[1];
        ev.time.tick = 0;
        snd_seq_event_output(sc.seq, &ev);
        snd_seq_drain_output(sc.seq);
        snd_seq_sync_output_queue(sc.seq);
        break;
      }
      case SND_SEQ_EVENT_PITCHBEND:
      {
        snd_seq_ev_set_fixed(&ev);
        ev.data.control.channel = event->data.d[0];
        ev.data.control.value = ((event->data.d[1]) | ((event->data.d[2]) << 7)) - 0x2000;
        ev.time.tick = 0;
        snd_seq_event_output(sc.seq, &ev);
        snd_seq_drain_output(sc.seq);
        snd_seq_sync_output_queue(sc.seq);
        break;
      }
      case SND_SEQ_EVENT_SYSEX:
      {
        snd_seq_ev_set_variable(&ev, event->data.length, event->sysex);
        ev.time.tick = 0;
        snd_seq_event_output(sc.seq, &ev);
        snd_seq_drain_output(sc.seq);
        snd_seq_sync_output_queue(sc.seq);
        break;
      }
      case SND_SEQ_EVENT_TEMPO:
      {
        snd_seq_ev_set_fixed(&ev);
        ev.dest.client = SND_SEQ_CLIENT_SYSTEM;
        ev.dest.port = SND_SEQ_PORT_SYSTEM_TIMER;
        ev.data.queue.queue = sc.queue;
        ev.data.queue.param.value = event->data.tempo;
        pthread_mutex_lock(&amidiplug_gettime_mutex);
        midifile.current_tempo = event->data.tempo;
        pthread_mutex_unlock(&amidiplug_gettime_mutex);
        ev.time.tick = 0;
        snd_seq_event_output(sc.seq, &ev);
        snd_seq_drain_output(sc.seq);
        snd_seq_sync_output_queue(sc.seq);
        break;
      }
    }
  }

  midifile.skip_offset = playing_tick;

  return;
}
