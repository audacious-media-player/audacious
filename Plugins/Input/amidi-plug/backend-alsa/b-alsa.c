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

#include "b-alsa.h"
#include "b-alsa-config.h"

/* sequencer instance */
static sequencer_client_t sc;
/* options */
static amidiplug_cfg_alsa_t amidiplug_cfg_alsa;


gint backend_info_get( gchar ** name , gchar ** longname , gchar ** desc , gint * ppos )
{
  if ( name != NULL )
    *name = g_strdup( "alsa" );
  if ( longname != NULL )
    *longname = g_strdup( "ALSA Backend " AMIDIPLUG_VERSION );
  if ( desc != NULL )
    *desc = g_strdup( _("This backend sends MIDI events to a group of user-chosen "
                        "ALSA sequencer ports. The ALSA sequencer interface is very "
                        "versatile, it can provide ports for audio cards hardware "
                        "synthesizers (i.e. emu10k1) but also for software synths, "
                        "external devices, etc.\n"
                        "This backend does not produce audio, MIDI events are handled "
                        "directly from devices/programs behind the ALSA ports; in example, "
                        "MIDI events sent to the hardware synth will be directly played.\n"
                        "Backend written by Giacomo Lozito.") );
  if ( ppos != NULL )
    *ppos = 1; /* preferred position in backend list */
  return 1;
}


gint backend_init( void )
{
  /* read configuration options */
  i_cfg_read();

  sc.seq = NULL;
  sc.client_port = 0;
  sc.queue = 0;
  sc.dest_port = NULL;
  sc.dest_port_num = 0;
  sc.queue_tempo = NULL;
  sc.is_start = FALSE;

  return 1;
}

gint backend_cleanup( void )
{
  /* free configuration options */
  i_cfg_free();

  return 1;
}


gint sequencer_get_port_count( void )
{
  return i_util_str_count( amidiplug_cfg_alsa.alsa_seq_wports , ':' );
}


gint sequencer_start( gchar * midi_fname )
{
  sc.is_start = TRUE;
  return 1; /* success */
}


gint sequencer_stop( void )
{
  return 1; /* success */
}


/* activate sequencer client */
gint sequencer_on( void )
{
  gchar * wports_str = amidiplug_cfg_alsa.alsa_seq_wports;

  if ( !i_seq_open() )
  {
    sc.seq = NULL;
    return 0;
  }

  if ( !i_seq_port_create() )
  {
    i_seq_close();
    sc.seq = NULL;
    return 0;
  }

  if ( !i_seq_queue_create() )
  {
    i_seq_close();
    sc.seq = NULL;
    return 0;
  }

  if (( sc.is_start == TRUE ) && ( wports_str ))
  {
    sc.is_start = FALSE;
    i_seq_port_wparse( wports_str );
  }

  if ( !i_seq_port_connect() )
  {
    i_seq_queue_free();
    i_seq_close();
    sc.seq = NULL;
    return 0;
  }

  /* success */
  return 1;
}


/* shutdown sequencer client */
gint sequencer_off( void )
{
  if ( sc.seq )
  {
    i_seq_port_disconnect();
    i_seq_queue_free();
    i_seq_close();
    sc.seq = NULL;
    /* return 1 here */
    return 1;
  }
  /* return 2 if it was already freed */
  return 2;
}


/* queue set tempo */
gint sequencer_queue_tempo( gint tempo , gint ppq )
{
  /* interpret and set tempo */
  snd_seq_queue_tempo_alloca( &sc.queue_tempo );
  snd_seq_queue_tempo_set_tempo( sc.queue_tempo , tempo );
  snd_seq_queue_tempo_set_ppq( sc.queue_tempo , ppq );

  if ( snd_seq_set_queue_tempo( sc.seq , sc.queue , sc.queue_tempo ) < 0 )
  {
    g_warning( "Cannot set queue tempo (%u/%i)\n",
               snd_seq_queue_tempo_get_tempo(sc.queue_tempo),
               snd_seq_queue_tempo_get_ppq(sc.queue_tempo) );
    return 0;
  }
  return 1;
}


gint sequencer_queue_start( void )
{
  return snd_seq_start_queue( sc.seq , sc.queue , NULL );
}


