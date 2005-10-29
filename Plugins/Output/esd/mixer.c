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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esd.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "esdout.h"

#ifdef HAVE_OSS
# include <Output/OSS/soundcard.h>
# define OSS_AVAILABLE TRUE
#else
# define OSS_AVAILABLE FALSE
#endif

#include <libaudacious/util.h>



static void esdout_get_oss_volume(int *l, int *r);
static void esdout_set_oss_volume(int l, int r);


static int player = -1;
static int lp = 100, rp = 100;

/*
 * Find the stream id, and set stream volume to 'persistent' value.
 */
void
esdout_mixer_init(void)
{
    esdout_fetch_volume(NULL, NULL);
    if (!(OSS_AVAILABLE && esd_cfg.use_oss_mixer && !esd_cfg.use_remote))
        esdout_set_volume(lp, rp);
}

/*
 * Grab the stream volume from the server.  The problem here is that
 * ESD does not have a built-in function for finding the player ID of
 * a specific player - nor does it let us know what the player ID is
 * when the player is created! So, we grab 'allinfo' and scan the
 * returned player list for the string which we know is our player
 * name (esd_cfg.playername) This function seems to take a long time
 * to run... I'm not sure where to start optimizing, however...
 */
void
esdout_fetch_volume(int *l, int *r)
{
    int fd;
    esd_info_t *all_info = NULL;
    esd_player_info_t *info;

    fd = esd_open_sound(esd_cfg.hostname);
    all_info = esd_get_all_info(fd);

    /* scan linked list for our playername */
    for (info = all_info->player_list; info != NULL; info = info->next)
        if (!strcmp(esd_cfg.playername, info->name))
            break;

    if (info) {
        player = info->source_id;
        if (l && r) {
            /*
             * Sometimes we call with NULL
             * args to fetch the player num
             */
            *l = (info->left_vol_scale * 100) / 256;
            *r = (info->right_vol_scale * 100) / 256;
        }
    }
    else
        g_warning("xmms: Couldn't find our player "
                  "(was looking for %s) at the server", esd_cfg.playername);

    if (all_info)
        esd_free_all_info(all_info);
    esd_close(fd);
}

void
esdout_get_volume(int *l, int *r)
{
    if (OSS_AVAILABLE && esd_cfg.use_oss_mixer && !esd_cfg.use_remote) {
        esdout_get_oss_volume(l, r);
        lp = *l;
        rp = *r;
    }
    else {
        /*
         * We assume that the volume hasn't changed from the
         * 'persistant' value. Constantly polling takes too
         * much time/resources.  Commenting this section out
         * will consistently check the ESD server to see if
         * someone else changed our stream volume.
         */
        *l = lp;
        *r = rp;
/*  		esdout_fetch_volume(l, r); */
    }
}

void
esdout_set_volume(int l, int r)
{
    lp = l;
    rp = r;

    if (OSS_AVAILABLE && esd_cfg.use_oss_mixer && !esd_cfg.use_remote) {
        esdout_set_oss_volume(l, r);
    }
    else if (player != -1 && esd_cfg.playername != NULL) {
        int fd = esd_open_sound(esd_cfg.hostname);
        if (fd >= 0) {
            esd_set_stream_pan(fd, player, (l * 256) / 100, (r * 256) / 100);
            esd_close(fd);
        }
    }
}

#ifdef HAVE_OSS

static void
esdout_get_oss_volume(int *l, int *r)
{
    int fd, v, devs;
    long cmd;

    if (esd_cfg.use_remote)
        return;

    fd = open(DEV_MIXER, O_RDONLY);
    if (fd != -1) {
        ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devs);
        if (devs & SOUND_MASK_PCM)
            cmd = SOUND_MIXER_READ_PCM;
        else if (devs & SOUND_MASK_VOLUME)
            cmd = SOUND_MIXER_READ_VOLUME;
        else {
            close(fd);
            return;
        }
        ioctl(fd, cmd, &v);
        *r = (v & 0xFF00) >> 8;
        *l = (v & 0x00FF);
        close(fd);
    }
}

static void
esdout_set_oss_volume(int l, int r)
{
    int fd, v, devs;
    long cmd;

    if (esd_cfg.use_remote)
        return;

    fd = open(DEV_MIXER, O_RDONLY);

    if (fd != -1) {
        ioctl(fd, SOUND_MIXER_READ_DEVMASK, &devs);
        if (devs & SOUND_MASK_PCM)
            cmd = SOUND_MIXER_WRITE_PCM;
        else if (devs & SOUND_MASK_VOLUME)
            cmd = SOUND_MIXER_WRITE_VOLUME;
        else {
            close(fd);
            return;
        }
        v = (r << 8) | l;
        ioctl(fd, cmd, &v);
        close(fd);
    }
}

#else

static void
esdout_get_oss_volume(int *l, int *r)
{
}

static void
esdout_set_oss_volume(int l, int r)
{
}

#endif
