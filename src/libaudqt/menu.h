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

#include <libaudcore/i18n.h>
#include <libaudcore/index.h>
#include <libaudcore/interface.h>
#include <libaudcore/plugins.h>

namespace audqt {

class MenuItem {
public:
    MenuItem (const char * name, const char * icon, void (* func) (void), const char * shortcut = nullptr, const char * domain = nullptr, bool sep = false);
    ~MenuItem ();

    void add_to_menu (QMenu * menu) const;
    void (* m_func) (void);

private:
    QAction * m_action;

    const char * m_name;
    const char * m_icon;
    const char * m_domain;
    const char * m_shortcut;
    bool m_sep;
};

} // namespace audqt

#endif