gint sequencer_event_init( void )
{
  /* common settings for all our events */
  snd_seq_ev_clear(&sc.ev);
  sc.ev.queue = sc.queue;
  sc.ev.source.port = 0;
  sc.ev.flags = SND_SEQ_TIME_STAMP_TICK;
  return 1;
}


gint sequencer_event_noteon( midievent_t * event )
{
  i_seq_event_common_init( event );
  snd_seq_ev_set_fixed(&sc.ev);
  sc.ev.data.note.channel = event->data.d[0];
  sc.ev.data.note.note = event->data.d[1];
  sc.ev.data.note.velocity = event->data.d[2];
  return 1;
}


gint sequencer_event_noteoff( midievent_t * event )
{
  i_seq_event_common_init( event );
  snd_seq_ev_set_fixed(&sc.ev);
  sc.ev.data.note.channel = event->data.d[0];
  sc.ev.data.note.note = event->data.d[1];
  sc.ev.data.note.velocity = event->data.d[2];
  return 1;
}


gint sequencer_event_keypress( midievent_t * event )
{
  i_seq_event_common_init( event );
  snd_seq_ev_set_fixed(&sc.ev);
  sc.ev.data.note.channel = event->data.d[0];
  sc.ev.data.note.note = event->data.d[1];
  sc.ev.data.note.velocity = event->data.d[2];
  return 1;
}


gint sequencer_event_controller( midievent_t * event )
{
  i_seq_event_common_init( event );
  snd_seq_ev_set_fixed(&sc.ev);
  sc.ev.data.control.channel = event->data.d[0];
  sc.ev.data.control.param = event->data.d[1];
  sc.ev.data.control.value = event->data.d[2];
  return 1;
}


gint sequencer_event_pgmchange( midievent_t * event )
{
  i_seq_event_common_init( event );
  snd_seq_ev_set_fixed(&sc.ev);
  sc.ev.data.control.channel = event->data.d[0];
  sc.ev.data.control.value = event->data.d[1];
  return 1;
}


gint sequencer_event_chanpress( midievent_t * event )
{
  i_seq_event_common_init( event );
  snd_seq_ev_set_fixed(&sc.ev);
  sc.ev.data.control.channel = event->data.d[0];
  sc.ev.data.control.value = event->data.d[1];
  return 1;
}


gint sequencer_event_pitchbend( midievent_t * event )
{
  i_seq_event_common_init( event );
  snd_seq_ev_set_fixed(&sc.ev);
  sc.ev.data.control.channel = event->data.d[0];
  sc.ev.data.control.value = ((event->data.d[1]) | ((event->data.d[2]) << 7)) - 0x2000;
  return 1;
}


gint sequencer_event_sysex( midievent_t * event )
{
  i_seq_event_common_init( event );
  snd_seq_ev_set_variable(&sc.ev, event->data.length, event->sysex);
  return 1;
}


gint sequencer_event_tempo( midievent_t * event )
{
  i_seq_event_common_init( event );
  snd_seq_ev_set_fixed(&sc.ev);
  sc.ev.dest.client = SND_SEQ_CLIENT_SYSTEM;
  sc.ev.dest.port = SND_SEQ_PORT_SYSTEM_TIMER;
  sc.ev.data.queue.queue = sc.queue;
  sc.ev.data.queue.param.value = event->data.tempo;
  return 1;
}


gint sequencer_event_other( midievent_t * event )
{
  /* unhandled */
  return 1;
}


gint sequencer_output( gpointer * buffer , gint * len )
{
  snd_seq_event_output( sc.seq , &sc.ev );
  snd_seq_drain_output( sc.seq );
  snd_seq_sync_output_queue( sc.seq );
  return 0;
}


