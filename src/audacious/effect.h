/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
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

#ifndef EFFECT_H
#define EFFECT_H

#include <glib.h>

typedef struct _EffectPluginData EffectPluginData;

struct _EffectPluginData {
    GList *effect_list;
    GList *enabled_list;
};

GList *get_effect_list(void);
void effect_about(gint i);
void effect_configure(gint i);
GList *get_effect_enabled_list(void);
void enable_effect_plugin(gint i, gboolean enable);
gboolean effect_enabled(gint i);
gchar *effect_stringify_enabled_list(void);
void effect_enable_from_stringified_list(const gchar * list);

extern EffectPluginData ep_data;

#endif
