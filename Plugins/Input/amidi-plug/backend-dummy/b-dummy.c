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

#include "b-dummy.h"
#include "b-dummy-config.h"

/* dummy sequencer instance */
static sequencer_client_t sc;
/* options */
static amidiplug_cfg_dumm_t amidiplug_cfg_dumm;


gint backend_info_get( gchar ** name , gchar ** longname , gchar ** desc , gint * ppos )
{
  if ( name != NULL )
    *name = g_strdup( "dummy" );
  if ( longname != NULL )
    *longname = g_strdup( "Dummy Backend " AMIDIPLUG_VERSION );
  if ( desc != NULL )
    *desc = g_strdup( _("This backend does not produce audio at all. It is mostly "
                        "useful for analysis and testing purposes, as it can log "
                        "all MIDI events to standard output, standard error or file.\n"
                        "Backend written by Giacomo Lozito.") );
  if ( ppos != NULL )
    *ppos = 3; /* preferred position in backend list */
  return 1;
}


gint backend_init( void )
{
  i_cfg_read(); /* read configuration options */

  return 1;
}


gint backend_cleanup( void )
{
  i_cfg_free(); /* free configuration options */

  return 1;
}


gint sequencer_get_port_count( void )
{
  return 1; /* always return a single port here */
}


gint sequencer_start( gchar * midi_fname )
{
  switch( amidiplug_cfg_dumm.dumm_logger_enable )
  {
    case 1:
    {
      sc.file = stdout; /* log to standard output */
      break;
    }
    case 2:
    {
      sc.file = stderr; /* log to standard error */
      break;
    }
    case 3:
    {
      switch ( amidiplug_cfg_dumm.dumm_logger_lfstyle )
      {
        case 0:
        {
          sc.file = fopen( amidiplug_cfg_dumm.dumm_logger_logfile , "w" );
          break;
        }
        case 1:
        {
          sc.file = fopen( amidiplug_cfg_dumm.dumm_logger_logfile , "a" );
          break;
        }
        case 2:
        {
          gchar *midi_basefname = G_PATH_GET_BASENAME( midi_fname );
          gchar *logfile = g_strjoin( "" , amidiplug_cfg_dumm.dumm_logger_logdir ,
                                      "/" , midi_basefname , ".log" , NULL );
          sc.file = fopen( logfile , "w" );
          g_free( logfile );
          g_free( midi_basefname );
          break;
        }
        default: /* shouldn't happen, let's handle this anyway */
        {
          sc.file = NULL;
          break;
        }
      }
      break;
    }
    case 0:
    default:
    {
      sc.file = NULL;
      break;
    }
  }

  if (( sc.file == NULL ) && ( amidiplug_cfg_dumm.dumm_logger_enable != 0 ))
  {
    DEBUGMSG( "Unable to get a FILE pointer\n" );
    return 0;
  }
  else
    return 1; /* success */
}


gint sequencer_stop( void )
{
  if (( sc.file != NULL ) && ( amidiplug_cfg_dumm.dumm_logger_enable == 3 ))
    fclose( sc.file );

  return 1; /* success */
}


/* activate sequencer client */
gint sequencer_on( void )
{
  sc.tick_offset = 0;
  if ( amidiplug_cfg_dumm.dumm_playback_speed == 0 )
    sc.timer_seq = g_timer_new(); /* create the sequencer timer */
  return 1; /* success */
}


/* shutdown sequencer client */
gint sequencer_off( void )
{
  if (( amidiplug_cfg_dumm.dumm_playback_speed == 0 ) && ( sc.timer_seq != NULL ))
  {
    g_timer_destroy( sc.timer_seq ); /* destroy the sequencer timer */
    sc.timer_seq = NULL;
  }
  return 1; /* success */
}


/* queue set tempo */
gint sequencer_queue_tempo( gint tempo , gint ppq )
{
  sc.ppq = ppq;
  sc.usec_per_tick = (gdouble)tempo / (gdouble)ppq;
  return 1;
}


gint sequencer_queue_start( void )
{
  if ( amidiplug_cfg_dumm.dumm_playback_speed == 0 )
    g_timer_start( sc.timer_seq ); /* reset the sequencer timer */
  return 1;
}


gint sequencer_event_init( void )
{
  /* common settings for all our events */
  return 1;
}


gint sequencer_event_noteon( midievent_t * event )
{
  i_sleep( event->tick_real );
  i_printf( sc.file , "NOTEON : ti %i : ch %i : no %i : ve %i\n" ,
            event->tick , event->data.d[0] , event->data.d[1] , event->data.d[2] );
  return 1;
}


gint sequencer_event_noteoff( midievent_t * event )
{
  i_sleep( event->tick_real );
  i_printf( sc.file , "NOTEOFF : ti %i : ch %i : no %i : ve %i\n" ,
            event->tick , event->data.d[0] , event->data.d[1] , event->data.d[2] );
  return 1;
}


gint sequencer_event_keypress( midievent_t * event )
{
  i_sleep( event->tick_real );
  i_printf( sc.file , "KEYPRESS : ti %i : ch %i : no %i : ve %i\n" ,
            event->tick , event->data.d[0] , event->data.d[1] , event->data.d[2] );
  return 1;
}


gint sequencer_event_controller( midievent_t * event )
{
  i_sleep( event->tick_real );
  i_printf( sc.file , "CONTROLLER : ti %i : ch %i : pa %i : va %i\n" ,
            event->tick , event->data.d[0] , event->data.d[1] , event->data.d[2] );
  return 1;
}


