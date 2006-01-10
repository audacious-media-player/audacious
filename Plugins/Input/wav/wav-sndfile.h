/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005 Audacious development team.
 *
 *  Based on the xmms_sndfile input plugin:
 *  Copyright (C) 2000, 2002 Erik de Castro Lopo
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef WAV_SNDFILE_H
#define WAV_SNDFILE_H

#define BUFFER_SIZE 			8192
#define	TITLE_LEN			64

static	void 	plugin_init (void);
static	int	is_our_file (char *filename);
static	void 	play_start (char *filename);
static	void 	play_stop (void);
static	void 	file_seek (int time);
static	int	get_time (void);
static	void 	get_song_info (char *filename, char **title, int *length);
static  void    wav_about (void);

#endif
