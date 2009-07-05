/*  Audacious
 *  Copyright (C) 2005-2007  Audacious development team.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "audstrings.h"

#include <glib/gi18n.h>
#include <string.h>
#include <ctype.h>

#ifdef USE_CHARDET
#include "../libguess/libguess.h"
#  ifdef HAVE_UDET
#    include <libudet_c.h>
#  endif
#endif

#include "main.h"

gchar *
cd_chardet_to_utf8(const gchar *str, gssize len,
                       gsize *arg_bytes_read, gsize *arg_bytes_write,
					   GError **arg_error)
{
#ifdef USE_CHARDET
	char  *det = NULL, *encoding = NULL;
#endif
	gchar *ret = NULL;
	gsize *bytes_read, *bytes_write;
	GError **error;
	gsize my_bytes_read, my_bytes_write;

	bytes_read  = arg_bytes_read ? arg_bytes_read : &my_bytes_read;
	bytes_write = arg_bytes_write ? arg_bytes_write : &my_bytes_write;
	error       = arg_error ? arg_error : NULL;

	g_return_val_if_fail(str != NULL, NULL);

#ifdef USE_CHARDET
	if(cfg.chardet_detector)
		det = cfg.chardet_detector;

	guess_init();

	if(det){
		encoding = (char *) guess_encoding(str, strlen(str), det);
		if (!encoding)
			goto fallback;

		ret = g_convert(str, len, "UTF-8", encoding, bytes_read, bytes_write, error);
	}

fallback:
#endif
	if(!ret && cfg.chardet_fallback){
		gchar **encs=NULL, **enc=NULL;
		encs = g_strsplit_set(cfg.chardet_fallback, " ,:;|/", 0);

		if(encs){
			enc = encs;
			for(enc=encs; *enc ; enc++){
				ret = g_convert(str, len, "UTF-8", *enc, bytes_read, bytes_write, error);
				if(len == *bytes_read){
					break;
				}
			}
			g_strfreev(encs);
		}
	}

	if(!ret){
		ret = g_convert(str, len, "UTF-8", "ISO-8859-1", bytes_read, bytes_write, error);
	}

	if(ret){
		if(g_utf8_validate(ret, -1, NULL))
			return ret;
		else {
			g_free(ret);
			ret = NULL;
		}
	}
	
	return NULL;	/* if I have no idea, return NULL. */
}

void chardet_init(void)
{
	chardet_to_utf8 = cd_chardet_to_utf8;
}
