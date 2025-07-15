/*
 * queue-manager-qt.cc
 * Copyright 2014 Ariadne Conill, John Lindgren
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

#include "libaudqt-internal.h"
#include "libaudqt.h"
#include "treeview.h"

#include <QAbstractListModel>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QPushButton>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>

namespace audqt
{

static inline QPoint position(QDropEvent * event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return event->position().toPoint();
#else
    return event->pos();
#endif
}

class QueueManagerModel : public QAbstractListModel
{
public:
    enum
    {
        ColumnEntry,
        ColumnTitle,
        NColumns
    };

    void update(QItemSelectionModel * sel);
    void selectionChanged(const QItemSelection & selected,
                          const QItemSelection & deselected);

protected:
    int rowCount(const QModelIndex & parent) const override
    {
        return parent.isValid() ? 0 : m_rows;
    }

    int columnCount(const QModelIndex &) const override { return NColumns; }

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    Qt::ItemFlags flags(const QModelIndex & index) const override
    {
        if (index.isValid())
            return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
                   Qt::ItemIsEnabled;
        else
            return Qt::ItemIsSelectable | Qt::ItemIsDropEnabled |
                   Qt::ItemIsEnabled;
    }

    QVariant data(const QModelIndex & index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

private:
    int m_rows = 0;
    bool m_in_update = false;
};

QVariant QueueManagerModel::data(const QModelIndex & index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        auto list = Playlist::active_playlist();
        int entry = list.queue_get_entry(index.row());

        if (index.column() == ColumnEntry)
            return entry + 1;
        else if (index.column() == ColumnTitle)
        {
            Tuple tuple = list.entry_tuple(entry, Playlist::NoWait);
            return QString((const char *)tuple.get_str(Tuple::FormattedTitle));
        }
    }
    else if (role == Qt::TextAlignmentRole && index.column() == ColumnEntry)
        return Qt::AlignRight;

    return QVariant();
}

QVariant QueueManagerModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
        case ColumnEntry:
            return QString("#");
        case ColumnTitle:
            return QString(_("Title"));
        }
    }
    else if (role == Qt::TextAlignmentRole && section == ColumnEntry)
        return Qt::AlignRight;

    return QVariant();
}

void QueueManagerModel::update(QItemSelectionModel * sel)
{
    auto list = Playlist::active_playlist();
    int rows = list.n_queued();
    int keep = aud::min(rows, m_rows);

    m_in_update = true;

    if (rows < m_rows)
    {
        beginRemoveRows(QModelIndex(), rows, m_rows - 1);
        m_rows = rows;
        endRemoveRows();
    }
    else if (rows > m_rows)
    {
        beginInsertRows(QModelIndex(), m_rows, rows - 1);
        m_rows = rows;
        endInsertRows();
    }

    if (keep > 0)
    {
        auto topLeft = createIndex(0, 0);
        auto bottomRight = createIndex(keep - 1, 0);
        emit dataChanged(topLeft, bottomRight);
    }

    for (int i = 0; i < rows; i++)
    {
        if (list.entry_selected(list.queue_get_entry(i)))
            sel->select(createIndex(i, 0), sel->Select | sel->Rows);
        else
            sel->select(createIndex(i, 0), sel->Deselect | sel->Rows);
    }

    m_in_update = false;
}

void QueueManagerModel::selectionChanged(const QItemSelection & selected,
                                         const QItemSelection & deselected)
{
    if (m_in_update)
        return;

    auto list = Playlist::active_playlist();

    for (auto & index : selected.indexes())
        list.select_entry(list.queue_get_entry(index.row()), true);

    for (auto & index : deselected.indexes())
        list.select_entry(list.queue_get_entry(index.row()), false);
}

class QueueManagerView : public audqt::TreeView
{
public:
    QueueManagerView();
    void removeSelected();
    void update() { m_model.update(selectionModel()); }

protected:
    void dropEvent(QDropEvent * event) override;
    void keyPressEvent(QKeyEvent * event) override;

private:
    QueueManagerModel m_model;

    void shiftRows(int before);

    const HookReceiver<QueueManagerView> //
        update_hook{"playlist update", this, &QueueManagerView::update},
        activate_hook{"playlist activate", this, &QueueManagerView::update};
};

QueueManagerView::QueueManagerView()
{
    setAllColumnsShowFocus(true);
    setDragDropMode(InternalMove);
    setFrameShape(QFrame::NoFrame);
    setIndentation(0);
    setModel(&m_model);
    setSelectionMode(ExtendedSelection);

    auto hdr = header();
    hdr->setSectionResizeMode(QueueManagerModel::ColumnEntry,
                              QHeaderView::Interactive);
    hdr->resizeSection(QueueManagerModel::ColumnEntry,
                       audqt::to_native_dpi(50));

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, &m_model,
            &QueueManagerModel::selectionChanged);
}

void QueueManagerView::removeSelected()
{
    auto list = Playlist::active_playlist();
    int count = list.n_queued();

    for (int i = 0; i < count;)
    {
        int entry = list.queue_get_entry(i);

        if (list.entry_selected(entry))
        {
            list.queue_remove(i);
            list.select_entry(entry, false);
            count--;
        }
        else
            i++;
    }
}

void QueueManagerView::dropEvent(QDropEvent * event)
{
    if (event->source() != this || event->proposedAction() != Qt::MoveAction)
        return;

    int from = currentIndex().row();
    if (from < 0)
        return;

    int to;
    switch (dropIndicatorPosition())
    {
    case AboveItem:
        to = indexAt(position(event)).row();
        break;
    case BelowItem:
        to = indexAt(position(event)).row() + 1;
        break;
    default:
        return;
    }

    shiftRows(to);
    event->acceptProposedAction();
}

void QueueManagerView::keyPressEvent(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Delete)
        removeSelected();

    QTreeView::keyPressEvent(event);
}

void QueueManagerView::shiftRows(int before)
{
    Index<int> shift;
    auto list = Playlist::active_playlist();
    int count = list.n_queued();

    for (int i = 0; i < count; i++)
    {
        int entry = list.queue_get_entry(i);

        if (list.entry_selected(entry))
        {
            shift.append(entry);

            if (i < before)
                before--;
        }
    }

    list.queue_remove_selected();

    for (int i = 0; i < shift.len(); i++)
        list.queue_insert(before + i, shift[i]);
}

class QueueManager : public QWidget
{
public:
    QueueManager(QWidget * parent = nullptr);

    QSize sizeHint() const override
    {
        return {3 * sizes.OneInch, 2 * sizes.OneInch};
    }

private:
    QueueManagerView m_view;
    QPushButton m_btn_unqueue;
};

QueueManager::QueueManager(QWidget * parent) : QWidget(parent)
{
    setWindowRole("queue-manager");

    m_btn_unqueue.setText(translate_str(N_("_Unqueue")));
    connect(&m_btn_unqueue, &QAbstractButton::clicked, &m_view,
            &QueueManagerView::removeSelected);

    auto hbox = audqt::make_hbox(nullptr);
    hbox->setContentsMargins(audqt::margins.TwoPt);
    hbox->addStretch(1);
    hbox->addWidget(&m_btn_unqueue);

    auto layout = make_vbox(this, 0);
    layout->addWidget(&m_view);
    layout->addLayout(hbox);

    m_view.update();
}

EXPORT void queue_manager_show()
{
    dock_show_simple("queue_manager", _("Queue Manager"),
                     []() -> QWidget * { return new QueueManager; });
}

EXPORT void queue_manager_hide() { dock_hide_simple("queue_manager"); }

} // namespace audqt
