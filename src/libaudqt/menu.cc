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

class MenuAction : public QAction
{
public:
    MenuAction (const MenuItem & item, const char * domain, QWidget * parent);

private:
    void toggle (bool checked);
    void update ();

    const MenuItem & m_item;
    SmartPtr<HookReceiver<MenuAction>> m_hook;
};

MenuAction::MenuAction (const MenuItem & item, const char * domain, QWidget * parent) :
    QAction (parent),
    m_item (item)
{
    if (item.m_sep)
    {
        setSeparator (true);
        return;
    }

    setText (translate_str (item.m_name, domain ? domain : item.m_domain));

    if (item.m_func)
        QObject::connect (this, & QAction::triggered, item.m_func);
    else if (item.m_cname)
    {
        setCheckable (true);
        setChecked (aud_get_bool (item.m_csect, item.m_cname));

        QObject::connect (this, & QAction::toggled, this, & MenuAction::toggle);

        if (item.m_chook)
            m_hook.capture (new HookReceiver<MenuAction> (item.m_chook, this, & MenuAction::update));
    }
    else if (item.m_items.len)
        setMenu (menu_build (item.m_items, domain, parent));
    else if (item.m_submenu)
        setMenu (item.m_submenu ());

#ifndef Q_OS_MAC
    if (item.m_icon && QIcon::hasThemeIcon (item.m_icon))
        setIcon (QIcon::fromTheme (item.m_icon));
#endif

    if (item.m_shortcut)
        setShortcut (QString (item.m_shortcut));
}

void MenuAction::toggle (bool checked)
{
    if (aud_get_bool (m_item.m_csect, m_item.m_cname) != checked)
    {
        aud_set_bool (m_item.m_csect, m_item.m_cname, checked);

        if (m_item.m_func)
            m_item.m_func ();
    }
}

void MenuAction::update ()
{
    setChecked (aud_get_bool (m_item.m_csect, m_item.m_cname));
}

EXPORT QAction * menu_action (const MenuItem & menu_item, const char * domain, QWidget * parent)
{
    return new MenuAction (menu_item, domain, parent);
}

EXPORT QMenu * menu_build (const ArrayRef<const MenuItem> menu_items, const char * domain, QWidget * parent)
{
    QMenu * m = new QMenu (parent);

    for (auto & it : menu_items)
        m->addAction (new MenuAction (it, domain, m));

    return m;
}

EXPORT void menubar_build (const ArrayRef<const MenuItem> menu_items, const char * domain, QMenuBar * menubar)
{
    for (auto & it : menu_items)
        menubar->addAction (new MenuAction (it, domain, menubar));
}

} // namespace audqt
