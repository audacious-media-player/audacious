/*
 * prefs-pluginlist-model.h
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

#ifndef PREFS_PLUGINLIST_MODEL_H
#define PREFS_PLUGINLIST_MODEL_H

#include <QAbstractItemModel>

enum class PluginType;
class PluginHandle;

namespace audqt {

class PluginListModel : public QAbstractItemModel
{
public:
    enum {
        NameColumn,
        AboutColumn,
        SettingsColumn,
        NumColumns
    };

    PluginListModel (QObject * parent) : QAbstractItemModel (parent) {}

    QModelIndex index (int row, int column, const QModelIndex & parent) const;
    QModelIndex parent (const QModelIndex & child) const;

    PluginHandle * pluginForIndex (const QModelIndex & index) const;
    QModelIndex indexForType (PluginType type) const;

    int rowCount (const QModelIndex & parent) const;
    int columnCount (const QModelIndex & parent) const;

    QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData (const QModelIndex & index, const QVariant & value, int role);
    Qt::ItemFlags flags (const QModelIndex & parent) const;
};

} // namespace audqt

#endif
