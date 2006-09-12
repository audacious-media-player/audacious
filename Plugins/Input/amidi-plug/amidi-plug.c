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

#include "amidi-plug.h"


InputPlugin *get_iplugin_info(void)
{
  amidiplug_ip.description = g_strdup_printf(N_("AMIDI-Plug %s (MIDI Player)"), AMIDIPLUG_VERSION);
  return &amidiplug_ip;
}


static gint amidiplug_is_our_file( gchar * filename )
{
#if defined(MIDIFILE_PROBE_MAGICBYTES)
    FILE * fp;
    gchar magic_bytes[4];

    fp = fopen( filename , "rb" );

    if ( fp == NULL )
      return FALSE;

    fread( magic_bytes , 1 , 4 , fp );

    if ( !strncmp( magic_bytes , "MThd" , 4 ) )
    {
      fclose( fp );
      DEBUGMSG( "MIDI found, %s is a standard midi file\n" , filename );
      return TRUE;
    }

    if ( !strncmp( magic_bytes , "RIFF" , 4 ) )
    {
      /* skip the four bytes after RIFF,
         then read the next four */
      fseek( fp , 4 , SEEK_CUR );
      fread( magic_bytes , 1 , 4 , fp );
      if ( !strncmp( magic_bytes , "RMID" , 4 ) )
      {
        fclose( fp );
        DEBUGMSG( "MIDI found, %s is a riff midi file\n" , filename );
        return TRUE;
      }
    }
    fclose( fp );
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
  /* read configuration for amidi-plug */
  i_configure_cfg_ap_read();
  amidiplug_playing_status = AMIDIPLUG_STOP;
  backend.gmodule = NULL;
  /* load the backend selected by user */
  i_backend_load( amidiplug_cfg_ap.ap_seq_backend );
}


static void amidiplug_cleanup( void )
{
  i_backend_unload(); /* unload currently loaded backend */
  g_free( amidiplug_ip.description );
}


static void amidiplug_configure( void )
{
  /* display the nice config dialog */
  DEBUGMSG( "opening config system\n" );
  i_configure_gui();
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
    if ( backend.autonomous_audio == FALSE )
      pthread_join( amidiplug_audio_thread , NULL );
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

  /* kill the sequencer (while it may have been already killed if coming
     from pause, it's safe to do anyway since it checks for multiple calls) */
  if ( backend.gmodule != NULL )
    backend.seq_off();

  /* call seq_stop */
  if ( backend.gmodule != NULL )
    backend.seq_stop();

  /* close audio if current backend works with output plugin */
  if (( backend.gmodule != NULL ) && ( backend.autonomous_audio == FALSE ))
  {
    DEBUGMSG( "STOP activated, closing audio output plugin\n" );
    amidiplug_ip.output->buffer_free();
    amidiplug_ip.output->buffer_free();
    amidiplug_ip.output->close_audio();
  }
  /* free midi data (if it has not been freed yet) */
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
    if ( backend.autonomous_audio == FALSE )
      pthread_join( amidiplug_audio_thread , NULL );
    DEBUGMSG( "PAUSE activated (play thread joined)\n" , midifile.playing_tick );

    if ( backend.autonomous_audio == FALSE )
      amidiplug_ip.output->pause(paused);

    /* kill the sequencer */
    backend.seq_off();
  }
  else
  {
    DEBUGMSG( "PAUSE deactivated, returning to tick %i\n" , midifile.playing_tick );
    /* revive the sequencer */
    backend.seq_on();
    /* re-set initial tempo */
    i_midi_setget_tempo( &midifile );
    backend.seq_queue_tempo( midifile.current_tempo , midifile.ppq );
    /* get back to the previous state */
    amidiplug_skipto( midifile.playing_tick );

    if ( backend.autonomous_audio == FALSE )
      amidiplug_ip.output->pause(paused);

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
  if ( backend.autonomous_audio == FALSE )
    pthread_join( amidiplug_audio_thread , NULL );
  DEBUGMSG( "SEEK requested (time %i), song paused\n" , time );
  /* kill the sequencer */
  backend.seq_off();
  /* revive the sequencer */
  backend.seq_on();
  /* re-set initial tempo */
  i_midi_setget_tempo( &midifile );
  backend.seq_queue_tempo( midifile.current_tempo , midifile.ppq );
  /* get back to the previous state */
  DEBUGMSG( "SEEK requested (time %i), moving to tick %i of %i\n" ,
            time , (gint)((time * 1000000) / midifile.avg_microsec_per_tick) , midifile.max_tick );
  midifile.playing_tick = (gint)((time * 1000000) / midifile.avg_microsec_per_tick);
  amidiplug_skipto( midifile.playing_tick );

  if ( backend.autonomous_audio == FALSE )
    amidiplug_ip.output->flush(time * 1000);

  /* play play play! */
  DEBUGMSG( "SEEK done, starting play thread again\n" );
  pthread_create(&amidiplug_play_thread, NULL, amidiplug_play_loop, NULL);
}


