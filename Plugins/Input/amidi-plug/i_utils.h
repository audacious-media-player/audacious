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

#ifndef _I_UTILS_H
#define _I_UTILS_H 1

#include "i_common.h"

#define AMIDIPLUG_MESSAGE_INFO	0
#define AMIDIPLUG_MESSAGE_WARN	1
#define AMIDIPLUG_MESSAGE_ERR	2


void i_about_gui( void );
gpointer i_message_gui( gchar * , gchar * , gint , gpointer , gboolean );

#endif /* !_I_UTILS_H */
