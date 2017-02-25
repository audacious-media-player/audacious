/*
 * prefs-pluginlist-model.cc
 * Copyright 2014-2017 John Lindgren and William Pitcock
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

#include "prefs-pluginlist-model.h"

#include <QIcon>

#include <libaudcore/i18n.h>
#include <libaudcore/plugins.h>
#include <libaudcore/runtime.h>

namespace audqt {

struct PluginCategory {
    PluginType type;
    const char * name;
};

static const PluginCategory categories[] = {
    { PluginType::General,   N_("General") },
    { PluginType::Effect,    N_("Effect") },
    { PluginType::Vis,       N_("Visualization") },
    { PluginType::Input,     N_("Input") },
    { PluginType::Playlist,  N_("Playlist") },
    { PluginType::Transport, N_("Transport") }
};

static constexpr int n_categories = aud::n_elems (categories);

// The model hierarchy is as follows:
//
// Root (invalid index)
//  + General category (index with row 0, null internal pointer)
//     + General plugin (index with row 0, internal pointer to PluginHandle)
//     + General plugin (index with row 1, internal pointer to PluginHandle)
//     + General plugin ...
//  + Effect category (index with row 1, null internal pointer)
//     + Effect plugin ...
//  + More categories ...

QModelIndex PluginListModel::index (int row, int column, const QModelIndex & parent) const
{
    // is parent the root node?
    if (! parent.isValid ())
        return createIndex (row, column, nullptr);

    // is parent a plugin node?
    if (parent.internalPointer () != nullptr)
        return QModelIndex ();

    // parent must be a category node
    int cat = parent.row ();
    if (cat < 0 || cat >= n_categories)
        return QModelIndex ();

    auto & list = aud_plugin_list (categories[cat].type);
    if (row < 0 || row >= list.len ())
        return QModelIndex ();

    return createIndex (row, column, list[row]);
}

// for a plugin node, return the category node
// for all other nodes, return an invalid index
QModelIndex PluginListModel::parent (const QModelIndex & child) const
{
    auto p = pluginForIndex (child);
    return p ? indexForType (aud_plugin_get_type (p)) : QModelIndex ();
}

// retrieve the PluginHandle from a plugin node
PluginHandle * PluginListModel::pluginForIndex (const QModelIndex & index) const
{
    return (PluginHandle *) index.internalPointer ();
}

// look up the category node for a given plugin type
QModelIndex PluginListModel::indexForType (PluginType type) const
{
    for (int cat = 0; cat < n_categories; cat ++)
    {
        if (categories[cat].type == type)
            return createIndex (cat, 0, nullptr);
    }

    return QModelIndex ();
}

int PluginListModel::rowCount (const QModelIndex & parent) const
{
    // for the root node, return the # of categories
    if (! parent.isValid ())
        return n_categories;

    // for a plugin node, return 0 (no children)
    if (parent.internalPointer () != nullptr)
        return 0;

    // for a category node, return the # of plugins
    int cat = parent.row ();
    if (cat < 0 || cat >= n_categories)
        return 0;

    return aud_plugin_list (categories[cat].type).len ();
}

int PluginListModel::columnCount (const QModelIndex & parent) const
{
    return NumColumns;
}

QVariant PluginListModel::data (const QModelIndex & index, int role) const
{
    auto p = pluginForIndex (index);

    if (! p) // category node?
    {
        if (role != Qt::DisplayRole || index.column () != 0)
            return QVariant ();

        int cat = index.row ();
        if (cat < 0 || cat >= n_categories)
            return QVariant ();

        return QString (_(categories[cat].name));
    }

    bool enabled = aud_plugin_get_enabled (p);

    switch (index.column ())
    {
    case NameColumn:
        if (role == Qt::DisplayRole)
            return QString (aud_plugin_get_name (p));
        if (role == Qt::CheckStateRole)
            return enabled ? Qt::Checked : Qt::Unchecked;

        break;

    case AboutColumn:
        if (role == Qt::DecorationRole && enabled && aud_plugin_has_about (p))
            return QIcon::fromTheme ("dialog-information");

        break;

    case SettingsColumn:
        if (role == Qt::DecorationRole && enabled && aud_plugin_has_configure (p))
            return QIcon::fromTheme ("preferences-system");

        break;
    }

    return QVariant ();
}

bool PluginListModel::setData (const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole)
        return false;

    auto p = pluginForIndex (index);
    if (! p)
        return false;

    aud_plugin_enable (p, value.toUInt () != Qt::Unchecked);
    emit dataChanged (index, index.sibling (index.row (), NumColumns - 1));
    return true;
}

Qt::ItemFlags PluginListModel::flags (const QModelIndex & index) const
{
    return (Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
}

} // namespace audqt
