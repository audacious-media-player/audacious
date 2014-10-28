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

void MenuItem::add_to_menu (QMenu * menu) const
{
    QAction * act = new QAction (menu);

    if (! m_sep)
    {
        act->setText (translate_str (m_name, m_domain));

        if (m_func)
            QObject::connect (act, &QAction::triggered, m_func);

        if (m_icon && QIcon::hasThemeIcon (m_icon))
            act->setIcon (QIcon::fromTheme (m_icon));

        if (m_shortcut)
            act->setShortcut (QString (m_shortcut));
    }
    else
        act->setSeparator (true);

    menu->addAction (act);
}

EXPORT QMenu * menu_build (const ArrayRef<MenuItem> menu_items, QMenu * parent)
{
    QMenu * m = new QMenu (parent);

    for (auto & it : menu_items)
        it.add_to_menu (m);

    return m;
}

} // namespace audqt
