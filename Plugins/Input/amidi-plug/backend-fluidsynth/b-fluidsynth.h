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

#ifndef _B_FLUIDSYNTH_H
#define _B_FLUIDSYNTH_H 1

#include <fluidsynth.h>
#include <math.h>
#include "../i_common.h"
#include "../pcfg/i_pcfg.h"
#include "../i_midievent.h"


typedef struct
{
  fluid_settings_t * settings;
  fluid_synth_t * synth;

  GArray *soundfont_ids;

  gint ppq;
  gdouble cur_microsec_per_tick;
  guint tick_offset;

  GTimer * timer_seq;
  GTimer * timer_sample;

  guint sample_rate;
  gdouble last_sample_time;
}
sequencer_client_t;


void i_sleep( guint );
void i_soundfont_load( void );
void i_cfg_read( void );
void i_cfg_free( void );

#endif /* !_B_FLUIDSYNTH_H */
