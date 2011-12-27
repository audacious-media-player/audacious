/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2011  Audacious development team
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

#ifndef AUDACIOUS_UTIL_H
#define AUDACIOUS_UTIL_H

#include <sys/types.h>
#include <libaudcore/core.h>

typedef bool_t(*DirForeachFunc) (const char * path,
                                   const char * basename,
                                   gpointer user_data);

bool_t dir_foreach (const char * path, DirForeachFunc func, void * user_data);

int file_get_mtime (const char * filename);
void make_directory(const char * path, mode_t mode);
char * write_temp_file (void * data, int64_t len);

char * get_path_to_self (void);

void describe_song (const char * filename, const Tuple * tuple,
 char * * title, char * * artist, char * * album);

#endif /* AUDACIOUS_UTIL_H */
