/*  XMMS - Software volume managment.
 *  Copyright (C) 2001-2003 Matthieu Sozeau
 *  Original implementation from a patch by Tomas Simonaitis <haden@homelan.lt>
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

#include <libaudacious/configdb.h>
#include "softvolume.h"


/**************************************************************************
 *
 *  soft_volume_load
 *
 *  TODO: make argument order consistent with soft_volume_save()
 *
 **************************************************************************/

void
soft_volume_load(const gchar * section, SoftVolumeConfig * c)
{
    ConfigDb *db;

    g_return_if_fail(c != NULL);

    c->enabled = FALSE;
    c->volume_left = 0;
    c->volume_right = 0;

    db = bmp_cfg_db_open();

    bmp_cfg_db_get_bool(db, section, "softvolume_enable", &c->enabled);
    bmp_cfg_db_get_int(db, section, "softvolume_left", &c->volume_left);
    bmp_cfg_db_get_int(db, section, "softvolume_right", &c->volume_right);

    bmp_cfg_db_close(db);
}


/**************************************************************************
 *
 *  soft_volume_save
 *
 *  TODO: make argument order consistent with soft_volume_load()
 *
 **************************************************************************/

void
soft_volume_save(SoftVolumeConfig * c, const gchar * section)
{
    ConfigDb *db;

    g_return_if_fail(c != NULL);

    db = bmp_cfg_db_open();

    bmp_cfg_db_set_bool(db, section, "softvolume_enable", c->enabled);
    bmp_cfg_db_set_int(db, section, "softvolume_left", c->volume_left);
    bmp_cfg_db_set_int(db, section, "softvolume_right", c->volume_right);

    bmp_cfg_db_close(db);
}



/**************************************************************************
 *
 *  soft_volume_get
 *
 **************************************************************************/

void
soft_volume_get(SoftVolumeConfig * c, gint * l, gint * r)
{
    if (c == NULL)
        return;

    *l = c->volume_left;
    *r = c->volume_right;
}



/**************************************************************************
 *
 * soft_volume_set
 *
 **************************************************************************/

void
soft_volume_set(SoftVolumeConfig * c, gint l, gint r)
{
    if (c == NULL)
        return;

    c->volume_left = l;
    c->volume_right = r;
}


/**************************************************************************
 *
 *  effect_16bit
 *
 **************************************************************************/

void
effect_16bit(gint max, gint min, guint length, gint16 * sdata,
             SoftVolumeConfig * c)
{
    guint i;
    gint v;

    for (i = 0; i < (length >> 2); ++i) {   /* ie. length/4 */
        v = (gint) ((*sdata * c->volume_left) / 100);
        *(sdata++) = (gint16) CLAMP(v, min, max);

        v = (gint) ((*sdata * c->volume_right) / 100);
        *(sdata++) = (gint16) CLAMP(v, min, max);
    }
}


/**************************************************************************
 *
 *  effect_8bit
 *
 **************************************************************************/

void
effect_8bit(gint max, gint min, guint length, gint8 * sdata,
            SoftVolumeConfig * c)
{
    guint i;
    gint v;

    for (i = 0; i < (length >> 1); ++i) {   /* ie. length/2 */
        v = (gint) ((*sdata * c->volume_left) / 100);
        *(sdata++) = (gint8) CLAMP(v, min, max);

        v = (gint) ((*sdata * c->volume_right) / 100);
        *(sdata++) = (gint8) CLAMP(v, min, max);
    }
}


/**************************************************************************
 *
 *  soft_volume_effect
 *
 **************************************************************************/

void
soft_volume_effect(SoftVolumeConfig * c, gpointer data, AFormat format,
                   gint length)
{
    if (!c)
        return;

    if (c->volume_right == -1)
        c->volume_right = c->volume_left;

    switch (format) {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    case FMT_S16_LE:
    case FMT_U16_LE:
        break;

    case FMT_U16_NE:
    case FMT_U16_BE:
        effect_16bit(65535, 0, length, data, c);
        break;

    case FMT_S16_BE:
    case FMT_S16_NE:
        effect_16bit(32767, -32768, length, data, c);
        break;
#else
    case FMT_S16_BE:
    case FMT_U16_BE:
        break;

    case FMT_U16_LE:
    case FMT_U16_NE:
        effect_16bit(65535, 0, length, data, c);
        break;

    case FMT_S16_LE:
    case FMT_S16_NE:
        effect_16bit(32767, -32768, length, data, c);
        break;
#endif

    case FMT_U8:
        effect_8bit(255, 0, length, data, c);
        break;

    case FMT_S8:
        effect_8bit(127, -128, length, data, c);
        break;
    }
}
