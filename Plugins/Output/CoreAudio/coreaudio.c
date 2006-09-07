/*  XMMS - Cross-platform multimedia player
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <glib.h>
#include <glib/gi18n.h>

#include "coreaudio.h"

#include "audacious/plugin.h"

OutputPlugin osx_op =
	{
		NULL,
		NULL,
		NULL, /* Description */
		osx_init,
		NULL,
		osx_about,
		osx_configure,
		osx_get_volume,
		osx_set_volume,
		osx_open,
		osx_write,
		osx_close,
		osx_flush,
		osx_pause,
		osx_free,
		osx_playing,
		osx_get_output_time,
		osx_get_written_time,
	};

OutputPlugin *get_oplugin_info(void)
{
	osx_op.description = g_strdup(_("CoreAudio Output Plugin"));
	return &osx_op;
}
