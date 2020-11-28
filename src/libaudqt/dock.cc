/*
 * dock.cc
 * Copyright 2020 John Lindgren
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

#include "dock.h"

#include <libaudcore/hook.h>
#include <libaudcore/plugins.h>

namespace audqt
{

class PluginItem : public DockItem
{
public:
    PluginItem(PluginHandle * plugin, QWidget * widget)
        : DockItem(aud_plugin_get_basename(plugin), aud_plugin_get_name(plugin),
                   widget),
          m_plugin(plugin)
    {
    }

    // explicitly closing the widget disables the plugin
    void user_close() override { aud_plugin_enable(m_plugin, false); }

    static PluginItem * lookup(PluginHandle * plugin);

private:
    PluginHandle * m_plugin;
};

static DockHost * s_host = nullptr;
static Index<DockItem *> s_items;

EXPORT DockItem::DockItem(const char * id, const char * name, QWidget * widget)
    : m_id(id), m_name(name), m_widget(widget)
{
    assert(s_host);
    s_host->add_dock_item(this);
    s_items.append(this);
}

EXPORT DockItem::~DockItem()
{
    assert(s_host);
    s_items.remove(s_items.find(this), 1);
    s_host->remove_dock_item(this);
    delete m_widget;
}

PluginItem * PluginItem::lookup(PluginHandle * plugin)
{
    for (int i = 0; i < s_items.len(); i++)
    {
        PluginItem * item = dynamic_cast<PluginItem *>(s_items[i]);
        if (item && item->m_plugin == plugin)
            return item;
    }

    return nullptr;
}

static void add_dock_plugin(void * plugin_, void *)
{
    auto plugin = (PluginHandle *)plugin_;
    auto widget = (QWidget *)aud_plugin_get_qt_widget(plugin);
    if (widget)
        new PluginItem(plugin, widget);
}

static void remove_dock_plugin(void * plugin_, void *)
{
    auto plugin = (PluginHandle *)plugin_;
    delete PluginItem::lookup(plugin);
}

EXPORT void register_dock_host(DockHost * host)
{
    assert(!s_host);
    s_host = host;

    for (PluginHandle * plugin : aud_plugin_list(PluginType::General))
    {
        if (aud_plugin_get_enabled(plugin))
            add_dock_plugin(plugin, nullptr);
    }

    for (PluginHandle * plugin : aud_plugin_list(PluginType::Vis))
    {
        if (aud_plugin_get_enabled(plugin))
            add_dock_plugin(plugin, nullptr);
    }

    hook_associate("dock plugin enabled", add_dock_plugin, nullptr);
    hook_associate("dock plugin disabled", remove_dock_plugin, nullptr);
}

EXPORT void unregister_dock_host()
{
    assert(s_host);

    hook_dissociate("dock plugin enabled", add_dock_plugin);
    hook_dissociate("dock plugin disabled", remove_dock_plugin);

    while (s_items.len() > 0)
        delete s_items[0];

    s_host = nullptr;
}

} // namespace audqt
