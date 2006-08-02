/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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

#include <glib.h>
#include <string.h>
#include <libaudacious/configdb.h>
#include "OSS.h"


OSSConfig oss_cfg;

void
oss_init(void)
{
    ConfigDb *db;

    memset(&oss_cfg, 0, sizeof(OSSConfig));

    oss_cfg.audio_device = 0;
    oss_cfg.mixer_device = 0;
    oss_cfg.buffer_size = 3000;
    oss_cfg.prebuffer = 25;
    oss_cfg.use_alt_audio_device = FALSE;
    oss_cfg.alt_audio_device = NULL;
    oss_cfg.use_master = 0;

    if ((db = bmp_cfg_db_open())) {
        bmp_cfg_db_get_int(db, "OSS", "audio_device", &oss_cfg.audio_device);
        bmp_cfg_db_get_int(db, "OSS", "mixer_device", &oss_cfg.mixer_device);
        bmp_cfg_db_get_int(db, "OSS", "buffer_size", &oss_cfg.buffer_size);
        bmp_cfg_db_get_int(db, "OSS", "prebuffer", &oss_cfg.prebuffer);
        bmp_cfg_db_get_bool(db, "OSS", "use_master", &oss_cfg.use_master);
        bmp_cfg_db_get_bool(db, "OSS", "use_alt_audio_device",
                            &oss_cfg.use_alt_audio_device);
        bmp_cfg_db_get_string(db, "OSS", "alt_audio_device",
                              &oss_cfg.alt_audio_device);
        bmp_cfg_db_get_bool(db, "OSS", "use_alt_mixer_device",
                            &oss_cfg.use_alt_mixer_device);
        bmp_cfg_db_get_string(db, "OSS", "alt_mixer_device",
                              &oss_cfg.alt_mixer_device);
        bmp_cfg_db_close(db);
    }
}
