/*
 * init.h
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

#ifndef AUDGUI_INIT_H
#define AUDGUI_INIT_H

#include <gtk/gtk.h>

#include <audacious/api.h>

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

void audgui_show_unique_window (int id, GtkWidget * widget);
bool_t audgui_reshow_unique_window (int id);
void audgui_hide_unique_window (int id);

void audgui_init (AudAPITable * table, int version);
void audgui_cleanup (void);

/* pixbufs.c */
void audgui_pixbuf_uncache (void);

#endif /* AUDGUI_INIT_H */
