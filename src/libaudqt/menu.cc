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

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>

#include <libaudcore/hook.h>
#include <libaudcore/runtime.h>

namespace audqt {

QAction * MenuItem::build_action (const char * domain, QWidget * parent) const
{
    QAction * act = new QAction (parent);

    if (! m_sep)
    {
        act->setText (translate_str (m_name, domain ? domain : m_domain));

        if (m_func)
            QObject::connect (act, &QAction::triggered, m_func);
        else if (m_cname)
        {
            act->setCheckable (true);
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
                submenu = menu_build (m_items, domain, parent);
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

    return act;
}

void MenuItem::add_to_menu (const char * domain, QMenu * menu) const
{
    QAction * act = build_action (domain);

    if (act)
        menu->addAction (act);
}

void MenuItem::hook_cb (void *, QAction * act)
{
    AUDDBG ("implement me\n");
}

EXPORT QMenu * menu_build (const ArrayRef<const MenuItem> menu_items, const char * domain, QWidget * parent)
{
    QMenu * m = new QMenu (parent);

    for (auto & it : menu_items)
        it.add_to_menu (domain, m);

    return m;
}

EXPORT void menubar_build (const ArrayRef<const MenuItem> menu_items, const char * domain, QMenuBar * menubar)
{
    for (auto & it : menu_items)
    {
        QAction * act = it.build_action (domain);

        if (act)
            menubar->addAction (act);
    }
}

} // namespace audqt
