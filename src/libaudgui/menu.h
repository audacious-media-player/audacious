/*
 * menu.h
 * Copyright 2011-2014 John Lindgren
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

#ifndef AUDGUI_MENU_H
#define AUDGUI_MENU_H

/* okay to use without audgui_init() */

#include <gtk/gtk.h>
#include <libaudcore/objects.h>

struct AudguiMenuItem {
    const char * name;
    const char * icon;
    unsigned key;
    GdkModifierType mod;

    /* for normal items */
    void (* func) ();

    /* for toggle items */
    const char * csect;
    const char * cname;
    const char * hook;

    /* for submenus */
    ArrayRef<AudguiMenuItem> items;

    /* for custom submenus */
    GtkWidget * (* get_sub) ();

    /* for separators */
    bool sep;
};

constexpr AudguiMenuItem MenuCommand (const char * name, const char * icon,
 unsigned key, GdkModifierType mod, void (* func) ())
    { return {name, icon, key, mod, func}; }

constexpr AudguiMenuItem MenuToggle (const char * name, const char * icon,
 unsigned key, GdkModifierType mod, const char * csect, const char * cname,
 void (* func) () = 0, const char * hook = 0)
    { return {name, icon, key, mod, func, csect, cname, hook}; }

constexpr AudguiMenuItem MenuSub (const char * name, const char * icon,
 ArrayRef<AudguiMenuItem> items)
    { return {name, icon, 0, (GdkModifierType) 0, 0, 0, 0, 0, items}; }

constexpr AudguiMenuItem MenuSub (const char * name, const char * icon,
 GtkWidget * (* get_sub) ())
    { return {name, icon, 0, (GdkModifierType) 0, 0, 0, 0, 0, 0, get_sub}; }

constexpr AudguiMenuItem MenuSep ()
    { return {0, 0, 0, (GdkModifierType) 0, 0, 0, 0, 0, 0, 0, true}; }

/* use nullptr for domain to skip translation */
GtkWidget * audgui_menu_item_new_with_domain (const AudguiMenuItem * item,
 GtkAccelGroup * accel, const char * domain);

void audgui_menu_init_with_domain (GtkWidget * shell,
 ArrayRef<AudguiMenuItem> items, GtkAccelGroup * accel,
 const char * domain);

#define audgui_menu_item_new(i, a) audgui_menu_item_new_with_domain (i, a, PACKAGE)
#define audgui_menu_init(s, i, a) audgui_menu_init_with_domain (s, i, a, PACKAGE)

#endif /* AUDGUI_MENU_H */
