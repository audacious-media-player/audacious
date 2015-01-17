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

#include <QString>
#include <libaudcore/objects.h>

class QBoxLayout;
class QPixmap;
class QWidget;

enum class PluginType;
class PluginHandle;
struct PreferencesWidget;

namespace audqt {

enum class FileMode {
    Open,
    OpenFolder,
    Add,
    AddFolder,
    count
};

struct MenuItem;

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
void fileopener_show (FileMode mode);

/* util.cc */
void cleanup ();
void window_bring_to_front (QWidget * win);
void simple_message (const char * title, const char * text);
QString translate_str (const char * str, const char * domain);

#ifdef PACKAGE
static inline QString translate_str (const char * str)
    { return translate_str (str, PACKAGE); }
#endif

/* prefs-builder.cc */
void prefs_populate (QBoxLayout * layout, ArrayRef<PreferencesWidget> widgets, const char * domain);

/* prefs-plugin.cc */
void plugin_about (PluginHandle * ph);
void plugin_prefs (PluginHandle * ph);

/* prefs-window.cc */
void prefswin_show ();
void prefswin_hide ();
void prefswin_show_page (int id, bool show = true);
void prefswin_show_plugin_page (PluginType type);

/* log-inspector.cc */
void log_inspector_show ();
void log_inspector_hide ();

/* art.cc */
QPixmap art_request (const char * filename, unsigned int w = 256, unsigned int h = 256, bool want_hidpi = true);
QPixmap art_request_current (unsigned int w = 256, unsigned int h = 256, bool want_hidpi = true);

/* infowin.cc */
void infowin_show (int playlist, int entry);
void infowin_show_current ();
void infowin_hide ();

/* queue-manager.cc */
void queue_manager_show ();
void queue_manager_hide ();

} // namespace audqt

#endif
