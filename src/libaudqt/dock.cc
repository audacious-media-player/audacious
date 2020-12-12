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
#include "libaudqt-internal.h"
#include "libaudqt.h"

#include <assert.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/plugins.h>
#include <libaudcore/runtime.h>

namespace audqt
{

class SimpleDockItem : public DockItem
{
public:
    SimpleDockItem(const char * id, const char * name, QWidget * widget)
        : DockItem(id, name, widget)
    {
    }

    void user_close() override { dock_hide_simple(id()); }

    static SimpleDockItem * lookup(const char * id);
};

class PluginItem : public DockItem
{
public:
    PluginItem(PluginHandle * plugin, QWidget * widget)
        : DockItem(aud_plugin_get_basename(plugin), aud_plugin_get_name(plugin),
                   widget),
          m_plugin(plugin)
    {
    }

    void grab_focus() override
    {
        DockItem::grab_focus();
        // invoke plugin-specific focus handling
        aud_plugin_send_message(m_plugin, "grab focus", nullptr, 0);
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

EXPORT void DockItem::grab_focus()
{
    assert(s_host);
    s_host->focus_dock_item(this);
}

EXPORT DockItem * DockItem::find_by_plugin(PluginHandle * plugin)
{
    return PluginItem::lookup(plugin);
}

SimpleDockItem * SimpleDockItem::lookup(const char * id)
{
    for (auto item_ : s_items)
    {
        auto item = dynamic_cast<SimpleDockItem *>(item_);
        if (item && !strcmp(item->id(), id))
            return item;
    }

    return nullptr;
}

void dock_show_simple(const char * id, const char * name, QWidget * create())
{
    if (!s_host)
    {
        AUDWARN("No UI can dock the widget %s\n", id);
        return;
    }

    auto cfg_key = str_concat({id, "_visible"});
    aud_set_bool("audqt", cfg_key, true);

    auto item = SimpleDockItem::lookup(id);
    if (!item)
        item = new SimpleDockItem(id, name, create());

    item->grab_focus();
}

void dock_hide_simple(const char * id)
{
    auto cfg_key = str_concat({id, "_visible"});
    aud_set_bool("audqt", cfg_key, false);

    delete SimpleDockItem::lookup(id);
}

PluginItem * PluginItem::lookup(PluginHandle * plugin)
{
    for (auto item_ : s_items)
    {
        auto item = dynamic_cast<PluginItem *>(item_);
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

    if (aud_get_bool("audqt", "eq_presets_visible"))
        eq_presets_show();
    if (aud_get_bool("audqt", "equalizer_visible"))
        equalizer_show();
    if (aud_get_bool("audqt", "queue_manager_visible"))
        queue_manager_show();

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
