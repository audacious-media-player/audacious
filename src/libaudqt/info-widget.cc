/*
 * info-widget.cc
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
#include "libaudqt-internal.h"
#include "libaudqt.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QPointer>

#include <libaudcore/i18n.h>
#include <libaudcore/probe.h>
#include <libaudcore/runtime.h>
#include <libaudcore/tuple.h>

namespace audqt
{

struct TupleFieldMap
{
    const char * name;
    Tuple::Field field;
    bool editable;
};

static const char * varied_str = N_("<various>");

static const TupleFieldMap tuple_field_map[] = {
    {N_("Metadata"), Tuple::Invalid, false},
    {N_("Title"), Tuple::Title, true},
    {N_("Artist"), Tuple::Artist, true},
    {N_("Album"), Tuple::Album, true},
    {N_("Album Artist"), Tuple::AlbumArtist, true},
    {N_("Track Number"), Tuple::Track, true},
    {N_("Genre"), Tuple::Genre, true},
    {N_("Comment"), Tuple::Comment, true},
    {N_("Description"), Tuple::Description, true},
    {N_("Composer"), Tuple::Composer, true},
    {N_("Performer"), Tuple::Performer, true},
    {N_("Recording Year"), Tuple::Year, true},
    {N_("Recording Date"), Tuple::Date, true},
    {N_("Publisher"), Tuple::Publisher, false},
    {N_("Catalog Number"), Tuple::CatalogNum, false},
    {N_("Disc Number"), Tuple::Disc, true},

    {nullptr, Tuple::Invalid, false},
    {N_("Technical"), Tuple::Invalid, false},
    {N_("Length"), Tuple::Length, false},
    {N_("Codec"), Tuple::Codec, false},
    {N_("Quality"), Tuple::Quality, false},
    {N_("Bitrate"), Tuple::Bitrate, false},
    {N_("Channels"), Tuple::Channels, false},
    {N_("MusicBrainz ID"), Tuple::MusicBrainzID, false}};

static const TupleFieldMap * to_field_map(const QModelIndex & index)
{
    int row = index.row();
    if (row < 0 || row >= aud::n_elems(tuple_field_map))
        return nullptr;

    return &tuple_field_map[row];
}

static QModelIndex sibling_field_index(const QModelIndex & current,
                                       int direction)
{
    QModelIndex index = current;

    while (1)
    {
        index = index.sibling(index.row() + direction, index.column());
        auto map = to_field_map(index);

        if (!map)
            return QModelIndex();
        if (map->field != Tuple::Invalid)
            return index;
    }
}

static QString tuple_field_to_str(const Tuple & tuple, Tuple::Field field)
{
    switch (tuple.get_value_type(field))
    {
    case Tuple::String:
        return QString(tuple.get_str(field));
    case Tuple::Int:
        return QString::number(tuple.get_int(field));
    default:
        return QString();
    }
}

static void tuple_field_set_from_str(Tuple & tuple, Tuple::Field field,
                                     const QString & str)
{
    if (str.isEmpty())
        tuple.unset(field);
    else if (Tuple::field_get_type(field) == Tuple::String)
        tuple.set_str(field, str.toUtf8());
    else
        tuple.set_int(field, str.toInt());
}

static bool tuple_field_is_same(const Tuple & a, const Tuple & b,
                                Tuple::Field field)
{
    auto a_type = a.get_value_type(field);
    if (a_type != b.get_value_type(field))
        return false;

    switch (a_type)
    {
    case Tuple::String:
        return (a.get_str(field) == b.get_str(field));
    case Tuple::Int:
        return (a.get_int(field) == b.get_int(field));
    default:
        return true;
    }
}

static void tuple_field_copy(Tuple & dest, const Tuple & src,
                             Tuple::Field field)
{
    switch (src.get_value_type(field))
    {
    case Tuple::String:
        dest.set_str(field, src.get_str(field));
        break;
    case Tuple::Int:
        dest.set_int(field, src.get_int(field));
        break;
    default:
        dest.unset(field);
        break;
    }
}

class InfoModel : public QAbstractTableModel
{
public:
    enum
    {
        Col_Name = 0,
        Col_Value = 1
    };

    InfoModel(QObject * parent = nullptr) : QAbstractTableModel(parent) {}

    int rowCount(const QModelIndex & parent) const override
    {
        return parent.isValid() ? 0 : aud::n_elems(tuple_field_map);
    }

    int columnCount(const QModelIndex &) const override { return 2; }

    QVariant data(const QModelIndex & index, int role) const override;
    bool setData(const QModelIndex & index, const QVariant & value,
                 int role) override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;

    void setItems(Index<PlaylistAddItem> && items)
    {
        m_items = std::move(items);
        revertTupleData();
    }

    void linkEnabled(QWidget * widget)
    {
        widget->setEnabled(m_changed_mask != 0);
        m_linked_widgets.append(widget);
    }

    void revertTupleData();
    bool updateFile() const;

private:
    void updateEnabled()
    {
        for (auto & widget : m_linked_widgets)
        {
            if (widget)
                widget->setEnabled(m_changed_mask != 0);
        }
    }

    /* set of filenames, original tuples, decoders */
    Index<PlaylistAddItem> m_items;
    /* local (possibly edited) tuple */
    Tuple m_tuple;
    /* bitmask of fields which vary between tuples and will be preserved */
    uint64_t m_varied_mask = 0;
    /* bitmask of fields which have been edited */
    uint64_t m_changed_mask = 0;
    /* list of widgets to enable when edits have been made */
    QList<QPointer<QWidget>> m_linked_widgets;
};

