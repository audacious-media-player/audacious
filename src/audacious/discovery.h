/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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

#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <glib.h>

typedef struct _DiscoveryPluginData DiscoveryPluginData;

struct _DiscoveryPluginData {
    GList *discovery_list;
    GList *enabled_list;
};

GList * discovery_devices;
typedef struct _discovery_device_t discovery_device_t;
struct _discovery_device_t {
    gchar *device_name;     /*some kind of description*/
    gchar *device_address;  /*some kind of device ID*/
    GList *device_playlist; /*contains all the songs on the device*/
};


GList *get_discovery_list(void);
GList *get_discovery_enabled_list(void);
void enable_discovery_plugin(gint i, gboolean enable);
gboolean discovery_enabled(gint i);
gchar *discovery_stringify_enabled_list(void);
void discovery_enable_from_stringified_list(const gchar * list);

extern DiscoveryPluginData dp_data;

#endif
