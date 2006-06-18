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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef PLAYLISTWIN_H
#define PLAYLISTWIN_H

#include <glib.h>

#include "mainwin.h"
#include "pbutton.h"
#include "playlist_list.h"


#define PLAYLISTWIN_FRAME_TOP_HEIGHT    20
#define PLAYLISTWIN_FRAME_BOTTOM_HEIGHT 38
#define PLAYLISTWIN_FRAME_LEFT_WIDTH    12
#define PLAYLISTWIN_FRAME_RIGHT_WIDTH   19

#define PLAYLISTWIN_MIN_WIDTH           MAINWIN_WIDTH
#define PLAYLISTWIN_MIN_HEIGHT          MAINWIN_HEIGHT
#define PLAYLISTWIN_WIDTH_SNAP          25
#define PLAYLISTWIN_HEIGHT_SNAP         29
#define PLAYLISTWIN_SHADED_HEIGHT       MAINWIN_SHADED_HEIGHT
#define PLAYLISTWIN_WIDTH               cfg.playlist_width
#define PLAYLISTWIN_HEIGHT \
    (cfg.playlist_shaded ? PLAYLISTWIN_SHADED_HEIGHT : cfg.playlist_height)

#define PLAYLISTWIN_DEFAULT_WIDTH       275
#define PLAYLISTWIN_DEFAULT_HEIGHT      232
#define PLAYLISTWIN_DEFAULT_POS_X       295
#define PLAYLISTWIN_DEFAULT_POS_Y       20

#define PLAYLISTWIN_DEFAULT_FONT        "Sans Bold 8"

gboolean playlistwin_is_shaded(void);
void playlistwin_update_list(void);
gboolean playlistwin_item_visible(gint index);
gint playlistwin_get_toprow(void);
void playlistwin_set_toprow(gint top);
void playlistwin_set_shade_menu_cb(gboolean shaded);
void playlistwin_set_shade(gboolean shaded);
void playlistwin_shade_toggle(void);
void playlistwin_create(void);
void draw_playlist_window(gboolean force);
void playlistwin_hide_timer(void);
void playlistwin_set_time(gint time, gint length, TimerMode mode);
void playlistwin_show(void);
void playlistwin_hide(void);
void playlistwin_set_back_pixmap(void);
void playlistwin_scroll(gint num);
void playlistwin_scroll_up_pushed(void);
void playlistwin_scroll_down_pushed(void);
void playlistwin_select_playlist_to_load(const gchar * default_filename);

extern GtkWidget *playlistwin;
extern PlayList_List *playlistwin_list;

extern PButton *playlistwin_shade, *playlistwin_close;

extern gboolean playlistwin_focus;

#endif
