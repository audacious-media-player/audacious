/*
 * info-widget.h
 * Copyright 2006-2014 William Pitcock, Tomasz Moń, Eugene Zagidullin,
 *                     John Lindgren, and Thomas Lange
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

#include <QAbstractTableModel>
#include <QTreeView>

#include <libaudcore/tuple.h>

class PluginHandle;

namespace audqt {

class InfoModel : public QAbstractTableModel {
public:
    InfoModel (QObject * parent = nullptr);

    int rowCount (const QModelIndex & parent = QModelIndex()) const;
    int columnCount (const QModelIndex & parent = QModelIndex()) const;
    QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData (const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags (const QModelIndex & index) const;

    void setTupleData (const Tuple & tuple, String filename, PluginHandle * plugin);
    bool updateFile () const;

private:
    Tuple m_tuple;
    String m_filename;
    PluginHandle * m_plugin;
    bool m_dirty;
};

class InfoWidget : public QTreeView {
public:
    InfoWidget (QWidget * parent = nullptr);
    ~InfoWidget ();

    void fillInfo (int playlist, int entry, const char * filename, const Tuple & tuple,
     PluginHandle * decoder, bool updating_enabled);
    bool updateFile ();

private:
    InfoModel m_model;
};

} // namespace audqt
