/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team.
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

#ifndef AUDACIOUS_LOGGER_H
#define AUDACIOUS_LOGGER_H

#include <glib.h>

G_BEGIN_DECLS

#define BMP_LOGGER_DEFAULT_LOG_LEVEL  G_LOG_LEVEL_MESSAGE

/* default log file max size: 512kb */
#define BMP_LOGGER_FILE_MAX_SIZE      ((size_t)1 << 19)


gboolean aud_logger_start(const gchar * filename);
void aud_logger_stop(void);

G_END_DECLS

#endif /* AUDACIOUS_LOGGER_H */
