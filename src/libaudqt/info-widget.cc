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

#include "info-widget.h"
#include "libaudqt.h"

#include <QHeaderView>

#include <libaudcore/i18n.h>
#include <libaudcore/probe.h>

namespace audqt {

struct TupleFieldMap {
    const char * name;
    Tuple::Field field;
    bool editable;
};
const TupleFieldMap tuple_field_map[] = {
    {N_("Metadata"), Tuple::Invalid, false},
    {N_("Artist"), Tuple::Artist, true},
    {N_("Album"), Tuple::Album, true},
    {N_("Title"), Tuple::Title, true},
    {N_("Track Number"), Tuple::Track, true},
    {N_("Genre"), Tuple::Genre, true},
    {N_("Comment"), Tuple::Comment, true},
    {N_("Album Artist"), Tuple::AlbumArtist, true},
    {N_("Composer"), Tuple::Composer, true},
    {N_("Performer"), Tuple::Performer, true},
    {N_("Recording Year"), Tuple::Year, true},
    {N_("Recording Date"), Tuple::Date, true},

    {"", Tuple::Invalid, false},
    {N_("Technical"), Tuple::Invalid, false},
    {N_("Length"), Tuple::Length, false},
    {N_("MIME Type"), Tuple::MIMEType, false},
    {N_("Codec"), Tuple::Codec, false},
    {N_("Quality"), Tuple::Quality, false},
    {N_("Bitrate"), Tuple::Bitrate, false},
};

EXPORT InfoWidget::InfoWidget (QWidget * parent) : QTreeView (parent)
{
    setModel (& m_model);
    header ()->hide ();
    setEditTriggers (QAbstractItemView::SelectedClicked);
    setIndentation (0);
}

EXPORT InfoWidget::~InfoWidget ()
{
}

EXPORT void InfoWidget::fillInfo (int playlist, int entry, const char * filename, const Tuple & tuple,
 PluginHandle * decoder, bool updating_enabled)
{
    m_model.setTupleData (tuple, String (filename), decoder);
}

EXPORT bool InfoWidget::updateFile ()
{
    return m_model.updateFile ();
}

InfoModel::InfoModel (QObject * parent) : QAbstractTableModel (parent)
{
}

int InfoModel::rowCount (const QModelIndex & parent) const
{
    auto r = ArrayRef<TupleFieldMap> (tuple_field_map);
    return r.len;
}

int InfoModel::columnCount (const QModelIndex & parent) const
{
    return 2;
}

bool InfoModel::updateFile () const
{
    if (! m_dirty)
        return true;

    Tuple t = m_tuple.ref ();
    t.set_filename (m_filename);

    return aud_file_write_tuple (m_filename, m_plugin, t);
}

bool InfoModel::setData (const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole)
        return false;

    Tuple::Field field_id = tuple_field_map [index.row ()].field;
    if (field_id == Tuple::Invalid)
        return false;

    m_dirty = true;

    auto t = Tuple::field_get_type (field_id);
    if (t == Tuple::String)
    {
        m_tuple.set_str (field_id, value.toString ().toLocal8Bit ());
        emit dataChanged (index, index, {role});
        return true;
    }
    else if (t == Tuple::Int)
    {
        m_tuple.set_int (field_id, value.toInt ());
        emit dataChanged (index, index, {role});
        return true;
    }

    return false;
}

QVariant InfoModel::data (const QModelIndex & index, int role) const
{
    Tuple::Field field_id = tuple_field_map [index.row ()].field;

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (index.column () == 0)
            return translate_str (tuple_field_map [index.row ()].name);
        else if (index.column () == 1 && m_tuple)
        {
            if (field_id == Tuple::Invalid)
                return QVariant ();

            auto t = Tuple::field_get_type (field_id);

            if (t == Tuple::String)
            {
                const char * res = m_tuple.get_str (field_id);
                if (res)
                    return QString (res);
            }
            else if (t == Tuple::Int)
            {
                int res = m_tuple.get_int (field_id);
                if (res == -1)
                    return QVariant ();
                return res;
            }
        }
    }
    else if (role == Qt::FontRole)
    {
        if (field_id == Tuple::Invalid)
        {
            QFont f;
            f.setBold (true);
            return f;
        }
        return QVariant ();
    }

    return QVariant ();
}

Qt::ItemFlags InfoModel::flags (const QModelIndex & index) const
{
    if (index.column () == 1)
    {
        auto & t = tuple_field_map [index.row ()];

        if (t.field == Tuple::Invalid)
            return Qt::ItemNeverHasChildren;
        else if (t.editable)
            return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;

        return Qt::ItemIsEnabled;
    }

    return Qt::ItemNeverHasChildren;
}

void InfoModel::setTupleData (const Tuple & tuple, String filename, PluginHandle * plugin)
{
    m_tuple = tuple.ref ();
    m_filename = filename;
    m_plugin = plugin;
    m_dirty = false;
}

} // namespace audqt
