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

#ifndef _I_SEQ_H
#define _I_SEQ_H 1

#include "i_common.h"
#include <alsa/asoundlib.h>

typedef struct
{
  snd_seq_t * seq;
  gint client_port;
  gint queue;

  snd_seq_addr_t * dest_port;
  gint dest_port_num;

  snd_seq_queue_tempo_t * queue_tempo;
}
sequencer_client_t;

extern sequencer_client_t sc;

gint i_seq_on( gshort , gchar * );
gint i_seq_off( void );
gint i_seq_open( void );
gint i_seq_close( void );
gint i_seq_port_create( void );
gint i_seq_port_connect( void );
gint i_seq_port_disconnect( void );
gint i_seq_queue_create( void );
gint i_seq_queue_free( void );
gint i_seq_queue_set_tempo( gint , gint );
gint i_seq_mixer_set_volume( gint , gint , gchar * , gchar * , gint );
gint i_seq_port_wparse( gchar * );
GSList * i_seq_port_get_list( void );
GSList * i_seq_card_get_list( void );
GSList * i_seq_mixctl_get_list( gint );
void i_seq_port_free_list( GSList * );
void i_seq_card_free_list( GSList * );
void i_seq_mixctl_free_list( GSList * );

#endif /* !_I_SEQ_H */
