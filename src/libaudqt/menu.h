/*
 * menu.h
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

#include "libaudqt.h"

#ifndef LIBAUDQT_MENU_H
#define LIBAUDQT_MENU_H

#include <libaudcore/objects.h>

class QAction;
class QMenu;
class QMenuBar;
class QWidget;

enum class AudMenuID;

namespace audqt {

typedef void (* MenuFunc) ();

struct MenuItemText {
    const char * name;
    const char * icon;
    const char * shortcut;
};

struct MenuItemConfig {
    const char * sect;
    const char * name;
    const char * hook;
};

struct MenuItem {
    MenuItemText text;
    void (* func) ();

    /* for toggle items */
    MenuItemConfig cfg;

    /* for submenus */
    ArrayRef<MenuItem> items;

    /* for custom submenus */
    QMenu * (* submenu) ();

    /* for separators */
    bool sep;
};

constexpr MenuItem MenuCommand (MenuItemText text, MenuFunc func)
    { return {text, func}; }
constexpr MenuItem MenuToggle (MenuItemText text, MenuItemConfig cfg, MenuFunc func = nullptr)
    { return {text, func, cfg}; }
constexpr MenuItem MenuSub (MenuItemText text, ArrayRef<MenuItem> items)
    { return {text, nullptr, {}, items}; }
constexpr MenuItem MenuSub (MenuItemText text, QMenu * (* submenu) ())
    { return {text, nullptr, {}, nullptr, submenu}; }
constexpr MenuItem MenuSep ()
    { return {{}, nullptr, {}, nullptr, nullptr, true}; }

/* menu.cc */
QAction * menu_action (const MenuItem & menu_item, const char * domain, QWidget * parent = nullptr);
QMenu * menu_build (ArrayRef<MenuItem> menu_items, const char * domain, QWidget * parent = nullptr);
QMenuBar * menubar_build (ArrayRef<MenuItem> menu_items, const char * domain, QWidget * parent = nullptr);

#ifdef PACKAGE
static inline QMenu * menu_build (ArrayRef<MenuItem> menu_items, QWidget * parent = nullptr)
    { return menu_build (menu_items, PACKAGE, parent); }
static inline QMenuBar * menubar_build (ArrayRef<MenuItem> menu_items, QWidget * parent = nullptr)
    { return menubar_build (menu_items, PACKAGE, parent); }
#endif

/* plugin-menu.cc */
QMenu * menu_get_by_id (AudMenuID id);
void menu_add (AudMenuID id, MenuFunc func, const char * name, const char * icon);
void menu_remove (AudMenuID id, MenuFunc func);

} // namespace audqt

#endif
