/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */

#ifndef EQUALIZER_H
#define EQUALIZER_H

#include <glib.h>
#include <gtk/gtk.h>

#include "pbutton.h"

#define EQUALIZER_HEIGHT         (gint)(cfg.equalizer_shaded ? 14 : 116)
#define EQUALIZER_WIDTH          (gint)275

#define EQUALIZER_DEFAULT_POS_X  20
#define EQUALIZER_DEFAULT_POS_Y  136

#define EQUALIZER_DEFAULT_DIR_PRESET "dir_default.preset"
#define EQUALIZER_DEFAULT_PRESET_EXT "preset"

void equalizerwin_set_shade_menu_cb(gboolean shaded);
void draw_equalizer_window(gboolean force);
void equalizerwin_create(void);
void equalizerwin_show(gboolean show);
void equalizerwin_real_show(void);
void equalizerwin_real_hide(void);
void equalizerwin_load_auto_preset(const gchar * filename);
void equalizerwin_set_volume_slider(gint percent);
void equalizerwin_set_balance_slider(gint percent);
void equalizerwin_eq_changed(void);
void equalizerwin_set_preamp(gfloat preamp);
void equalizerwin_set_band(gint band, gfloat value);
gfloat equalizerwin_get_preamp(void);
gfloat equalizerwin_get_band(gint band);

gboolean equalizerwin_has_focus(void);

extern GtkWidget *equalizerwin;
extern PButton *equalizerwin_close;
extern gboolean equalizerwin_focus;

#endif
