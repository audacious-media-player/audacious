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

#include "b-fluidsynth.h"
#include "b-fluidsynth-config.h"

/* sequencer instance */
static sequencer_client_t sc;
/* options */
static amidiplug_cfg_fsyn_t amidiplug_cfg_fsyn;


gint backend_info_get( gchar ** name , gchar ** longname , gchar ** desc , gint * ppos )
{
  if ( name != NULL )
    *name = g_strdup( "fluidsynth" );
  if ( longname != NULL )
    *longname = g_strdup( "FluidSynth Backend " AMIDIPLUG_VERSION );
  if ( desc != NULL )
    *desc = g_strdup( _("This backend produces audio by sending MIDI events "
                        "to FluidSynth, a real-time software synthesizer based "
                        "on the SoundFont2 specification (www.fluidsynth.org).\n"
                        "Produced audio can be manipulated via player effect "
                        "plugins and is processed by chosen ouput plugin.\n"
                        "Backend written by Giacomo Lozito.") );
  if ( ppos != NULL )
    *ppos = 2; /* preferred position in backend list */
  return 1;
}


gint backend_init( void )
{
  /* read configuration options */
  i_cfg_read();

  sc.soundfont_id = -1;
  sc.sample_rate = amidiplug_cfg_fsyn.fsyn_synth_samplerate;
  sc.settings = new_fluid_settings();

  fluid_settings_setnum( sc.settings , "synth.sample-rate" , amidiplug_cfg_fsyn.fsyn_synth_samplerate );
  if ( amidiplug_cfg_fsyn.fsyn_synth_gain != -1 )
    fluid_settings_setnum( sc.settings , "synth.gain" , (gdouble)amidiplug_cfg_fsyn.fsyn_synth_gain / 10 );
  if ( amidiplug_cfg_fsyn.fsyn_synth_poliphony != -1 )
    fluid_settings_setint( sc.settings , "synth.poliphony" , amidiplug_cfg_fsyn.fsyn_synth_poliphony );
  if ( amidiplug_cfg_fsyn.fsyn_synth_reverb == 1 )
    fluid_settings_setstr( sc.settings , "synth.reverb.active" , "yes" );
  else if ( amidiplug_cfg_fsyn.fsyn_synth_reverb == 0 )
    fluid_settings_setstr( sc.settings , "synth.reverb.active" , "no" );
  if ( amidiplug_cfg_fsyn.fsyn_synth_chorus == 1 )
    fluid_settings_setstr( sc.settings , "synth.chorus.active" , "yes" );
  else if ( amidiplug_cfg_fsyn.fsyn_synth_chorus == 0 )
    fluid_settings_setstr( sc.settings , "synth.chorus.active" , "no" );

  sc.synth = new_fluid_synth( sc.settings );

  /* soundfont loader, check if we should load soundfont on backend init */
  if ( amidiplug_cfg_fsyn.fsyn_soundfont_load == 0 )
    i_soundfont_load();

  return 1;
}


gint backend_cleanup( void )
{
  if ( sc.soundfont_id != -1 )
    fluid_synth_sfunload( sc.synth , sc.soundfont_id , 0 );
  delete_fluid_synth( sc.synth );
  delete_fluid_settings( sc.settings );

  i_cfg_free(); /* free configuration options */

  return 1;
}


gint sequencer_get_port_count( void )
{
  /* always return a single port here */
  return 1;
}


gint sequencer_start( gchar * midi_fname )
{
  /* soundfont loader, check if we should load soundfont on first midifile play */
  if (( amidiplug_cfg_fsyn.fsyn_soundfont_load == 1 ) && (sc.soundfont_id == -1 ))
    i_soundfont_load();

  return 1; /* success */
}


gint sequencer_stop( void )
{
  return 1; /* success */
}


/* activate sequencer client */
gint sequencer_on( void )
{
  sc.last_sample_time = 0;
  sc.tick_offset = 0;

  sc.timer_seq = g_timer_new(); /* create the sequencer timer */
  sc.timer_sample = g_timer_new(); /* create the sampler timer */

  return 1; /* success */
}


