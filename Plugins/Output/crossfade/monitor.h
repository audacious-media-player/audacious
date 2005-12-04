/* 
 *  XMMS Crossfade Plugin
 *  Copyright (C) 2000-2004  Peter Eisenlohr <peter@eisenlohr.org>
 *
 *  based on the original OSS Output Plugin
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#ifndef _MONITOR_H_
#define _MONITOR_H_

#include <glib.h>

/* time between updates in ms (default: 33 -> 30 updates per second) */
#define UPDATE_INTERVAL 33

void xfade_check_monitor_win();
gint xfade_update_monitor   (gpointer userdata);
void xfade_start_monitor    ();
void xfade_stop_monitor     ();

extern GtkWidget   *monitor_win;
extern GtkWidget   *monitor_display_drawingarea;
extern GtkEntry    *monitor_output_entry;
extern GtkProgress *monitor_output_progress;

#endif  /* _MONITOR_H_ */
