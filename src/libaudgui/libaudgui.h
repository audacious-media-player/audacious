/*
 * libaudgui.h
 * Copyright 2007-2009 Giacomo Lozito and Tomasz Moń

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#ifndef LIBAUDGUI_H
#define LIBAUDGUI_H

#include <libaudcore/core.h>

/* macro defines for Audacious stock icons */
#define AUD_STOCK_PLAYLIST    "aud-playlist"
#define AUD_STOCK_PLUGIN      "aud-plugin"
#define AUD_STOCK_QUEUETOGGLE "aud-queuetoggle"
#define AUD_STOCK_RANDOMIZEPL "aud-randomizepl"

void audgui_register_stock_icons(void);

void audgui_show_add_url_window(bool_t open);

void audgui_jump_to_track(void);
void audgui_jump_to_track_hide(void);

void audgui_run_filebrowser(bool_t clear_pl_on_ok);
void audgui_hide_filebrowser(void);

/* about.c */
void audgui_show_about_window (void);

/* confirm.c */
void audgui_confirm_playlist_delete (int playlist);
void audgui_show_playlist_rename (int playlist);

/* equalizer.c */
void audgui_show_equalizer_window (void);
void audgui_hide_equalizer_window (void);

/* infopopup.c */
void audgui_infopopup_show (int playlist, int entry);
void audgui_infopopup_show_current (void);
void audgui_infopopup_hide (void);

/* infowin.c */
void audgui_infowin_show (int playlist, int entry);
void audgui_infowin_show_current (void);

/* jump-to-time.c */
void audgui_jump_to_time (void);

/* playlists.c */
void audgui_import_playlist (void);
void audgui_export_playlist (void);

/* queue-manager.c */
void audgui_queue_manager_show (void);

/* ui_playlist_manager.c */
void audgui_playlist_manager (void);

/* urilist.c */
void audgui_urilist_open (const char * list);
void audgui_urilist_insert (int playlist, int position, const char * list);
char * audgui_urilist_create_from_selected (int playlist);

/* util.c */
void audgui_set_default_icon (void);

#endif /* LIBAUDGUI_H */