gint sequencer_event_pgmchange( midievent_t * event )
{
  i_sleep( event->tick_real );
  i_printf( sc.file , "PGMCHANGE : ti %i : ch %i : va %i\n" ,
            event->tick , event->data.d[0] , event->data.d[1] );
  return 1;
}


gint sequencer_event_chanpress( midievent_t * event )
{
  i_sleep( event->tick_real );
  i_printf( sc.file , "CHANPRESS : ti %i : ch %i : va %i\n" ,
            event->tick , event->data.d[0] , event->data.d[1] );
  return 1;
}


gint sequencer_event_pitchbend( midievent_t * event )
{
  i_sleep( event->tick_real );
  i_printf( sc.file , "PITCHBEND : ti %i : ch %i : va %i\n" ,
            event->tick ,  event->data.d[0] ,
            ((((event->data.d[2]) & 0x7f) << 7) | ((event->data.d[1]) & 0x7f)) );
  return 1;
}


gint sequencer_event_sysex( midievent_t * event )
{
  i_sleep( event->tick_real );
  i_printf( sc.file , "SYSEX : ti %i\n" ,
            event->tick );
  return 1;
}


gint sequencer_event_tempo( midievent_t * event )
{
  i_sleep( event->tick_real );
  i_printf( sc.file , "TEMPOCHANGE : ti %i : va %i\n" ,
            event->tick , event->data.tempo );
  sc.usec_per_tick = (gdouble)event->data.tempo / (gdouble)sc.ppq;
  if ( amidiplug_cfg_dumm.dumm_playback_speed == 0 )
    g_timer_start( sc.timer_seq ); /* reset the sequencer timer */
  sc.tick_offset = event->tick_real;
  return 1;
}


gint sequencer_event_other( midievent_t * event )
{
  return 1;
}


gint sequencer_output( gpointer * buffer , gint * len )
{
  return 0;
}


gint sequencer_output_shut( guint max_tick , gint skip_offset )
{
  return 1;
}


/* unimplemented (useless for dummy backend) */
gint audio_volume_get( gint * left_volume , gint * right_volume )
{
  return 0; 
}
gint audio_volume_set( gint left_volume , gint right_volume )
{
  return 0;
}


gint audio_info_get( gint * channels , gint * bitdepth , gint * samplerate )
{
  /* not applicable for dummy backend */
  *channels = -1;
  *bitdepth = -1;
  *samplerate = -1;
  return 0; /* not valid information */
}


gboolean audio_check_autonomous( void )
{
  return TRUE; /* Dummy deals itself with audio (well, it doesn't produce any at all :)) */
}



/* ******************************************************************
   *** INTERNALS ****************************************************
   ****************************************************************** */


void i_sleep( guint tick )
{
  if ( amidiplug_cfg_dumm.dumm_playback_speed == 0 )
  {
    gdouble elapsed_tick_usecs = (gdouble)(tick - sc.tick_offset) * sc.usec_per_tick;
    gdouble elapsed_seq_usecs = g_timer_elapsed( sc.timer_seq , NULL ) * 1000000;
    if ( elapsed_seq_usecs < elapsed_tick_usecs )
    {
      G_USLEEP( elapsed_tick_usecs - elapsed_seq_usecs );
    }
  }
}


void i_printf( FILE * fp , const gchar * format , ... )
{
  va_list args;
  va_start( args , format );
  if ( fp != NULL )
    G_VFPRINTF( fp , format , args );
  va_end( args );
}


void i_cfg_read( void )
{
  pcfg_t *cfgfile;
  gchar * def_logfile = g_strjoin( "" , g_get_home_dir() , "/amidi-plug.log" , NULL );
  gchar * def_logdir = (gchar*)g_get_home_dir();
  gchar * config_pathfilename = g_strjoin( "" , g_get_home_dir() , "/" ,
                                           PLAYER_LOCALRCDIR , "/amidi-plug.conf" , NULL );
  cfgfile = i_pcfg_new_from_file( config_pathfilename );

  if ( !cfgfile )
  {
    /* fluidsynth backend defaults */
    amidiplug_cfg_dumm.dumm_logger_enable = 0;
    amidiplug_cfg_dumm.dumm_logger_lfstyle = 0;
    amidiplug_cfg_dumm.dumm_playback_speed = 0;
    amidiplug_cfg_dumm.dumm_logger_logfile = g_strdup( def_logfile );
    amidiplug_cfg_dumm.dumm_logger_logdir = g_strdup( def_logdir );
  }
  else
  {
    i_pcfg_read_integer( cfgfile , "dumm" , "dumm_logger_enable" ,
                         &amidiplug_cfg_dumm.dumm_logger_enable , 0 );
    i_pcfg_read_integer( cfgfile , "dumm" , "dumm_logger_lfstyle" ,
                         &amidiplug_cfg_dumm.dumm_logger_lfstyle , 0 );
    i_pcfg_read_integer( cfgfile , "dumm" , "dumm_playback_speed" ,
                         &amidiplug_cfg_dumm.dumm_playback_speed , 0 );
    i_pcfg_read_string( cfgfile , "dumm" , "dumm_logger_logfile" ,
                        &amidiplug_cfg_dumm.dumm_logger_logfile , def_logfile );
    i_pcfg_read_string( cfgfile , "dumm" , "dumm_logger_logdir" ,
                        &amidiplug_cfg_dumm.dumm_logger_logdir , def_logdir );

    i_pcfg_free( cfgfile );
  }

  g_free( config_pathfilename );
  g_free( def_logfile );
}


void i_cfg_free( void )
{
  g_free( amidiplug_cfg_dumm.dumm_logger_logfile );
  g_free( amidiplug_cfg_dumm.dumm_logger_logdir );
}
