/*
 * internal.h
 * Copyright 2010-2013 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#ifndef AUDGUI_INTERNAL_H
#define AUDGUI_INTERNAL_H

#include <gtk/gtk.h>
#include <libaudcore/core.h>

enum {
    AUDGUI_ABOUT_WINDOW,
    AUDGUI_EQUALIZER_WINDOW,
    AUDGUI_FILEBROWSER_WINDOW,
    AUDGUI_INFOPOPUP_WINDOW,
    AUDGUI_INFO_WINDOW,
    AUDGUI_JUMP_TO_TIME_WINDOW,
    AUDGUI_JUMP_TO_TRACK_WINDOW,
    AUDGUI_PLAYLIST_EXPORT_WINDOW,
    AUDGUI_PLAYLIST_IMPORT_WINDOW,
    AUDGUI_PLAYLIST_MANAGER_WINDOW,
    AUDGUI_QUEUE_MANAGER_WINDOW,
    AUDGUI_URL_OPENER_WINDOW,
    AUDGUI_NUM_UNIQUE_WINDOWS
};

#ifdef __cplusplus
extern "C" {
#endif

void audgui_show_unique_window (int id, GtkWidget * widget);
bool_t audgui_reshow_unique_window (int id);
void audgui_hide_unique_window (int id);

/* pixbufs.c */
void audgui_pixbuf_uncache (void);

/* plugin-menu.c */
void plugin_menu_cleanup (void);

/* plugin-prefs.c */
void plugin_prefs_cleanup (void);

/* plugin-view.c */
GtkWidget * plugin_view_new (int type);

/* status.c */
void status_init (void);
void status_cleanup (void);

#ifdef __cplusplus
}
#endif

#endif /* AUDGUI_INTERNAL_H */
