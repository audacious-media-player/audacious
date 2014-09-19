/*
 * prefs-pluginlist-model.h
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
#include <libaudcore/index.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#ifndef PREFS_PLUGINLIST_MODEL_H
#define PREFS_PLUGINLIST_MODEL_H

class PluginHandle;

namespace audqt {

class PluginListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    PluginListModel (QObject * parent, int category_id);
    ~PluginListModel ();

    int rowCount (const QModelIndex & parent = QModelIndex ()) const;
    int columnCount (const QModelIndex & parent = QModelIndex ()) const;
    QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole);
    Qt::ItemFlags flags (const QModelIndex & parent = QModelIndex ()) const;

    bool insertRows (int row, int count, const QModelIndex & parent = QModelIndex ());
    bool removeRows (int row, int count, const QModelIndex & parent = QModelIndex ());
    void updateRows (int row, int count);
    void updateRow (int row);

private:
    const Index<PluginHandle *> & m_list;
};

}

#endif
