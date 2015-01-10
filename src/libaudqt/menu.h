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

namespace audqt {

struct MenuItem {
    const char * m_name;
    const char * m_icon;
    const char * m_domain;
    const char * m_shortcut;
    void (* m_func) ();
    bool m_sep;

    /* for toggle items */
    const char * m_csect;
    const char * m_cname;
    const char * m_chook;

    /* for submenus */
    const ArrayRef<const MenuItem> m_items;

    /* for custom submenus */
    QMenu * (* m_submenu) ();
};

constexpr MenuItem MenuCommand (const char * name, void (* func) (), const char * shortcut = nullptr, const char * icon = nullptr, const char * domain = nullptr)
    { return { name, icon, domain, shortcut, func, false }; }
constexpr MenuItem MenuToggle (const char * name, void (* func) () = nullptr, const char * shortcut = nullptr, const char * icon = nullptr, const char * domain = nullptr,
 const char * csect = nullptr, const char * cname = nullptr, const char * chook = nullptr)
    { return { name, icon, domain, shortcut, func, false, csect, cname, chook }; }
constexpr MenuItem MenuSub (const char * name, const ArrayRef<const MenuItem> items, const char * icon = nullptr, const char * domain = nullptr)
    { return { name, icon, domain, nullptr, nullptr, false, nullptr, nullptr, nullptr, items }; }
constexpr MenuItem MenuSub (const char * name, QMenu * (* submenu) (), const char * icon = nullptr, const char * domain = nullptr)
    { return { name, icon, domain, nullptr, nullptr, false, nullptr, nullptr, nullptr, nullptr, submenu }; }
constexpr MenuItem MenuSep ()
    { return { nullptr, nullptr, nullptr, nullptr, nullptr, true }; }

QAction * menu_action (const MenuItem & menu_item, const char * domain, QWidget * parent = nullptr);
QMenu * menu_build (const ArrayRef<const MenuItem> menu_items, const char * domain, QWidget * parent = nullptr);
void menubar_build (const ArrayRef<const MenuItem> menu_items, const char * domain, QMenuBar * parent);

#ifdef PACKAGE
static inline QMenu * menu_build (const ArrayRef<const MenuItem> menu_items, QWidget * parent = nullptr)
    { return menu_build (menu_items, PACKAGE, parent); }
static inline void menubar_build (const ArrayRef<const MenuItem> menu_items, QMenuBar * parent)
    { menubar_build (menu_items, PACKAGE, parent); }
#endif

} // namespace audqt

#endif
