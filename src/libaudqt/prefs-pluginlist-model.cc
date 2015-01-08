/*
 * prefs-pluginlist-model.cc
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

#include "prefs-pluginlist-model.h"

#include <QIcon>
#include <libaudcore/plugins.h>

namespace audqt {

PluginListModel::PluginListModel (QObject * parent, PluginType category_id) : QAbstractListModel (parent),
    m_list (aud_plugin_list (category_id))
{

}

PluginListModel::~PluginListModel ()
{

}

int PluginListModel::rowCount (const QModelIndex & parent) const
{
    return m_list.len ();
}

int PluginListModel::columnCount (const QModelIndex & parent) const
{
    return NumColumns;
}

QVariant PluginListModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    return QVariant ();
}

QVariant PluginListModel::data (const QModelIndex &index, int role) const
{
    int row = index.row ();
    if (row < 0 || row >= m_list.len ())
        return QVariant ();

    PluginHandle * p = m_list[row];
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
    int row = index.row ();
    if (row < 0 || row >= m_list.len ())
        return false;

    if (role == Qt::CheckStateRole)
    {
        aud_plugin_enable (m_list[row], value.toUInt() != Qt::Unchecked);
    }

    emit dataChanged (index, index.sibling (index.row (), NumColumns - 1));
    return true;
}

Qt::ItemFlags PluginListModel::flags (const QModelIndex & index) const
{
    return (Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
}

bool PluginListModel::insertRows (int row, int count, const QModelIndex & parent)
{
    int last = row + count - 1;
    beginInsertRows (parent, row, last);
    endInsertRows ();
    return true;
}

bool PluginListModel::removeRows (int row, int count, const QModelIndex & parent)
{
    int last = row + count - 1;
    beginRemoveRows (parent, row, last);
    endRemoveRows ();
    return true;
}

void PluginListModel::updateRows (int row, int count)
{
    int bottom = row + count - 1;
    auto topLeft = createIndex (row, 0);
    auto bottomRight = createIndex (bottom, columnCount () - 1);
    emit dataChanged (topLeft, bottomRight);
}

void PluginListModel::updateRow (int row)
{
    updateRows (row, 1);
}

} // namespace audqt
