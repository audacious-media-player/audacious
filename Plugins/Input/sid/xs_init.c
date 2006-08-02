/*  
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   Plugin initialization point
   
   Programmed and designed by Matti 'ccr' Hamalainen <ccr@tnsp.org>
   (C) Copyright 1999-2005 Tecnic Software productions (TNSP)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "xmms-sid.h"
#include "xs_config.h"
#include "xs_fileinfo.h"

InputPlugin xs_plugin_ip = {
	NULL,			/* FILLED BY XMMS */
	NULL,			/* FILLED BY XMMS */
	"SID Tune Plugin",	/* Plugin description */
	xs_init,		/* Initialization */
	xs_about,		/* Show aboutbox */
	xs_configure,		/* Show/edit configuration */
	xs_is_our_file,		/* Check file */
	NULL,			/* Scan directory */
	xs_play_file,		/* Play given file */
	xs_stop,		/* Stop playing */
	xs_pause,		/* Pause playing */
	xs_seek,		/* Seek time */
	NULL,			/* Set equalizer */
	xs_get_time,		/* Get playing time */
	NULL,			/* Get volume */
	NULL,			/* Set volume */
	xs_close,		/* Cleanup */
	NULL,			/* OBSOLETE! */
	NULL,			/* Send data to Visualization plugin */
	NULL, NULL,		/* FILLED BY XMMS */
	xs_get_song_info,	/* Get song title and length */
	xs_fileinfo,		/* Show file-information dialog */
	NULL			/* FILLED BY XMMS */
};



/*
 * Return plugin information
 */
InputPlugin *get_iplugin_info(void)
{
	return &xs_plugin_ip;
}
