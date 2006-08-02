/*  XMMS - ALSA output plugin
 *  Copyright (C) 2001-2003 Matthieu Sozeau <mattam@altern.org>
 *  Copyright (C) 2003-2005 Haavard Kvaalen
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
#include "audacious/main.h"
#include "alsa.h"
#include <dlfcn.h>
#include <ctype.h>

struct alsa_config alsa_cfg;

void alsa_init(void)
{
	ConfigDb *cfgfile;

	memset(&alsa_cfg, 0, sizeof (alsa_cfg));
	alsa_cfg.buffer_time = 500;
	alsa_cfg.period_time = 100;
	alsa_cfg.debug = 0;
	alsa_cfg.vol.left = 100;
	alsa_cfg.vol.right = 100;

	cfgfile = bmp_cfg_db_open();
	if (!bmp_cfg_db_get_string(cfgfile, "ALSA", "pcm_device",
				  &alsa_cfg.pcm_device))
		alsa_cfg.pcm_device = g_strdup("default");
	g_message("device: %s", alsa_cfg.pcm_device);
	if (!bmp_cfg_db_get_string(cfgfile, "ALSA", "mixer_device",
				  &alsa_cfg.mixer_device))
		alsa_cfg.mixer_device = g_strdup("PCM");
	bmp_cfg_db_get_int(cfgfile, "ALSA", "mixer_card", &alsa_cfg.mixer_card);
	bmp_cfg_db_get_int(cfgfile, "ALSA", "buffer_time", &alsa_cfg.buffer_time);
	bmp_cfg_db_get_int(cfgfile, "ALSA", "period_time", &alsa_cfg.period_time);

	bmp_cfg_db_get_bool(cfgfile, "ALSA", "soft_volume",
			      &alsa_cfg.soft_volume);
	bmp_cfg_db_get_int(cfgfile, "ALSA", "volume_left", &alsa_cfg.vol.left);
	bmp_cfg_db_get_int(cfgfile, "ALSA", "volume_right", &alsa_cfg.vol.right);

	bmp_cfg_db_get_bool(cfgfile, "ALSA", "debug", &alsa_cfg.debug);
	bmp_cfg_db_close(cfgfile);

	if (dlopen("libasound.so.2", RTLD_NOW | RTLD_GLOBAL) == NULL)
	{
		g_message("Cannot load alsa library: %s", dlerror());
		/* FIXME, this plugin wont work... */
	}

	alsa_mutex = g_mutex_new();
}
