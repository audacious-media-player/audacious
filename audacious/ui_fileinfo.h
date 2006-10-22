/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _UI_FILEINFO_H_
#define _UI_FILEINFO_H_

void create_fileinfo_window(void);
void create_filepopup_window(void);
void fileinfo_show_for_tuple(TitleInput *tuple);
void filepopup_show_for_tuple(TitleInput *tuple);
gchar* fileinfo_recursive_get_image(const gchar* path, const gchar* file_name, gint depth);
void fileinfo_show_for_path(gchar *path);

void filepopup_hide(gpointer unused);

#endif
