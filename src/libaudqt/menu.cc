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
#include "libaudqt/menu.h"

namespace audqt {

MenuItem::MenuItem (const char * name, const char * icon, void (* func) (void), const char * shortcut, const char * domain, bool sep) :
    m_func (func),
    m_name (name),
    m_icon (icon),
    m_domain (domain),
    m_shortcut (shortcut),
    m_sep (sep)
{
    if (! m_sep)
        m_action = new QAction (translate_str (m_name, m_domain), nullptr);
    else
    {
        m_action = new QAction (nullptr);
        m_action->setSeparator (true);
    }

    if (m_func)
        QObject::connect (m_action, &QAction::triggered, m_func);

    if (m_icon && QIcon::hasThemeIcon (m_icon))
        m_action->setIcon (QIcon::fromTheme (m_icon));

    if (m_shortcut)
        m_action->setShortcut (QString (m_shortcut));
}

MenuItem::~MenuItem ()
{
    delete m_action;
}

void MenuItem::add_to_menu (QMenu * menu) const
{
    menu->addAction (m_action);
}

} // namespace audqt
