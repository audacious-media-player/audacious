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

static aud::array<AudMenuID, Index<SmartPtr<MenuItem>>> items;
static aud::array<AudMenuID, QMenu *> menus;

static void show_prefs (void)
{
    prefswin_show_plugin_page (PluginType::General);
}

MenuItem default_menu_items[] = {
    MenuCommand ({N_("Plugins ...")}, show_prefs),
    MenuSep (),
};

EXPORT QMenu * menu_get_by_id (AudMenuID id)
{
    if (menus[id])
        return menus[id];

    menus[id] = new QMenu (translate_str ("Services"));

    for (auto & item : default_menu_items)
        menus[id]->addAction (menu_action (item, PACKAGE, menus[id]));

    for (auto & item : items[id])
        menus[id]->addAction (menu_action (* item, nullptr, menus[id]));

    return menus[id];
}

EXPORT void menu_add (AudMenuID id, void (* func) (void), const char * name, const char * icon)
{
    MenuItem * item = new MenuItem (MenuCommand ({name, icon}, func));
    items[id].append (item);

    if (menus[id])
        menus[id]->addAction (menu_action (* item, nullptr, menus[id]));
}

EXPORT void menu_remove (AudMenuID id, void (* func) (void))
{
    // FIXME: remove the QAction
    auto is_match = [func] (SmartPtr<MenuItem> & item)
        { return item->func == func; };

    items[id].remove_if (is_match, true);
}

} // namespace audqt
