/*
*
* Author: Giacomo Lozito <james@develia.org>, (C) 2005-2006
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
*/

#ifndef _I_COMMON_H
#define _I_COMMON_H 1


#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <glib.h>

#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)

#define WARNANDBREAK(...) { g_warning(__VA_ARGS__); break; }
#define WARNANDBREAKANDPLAYERR(...) { amidiplug_playing_status = AMIDIPLUG_ERR; g_warning(__VA_ARGS__); break; }

#ifdef DEBUG
#define DEBUGMSG(...) { fprintf(stderr, "amidi-plug(%s:%s:%d): ", __FILE__, __FUNCTION__, (int) __LINE__); fprintf(stderr, __VA_ARGS__); }
#else
#define DEBUGMSG(...)
#endif /* DEBUG */


#define AMIDIPLUG_VERSION "0.1a"
#define PLAYER_NAME "Audacious"
#define G_PATH_GET_BASENAME(x) g_path_get_basename(x)
#define G_STRING_PRINTF(...) g_string_printf(__VA_ARGS__)

/* multi-purpose data bucket */
typedef struct
{
  gint bint[2];
  gchar * bcharp[2];
  gpointer bpointer[2];
}
data_bucket_t;

#endif /* !_I_COMMON_H */