/* shutdown sequencer client */
gint sequencer_off( void )
{
  if ( sc.timer_seq != NULL )
  {
    g_timer_destroy( sc.timer_seq ); /* destroy the sequencer timer */
    sc.timer_seq = NULL;
  }
  if ( sc.timer_sample != NULL )
  {
    g_timer_destroy( sc.timer_sample ); /* destroy the sampler timer */
    sc.timer_sample = NULL;
  }
  return 1; /* success */
}


/* queue set tempo */
gint sequencer_queue_tempo( gint tempo , gint ppq )
{
  sc.ppq = ppq;
  /* sc.cur_tick_per_sec = (gdouble)( ppq * 1000000 ) / (gdouble)tempo; */
  sc.cur_microsec_per_tick = (gdouble)tempo / (gdouble)ppq;
  return 1;
}


gint sequencer_queue_start( void )
{
  g_timer_start( sc.timer_seq ); /* reset the sequencer timer */
  g_timer_start( sc.timer_sample ); /* reset the sampler timer */
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
  fluid_synth_noteon( sc.synth ,
                      event->data.d[0] ,
                      event->data.d[1] ,
                      event->data.d[2] );
  return 1;
}


gint sequencer_event_noteoff( midievent_t * event )
{
  i_sleep( event->tick_real );
  fluid_synth_noteoff( sc.synth ,
                       event->data.d[0] ,
                       event->data.d[1] );
  return 1;
}


gint sequencer_event_keypress( midievent_t * event )
{
  /* KEY PRESSURE events are not handled by FluidSynth sequencer? */
  DEBUGMSG( "KEYPRESS EVENT with FluidSynth backend (unhandled)\n" );
  i_sleep( event->tick_real );
  return 1;
}


gint sequencer_event_controller( midievent_t * event )
{
  i_sleep( event->tick_real );
  fluid_synth_cc( sc.synth ,
                  event->data.d[0] ,
                  event->data.d[1] ,
                  event->data.d[2] );
  return 1;
}


gint sequencer_event_pgmchange( midievent_t * event )
{
  i_sleep( event->tick_real );
  fluid_synth_program_change( sc.synth ,
                              event->data.d[0] ,
                              event->data.d[1] );
  return 1;
}


gint sequencer_event_chanpress( midievent_t * event )
{
  /* CHANNEL PRESSURE events are not handled by FluidSynth sequencer? */
  DEBUGMSG( "CHANPRESS EVENT with FluidSynth backend (unhandled)\n" );
  i_sleep( event->tick_real );
  return 1;
}


gint sequencer_event_pitchbend( midievent_t * event )
{
  gint pb_value = (((event->data.d[2]) & 0x7f) << 7) | ((event->data.d[1]) & 0x7f);
  i_sleep( event->tick_real );
  fluid_synth_pitch_bend( sc.synth ,
                          event->data.d[0] ,
                          pb_value );
  return 1;
}


gint sequencer_event_sysex( midievent_t * event )
{
  DEBUGMSG( "SYSEX EVENT with FluidSynth backend (unhandled)\n" );
  i_sleep( event->tick_real );
  return 1;
}


gint sequencer_event_tempo( midievent_t * event )
{
  i_sleep( event->tick_real );
  /* sc.cur_tick_per_sec = (gdouble)( sc.ppq * 1000000 ) / (gdouble)event->data.tempo; */
  sc.cur_microsec_per_tick = (gdouble)event->data.tempo / (gdouble)sc.ppq;
  g_timer_start( sc.timer_seq ); /* reset the sequencer timer */
  sc.tick_offset = event->tick_real;
  return 1;
}


gint sequencer_event_other( midievent_t * event )
{
  /* unhandled */
  i_sleep( event->tick_real );
  return 1;
}


