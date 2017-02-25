/*
 * libaudgui.h
 * Copyright 2007-2013 Giacomo Lozito, Tomasz Moń, and John Lindgren
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

#ifndef LIBAUDGUI_H
#define LIBAUDGUI_H

#include <libaudcore/index.h>
#include <libaudcore/objects.h>

enum class AudMenuID;
enum class PluginType;
class Playlist;
class PluginHandle;
struct EqualizerPreset;

/* about.c */
void audgui_show_about_window ();
void audgui_hide_about_window ();

/* confirm.c */
void audgui_confirm_playlist_delete (Playlist playlist);
void audgui_show_playlist_rename (Playlist playlist);

/* eq-preset.c */
void audgui_show_eq_preset_window ();
void audgui_import_eq_presets (const Index<EqualizerPreset> & presets);

/* equalizer.c */
void audgui_show_equalizer_window ();
void audgui_hide_equalizer_window ();

/* infopopup.c */
void audgui_infopopup_show (Playlist playlist, int entry);
void audgui_infopopup_show_current ();
void audgui_infopopup_hide ();

/* file-opener.c */
void audgui_run_filebrowser (bool open);
void audgui_hide_filebrowser ();

/* infowin.c */
void audgui_infowin_show (Playlist playlist, int entry);
void audgui_infowin_show_current ();
void audgui_infowin_hide ();

/* init.c */
void audgui_init ();
void audgui_cleanup ();

/* jump-to-time.c */
void audgui_jump_to_time ();

/* jump-to-track.c */
void audgui_jump_to_track ();
void audgui_jump_to_track_hide ();

/* playlists.c */
void audgui_import_playlist ();
void audgui_export_playlist ();

/* plugin-menu.c */
void audgui_plugin_menu_add (AudMenuID id, void (* func) (), const char * name, const char * icon);
void audgui_plugin_menu_remove (AudMenuID id, void (* func) ());

/* plugin-prefs.c */
void audgui_show_plugin_about (PluginHandle * plugin);
void audgui_show_plugin_prefs (PluginHandle * plugin);

/* prefs-window.c */
void audgui_show_prefs_window ();
void audgui_show_prefs_for_plugin_type (PluginType type);
void audgui_hide_prefs_window ();

/* queue-manager.c */
void audgui_queue_manager_show ();

/* urilist.c */
void audgui_urilist_open (const char * list);
void audgui_urilist_insert (Playlist playlist, int position, const char * list);
Index<char> audgui_urilist_create_from_selected (Playlist playlist);

/* url-opener.c */
void audgui_show_add_url_window (bool open);

#endif /* LIBAUDGUI_H */
