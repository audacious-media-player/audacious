/*  XMMS - ALSA output plugin
 *    Copyright (C) 2001 Matthieu Sozeau
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

#include "alsa.h"

OutputPlugin alsa_op =
{
	NULL,
	NULL,
	NULL,
	alsa_init,
	alsa_about,
	alsa_configure,
	alsa_get_volume,
	alsa_set_volume,
	alsa_open,
	alsa_write,
	alsa_close,
	alsa_flush,
	alsa_pause,
	alsa_free,
	alsa_playing,
	alsa_get_output_time,
	alsa_get_written_time,
};

OutputPlugin *get_oplugin_info(void)
{
	alsa_op.description = g_strdup_printf(_("ALSA %s output plugin"), VERSION);
	return &alsa_op;
}