gint sequencer_output( gpointer * buffer , gint * len )
{
  gdouble current_time = g_timer_elapsed( sc.timer_sample , NULL );
  if (( current_time > 0.000500 ) && 
      ( (current_time - sc.last_sample_time) * 1000000 >=
        ((gdouble)amidiplug_cfg_fsyn.fsyn_buffer_size * 1000000 / sc.sample_rate) ))
  {
    sc.last_sample_time = g_timer_elapsed( sc.timer_sample , NULL ) - 0.000500;
    /* number of samples to get = buffer size + incremental margin */
    /* g_print( "buf increment: %i\n" , amidiplug_cfg_fsyn.fsyn_buffer_margin +
             ((gint)sc.last_sample_time / amidiplug_cfg_fsyn.fsyn_buffer_increment) ); */
    *len = (amidiplug_cfg_fsyn.fsyn_buffer_size + amidiplug_cfg_fsyn.fsyn_buffer_margin +
           ((gint)sc.last_sample_time / amidiplug_cfg_fsyn.fsyn_buffer_increment) );
    *buffer = g_realloc( *buffer , *len * 4 );
    fluid_synth_write_s16( sc.synth , *len , *buffer , 0 , 2 , *buffer , 1 , 2 );
    *len = *len * 4; /* return the real buffer size in bytes */
    return 1;
  }
  else
  {
    G_USLEEP( 500 );
    return 0;
  }
}


gint sequencer_output_shut( guint max_tick , gint skip_offset )
{
  fluid_synth_system_reset( sc.synth ); /* all notes off and channels reset */
  return 1;
}


/* unimplemented, for autonomous audio == FALSE volume is set by the
   output plugin mixer controls and is not handled by input plugins */
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
  *channels = 2;
  *bitdepth = 16; /* always 16 bit, we use fluid_synth_write_s16() */
  *samplerate = amidiplug_cfg_fsyn.fsyn_synth_samplerate;
  return 1; /* valid information */
}


gboolean audio_check_autonomous( void )
{
  return FALSE; /* FluidSynth gives produced audio back to player */
}



/* ******************************************************************
   *** INTERNALS ****************************************************
   ****************************************************************** */


void i_sleep( guint tick )
{
  gdouble elapsed_tick_usecs = (gdouble)(tick - sc.tick_offset) * sc.cur_microsec_per_tick;
  gdouble elapsed_seq_usecs = g_timer_elapsed( sc.timer_seq , NULL ) * 1000000;
  if ( elapsed_seq_usecs < elapsed_tick_usecs )
  {
    G_USLEEP( elapsed_tick_usecs - elapsed_seq_usecs );
  }
}


void i_soundfont_load( void )
{
  if ( strcmp( amidiplug_cfg_fsyn.fsyn_soundfont_file , "" ) )
  {
    DEBUGMSG( "loading soundfont %s\n" , amidiplug_cfg_fsyn.fsyn_soundfont_file );
    sc.soundfont_id = fluid_synth_sfload( sc.synth , amidiplug_cfg_fsyn.fsyn_soundfont_file , 0 );
    if ( sc.soundfont_id == -1 )
      g_warning( "unable to load SoundFont file %s\n" , amidiplug_cfg_fsyn.fsyn_soundfont_file );
#ifdef DEBUG
    else
      DEBUGMSG( "soundfont %s successfully loaded\n" , amidiplug_cfg_fsyn.fsyn_soundfont_file );
#endif
  }
  else
  {
    g_warning( "FluidSynth backend was selected, but no SoundFont has been specified\n" );
    sc.soundfont_id = -1;
  }
}


gboolean i_bounds_check( gint value , gint min , gint max )
{
  if (( value >= min ) && ( value <= max ))
    return TRUE;
  else
    return FALSE;
}