gint sequencer_output_shut( guint max_tick , gint skip_offset )
{
  gint i = 0 , c = 0;
  /* time to shutdown playback! */
  /* send "ALL SOUNDS OFF" to all channels on all ports */
  sc.ev.type = SND_SEQ_EVENT_CONTROLLER;
  sc.ev.time.tick = 0;
  snd_seq_ev_set_fixed(&sc.ev);
  sc.ev.data.control.param = MIDI_CTL_ALL_SOUNDS_OFF;
  sc.ev.data.control.value = 0;
  for ( i = 0 ; i < sc.dest_port_num ; i++ )
  {
    sc.ev.queue = sc.queue;
    sc.ev.dest = sc.dest_port[i];

    for ( c = 0 ; c < 16 ; c++ )
    {
      sc.ev.data.control.channel = c;
      snd_seq_event_output(sc.seq, &sc.ev);
      snd_seq_drain_output(sc.seq);
    }
  }

  /* schedule queue stop at end of song */
  snd_seq_ev_clear(&sc.ev);
  sc.ev.queue = sc.queue;
  sc.ev.source.port = 0;
  sc.ev.flags = SND_SEQ_TIME_STAMP_TICK;

  snd_seq_ev_set_fixed(&sc.ev);
  sc.ev.type = SND_SEQ_EVENT_STOP;
  sc.ev.time.tick = max_tick - skip_offset;
  sc.ev.dest.client = SND_SEQ_CLIENT_SYSTEM;
  sc.ev.dest.port = SND_SEQ_PORT_SYSTEM_TIMER;
  sc.ev.data.queue.queue = sc.queue;
  snd_seq_event_output(sc.seq, &sc.ev);
  snd_seq_drain_output(sc.seq);
  /* snd_seq_sync_output_queue(sc.seq); */

  return 1;
}


gint audio_volume_get( gint * left_volume , gint * right_volume )
{
  snd_mixer_t * mixer_h = NULL;
  snd_mixer_elem_t * mixer_elem = NULL;
  gchar mixer_card[10];
  snprintf( mixer_card , 8 , "hw:%i" , amidiplug_cfg_alsa.alsa_mixer_card_id );
  mixer_card[9] = '\0';

  if ( snd_mixer_open( &mixer_h , 0 ) > -1 )
    i_seq_mixer_find_selem( mixer_h , mixer_card ,
                            amidiplug_cfg_alsa.alsa_mixer_ctl_name ,
                            amidiplug_cfg_alsa.alsa_mixer_ctl_id ,
                            &mixer_elem );
  else
    mixer_h = NULL;

  if ( ( mixer_elem ) && ( snd_mixer_selem_has_playback_volume( mixer_elem ) ) )
  {
    glong pv_min , pv_max , pv_range;
    glong lc, rc;

    snd_mixer_selem_get_playback_volume_range( mixer_elem , &pv_min , &pv_max );
    pv_range = pv_max - pv_min;
    if ( pv_range > 0 )
    {
      if ( snd_mixer_selem_has_playback_channel( mixer_elem , SND_MIXER_SCHN_FRONT_LEFT ) )
      {
        snd_mixer_selem_get_playback_volume( mixer_elem , SND_MIXER_SCHN_FRONT_LEFT , &lc );
        /* convert the range to 0-100 (for the case that pv_range is not 0-100 already) */
        *left_volume = (gint)(((lc - pv_min) * 100) / pv_range);
        DEBUGMSG( "GET VOLUME requested, get left channel (%i)\n" , *left_volume );
      }
      if ( snd_mixer_selem_has_playback_channel( mixer_elem , SND_MIXER_SCHN_FRONT_RIGHT ) )
      {
        snd_mixer_selem_get_playback_volume( mixer_elem , SND_MIXER_SCHN_FRONT_RIGHT , &rc );
        /* convert the range to 0-100 (for the case that pv_range is not 0-100 already) */
        *right_volume = (gint)(((rc - pv_min) * 100) / pv_range);
        DEBUGMSG( "GET VOLUME requested, get right channel (%i)\n" , *right_volume );
      }
    }
  }

  if ( mixer_h )
    snd_mixer_close( mixer_h );
  /* always return 1 here */
  return 1;
}


