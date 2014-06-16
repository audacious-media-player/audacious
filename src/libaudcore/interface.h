/*
 * interface.h
 * Copyright 2014 John Lindgren
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

#ifndef LIBAUDCORE_INTERFACE_H
#define LIBAUDCORE_INTERFACE_H

#include <libaudcore/core.h>

enum {
 AUD_MENU_MAIN,
 AUD_MENU_PLAYLIST,
 AUD_MENU_PLAYLIST_ADD,
 AUD_MENU_PLAYLIST_REMOVE,
 AUD_MENU_COUNT};

enum {
 AUD_VIS_TYPE_CLEAR,        /* for VisClearFunc */
 AUD_VIS_TYPE_MONO_PCM,     /* for VisMonoPCMFunc */
 AUD_VIS_TYPE_MULTI_PCM,    /* for VisMultiPCMFunc */
 AUD_VIS_TYPE_FREQ,         /* for VisFreqFunc */
 AUD_VIS_TYPES};

/* generic type */
typedef void (* VisFunc) (void);

void aud_ui_show (bool_t show);
bool_t aud_ui_is_shown (void);

void aud_ui_show_error (const char * message);  /* thread-safe */

void aud_ui_show_about_window (void);
void aud_ui_hide_about_window (void);
void aud_ui_show_filebrowser (bool_t open);
void aud_ui_hide_filebrowser (void);
void aud_ui_show_jump_to_song (void);
void aud_ui_hide_jump_to_song (void);
void aud_ui_show_prefs_window (void);
void aud_ui_hide_prefs_window (void);

void aud_plugin_menu_add (int id, void (* func) (void), const char * name, const char * icon);
void aud_plugin_menu_remove (int id, void (* func) (void));

void aud_vis_func_add (int type, VisFunc func);
void aud_vis_func_remove (VisFunc func);

#endif
