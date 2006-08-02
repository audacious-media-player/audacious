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
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*
*/

#ifndef _I_PCFG_H
#define _I_PCFG_H 1

#include <glib.h>

typedef void pcfg_t;

pcfg_t * i_pcfg_new( void );
pcfg_t * i_pcfg_new_from_file( gchar * );
void i_pcfg_free( pcfg_t * );
gboolean i_pcfg_write_to_file( pcfg_t * , gchar * );
gboolean i_pcfg_read_string( pcfg_t * , const gchar * , const gchar * , gchar ** , gchar * );
gboolean i_pcfg_read_integer( pcfg_t * , const gchar * , const gchar * , gint * , gint );
gboolean i_pcfg_read_boolean( pcfg_t * , const gchar * , const gchar * , gboolean * , gboolean );
gboolean i_pcfg_write_string( pcfg_t * , const gchar * , const gchar * , gchar * );
gboolean i_pcfg_write_integer( pcfg_t * , const gchar * , const gchar * , gint );
gboolean i_pcfg_write_boolean( pcfg_t * , const gchar * , const gchar * , gboolean );

#endif /* !_I_PCFG_H */
