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

#include <libaudcore/visualizer.h>

enum class AudMenuID {
    Main,
    Playlist,
    PlaylistAdd,
    PlaylistRemove,
    count
};

void aud_ui_show (bool show);
bool aud_ui_is_shown ();

void aud_ui_startup_notify (const char * id);
void aud_ui_show_error (const char * message);  /* thread-safe */

void aud_ui_show_about_window ();
void aud_ui_hide_about_window ();
void aud_ui_show_filebrowser (bool open);
void aud_ui_hide_filebrowser ();
void aud_ui_show_jump_to_song ();
void aud_ui_hide_jump_to_song ();
void aud_ui_show_prefs_window ();
void aud_ui_hide_prefs_window ();

void aud_plugin_menu_add (AudMenuID id, void (* func) (), const char * name, const char * icon);
void aud_plugin_menu_remove (AudMenuID id, void (* func) ());

void aud_visualizer_add (Visualizer * vis);
void aud_visualizer_remove (Visualizer * vis);

#endif
