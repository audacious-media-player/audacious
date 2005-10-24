/*      xmms - esound output plugin
 *    Copyright (C) 1999      Galex Yen
 *      
 *      this program is free software
 *      
 *      Description:
 *              This program is an output plugin for xmms v0.9 or greater.
 *              The program uses the esound daemon to output audio in order
 *              to allow more than one program to play audio on the same
 *              device at the same time.
 *
 *              Contains code Copyright (C) 1998-1999 Mikael Alm, Olle Hallnas,
 *              Thomas Nillson and 4Front Technologies
 *
 */

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <esd.h>

#include <libaudacious/configdb.h>

#include "esdout.h"


ESDConfig esd_cfg;
esd_info_t *all_info;
esd_player_info_t *player_info;


void
esdout_init(void)
{
    ConfigDb *db;
    char *env;

    memset(&esd_cfg, 0, sizeof(ESDConfig));
    esd_cfg.port = ESD_DEFAULT_PORT;
    esd_cfg.buffer_size = 3000;
    esd_cfg.prebuffer = 25;

    db = bmp_cfg_db_open();

    if ((env = getenv("ESPEAKER")) != NULL) {
        char *temp;
        esd_cfg.use_remote = TRUE;
        esd_cfg.server = g_strdup(env);
        temp = strchr(esd_cfg.server, ':');
        if (temp != NULL) {
            *temp = '\0';
            esd_cfg.port = atoi(temp + 1);
            if (esd_cfg.port == 0)
                esd_cfg.port = ESD_DEFAULT_PORT;
        }
    }
    else {
        bmp_cfg_db_get_bool(db, "ESD", "use_remote", &esd_cfg.use_remote);
        bmp_cfg_db_get_string(db, "ESD", "remote_host", &esd_cfg.server);
        bmp_cfg_db_get_int(db, "ESD", "remote_port", &esd_cfg.port);
    }
    bmp_cfg_db_get_bool(db, "ESD", "use_oss_mixer", &esd_cfg.use_oss_mixer);
    bmp_cfg_db_get_int(db, "ESD", "buffer_size", &esd_cfg.buffer_size);
    bmp_cfg_db_get_int(db, "ESD", "prebuffer", &esd_cfg.prebuffer);
    bmp_cfg_db_close(db);

    if (!esd_cfg.server)
        esd_cfg.server = g_strdup("localhost");
}
