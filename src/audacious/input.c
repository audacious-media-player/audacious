/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include <mowgli.h>

#include "input.h"
#include "output.h"
#include "plugin-registry.h"

PlaybackData ip_data =
{
    NULL,
    FALSE,
    FALSE,
    FALSE,
    NULL
};

InputPlayback *
get_current_input_playback(void)
{
    return ip_data.current_input_playback;
}

void
set_current_input_playback(InputPlayback * ip)
{
    ip_data.current_input_playback = ip;
}

static gboolean plugin_list_func (void * plugin, void * data)
{
    GList * * list = data;

    * list = g_list_prepend (* list, plugin);
    return TRUE;
}

GList * get_input_list (void)
{
    static GList * list = NULL;

    if (list == NULL)
        plugin_for_each (PLUGIN_TYPE_INPUT, plugin_list_func, & list);

    return list;
}

void
input_get_volume(gint * l, gint * r)
{
    if (ip_data.playing && ip_data.current_input_playback &&
     ip_data.current_input_playback->plugin->get_volume &&
     ip_data.current_input_playback->plugin->get_volume (l, r))
        return;

    output_get_volume (l, r);
}

void
input_set_volume(gint l, gint r)
{
    gint h_vol[2];

    h_vol[0] = l;
    h_vol[1] = r;
    hook_call("volume set", h_vol);

    if (ip_data.playing && ip_data.current_input_playback &&
     ip_data.current_input_playback->plugin->set_volume &&
     ip_data.current_input_playback->plugin->set_volume (l, r))
        return;

    output_set_volume (l, r);
}
