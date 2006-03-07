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

#ifndef _I_CONFIGURE_H
#define _I_CONFIGURE_H 1

#include "i_common.h"
#include <gtk/gtk.h>
#include "libaudacious/configfile.h"


enum
{
  LISTPORT_TOGGLE_COLUMN,
  LISTPORT_PORTNUM_COLUMN,
  LISTPORT_CLIENTNAME_COLUMN,
  LISTPORT_PORTNAME_COLUMN,
  LISTPORT_POINTER_COLUMN,
  LISTPORT_N_COLUMNS
};

enum
{
  LISTMIXER_DESC_COLUMN,
  LISTMIXER_CARDID_COLUMN,
  LISTMIXER_MIXCTLID_COLUMN,
  LISTMIXER_MIXCTLNAME_COLUMN,
  LISTMIXER_N_COLUMNS
};

typedef struct
{
  gchar * seq_writable_ports;
  gint mixer_card_id;
  gchar * mixer_control_name;
  gint mixer_control_id;
  gint length_precalc_enable;
}
amidiplug_cfg_t;

extern amidiplug_cfg_t amidiplug_cfg;

typedef struct
{
  GtkWidget * config_win;
  GtkWidget * port_treeview;
  GtkWidget * mixercard_combo;
  GtkWidget * precalc_checkbt;
  GtkTooltips * config_tips;
}
amidiplug_gui_prefs_t;

static amidiplug_gui_prefs_t amidiplug_gui_prefs = { NULL , NULL , NULL , NULL , NULL };

void i_configure_gui( GSList * , GSList * );
void i_configure_ev_destroy( void );
void i_configure_ev_bcancel( void );
void i_configure_ev_bok( void );
void i_configure_cfg_save( void );
void i_configure_cfg_read( void );


#endif /* !_I_CONFIGURE_H */
