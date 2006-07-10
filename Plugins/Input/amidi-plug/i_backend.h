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

#ifndef _I_BACKEND_H
#define _I_BACKEND_H 1

#include <gmodule.h>
#include "i_midievent.h"
#include "i_common.h"


typedef struct
{
  gchar * desc;
  gchar * filename;
  gchar * longname;
  gchar * name;
  gint ppos;
}
amidiplug_sequencer_backend_name_t;


typedef struct
{
  gint id;
  GModule * gmodule;
  gchar * name;
  gint (*init)( void );
  gint (*cleanup)( void );
  gint (*audio_info_get)( gint * , gint * , gint * );
  gint (*audio_volume_get)( gint * , gint * );
  gint (*audio_volume_set)( gint , gint );
  gint (*seq_start)( gchar * );
  gint (*seq_stop)( void );
  gint (*seq_on)( void );
  gint (*seq_off)( void );
  gint (*seq_queue_tempo)( gint , gint );
  gint (*seq_queue_start)( void );
  gint (*seq_event_init)( void );
  gint (*seq_event_noteon)( midievent_t * );
  gint (*seq_event_noteoff)( midievent_t * );
  gint (*seq_event_keypress)( midievent_t * );
  gint (*seq_event_controller)( midievent_t * );
  gint (*seq_event_pgmchange)( midievent_t * );
  gint (*seq_event_chanpress)( midievent_t * );
  gint (*seq_event_pitchbend)( midievent_t * );
  gint (*seq_event_sysex)( midievent_t * );
  gint (*seq_event_tempo)( midievent_t * );
  gint (*seq_event_other)( midievent_t * );
  gint (*seq_output)( gpointer * , gint * );
  gint (*seq_output_shut)( guint , gint );
  gint (*seq_get_port_count)( void );
  gboolean autonomous_audio;
}
amidiplug_sequencer_backend_t;

extern amidiplug_sequencer_backend_t backend;


GSList * i_backend_list_lookup( void );
void i_backend_list_free( GSList * );
gint i_backend_load( gchar * );
gint i_backend_unload( void );

#endif /* !_I_BACKEND_H */
