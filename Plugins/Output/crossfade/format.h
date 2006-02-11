/*
 *  XMMS Crossfade Plugin
 *  Copyright (C) 2000-2004  Peter Eisenlohr <peter@eisenlohr.org>
 *
 *  based on the original OSS Output Plugin
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

/*
 *  Verify format and setup a struct format_t
 *  Returns -1 if an illegal format has been specified
 */

#ifndef __FORMAT_H__
#define __FORMAT_H__

#include "crossfade.h"

typedef struct {
	AFormat  fmt;
	gint     rate;
	gint     nch;

	gint     bps;             /* format is valid if (bps > 0) */
	gboolean is_8bit;
	gboolean is_swapped;
	gboolean is_unsigned;
}
format_t;

gboolean setup_format(AFormat   fmt,
		      gint      rate,
		      gint      nch,
		      format_t *format);

gboolean format_match(AFormat fmt1, AFormat fmt2);
gchar *  format_name (AFormat fmt);

void format_copy(format_t *dest, format_t *src);

#endif  /* _FORMAT_H_ */
