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

#ifndef _I_FILEINFO_H
#define _I_FILEINFO_H 1

#include "i_common.h"
#include <gtk/gtk.h>
#include <audacious/util.h>


typedef struct
{
  GtkWidget * fileinfo_win;
}
amidiplug_gui_fileinfo_t;

void i_fileinfo_gui( gchar * );


#endif /* !_I_FILEINFO_H */
