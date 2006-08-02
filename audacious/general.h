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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */

#ifndef GENERIC_H
#define GENERIC_H

#include <glib.h>

typedef struct _GeneralPluginData GeneralPluginData;

struct _GeneralPluginData {
    GList *general_list;
    GList *enabled_list;
};

GList *get_general_list(void);
GList *get_general_enabled_list(void);
void enable_general_plugin(gint i, gboolean enable);
void general_about(gint i);
void general_configure(gint i);
gboolean general_enabled(gint i);
gchar *general_stringify_enabled_list(void);
void general_enable_from_stringified_list(const gchar * list);

extern GeneralPluginData gp_data;

#endif
