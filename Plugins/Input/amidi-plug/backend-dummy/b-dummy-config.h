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

#ifndef _B_DUMMY_CONFIG_H
#define _B_DUMMY_CONFIG_H 1


typedef struct
{
  gint		dumm_logger_enable;
  gint		dumm_logger_lfstyle;
  gint		dumm_playback_speed;
  gchar *	dumm_logger_logfile;
  gchar *	dumm_logger_logdir;
}
amidiplug_cfg_dumm_t;


#endif /* !_B_DUMMY_CONFIG_H */