gint audio_volume_set( gint left_volume , gint right_volume )
{
  snd_mixer_t * mixer_h = NULL;
  snd_mixer_elem_t * mixer_elem = NULL;
  gchar mixer_card[10];
  snprintf( mixer_card , 8 , "hw:%i" , amidiplug_cfg_alsa.alsa_mixer_card_id );
  mixer_card[9] = '\0';

  if ( snd_mixer_open( &mixer_h , 0 ) > -1 )
    i_seq_mixer_find_selem( mixer_h , mixer_card ,
                            amidiplug_cfg_alsa.alsa_mixer_ctl_name ,
                            amidiplug_cfg_alsa.alsa_mixer_ctl_id ,
                            &mixer_elem );
  else
    mixer_h = NULL;

  if ( ( mixer_elem ) && ( snd_mixer_selem_has_playback_volume( mixer_elem ) ) )
  {
    glong pv_min , pv_max , pv_range;

    snd_mixer_selem_get_playback_volume_range( mixer_elem , &pv_min , &pv_max );
    pv_range = pv_max - pv_min;
    if ( pv_range > 0 )
    {
      if ( snd_mixer_selem_has_playback_channel( mixer_elem , SND_MIXER_SCHN_FRONT_LEFT ) )
      {
        DEBUGMSG( "SET VOLUME requested, setting left channel to %i%%\n" , left_volume );
        snd_mixer_selem_set_playback_volume( mixer_elem , SND_MIXER_SCHN_FRONT_LEFT ,
                                             (gint)((gdouble)(0.01 * (gdouble)(left_volume * pv_range)) + pv_min) );
      }
      if ( snd_mixer_selem_has_playback_channel( mixer_elem , SND_MIXER_SCHN_FRONT_RIGHT ) )
      {
        DEBUGMSG( "SET VOLUME requested, setting right channel to %i%%\n" , right_volume );
        snd_mixer_selem_set_playback_volume( mixer_elem , SND_MIXER_SCHN_FRONT_RIGHT ,
                                             (gint)((gdouble)(0.01 * (gdouble)(right_volume * pv_range)) + pv_min) );
      }
    }
  }

  if ( mixer_h )
    snd_mixer_close( mixer_h );
  /* always return 1 here */
  return 1;
}


gint audio_info_get( gint * channels , gint * bitdepth , gint * samplerate )
{
  /* not applicable for ALSA backend */
  *channels = -1;
  *bitdepth = -1;
  *samplerate = -1;
  return 0; /* not valid information */
}


gboolean audio_check_autonomous( void )
{
  return TRUE; /* ALSA deals directly with audio production */
}



/* ******************************************************************
   *** EXTRA FUNCTIONS **********************************************
   ****************************************************************** */


/* get a list of writable ALSA MIDI ports
   use the data_bucket_t here...
   bint[0] = client id , bint[1] = port id
   bcharp[0] = client name , bcharp[1] = port name
   bpointer[0] = (not used) , bpointer[1] = (not used) */
