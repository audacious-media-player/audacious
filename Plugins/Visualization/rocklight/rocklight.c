/***************************************************************************
 *            rocklight.c
 *
 *  Sun Dec 26 18:26:59 2004
 *  Copyright  2004  Benedikt 'Hunz' Heinz
 *  rocklight@hunz.org
 *  Audacious implementation by Tony Vroon <chainsaw@gentoo.org> in August 2006
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "audacious/plugin.h"
#include "thinklight.h"

static int fd, last, state;

static void rocklight_init(void) {
	fd=thinklight_open();
}

static void rocklight_cleanup(void) {
	thinklight_close(fd);
}

static void rocklight_playback_start(void) {
	last=state=thinklight_get(fd);
}

static void rocklight_playback_stop(void) {
	if(last!=state)
		thinklight_set(fd,state);
}

static void rocklight_render_freq(gint16 data[2][256]) {
	int new=((data[0][0]+data[1][0])>>7)>=80?1:0;
	
	if(new!=last) {
		thinklight_set(fd,new);
		last=new;
	}
}

VisPlugin rocklight_vp = {
	NULL,
	NULL,
	0,  
	"RockLight",
	0,
	2,
	rocklight_init,
	rocklight_cleanup,
	NULL,
	NULL,
	NULL,
	rocklight_playback_start,
	rocklight_playback_stop,
	NULL,
	rocklight_render_freq
};

VisPlugin *get_vplugin_info(void) {
	return &rocklight_vp;
}
