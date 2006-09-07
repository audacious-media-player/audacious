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
#include "coreaudio.h"
#include "libaudacious/configdb.h"

OSXConfig osx_cfg;

extern float left_volume, right_volume;

void osx_init(void)
{
	ConfigDb *cfgfile;

	left_volume = 1.0;
	right_volume = 1.0;

	memset(&osx_cfg, 0, sizeof (OSXConfig));

	osx_cfg.audio_device = 0;
	osx_cfg.mixer_device = 0;
	osx_cfg.buffer_size = 3000;
	osx_cfg.prebuffer = 25;
	osx_cfg.use_alt_audio_device = FALSE;
	osx_cfg.alt_audio_device = NULL;
	osx_cfg.use_master=0;
	
	if ((cfgfile = bmp_cfg_db_open()))
	{
		bmp_cfg_db_get_int(cfgfile, "OSX", "audio_device", &osx_cfg.audio_device);
		bmp_cfg_db_get_int(cfgfile, "OSX", "mixer_device", &osx_cfg.mixer_device);
		bmp_cfg_db_get_int(cfgfile, "OSX", "buffer_size", &osx_cfg.buffer_size);
		bmp_cfg_db_get_int(cfgfile, "OSX", "prebuffer", &osx_cfg.prebuffer);
		bmp_cfg_db_get_bool(cfgfile, "OSX", "use_master", &osx_cfg.use_master);
		bmp_cfg_db_get_bool(cfgfile, "OSX", "use_alt_audio_device", &osx_cfg.use_alt_audio_device);
		bmp_cfg_db_get_string(cfgfile, "OSX", "alt_audio_device", &osx_cfg.alt_audio_device);
		bmp_cfg_db_get_bool(cfgfile, "OSX", "use_alt_mixer_device", &osx_cfg.use_alt_mixer_device);
		bmp_cfg_db_get_string(cfgfile, "OSX", "alt_mixer_device", &osx_cfg.alt_mixer_device);
		bmp_cfg_db_close(cfgfile);
	}
}