GSList * sequencer_port_get_list( void )
{
  snd_seq_t * pseq;
  snd_seq_open( &pseq , "default" , SND_SEQ_OPEN_DUPLEX , 0 );

  GSList * wports = NULL;
  snd_seq_client_info_t *cinfo;
  snd_seq_port_info_t *pinfo;

  snd_seq_client_info_alloca( &cinfo );
  snd_seq_port_info_alloca( &pinfo );

  snd_seq_client_info_set_client( cinfo , -1 );
  while ( snd_seq_query_next_client( pseq , cinfo ) >= 0 )
  {
    gint client = snd_seq_client_info_get_client( cinfo );
    snd_seq_port_info_set_client( pinfo , client );
    snd_seq_port_info_set_port( pinfo , -1 );
    while (snd_seq_query_next_port( pseq , pinfo ) >= 0 )
    {
      if ((snd_seq_port_info_get_capability(pinfo)
           & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
          == (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
      {
        data_bucket_t * portinfo = (data_bucket_t*)g_malloc(sizeof(data_bucket_t));
        portinfo->bint[0] = snd_seq_port_info_get_client( pinfo );
        portinfo->bint[1] = snd_seq_port_info_get_port( pinfo );
        portinfo->bcharp[0] = g_strdup(snd_seq_client_info_get_name(cinfo));
        portinfo->bcharp[1] = g_strdup(snd_seq_port_info_get_name(pinfo));
        wports = g_slist_append( wports , portinfo );
      }
    }
  }
  /* snd_seq_port_info_free( pinfo );
     snd_seq_client_info_free( cinfo ); */
  snd_seq_close( pseq );
  return wports;
}


void sequencer_port_free_list( GSList * wports )
{
  GSList * start = wports;
  while ( wports != NULL )
  {
    data_bucket_t * portinfo = wports->data;
    g_free( (gpointer)portinfo->bcharp[0] );
    g_free( (gpointer)portinfo->bcharp[1] );
    g_free( portinfo );
    wports = wports->next;
  }
  g_slist_free( start );
  return;
}


/* get a list of available sound cards and relative mixer controls;
   use the data_bucket_t here...
   bint[0] = card id , bint[1] = (not used)
   bcharp[0] = card name , bcharp[1] = (not used)
   bpointer[0] = list (GSList) of mixer controls on the card , bpointer[1] = (not used) */
GSList * alsa_card_get_list( void )
{
  gint soundcard_id = -1;
  GSList * scards = NULL;

  snd_card_next( &soundcard_id );
  while ( soundcard_id > -1 )
  {
    /* card container */
    data_bucket_t * cardinfo = (data_bucket_t*)g_malloc(sizeof(data_bucket_t));
    cardinfo->bint[0] = soundcard_id;
    /* snd_card_get_name calls strdup on its own */
    snd_card_get_name( soundcard_id , &cardinfo->bcharp[0] );
    /* for each sound card, get a list of available mixer controls */
    cardinfo->bpointer[0] = i_seq_mixctl_get_list( soundcard_id );

    scards = g_slist_append( scards , cardinfo );
    snd_card_next( &soundcard_id );
  }
  return scards;
}


void alsa_card_free_list( GSList * scards )
{
  GSList * start = scards;
  while ( scards != NULL )
  {
    data_bucket_t * cardinfo = scards->data;
    /* free the list of mixer controls for the sound card */
    i_seq_mixctl_free_list( (GSList*)cardinfo->bpointer[0] );
    g_free( (gpointer)cardinfo->bcharp[0] );
    g_free( cardinfo );
    scards = scards->next;
  }
  g_slist_free( start );
  return;
}



/* ******************************************************************
   *** INTERNALS ****************************************************
   ****************************************************************** */


/* create sequencer client */
gint i_seq_open( void )
{
  gint err;
  err = snd_seq_open( &sc.seq , "default" , SND_SEQ_OPEN_DUPLEX , 0 );
  if (err < 0)
    return 0;
  snd_seq_set_client_name( sc.seq , "amidi-plug" );
  return 1;
}


/* free sequencer client */
gint i_seq_close( void )
{
  if ( snd_seq_close( sc.seq ) < 0 )
    return 0; /* fail */
  else
    return 1; /* success */
}


/* create queue */
gint i_seq_queue_create( void )
{
  sc.queue = snd_seq_alloc_named_queue( sc.seq , "AMIDI-Plug" );
  if ( sc.queue < 0 )
    return 0; /* fail */
  else
    return 1; /* success */
}


/* free queue */
gint i_seq_queue_free( void )
{
  if ( snd_seq_free_queue( sc.seq , sc.queue ) < 0 )
    return 0; /* fail */
  else
    return 1; /* success */
}


/* create sequencer port */
gint i_seq_port_create( void )
{
  sc.client_port = snd_seq_create_simple_port( sc.seq , "AMIDI-Plug" , 0 ,
                                               SND_SEQ_PORT_TYPE_MIDI_GENERIC |
                                               SND_SEQ_PORT_TYPE_APPLICATION );
  if ( sc.client_port < 0 )
    return 0; /* fail */
  else
    return 1; /* success */
}


/* port connection */
gint i_seq_port_connect( void )
{
  gint i = 0 , err = 0;
  for ( i = 0 ; i < sc.dest_port_num ; i++ )
  {
    if ( snd_seq_connect_to( sc.seq , sc.client_port ,
                             sc.dest_port[i].client ,
                             sc.dest_port[i].port ) < 0 )
      ++err;
  }
  /* if these values are equal, it means
     that all port connections failed */
  if ( err == i )
    return 0; /* fail */
  else
    return 1; /* success */
}


/* port disconnection */
gint i_seq_port_disconnect( void )
{
  gint i = 0 , err = 0;
  for ( i = 0 ; i < sc.dest_port_num ; i++ )
  {
    if ( snd_seq_disconnect_to( sc.seq , sc.client_port ,
                                sc.dest_port[i].client ,
                                sc.dest_port[i].port ) < 0 )
      ++err;
  }
  /* if these values are equal, it means
     that all port disconnections failed */
  if ( err == i )
    return 0; /* fail */
  else
    return 1; /* success */
}


/* parse writable ports */
gint i_seq_port_wparse( gchar * wportlist )
{
  gint i = 0 , err = 0;
  gchar **portstr = g_strsplit( wportlist , "," , 0 );

  sc.dest_port_num = 0;

  /* fill sc.dest_port_num with the writable port number */
  while ( portstr[sc.dest_port_num] != NULL )
    ++sc.dest_port_num;

  /* check if there is already an allocated array and free it */
  if ( sc.dest_port )
    free( sc.dest_port );

  if ( sc.dest_port_num > 0 )
  /* allocate the array of writable ports */
    sc.dest_port = calloc( sc.dest_port_num , sizeof(snd_seq_addr_t) );

  for ( i = 0 ; i < sc.dest_port_num ; i++ )
  {
    if ( snd_seq_parse_address( sc.seq , &sc.dest_port[i] , portstr[i] ) < 0 )
      ++err;
  }

  g_strfreev( portstr );

  /* if these values are equal, it means
     that all port translations failed */
  if ( err == i )
    return 0; /* fail */
  else
    return 1; /* success */
}


gint i_seq_event_common_init( midievent_t * event )
{
  sc.ev.type = event->type;
  sc.ev.time.tick = event->tick_real;
  sc.ev.dest = sc.dest_port[event->port];
  return 1;
}


/* get a list of available mixer controls for a given sound card;
   use the data_bucket_t here...
   bint[0] = control id , bint[1] = (not used)
   bcharp[0] = control name , bcharp[1] = (not used)
   bpointer[0] = (not used) , bpointer[1] = (not used) */
GSList * i_seq_mixctl_get_list( gint soundcard_id )
{
  GSList * mixctls = NULL;
  snd_mixer_t * mixer_h;
  snd_mixer_selem_id_t * mixer_selem_id;
  snd_mixer_elem_t * mixer_elem;
  gchar card[10];

  snprintf( card , 8 , "hw:%i" , soundcard_id );
  card[9] = '\0';

  snd_mixer_selem_id_alloca( &mixer_selem_id );
  snd_mixer_open( &mixer_h , 0 );
  snd_mixer_attach( mixer_h , card );
  snd_mixer_selem_register( mixer_h , NULL , NULL );
  snd_mixer_load( mixer_h );
  for ( mixer_elem = snd_mixer_first_elem( mixer_h ) ; mixer_elem ;
        mixer_elem = snd_mixer_elem_next( mixer_elem ) )
  {
    data_bucket_t * mixctlinfo = (data_bucket_t*)g_malloc(sizeof(data_bucket_t));
    snd_mixer_selem_get_id( mixer_elem , mixer_selem_id );
    mixctlinfo->bint[0] = snd_mixer_selem_id_get_index(mixer_selem_id);
    mixctlinfo->bcharp[0] = g_strdup(snd_mixer_selem_id_get_name(mixer_selem_id));
    mixctls = g_slist_append( mixctls , mixctlinfo );
  }
  snd_mixer_close( mixer_h );
  return mixctls;
}


void i_seq_mixctl_free_list( GSList * mixctls )
{
  GSList * start = mixctls;
  while ( mixctls != NULL )
  {
    data_bucket_t * mixctlinfo = mixctls->data;
    g_free( (gpointer)mixctlinfo->bcharp[0] );
    g_free( mixctlinfo );
    mixctls = mixctls->next;
  }
  g_slist_free( start );
  return;
}


gint i_seq_mixer_find_selem( snd_mixer_t * mixer_h , gchar * mixer_card ,
                             gchar * mixer_control_name , gint mixer_control_id ,
                             snd_mixer_elem_t ** mixer_elem )
{
  snd_mixer_selem_id_t * mixer_selem_id = NULL;
  snd_mixer_selem_id_alloca( &mixer_selem_id );
  snd_mixer_selem_id_set_index( mixer_selem_id , mixer_control_id );
  snd_mixer_selem_id_set_name( mixer_selem_id , mixer_control_name );
  snd_mixer_attach( mixer_h , mixer_card );
  snd_mixer_selem_register( mixer_h , NULL , NULL);
  snd_mixer_load( mixer_h );
  /* assign the mixer element (can be NULL if there is no such element) */
  *mixer_elem = snd_mixer_find_selem( mixer_h , mixer_selem_id );
  /* always return 1 here */
  return 1;
}


gchar * i_configure_read_seq_ports_default( void )
{
  FILE * fp = NULL;
  /* first try, get seq ports from proc on card0 */
  fp = fopen( "/proc/asound/card0/wavetableD1" , "rb" );
  if ( fp )
  {
    gchar buffer[100];
    while ( !feof( fp ) )
    {
      fgets( buffer , 100 , fp );
      if (( strlen( buffer ) > 11 ) && ( !strncasecmp( buffer , "addresses: " , 11 ) ))
      {
        /* change spaces between ports (65:0 65:1 65:2 ...)
           into commas (65:0,65:1,65:2,...) */
        g_strdelimit( &buffer[11] , " " , ',' );
        /* remove lf and cr from the end of the string */
        g_strdelimit( &buffer[11] , "\r\n" , '\0' );
        /* ready to go */
        DEBUGMSG( "init, default values for seq ports detected: %s\n" , &buffer[11] );
        fclose( fp );
        return g_strdup( &buffer[11] );
      }
    }
    fclose( fp );
  }

  /* second option: do not set ports at all, let the user
     select the right ones in the nice config window :) */
  return g_strdup( "" );
}


/* count the number of occurrencies of a specific character 'c'
   in the string 'string' (it must be a null-terminated string) */
gint i_util_str_count( gchar * string , gchar c )
{
  gint i = 0 , count = 0;
  while ( string[i] != '\0' )
  {
    if ( string[i] == c )
      ++count;
    ++i;
  }
  return count;
}


void i_cfg_read( void )
{
  pcfg_t *cfgfile;
  gchar * config_pathfilename = g_strjoin( "" , g_get_home_dir() , "/" ,
                                           PLAYER_LOCALRCDIR , "/amidi-plug.conf" , NULL );
  cfgfile = i_pcfg_new_from_file( config_pathfilename );

  if ( !cfgfile )
  {
    /* alsa backend defaults */
    amidiplug_cfg_alsa.alsa_seq_wports = i_configure_read_seq_ports_default();
    amidiplug_cfg_alsa.alsa_mixer_card_id = 0;
    amidiplug_cfg_alsa.alsa_mixer_ctl_name = g_strdup( "Synth" );
    amidiplug_cfg_alsa.alsa_mixer_ctl_id = 0;
  }
  else
  {
    i_pcfg_read_string( cfgfile , "alsa" , "alsa_seq_wports" ,
                        &amidiplug_cfg_alsa.alsa_seq_wports , NULL );
    if ( amidiplug_cfg_alsa.alsa_seq_wports == NULL )
      amidiplug_cfg_alsa.alsa_seq_wports = i_configure_read_seq_ports_default(); /* pick default values */

    i_pcfg_read_integer( cfgfile , "alsa" , "alsa_mixer_card_id" ,
                         &amidiplug_cfg_alsa.alsa_mixer_card_id , 0 );

    i_pcfg_read_string( cfgfile , "alsa" , "alsa_mixer_ctl_name" ,
                        &amidiplug_cfg_alsa.alsa_mixer_ctl_name , "Synth" );

    i_pcfg_read_integer( cfgfile , "alsa" , "alsa_mixer_ctl_id" ,
                         &amidiplug_cfg_alsa.alsa_mixer_ctl_id , 0 );

    i_pcfg_free( cfgfile );
  }

  g_free( config_pathfilename );
}


void i_cfg_free( void )
{
  g_free( amidiplug_cfg_alsa.alsa_seq_wports );
  g_free( amidiplug_cfg_alsa.alsa_mixer_ctl_name );
}