static gint amidiplug_get_time( void )
{
  if ( backend.autonomous_audio == FALSE )
  {
    pthread_mutex_lock( &amidiplug_playing_mutex );
    if (( amidiplug_playing_status == AMIDIPLUG_PLAY ) ||
        ( amidiplug_playing_status == AMIDIPLUG_PAUSE ) ||
        (( amidiplug_playing_status == AMIDIPLUG_STOP ) && ( amidiplug_ip.output->buffer_playing() )))
    {
      pthread_mutex_unlock( &amidiplug_playing_mutex );
      return amidiplug_ip.output->output_time();
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
      DEBUGMSG( "GETTIME on halted song (an error occurred?), returning -1 and stopping the player\n" );
      xmms_remote_stop(0);
      return -1;
    }
  }
  else
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
}


static void amidiplug_get_volume( gint * l_p , gint * r_p )
{
  if ( backend.autonomous_audio == TRUE )
    backend.audio_volume_get( l_p , r_p );
  else
    amidiplug_ip.output->get_volume( l_p , r_p );
  return;
}


static void amidiplug_set_volume( gint  l , gint  r )
{
  if ( backend.autonomous_audio == TRUE )
    backend.audio_volume_set( l , r );
  else
    amidiplug_ip.output->set_volume( l , r );
  return;
}


static void amidiplug_get_song_info( gchar * filename , gchar ** title , gint * length )
{
  /* song title, get it from the filename */
  *title = G_PATH_GET_BASENAME(filename);

  /* sure, it's possible to calculate the length of a MIDI file anytime,
     but the file must be entirely parsed to calculate it; this could
     lead to a bit of performance loss, so let the user decide here */
  if ( amidiplug_cfg_ap.ap_opts_length_precalc )
  {
    /* let's calculate the midi length, using this nice helper function that
       will return 0 if a problem occurs and the length can't be calculated */
    midifile_t mf;

    if ( i_midi_parse_from_filename( filename , &mf ) )
      *length = (gint)(mf.length / 1000);
    else
      *length = -1;

    i_midi_free( &mf );
  }
  else  
    *length = -1;

  return;
}


