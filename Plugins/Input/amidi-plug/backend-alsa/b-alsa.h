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

#ifndef _B_ALSA_H
#define _B_ALSA_H 1

#include <alsa/asoundlib.h>
#include "../i_common.h"
#include "../pcfg/i_pcfg.h"
#include "../i_midievent.h"


typedef struct
{
  snd_seq_t * seq;
  gint client_port;
  gint queue;

  snd_seq_addr_t * dest_port;
  gint dest_port_num;

  snd_seq_queue_tempo_t * queue_tempo;

  snd_seq_event_t ev;

  gboolean is_start;
}
sequencer_client_t;

gint i_seq_open( void );
gint i_seq_close( void );
gint i_seq_queue_create( void );
gint i_seq_queue_free( void );
gint i_seq_port_create( void );
gint i_seq_port_connect( void );
gint i_seq_port_disconnect( void );
gint i_seq_port_wparse( gchar * );
gint i_seq_event_common_init( midievent_t * );
GSList * i_seq_mixctl_get_list( gint );
void i_seq_mixctl_free_list( GSList * );
gint i_seq_mixer_find_selem( snd_mixer_t * , gchar * , gchar * , gint , snd_mixer_elem_t ** );
void i_cfg_read( void );
void i_cfg_free( void );
gint i_util_str_count( gchar * , gchar );

#endif /* !_B_ALSA_H */
