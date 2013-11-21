/*
 * ui_preferences.h
 * Copyright 2006-2010 William Pitcock, Tomasz Moń, and John Lindgren
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

#ifndef AUDACIOUS_UI_PREFERENCES_H
#define AUDACIOUS_UI_PREFERENCES_H

#include <gtk/gtk.h>

#include "preferences.h"

void show_prefs_window(void);
void hide_prefs_window(void);

/* plugin-preferences.c */
void plugin_make_about_window (PluginHandle * plugin);
void plugin_make_config_window (PluginHandle * plugin);
void plugin_misc_cleanup (PluginHandle * plugin);

/* plugin-view.c */
GtkWidget * plugin_view_new (int type);

#endif /* AUDACIOUS_UI_PREFERENCES_H */
