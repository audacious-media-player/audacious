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
* 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307  USA
*
*/

#ifndef _B_FLUIDSYNTH_CONFIG_H
#define _B_FLUIDSYNTH_CONFIG_H 1


typedef struct
{
  gchar *	fsyn_soundfont_file;
  gint		fsyn_soundfont_load;
  gint		fsyn_synth_samplerate;
  gint		fsyn_synth_gain;
  gint		fsyn_synth_poliphony;
  gint		fsyn_synth_reverb;
  gint		fsyn_synth_chorus;
  gint		fsyn_buffer_size;
  gint		fsyn_buffer_margin;
  gint		fsyn_buffer_increment;
}
amidiplug_cfg_fsyn_t;


#endif /* !_B_FLUIDSYNTH_CONFIG_H */
