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

#ifndef AUDACIOUS_GENERAL_H
#define AUDACIOUS_GENERAL_H

#include <glib.h>

typedef struct _GeneralPluginData GeneralPluginData;

struct _GeneralPluginData {
    GList *general_list;
    GList *enabled_list;
};

GList *get_general_list(void);
GList *get_general_enabled_list(void);
void general_enable_plugin(GeneralPlugin *plugin, gboolean enable);
gchar *general_stringify_enabled_list(void);
void general_enable_from_stringified_list(const gchar * list);

extern GeneralPluginData gp_data;

#endif /* AUDACIOUS_GENERAL_H */
