/*
 * info-widget.h
 * Copyright 2006-2017 René Bertin, Thomas Lange, John Lindgren,
 *                     Ariadne Conill, Tomasz Moń, and Eugene Zagidullin
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
#include "libaudqt-internal.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QPointer>

#include <libaudcore/i18n.h>
#include <libaudcore/probe.h>
#include <libaudcore/tuple.h>

namespace audqt {

struct TupleFieldMap {
    const char * name;
    Tuple::Field field;
    bool editable;
};

static const TupleFieldMap tuple_field_map[] = {
    {N_("Metadata"), Tuple::Invalid, false},
    {N_("Artist"), Tuple::Artist, true},
    {N_("Album"), Tuple::Album, true},
    {N_("Title"), Tuple::Title, true},
    {N_("Track Number"), Tuple::Track, true},
    {N_("Genre"), Tuple::Genre, true},
    {N_("Comment"), Tuple::Comment, true},
    {N_("Description"), Tuple::Description, true},
    {N_("Album Artist"), Tuple::AlbumArtist, true},
    {N_("Composer"), Tuple::Composer, true},
    {N_("Performer"), Tuple::Performer, true},
    {N_("Recording Year"), Tuple::Year, true},
    {N_("Recording Date"), Tuple::Date, true},

    {nullptr, Tuple::Invalid, false},
    {N_("Technical"), Tuple::Invalid, false},
    {N_("Length"), Tuple::Length, false},
    {N_("Codec"), Tuple::Codec, false},
    {N_("Quality"), Tuple::Quality, false},
    {N_("Bitrate"), Tuple::Bitrate, false},
    {N_("MusicBrainz ID"), Tuple::MusicBrainzID, false}
};

static const TupleFieldMap * to_field_map (const QModelIndex & index)
{
    int row = index.row ();
    if (row < 0 || row >= aud::n_elems (tuple_field_map))
        return nullptr;

    return & tuple_field_map[row];
}

static QModelIndex sibling_field_index (const QModelIndex & current, int direction)
{
    QModelIndex index = current;

    while (1)
    {
        index = index.sibling (index.row () + direction, index.column ());
        auto map = to_field_map (index);

        if (! map)
            return QModelIndex ();
        if (map->field != Tuple::Invalid)
            return index;
    }
}

class InfoModel : public QAbstractTableModel
{
public:
    enum {
        Col_Name = 0,
        Col_Value = 1
    };

    InfoModel (QObject * parent = nullptr) :
        QAbstractTableModel (parent) {}

    int rowCount (const QModelIndex &) const override
        { return aud::n_elems (tuple_field_map); }
    int columnCount (const QModelIndex &) const override
        { return 2; }

    QVariant data (const QModelIndex & index, int role) const override;
    bool setData (const QModelIndex & index, const QVariant & value, int role) override;
    Qt::ItemFlags flags (const QModelIndex & index) const override;

    void setTupleData (const Tuple & tuple, String filename, PluginHandle * plugin)
    {
        m_tuple = tuple.ref ();
        m_orig_tuple = tuple.ref ();
        m_filename = filename;
        m_plugin = plugin;
        setDirty (false);
    }

    void revertTupleData ()
    {
        m_tuple = m_orig_tuple.ref ();
        setDirty (false);
    }

    void linkEnabled (QWidget * widget)
    {
        widget->setEnabled (m_dirty);
        m_linked_widgets.append (widget);
    }

    bool updateFile () const;

private:
    void setDirty (bool dirty)
    {
        m_dirty = dirty;
        for (auto & widget : m_linked_widgets)
        {
            if (widget)
                widget->setEnabled (dirty);
        }
    }

    Tuple m_tuple, m_orig_tuple;
    String m_filename;
    PluginHandle * m_plugin = nullptr;
    bool m_dirty = false;
    QList<QPointer<QWidget>> m_linked_widgets;
};

EXPORT InfoWidget::InfoWidget (QWidget * parent) :
    QTreeView (parent),
    m_model (new InfoModel (this))
{
    setModel (m_model);
    header ()->hide ();
    setIndentation (0);
    resizeColumnToContents (0);
    setContextMenuPolicy (Qt::CustomContextMenu);

    connect (this, & QWidget::customContextMenuRequested, [this] (const QPoint & pos)
    {
        auto index = indexAt (pos);
        if (index.column () != InfoModel::Col_Value)
            return;
        auto text = m_model->data (index, Qt::DisplayRole).toString ();
        if (! text.isEmpty ())
            show_copy_context_menu (this, mapToGlobal (pos), text);
    });
}

EXPORT InfoWidget::~InfoWidget ()
{
}

EXPORT void InfoWidget::fillInfo (const char * filename, const Tuple & tuple,
 PluginHandle * decoder, bool updating_enabled)
{
    m_model->setTupleData (tuple, String (filename), decoder);
    reset ();

    setEditTriggers (updating_enabled ? QAbstractItemView::AllEditTriggers :
                                        QAbstractItemView::NoEditTriggers);

    auto initial_index = m_model->index (1 /* title */, InfoModel::Col_Value);
    setCurrentIndex (initial_index);
}

