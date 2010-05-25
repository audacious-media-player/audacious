/*
 * probe.h
 * Copyright 2009-2010 John Lindgren
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

#ifndef AUDACIOUS_PROBE_H
#define AUDACIOUS_PROBE_H

#include "plugin.h"

InputPlugin * file_find_decoder (const gchar * filename, gboolean fast);
Tuple * file_read_tuple (const gchar * filename, InputPlugin * decoder);
gboolean file_read_image (const gchar * filename, InputPlugin * decoder,
 void * * data, gint * size);
gboolean file_can_write_tuple (const gchar * filename, InputPlugin * decoder);
gboolean file_write_tuple (const gchar * filename, InputPlugin * decoder,
 Tuple * tuple);

gboolean custom_infowin (const gchar * filename, InputPlugin * decoder);

#endif
