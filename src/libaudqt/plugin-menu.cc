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

struct ItemData {
    MenuItem item;
    SmartPtr<QAction> action;
};

static aud::array<AudMenuID, Index<ItemData>> items;
static aud::array<AudMenuID, QMenu *> menus;

static void show_prefs ()
{
    prefswin_show_plugin_page (PluginType::General);
}

MenuItem default_menu_items[] = {
    MenuCommand ({N_("Plugins ..."), "preferences-system"}, show_prefs),
};

void menu_rebuild (AudMenuID id)
{
    if (menus[id])
        menus[id]->clear ();
    else
        menus[id] = new QMenu (_("Services"));

    for (auto & item : items[id])
    {
        item.action.capture (menu_action (item.item, nullptr));
        menus[id]->addAction (item.action.get ());
    }

    if (! menus[id]->isEmpty ())
        menus[id]->addAction (menu_action (MenuSep (), PACKAGE, menus[id]));

    for (auto & item : default_menu_items)
        menus[id]->addAction (menu_action (item, PACKAGE, menus[id]));
}

EXPORT QMenu * menu_get_by_id (AudMenuID id)
{
    if (! menus[id])
        menu_rebuild (id);

    return menus[id];
}

EXPORT void menu_add (AudMenuID id, MenuFunc func, const char * name, const char * icon)
{
    items[id].append (MenuCommand ({name, icon}, func));

    menu_rebuild(id);
}

EXPORT void menu_remove (AudMenuID id, MenuFunc func)
{
    auto is_match = [func] (ItemData & item)
        { return item.item.func == func; };

    if (items[id].remove_if (is_match, true))
        menu_rebuild (id);
}

} // namespace audqt