EXPORT void InfoWidget::linkEnabled (QWidget * widget)
{
    m_model->linkEnabled (widget);
}

EXPORT void InfoWidget::revertInfo ()
{
    m_model->revertTupleData ();
    reset ();
}

EXPORT bool InfoWidget::updateFile ()
{
    return m_model->updateFile ();
}

void InfoWidget::keyPressEvent (QKeyEvent * event)
{
    if (event->type () == QEvent::KeyPress &&
        (event->key () == Qt::Key_Up || event->key () == Qt::Key_Down))
    {
        auto index = sibling_field_index (currentIndex (), (event->key () == Qt::Key_Up) ? -1 : 1);
        if (index.isValid ())
            setCurrentIndex (index);

        event->accept ();
        return;
    }

    QTreeView::keyPressEvent (event);
}

bool InfoModel::updateFile () const
{
    if (! m_dirty)
        return true;

    return aud_file_write_tuple (m_filename, m_plugin, m_tuple);
}

bool InfoModel::setData (const QModelIndex & index, const QVariant & value, int role)
{
    if (role != Qt::EditRole)
        return false;

    auto map = to_field_map (index);
    if (! map || map->field == Tuple::Invalid)
        return false;

    auto t = Tuple::field_get_type (map->field);
    auto str = value.toString ();

    bool changed = false;

    if (str.isEmpty ())
    {
        changed = m_tuple.is_set (map->field);
        m_tuple.unset (map->field);
    }
    else if (t == Tuple::String)
    {
        auto utf8 = str.toUtf8 ();
        changed = (!m_tuple.is_set (map->field) || m_tuple.get_str (map->field) != utf8);
        m_tuple.set_str (map->field, utf8);
    }
    else /* t == Tuple::Int */
    {
        int val = str.toInt ();
        changed = (!m_tuple.is_set (map->field) || m_tuple.get_int (map->field) != val);
        m_tuple.set_int (map->field, val);
    }

    if (changed)
        setDirty (true);

    emit dataChanged (index, index, {role});
    return true;
}

QVariant InfoModel::data (const QModelIndex & index, int role) const
{
    auto map = to_field_map (index);
    if (! map)
        return QVariant ();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (index.column () == InfoModel::Col_Name)
            return translate_str (map->name);
        else if (index.column () == InfoModel::Col_Value)
        {
            if (map->field == Tuple::Invalid)
                return QVariant ();

            switch (m_tuple.get_value_type (map->field))
            {
            case Tuple::String:
                return QString (m_tuple.get_str (map->field));
            case Tuple::Int:
                /* convert to string so Qt allows clearing the field */
                return QString::number (m_tuple.get_int (map->field));
            default:
                return QVariant ();
            }
        }
    }
    else if (role == Qt::FontRole)
    {
        if (map->field == Tuple::Invalid)
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
    if (index.column () == InfoModel::Col_Value)
    {
        auto map = to_field_map (index);

        if (! map || map->field == Tuple::Invalid)
            return Qt::ItemNeverHasChildren;
        else if (map->editable)
            return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;

        return Qt::ItemIsEnabled;
    }

    return Qt::ItemNeverHasChildren;
}

} // namespace audqt