void i_cfg_read( void )
{
  pcfg_t *cfgfile;
  gchar * config_pathfilename = g_strjoin( "" , g_get_home_dir() , "/" ,
                                           PLAYER_LOCALRCDIR , "/amidi-plug.conf" , NULL );
  cfgfile = i_pcfg_new_from_file( config_pathfilename );

  if ( !cfgfile )
  {
    /* fluidsynth backend defaults */
    amidiplug_cfg_fsyn.fsyn_soundfont_file = g_strdup( "" );
    amidiplug_cfg_fsyn.fsyn_soundfont_load = 1;
    amidiplug_cfg_fsyn.fsyn_synth_samplerate = 44100;
    amidiplug_cfg_fsyn.fsyn_synth_gain = -1;
    amidiplug_cfg_fsyn.fsyn_synth_poliphony = -1;
    amidiplug_cfg_fsyn.fsyn_synth_reverb = -1;
    amidiplug_cfg_fsyn.fsyn_synth_chorus = -1;
    amidiplug_cfg_fsyn.fsyn_buffer_size = 512;
    amidiplug_cfg_fsyn.fsyn_buffer_margin = 10;
    amidiplug_cfg_fsyn.fsyn_buffer_increment = 18;
  }
  else
  {
    i_pcfg_read_string( cfgfile , "fsyn" , "fsyn_soundfont_file" ,
                        &amidiplug_cfg_fsyn.fsyn_soundfont_file , "" );

    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_soundfont_load" ,
                         &amidiplug_cfg_fsyn.fsyn_soundfont_load , 1 );

    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_synth_samplerate" ,
                         &amidiplug_cfg_fsyn.fsyn_synth_samplerate , 44100 );
    if ( !i_bounds_check( amidiplug_cfg_fsyn.fsyn_synth_samplerate , 22050 , 96000 ) )
      amidiplug_cfg_fsyn.fsyn_synth_samplerate = 44100;

    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_synth_gain" ,
                         &amidiplug_cfg_fsyn.fsyn_synth_gain , -1 );
    if (( amidiplug_cfg_fsyn.fsyn_synth_gain != -1 ) &&
        ( !i_bounds_check( amidiplug_cfg_fsyn.fsyn_synth_gain , 0 , 100 ) ))
      amidiplug_cfg_fsyn.fsyn_synth_gain = -1;

    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_synth_poliphony" ,
                         &amidiplug_cfg_fsyn.fsyn_synth_poliphony , -1 );
    if (( amidiplug_cfg_fsyn.fsyn_synth_poliphony != -1 ) &&
        ( !i_bounds_check( amidiplug_cfg_fsyn.fsyn_synth_poliphony , 0 , 100 ) ))
      amidiplug_cfg_fsyn.fsyn_synth_poliphony = -1;

    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_synth_reverb" ,
                         &amidiplug_cfg_fsyn.fsyn_synth_reverb , -1 );

    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_synth_chorus" ,
                         &amidiplug_cfg_fsyn.fsyn_synth_chorus , -1 );

    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_buffer_size" ,
                         &amidiplug_cfg_fsyn.fsyn_buffer_size , 512 );
    if ( !i_bounds_check( amidiplug_cfg_fsyn.fsyn_buffer_size , 100 , 99999 ) )
      amidiplug_cfg_fsyn.fsyn_buffer_size = 512;

    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_buffer_margin" ,
                         &amidiplug_cfg_fsyn.fsyn_buffer_margin , 15 );
    if ( !i_bounds_check( amidiplug_cfg_fsyn.fsyn_buffer_margin , 0 , 100 ) )
      amidiplug_cfg_fsyn.fsyn_buffer_margin = 15;

    i_pcfg_read_integer( cfgfile , "fsyn" , "fsyn_buffer_increment" ,
                         &amidiplug_cfg_fsyn.fsyn_buffer_increment , 18 );
    if ( !i_bounds_check( amidiplug_cfg_fsyn.fsyn_buffer_increment , 6 , 1000 ) )
      amidiplug_cfg_fsyn.fsyn_buffer_increment = 18;

    i_pcfg_free( cfgfile );
  }

  g_free( config_pathfilename );
}


void i_cfg_free( void )
{
  g_free( amidiplug_cfg_fsyn.fsyn_soundfont_file );
}
