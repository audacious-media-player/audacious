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
* 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307  USA
*
*/

#ifndef _I_CONFIGURE_PRIVATE_H
#define _I_CONFIGURE_PRIVATE_H 1

#include "i_common.h"
#include "i_backend.h"
#include "pcfg/i_pcfg.h"
#include <gtk/gtk.h>


typedef struct
{
  gpointer alsa; /* advanced linux sound architecture */
  gpointer fsyn; /* fluidsynth */
  gpointer dumm; /* dummy */
}
amidiplug_cfg_backend_t;


GtkWidget * i_configure_gui_draw_title( gchar * );
void i_configure_ev_browse_for_entry( GtkWidget * );

#endif /* !_I_CONFIGURE_PRIVATE_H */
