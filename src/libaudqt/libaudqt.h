/*
 * libaudqt.h
 * Copyright 2014 William Pitcock
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

#ifndef LIBAUDQT_H
#define LIBAUDQT_H

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <libaudcore/i18n.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>
#include <libaudcore/plugin.h>
#include <libaudcore/plugins.h>

namespace audqt {

/* about.cc */
void aboutwindow_show ();
void aboutwindow_hide ();

/* playlist-management.cc */
void playlist_confirm_delete (int playlist);
void playlist_confirm_rename (int playlist);

/* equalizer.cc */
void equalizer_show ();
void equalizer_hide ();

/* fileopener.cc */
void fileopener_show (bool add);

/* util.cc */
void window_bring_to_front (QWidget * win);
void simple_message (const char * title, const char * text, const char * domain = nullptr);
const char * translate_str (const char * str, const char * domain = nullptr);

/* plugin-menu.cc */
QMenu * menu_get_by_id (int id);
void menu_add (int id, void (* func) (void), const char * name, const char * icon, const char * domain);
void menu_remove (int id, void (* func) (void));

/* prefs-builder.cc */
void prefs_populate (QLayout * layout, ArrayRef<const PreferencesWidget> widgets, const char * domain);

/* prefs-plugin.cc */
void plugin_about (PluginHandle * ph);
void plugin_prefs (PluginHandle * ph);

/* prefs-window.cc */
void prefswin_show ();
void prefswin_hide ();
void prefswin_show_page (int id, bool show = true);
void prefswin_show_plugin_page (int type);

};

#endif

