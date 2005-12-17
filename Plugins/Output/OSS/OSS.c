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

#include "OSS.h"

#include <glib.h>
#include <glib/gi18n.h>

OutputPlugin oss_op = {
    NULL,
    NULL,
    NULL,                       /* Description */
    oss_init,
    NULL,
    oss_about,
    oss_configure,
    oss_get_volume,
    oss_set_volume,
    oss_open,
    oss_write,
    oss_close,
    oss_flush,
    oss_pause,
    oss_free,
    oss_playing,
    oss_get_output_time,
    oss_get_written_time,
};

OutputPlugin *
get_oplugin_info(void)
{
    oss_op.description = g_strdup_printf(_("OSS Output Plugin"));
    return &oss_op;
}
