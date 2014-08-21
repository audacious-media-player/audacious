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

#include <QtGui>
#include <QtWidgets>
#include <QAbstractTableModel>

#include <libaudcore/i18n.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>
#include <libaudcore/plugin.h>
#include <libaudcore/plugins.h>

#include "prefs-pluginlist-model.h"
#include "prefs-pluginlist-model.moc"

namespace audqt {

PluginListModel::PluginListModel (QObject * parent, int category_id) : QAbstractListModel (parent),
    m_category_id (category_id)
{

}

PluginListModel::~PluginListModel ()
{

}

int PluginListModel::rowCount (const QModelIndex & parent) const
{
    return aud_plugin_count (m_category_id);
}

int PluginListModel::columnCount (const QModelIndex & parent) const
{
    return PLUGINLIST_COLS;
}

QVariant PluginListModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    static const char * col_names [PLUGINLIST_COLS] = {
        "Enabled",
        "Name",
    };

    if (role == Qt::DisplayRole)
        return QString (col_names [section]);

    return QVariant ();
}

QVariant PluginListModel::data (const QModelIndex &index, int role) const
{
    PluginHandle * ph = nullptr;

    switch (role)
    {
    case Qt::DisplayRole:
        ph = aud_plugin_by_index (m_category_id, index.row ());

        switch (index.column ())
        {
        case PLUGINLIST_ENABLED:
            return QString ();
        case PLUGINLIST_NAME:
            return QString (aud_plugin_get_name (ph));
        }

        break;

    default:
        break;
    }

    return QVariant ();
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

};