EXPORT InfoWidget::InfoWidget(QWidget * parent)
    : QTreeView(parent), m_model(new InfoModel(this))
{
    setModel(m_model);
    header()->hide();
    setIndentation(0);
    resizeColumnToContents(0);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QWidget::customContextMenuRequested,
            [this](const QPoint & pos) {
                auto index = indexAt(pos);
                if (index.column() != InfoModel::Col_Value)
                    return;
                auto text = m_model->data(index, Qt::DisplayRole).toString();
                if (!text.isEmpty())
                    show_copy_context_menu(this, mapToGlobal(pos), text);
            });
}

EXPORT InfoWidget::~InfoWidget() {}

EXPORT void InfoWidget::fillInfo(const char * filename, const Tuple & tuple,
                                 PluginHandle * decoder, bool updating_enabled)
{
    Index<PlaylistAddItem> items;
    items.append(String(filename), tuple.ref(), decoder);
    fillInfo(std::move(items), updating_enabled);
}

EXPORT void InfoWidget::fillInfo(Index<PlaylistAddItem> && items,
                                 bool updating_enabled)
{
    m_model->setItems(std::move(items));
    reset();

    setEditTriggers(updating_enabled ? QAbstractItemView::AllEditTriggers
                                     : QAbstractItemView::NoEditTriggers);

    auto initial_index = m_model->index(1 /* title */, InfoModel::Col_Value);
    setCurrentIndex(initial_index);
}

EXPORT void InfoWidget::linkEnabled(QWidget * widget)
{
    m_model->linkEnabled(widget);
}

EXPORT void InfoWidget::revertInfo()
{
    m_model->revertTupleData();
    reset();
}

EXPORT bool InfoWidget::updateFile() { return m_model->updateFile(); }

void InfoWidget::keyPressEvent(QKeyEvent * event)
{
    if (event->type() == QEvent::KeyPress &&
        (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down))
    {
        auto index = sibling_field_index(currentIndex(),
                                         (event->key() == Qt::Key_Up) ? -1 : 1);
        if (index.isValid())
            setCurrentIndex(index);

        event->accept();
        return;
    }

    QTreeView::keyPressEvent(event);
}

void InfoModel::revertTupleData()
{
    m_tuple = (m_items.len() > 0) ? m_items[0].tuple.ref() : Tuple();
    m_varied_mask = 0;
    m_changed_mask = 0;

    for (auto & item : m_items)
    {
        for (auto field : Tuple::all_fields())
        {
            if (!tuple_field_is_same(item.tuple, m_tuple, field))
                m_varied_mask |= (uint64_t)1 << (int)field;
        }
    }

    updateEnabled();
}

bool InfoModel::updateFile() const
{
    if (m_changed_mask == 0)
        return true;

    int n_saved = 0;

    for (auto & item : m_items)
    {
        auto new_tuple = item.tuple.ref();

        for (auto field : Tuple::all_fields())
        {
            uint64_t mask = ((uint64_t)1 << (int)field);
            if ((m_changed_mask & mask) != 0)
                tuple_field_copy(new_tuple, m_tuple, field);
        }

        if (aud_file_write_tuple(item.filename, item.decoder, new_tuple))
            n_saved++;
    }

    return (n_saved == m_items.len());
}

bool InfoModel::setData(const QModelIndex & index, const QVariant & value,
                        int role)
{
    if (role != Qt::EditRole)
        return false;

    auto map = to_field_map(index);
    if (!map || map->field == Tuple::Invalid)
        return false;

    uint64_t mask = ((uint64_t)1 << (int)map->field);
    auto str = value.toString();

    /* prevent accidental overwrite of varied values */
    if ((m_varied_mask & mask) != 0 && str == _(varied_str))
        return false;

    if ((m_varied_mask & mask) != 0 ||
        str != tuple_field_to_str(m_tuple, map->field))
    {
        tuple_field_set_from_str(m_tuple, map->field, str);
        m_varied_mask &= ~mask;
        m_changed_mask |= mask;
        updateEnabled();
    }

    emit dataChanged(index, index, {role});
    return true;
}

QVariant InfoModel::data(const QModelIndex & index, int role) const
{
    auto map = to_field_map(index);
    if (!map)
        return QVariant();

    uint64_t mask = 0;
    if (map->field != Tuple::Invalid)
        mask = ((uint64_t)1 << (int)map->field);

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (index.column() == InfoModel::Col_Name)
            return translate_str(map->name);
        else if (index.column() == InfoModel::Col_Value)
        {
            if (map->field == Tuple::Invalid)
                return QVariant();

            if ((m_varied_mask & mask) != 0)
                return QString(_(varied_str));

            return tuple_field_to_str(m_tuple, map->field);
        }
    }
    else if (role == Qt::FontRole)
    {
        if ((index.column() == Col_Name && map->field == Tuple::Invalid) ||
            (index.column() == Col_Value && (m_changed_mask & mask) != 0))
        {
            QFont f;
            f.setBold(true);
            return f;
        }
        else if (index.column() == Col_Value && (m_varied_mask & mask) != 0)
        {
            QFont f;
            f.setItalic(true);
            return f;
        }

        return QVariant();
    }

    return QVariant();
}

Qt::ItemFlags InfoModel::flags(const QModelIndex & index) const
{
    if (index.column() == InfoModel::Col_Value)
    {
        auto map = to_field_map(index);

        if (!map || map->field == Tuple::Invalid)
            return Qt::ItemNeverHasChildren;
        else if (map->editable)
            return Qt::ItemIsSelectable | Qt::ItemIsEditable |
                   Qt::ItemIsEnabled;

        return Qt::ItemIsEnabled;
    }

    return Qt::ItemNeverHasChildren;
}

} // namespace audqt
