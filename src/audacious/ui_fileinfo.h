/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  If not, see <http://www.gnu.org/licenses>.
 */

#ifndef AUDACIOUS_UI_FILEINFO_H
#define AUDACIOUS_UI_FILEINFO_H

#include "tuple.h"
#include "plugin.h"
#include <glib.h>

void create_fileinfo_window(void);
gchar* fileinfo_recursive_get_image(const gchar* path, const gchar* file_name, gint depth);

void ui_fileinfo_show (gint playlist, gint entry);
void ui_fileinfo_show_current (void);

#endif /* AUDACIOUS_UI_FILEINFO_H */
