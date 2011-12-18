/*
 * visualization.h
 * Copyright 2010-2011 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef AUDACIOUS_VISUALIZATION_H
#define AUDACIOUS_VISUALIZATION_H

#include "plugins.h"

void vis_send_clear (void);
void vis_send_audio (const float * data, int channels);

void vis_init (void);
void vis_cleanup (void);

bool_t vis_plugin_start (PluginHandle * plugin);
void vis_plugin_stop (PluginHandle * plugin);

PluginHandle * vis_plugin_by_widget (/* GtkWidget * */ void * widget);

#endif
