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

#include <gtk/gtk.h>
#include <libaudcore/core.h>

typedef struct _AudguiMenuItem {
    const char * name;
    const char * icon;
    unsigned key;
    GdkModifierType mod;

    /* for normal items */
    void (* func) (void);

    /* for toggle items */
    const char * csect;
    const char * cname;
    const char * hook;

    /* for submenus */
    const struct _AudguiMenuItem * items;
    int n_items;

    /* for custom submenus */
    GtkWidget * (* get_sub) (void);

    /* for separators */
    bool_t sep;
} AudguiMenuItem;

/* use NULL for domain to skip translation */
GtkWidget * audgui_menu_item_new_with_domain (const AudguiMenuItem * item,
 GtkAccelGroup * accel, const char * domain);

void audgui_menu_init_with_domain (GtkWidget * shell,
 const AudguiMenuItem * items, int n_items, GtkAccelGroup * accel,
 const char * domain);

#define audgui_menu_item_new(i, a) audgui_menu_item_new_with_domain (i, a, PACKAGE)
#define audgui_menu_init(s, i, n, a) audgui_menu_init_with_domain (s, i, n, a, PACKAGE)

#endif /* AUDGUI_MENU_H */
