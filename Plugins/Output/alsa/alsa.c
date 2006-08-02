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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "alsa.h"
#include <stdlib.h>

OutputPlugin alsa_op =
{
	NULL,
	NULL,
	NULL,
	alsa_init,
	alsa_cleanup,
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
	alsa_tell
};

OutputPlugin *get_oplugin_info(void)
{
	alsa_op.description = g_strdup_printf(_("ALSA %s output plugin"), VERSION);
	return &alsa_op;
}


void alsa_cleanup(void)
{
	g_free(alsa_op.description);
	alsa_op.description = NULL;

	if (alsa_cfg.pcm_device) {
		free(alsa_cfg.pcm_device);
		alsa_cfg.pcm_device = NULL;
	}

	if (alsa_cfg.mixer_device) {
		free(alsa_cfg.mixer_device);
		alsa_cfg.mixer_device = NULL;
	}

	/* release our mutex */
	g_mutex_free(alsa_mutex);
	alsa_mutex = NULL;
}
