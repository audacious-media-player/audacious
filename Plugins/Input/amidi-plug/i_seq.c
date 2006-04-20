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


#include "i_seq.h"


/* activate sequencer client */
gint i_seq_on( gshort with_wparse , gchar * wports_str )
{
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

  if (( with_wparse > 0 ) && ( wports_str ))
    i_seq_port_wparse( wports_str );

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
gint i_seq_off( void )
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


/* create sequencer client */
gint i_seq_open( void )
{
  gint err;
  err = snd_seq_open(&sc.seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
  if (err < 0)
    return 0;
  snd_seq_set_client_name(sc.seq, "amidi-plug");
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


/* queue set tempo */
gint i_seq_queue_set_tempo( gint tempo , gint ppq )
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


/* get a list of writable ALSA MIDI ports
   use the data_bucket_t here...
   bint[0] = client id , bint[1] = port id
   bcharp[0] = client name , bcharp[1] = port name
   bpointer[0] = (not used) , bpointer[1] = (not used) */
GSList * i_seq_port_get_list( void )
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


void i_seq_port_free_list( GSList * wports )
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
GSList * i_seq_card_get_list( void )
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


void i_seq_card_free_list( GSList * scards )
{
  GSList * start = scards;
  while ( scards != NULL )
  {
    data_bucket_t * cardinfo = scards->data;
    g_free( (gpointer)cardinfo->bcharp[0] );
    g_free( cardinfo );
    scards = scards->next;
  }
  g_slist_free( start );
  return;
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


gint i_seq_mixer_get_volume( gint * left_volume , gint * right_volume ,
                            gchar * mixer_card , gchar * mixer_control_name , gint mixer_control_id )
{
  snd_mixer_t * mixer_h = NULL;
  snd_mixer_elem_t * mixer_elem = NULL;

  if ( snd_mixer_open( &mixer_h , 0 ) > -1 )
    i_seq_mixer_find_selem( mixer_h , mixer_card , mixer_control_name , mixer_control_id , &mixer_elem );
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

  /* for now, always return 1 here */
  return 1;
}


gint i_seq_mixer_set_volume( gint left_volume , gint right_volume ,
                            gchar * mixer_card , gchar * mixer_control_name , gint mixer_control_id )
{
  snd_mixer_t * mixer_h = NULL;
  snd_mixer_elem_t * mixer_elem = NULL;

  if ( snd_mixer_open( &mixer_h , 0 ) > -1 )
    i_seq_mixer_find_selem( mixer_h , mixer_card , mixer_control_name , mixer_control_id , &mixer_elem );
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

  /* for now, always return 1 here */
  return 1;
}
