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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include "format.h"

gboolean
setup_format(AFormat fmt, gint rate, gint nch, format_t * format)
{
	format->bps = 0;
	format->fmt = fmt;
	format->is_8bit = FALSE;
	format->is_swapped = FALSE;
	format->is_unsigned = FALSE;

	/* check format */
	switch (fmt)
	{
		case FMT_U8:
			format->is_8bit = TRUE;
			format->is_unsigned = TRUE;
			break;
		case FMT_S8:
			format->is_8bit = TRUE;
			break;
		case FMT_U16_LE:
			format->is_unsigned = TRUE;
#ifdef WORDS_BIGENDIAN
			format->is_swapped = TRUE;
#endif
			break;
		case FMT_U16_BE:
			format->is_unsigned = TRUE;
#ifndef WORDS_BIGENDIAN
			format->is_swapped = TRUE;
#endif
			break;
		case FMT_U16_NE:
			format->is_unsigned = TRUE;
			break;
		case FMT_S16_LE:
#ifdef WORDS_BIGENDIAN
			format->is_swapped = TRUE;
#endif
			break;
		case FMT_S16_BE:
#ifndef WORDS_BIGENDIAN
			format->is_swapped = TRUE;
#endif
			break;
		case FMT_S16_NE:
			break;
		default:
			DEBUG(("[crossfade] setup_format: unknown format (%d)!\n", fmt));
			return -1;
	}

	/* check rate */
	if ((rate < 1) || (rate > 65535))
	{
		DEBUG(("[crossfade] setup_format: illegal rate (%d)!\n", rate));
		return -1;
	}
	format->rate = rate;

	/* check channels */
	switch (nch)
	{
		case 1:
		case 2:
			format->nch = nch;
			break;
		default:
			DEBUG(("[crossfade] setup_format: illegal # of channels (%d)!\n", nch));
			return -1;
	}

	/* calculate bps */
	format->bps = rate * nch;
	if (!format->is_8bit)
		format->bps *= 2;

	return 0;
}

gboolean
format_match(AFormat fmt1, AFormat fmt2)
{
	if (fmt1 == fmt2)
		return TRUE;	/* exact match */

	if ((fmt2 == FMT_U16_NE) || (fmt2 == FMT_S16_NE))
	{
		AFormat fmt = fmt1;
		fmt1 = fmt2;
		fmt2 = fmt;
	}

#ifdef WORDS_BIGENDIAN
	if (((fmt1 == FMT_U16_NE) && (fmt2 == FMT_U16_BE)) || ((fmt1 == FMT_S16_NE) && (fmt2 == FMT_S16_BE)))
		return TRUE;
#else
	if (((fmt1 == FMT_U16_NE) && (fmt2 == FMT_U16_LE)) || ((fmt1 == FMT_S16_NE) && (fmt2 == FMT_S16_LE)))
		return TRUE;
#endif

	return FALSE;		/* no match */
}

gchar *
format_name(AFormat fmt)
{
	switch (fmt)
	{
		case FMT_U8:
			return "FMT_U8";
		case FMT_S8:
			return "FMT_S8";
		case FMT_U16_LE:
			return "FMT_U16_LE";
		case FMT_U16_BE:
			return "FMT_U16_BE";
		case FMT_U16_NE:
			return "FMT_U16_NE";
		case FMT_S16_LE:
			return "FMT_S16_LE";
		case FMT_S16_BE:
			return "FMT_S16_BE";
		case FMT_S16_NE:
			return "FMT_S16_NE";
	}
	return "UNKNOWN";
}

void
format_copy(format_t * dest, format_t * src)
{
	memcpy(dest, src, sizeof(format_t));
}
