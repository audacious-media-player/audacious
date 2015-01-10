/*
 * libaudgui-gtk.h
 * Copyright 2010-2012 John Lindgren
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

#ifndef LIBAUDGUI_GTK_H
#define LIBAUDGUI_GTK_H

#include <stdint.h>
#include <gtk/gtk.h>

#include <libaudcore/objects.h>

#define audgui_create_widgets(b, w) audgui_create_widgets_with_domain (b, w, PACKAGE)

enum class AudMenuID;
struct PreferencesWidget;

typedef void (* AudguiCallback) (void * data);

/* pixbufs.c */
GdkPixbuf * audgui_pixbuf_from_data (const void * data, int64_t size);
GdkPixbuf * audgui_pixbuf_fallback ();
void audgui_pixbuf_scale_within (GdkPixbuf * * pixbuf, int size);
GdkPixbuf * audgui_pixbuf_request (const char * filename, bool * queued = nullptr);
GdkPixbuf * audgui_pixbuf_request_current (bool * queued = nullptr);

/* plugin-menu.c */
GtkWidget * audgui_get_plugin_menu (AudMenuID id);

/* prefs-widget.c */
void audgui_create_widgets_with_domain (GtkWidget * box,
 ArrayRef<PreferencesWidget> widgets, const char * domain);

/* scaled-image.c -- okay to use without audgui_init() */
GtkWidget * audgui_scaled_image_new (GdkPixbuf * pixbuf);
void audgui_scaled_image_set (GtkWidget * widget, GdkPixbuf * pixbuf);

/* util.c -- okay to use without audgui_init() */
int audgui_get_digit_width (GtkWidget * widget);
void audgui_get_mouse_coords (GtkWidget * widget, int * x, int * y);
void audgui_get_mouse_coords (GdkScreen * screen, int * x, int * y);
void audgui_get_monitor_geometry (GdkScreen * screen, int x, int y, GdkRectangle * geom);
void audgui_destroy_on_escape (GtkWidget * widget);
void audgui_simple_message (GtkWidget * * widget, GtkMessageType type,
 const char * title, const char * text);

GtkWidget * audgui_button_new (const char * text, const char * icon,
 AudguiCallback callback, void * data);

GtkWidget * audgui_dialog_new (GtkMessageType type, const char * title,
 const char * text, GtkWidget * button1, GtkWidget * button2);
void audgui_dialog_add_widget (GtkWidget * dialog, GtkWidget * widget);

#endif
