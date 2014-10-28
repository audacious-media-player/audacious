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

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>

#include <libaudcore/i18n.h>
#include <libaudcore/index.h>
#include <libaudcore/interface.h>
#include <libaudcore/plugins.h>
#include <libaudcore/runtime.h>
#include <libaudcore/hook.h>

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
    QMenu * (* m_submenu) (void);

    void add_to_menu (QMenu * menu) const;
    QAction * build_action (QWidget * parent = nullptr) const;

    static void hook_cb (void *, QAction * act);
};

constexpr MenuItem MenuCommand (const char * name, void (* func) (void), const char * shortcut = nullptr, const char * icon = nullptr, const char * domain = nullptr)
    { return { name, icon, domain, shortcut, func, false }; }
constexpr MenuItem MenuToggle (const char * name, void (* func) (void) = nullptr, const char * shortcut = nullptr, const char * icon = nullptr, const char * domain = nullptr,
 const char * csect = nullptr, const char * cname = nullptr, const char * chook = nullptr)
    { return { name, icon, domain, shortcut, func, false, csect, cname, chook }; }
constexpr MenuItem MenuSub (const char * name, const ArrayRef<const MenuItem> items, const char * icon = nullptr, const char * domain = nullptr)
    { return { name, icon, domain, nullptr, nullptr, false, nullptr, nullptr, nullptr, items }; }
constexpr MenuItem MenuSub (const char * name, QMenu * (* submenu) (void), const char * icon = nullptr, const char * domain = nullptr)
    { return { name, icon, domain, nullptr, nullptr, false, nullptr, nullptr, nullptr, nullptr, submenu }; }
constexpr MenuItem MenuSep ()
    { return { nullptr, nullptr, nullptr, nullptr, nullptr, true }; }

} // namespace audqt

#endif
