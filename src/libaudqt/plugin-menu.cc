/*
 * plugin-menu.cc
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

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugin.h>
#include <libaudcore/plugins.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>
#include <libaudcore/interface.h>

#include "libaudqt.h"

namespace audqt {

class PluginMenuItem {
public:
    PluginMenuItem (const char * name, const char * icon, void (* func) (void), const char * domain = nullptr) :
        m_func (func),
        m_name (name),
        m_icon (icon),
        m_domain (domain)
    {
        m_action = new QAction (translate_str (m_name, m_domain), nullptr);

        if (m_func)
            QObject::connect (m_action, &QAction::triggered, m_func);

        if (m_icon && QIcon::hasThemeIcon (m_icon))
            m_action->setIcon (QIcon::fromTheme (m_icon));
    }

    ~PluginMenuItem ()
    {
        delete m_action;
    }

    void add_to_menu (QMenu * menu) const
    {
        menu->addAction (m_action);
    }

    void (* m_func) (void);

private:
    QAction * m_action;

    const char * m_name;
    const char * m_icon;
    const char * m_domain;
};

static Index <PluginMenuItem *> items [AUD_MENU_COUNT];
static QMenu * menus [AUD_MENU_COUNT];

EXPORT QMenu * menu_get_by_id (int id)
{
    if (menus[id])
        return menus[id];

    menus[id] = new QMenu;

    for (const PluginMenuItem * it : items[id])
    {
        it->add_to_menu (menus[id]);
    }

    return menus[id];
}

EXPORT void menu_add (int id, void (* func) (void), const char * name, const char * icon, const char * domain)
{
    PluginMenuItem * it = new PluginMenuItem (name, icon, func, domain);

    items[id].append (it);

    if (menus[id])
        it->add_to_menu (menus[id]);
}

EXPORT void menu_remove (int id, void (* func) (void))
{
    PluginMenuItem * item = nullptr;

    for (PluginMenuItem * it : items[id])
    {
        if (it->m_func == func)
        {
            item = it;
            break;
        }
    }

    if (! item)
        return;

    delete item;
}

};
