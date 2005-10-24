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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef SOFTVOLUME_H
#define SOFTVOLUME_H

#include "plugin.h"             /* glib and AFomat definition */

struct _SoftVolumeConfig {
    gboolean enabled;
    gint volume_left;
    gint volume_right;
};

typedef struct _SoftVolumeConfig SoftVolumeConfig;

/**************************************************************************
 *
 *   Functions to read/write a particular soft volume configuration.
 *   If section is NULL, the global ("xmms") section is used.
 *
 **************************************************************************/

void soft_volume_load(const gchar * section, SoftVolumeConfig * c);

void soft_volume_save(SoftVolumeConfig * c, const gchar * section);


/**************************************************************************
 *
 *   Functions to set the volume and get the current volume
 *
 **************************************************************************/

void soft_volume_set(SoftVolumeConfig * c, gint l, gint r);

void soft_volume_get(SoftVolumeConfig * c, gint * l, gint * r);


/**************************************************************************
 *
 *   Modify the buffer according to volume settings
 *
 **************************************************************************/

void soft_volume_effect(SoftVolumeConfig * c,
                        gpointer data, AFormat format, gint length);

#endif