static void amidiplug_play( gchar * filename )
{
  gint port_count = 0;
  gint au_samplerate = -1, au_bitdepth = -1, au_channels = -1;

  if ( backend.gmodule == NULL )
  {
    g_warning( "No sequencer backend selected\n" );
    i_message_gui( _("AMIDI-Plug - warning") ,
                   _("No sequencer backend has been selected!\nPlease configure AMIDI-Plug before playing.") ,
                   AMIDIPLUG_MESSAGE_WARN , NULL , TRUE );
    amidiplug_playing_status = AMIDIPLUG_ERR;
    return;
  }

  /* get information about audio from backend, if available */
  backend.audio_info_get( &au_channels , &au_bitdepth , &au_samplerate );
  DEBUGMSG( "PLAY requested, audio details: channels -> %i , bitdepth -> %i , samplerate -> %i\n" ,
            au_channels , au_bitdepth , au_samplerate );

  if ( backend.autonomous_audio == FALSE )
  {
    DEBUGMSG( "PLAY requested, opening audio output plugin\n" );
    amidiplug_ip.output->open_audio( FMT_S16_NE , au_samplerate , au_channels );
  }

  DEBUGMSG( "PLAY requested, midifile init\n" );
  /* midifile init */
  i_midi_init( &midifile );

  /* get the number of selected ports */
  port_count = backend.seq_get_port_count();
  if ( port_count < 1 )
  {
    g_warning( "No ports selected\n" );
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

      DEBUGMSG( "PLAY requested, sequencer start\n" );
      /* sequencer start */
      if ( !backend.seq_start( filename ) )
        WARNANDBREAKANDPLAYERR( "%s: problem with seq_start, play aborted\n" , filename );

      DEBUGMSG( "PLAY requested, sequencer on\n" );
      /* sequencer on */
      if ( !backend.seq_on() )
        WARNANDBREAKANDPLAYERR( "%s: problem with seq_on, play aborted\n" , filename );

      DEBUGMSG( "PLAY requested, setting sequencer queue tempo...\n" );
      /* set sequencer queue tempo using ppq and tempo (call only after i_midi_setget_tempo) */
      if ( !backend.seq_queue_tempo( midifile.current_tempo , midifile.ppq ) )
      {
        backend.seq_off(); /* kill the sequencer */
        WARNANDBREAKANDPLAYERR( "%s: ALSA queue problem, play aborted\n" , filename );
      }

      /* fill midifile.length, keeping in count tempo-changes */
      i_midi_setget_length( &midifile );
      DEBUGMSG( "PLAY requested, song length calculated: %i msec\n" , (gint)(midifile.length / 1000) );

      /* our length is in microseconds, but the player wants milliseconds */
      amidiplug_ip.set_info( G_PATH_GET_BASENAME(filename) ,
                             (gint)(midifile.length / 1000) ,
                             au_bitdepth * au_samplerate * au_channels / 8 ,
                             au_samplerate , au_channels );

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
  gint i = 0;
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
    backend.seq_queue_start();
  }

  if ( backend.autonomous_audio == FALSE )
  {
    pthread_create(&amidiplug_audio_thread, NULL, amidiplug_audio_loop, NULL);
  }

  /* common settings for all our events */
  backend.seq_event_init();

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
    /* consider the midifile.skip_offset */
    event->tick_real = event->tick - midifile.skip_offset;


    switch (event->type)
    {
      case SND_SEQ_EVENT_NOTEON:
        backend.seq_event_noteon( event );
        break;
      case SND_SEQ_EVENT_NOTEOFF:
        backend.seq_event_noteoff( event );
        break;
      case SND_SEQ_EVENT_KEYPRESS:
        backend.seq_event_keypress( event );
        break;
      case SND_SEQ_EVENT_CONTROLLER:
        backend.seq_event_controller( event );
        break;
      case SND_SEQ_EVENT_PGMCHANGE:
        backend.seq_event_pgmchange( event );
        break;
      case SND_SEQ_EVENT_CHANPRESS:
        backend.seq_event_chanpress( event );
        break;
      case SND_SEQ_EVENT_PITCHBEND:
        backend.seq_event_pitchbend( event );
        break;
      case SND_SEQ_EVENT_SYSEX:
        backend.seq_event_sysex( event );
        break;
      case SND_SEQ_EVENT_TEMPO:
        backend.seq_event_tempo( event );
        DEBUGMSG( "PLAY thread, processing tempo event with value %i on tick %i\n" ,
                  event->data.tempo , event->tick );
        pthread_mutex_lock(&amidiplug_gettime_mutex);
        midifile.current_tempo = event->data.tempo;
        pthread_mutex_unlock(&amidiplug_gettime_mutex);
        break;
      case SND_SEQ_EVENT_META_TEXT:
        /* do nothing */
        break;
      case SND_SEQ_EVENT_META_LYRIC:
        /* do nothing */
        break;
      default:
        DEBUGMSG( "PLAY thread, encountered invalid event type %i\n" , event->type );
        break;
    }

    pthread_mutex_lock(&amidiplug_gettime_mutex);
    midifile.playing_tick = event->tick;
    pthread_mutex_unlock(&amidiplug_gettime_mutex);

    if ( backend.autonomous_audio == TRUE )
    {
      /* these backends deal with audio production themselves (i.e. ALSA) */
      backend.seq_output( NULL , NULL );
    }
  }

  backend.seq_output_shut( midifile.max_tick , midifile.skip_offset );

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
  gint i;

  /* this check is always made, for safety*/
  if ( playing_tick >= midifile.max_tick )
    playing_tick = midifile.max_tick - 1;

  /* initialize current position in each track */
  for (i = 0; i < midifile.num_tracks; ++i)
    midifile.tracks[i].current_event = midifile.tracks[i].first_event;

  /* common settings for all our events */
  backend.seq_event_init();
  backend.seq_queue_start();

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
    /* set the time tick to 0 */
    event->tick_real = 0;

    switch (event->type)
    {
      /* do nothing for these
      case SND_SEQ_EVENT_NOTEON:
      case SND_SEQ_EVENT_NOTEOFF:
      case SND_SEQ_EVENT_KEYPRESS:
      {
        break;
      } */
      case SND_SEQ_EVENT_CONTROLLER:
        backend.seq_event_controller( event );
        break;
      case SND_SEQ_EVENT_PGMCHANGE:
        backend.seq_event_pgmchange( event );
        break;
      case SND_SEQ_EVENT_CHANPRESS:
        backend.seq_event_chanpress( event );
        break;
      case SND_SEQ_EVENT_PITCHBEND:
        backend.seq_event_pitchbend( event );
        break;
      case SND_SEQ_EVENT_SYSEX:
        backend.seq_event_sysex( event );
        break;
      case SND_SEQ_EVENT_TEMPO:
        backend.seq_event_tempo( event );
        pthread_mutex_lock(&amidiplug_gettime_mutex);
        midifile.current_tempo = event->data.tempo;
        pthread_mutex_unlock(&amidiplug_gettime_mutex);
        break;
    }

    if ( backend.autonomous_audio == TRUE )
    {
      /* these backends deal with audio production themselves (i.e. ALSA) */
      backend.seq_output( NULL , NULL );
    }
  }

  midifile.skip_offset = playing_tick;

  return;
}


void * amidiplug_audio_loop( void * arg )
{
  gboolean going = 1;
  gpointer buffer = NULL;
  gint buffer_size = 0;
  while ( going )
  {
    if ( backend.seq_output( &buffer , &buffer_size ) )
    {
      while( ( amidiplug_ip.output->buffer_free() < buffer_size ) && ( going == TRUE ) )
        G_USLEEP(10000);
      produce_audio( amidiplug_ip.output->written_time() ,
                     FMT_S16_NE , 2 , buffer_size , buffer , &going );
    }
    pthread_mutex_lock( &amidiplug_playing_mutex );
    if ( amidiplug_playing_status != AMIDIPLUG_PLAY )
      going = FALSE;
    pthread_mutex_unlock( &amidiplug_playing_mutex );
  }
  if ( buffer != NULL )
    g_free( buffer );
  pthread_exit(NULL);
}
