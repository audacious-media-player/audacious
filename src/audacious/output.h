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

#ifndef AUDACIOUS_OUTPUT_H
#define AUDACIOUS_OUTPUT_H

#include "plugin.h"

extern const struct OutputAPI output_api;

void output_get_volume(int * l, int * r);
void output_set_volume(int l, int r);

int get_output_time (void);
int get_raw_output_time (void);
void output_drain (void);

PluginHandle * output_plugin_probe (void);
PluginHandle * output_plugin_get_current (void);
bool_t output_plugin_set_current (PluginHandle * plugin);

#endif /* AUDACIOUS_OUTPUT_H */
