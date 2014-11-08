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

#include "libaudqt.h"
#include "libaudqt/menu.h"

#include <QMenu>

#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/plugins.h>

namespace audqt {

static Index <MenuItem *> items [AUD_MENU_COUNT];
static QMenu * menus [AUD_MENU_COUNT];

static void show_prefs (void)
{
    prefswin_show_plugin_page (PLUGIN_TYPE_GENERAL);
}

MenuItem default_menu_items[] = {
    MenuCommand (N_("Plugins ..."), show_prefs),
    MenuSep (),
};

EXPORT QMenu * menu_get_by_id (int id)
{
    if (menus[id])
        return menus[id];

    menus[id] = new QMenu (translate_str ("Services"));

    for (auto & it : default_menu_items)
        it.add_to_menu (PACKAGE, menus[id]);

    for (const MenuItem * it : items[id])
        it->add_to_menu (nullptr, menus[id]);

    return menus[id];
}

EXPORT void menu_add (int id, void (* func) (void), const char * name, const char * icon, const char * domain)
{
    MenuItem * it = new MenuItem;

    it->m_name = name;
    it->m_icon = icon;
    it->m_func = func;
    it->m_domain = domain;
    it->m_shortcut = nullptr;	/* XXX */
    it->m_sep = false;

    items[id].append (it);

    if (menus[id])
        it->add_to_menu (nullptr, menus[id]);
}

EXPORT void menu_remove (int id, void (* func) (void))
{
    MenuItem * item = nullptr;

    for (MenuItem * it : items[id])
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

} // namespace audqt
