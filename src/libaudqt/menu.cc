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
        else if (m_cname)
        {
            act->setChecked (aud_get_bool (m_csect, m_cname));

            QObject::connect (act, &QAction::toggled, [=] (bool checked) {
                if (aud_get_bool (m_csect, m_cname) == checked)
                    return;

                aud_set_bool (m_csect, m_cname, checked);

                if (m_func)
                    m_func ();
            });

            if (m_chook)
                hook_associate (m_chook, (HookFunction) MenuItem::hook_cb, act);
        }
        else if (m_items.len || m_submenu)
        {
            QMenu * submenu = nullptr;

            if (m_items.len)
                submenu = menu_build(m_items, menu);
            else if (m_submenu)
                submenu = m_submenu ();

            if (submenu)
                act->setMenu (submenu);
        }

#ifndef Q_OS_MAC
        if (m_icon && QIcon::hasThemeIcon (m_icon))
            act->setIcon (QIcon::fromTheme (m_icon));
#endif

        if (m_shortcut)
            act->setShortcut (QString (m_shortcut));
    }
    else
        act->setSeparator (true);

    menu->addAction (act);
}

void MenuItem::hook_cb (void *, QAction * act)
{
    AUDDBG ("implement me\n");
}

EXPORT QMenu * menu_build (const ArrayRef<const MenuItem> menu_items, QMenu * parent)
{
    QMenu * m = new QMenu (parent);

    for (auto & it : menu_items)
        it.add_to_menu (m);

    return m;
}

} // namespace audqt
