/*
 * queue-manager.cc
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

#include <QAbstractListModel>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QPointer>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>

/*
 * TODO:
 *     - shifting of selection entries
 */

namespace audqt
{

class QueueManagerModel : public QAbstractListModel
{
public:
    void update(QItemSelectionModel * sel);
    void selectionChanged(const QItemSelection & selected,
                          const QItemSelection & deselected);

protected:
    int rowCount(const QModelIndex & parent) const
    {
        return parent.isValid() ? 0 : m_rows;
    }

    int columnCount(const QModelIndex & parent) const { return 2; }
    QVariant data(const QModelIndex & index, int role) const;

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

        if (index.column() == 0)
            return entry + 1;
        else
        {
            Tuple tuple = list.entry_tuple(entry, Playlist::NoWait);
            return QString((const char *)tuple.get_str(Tuple::FormattedTitle));
        }
    }
    else if (role == Qt::TextAlignmentRole && index.column() == 0)
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

class QueueManager : public QWidget
{
public:
    QueueManager(QWidget * parent = nullptr);

    QSize sizeHint() const override
    {
        return {4 * sizes.OneInch, 3 * sizes.OneInch};
    }

private:
    QTreeView m_treeview;
    QPushButton m_btn_unqueue;
    QueueManagerModel m_model;

    void removeSelected();
    void update() { m_model.update(m_treeview.selectionModel()); }

    const HookReceiver<QueueManager> update_hook{"playlist update", this,
                                                 &QueueManager::update},
        activate_hook{"playlist activate", this, &QueueManager::update};
};

QueueManager::QueueManager(QWidget * parent) : QWidget(parent)
{
    m_btn_unqueue.setText(translate_str(N_("_Unqueue")));

    connect(&m_btn_unqueue, &QAbstractButton::clicked, this,
            &QueueManager::removeSelected);

    auto hbox = audqt::make_hbox(nullptr);
    hbox->setContentsMargins(audqt::margins.TwoPt);
    hbox->addStretch(1);
    hbox->addWidget(&m_btn_unqueue);

    auto layout = make_vbox(this, 0);
    layout->addWidget(&m_treeview);
    layout->addLayout(hbox);

    m_treeview.setAllColumnsShowFocus(true);
    m_treeview.setFrameShape(QFrame::NoFrame);
    m_treeview.setIndentation(0);
    m_treeview.setModel(&m_model);
    m_treeview.setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeview.setHeaderHidden(true);

    update();

    connect(m_treeview.selectionModel(), &QItemSelectionModel::selectionChanged,
            &m_model, &QueueManagerModel::selectionChanged);
}

void QueueManager::removeSelected()
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

EXPORT void queue_manager_show()
{
    dock_show_simple("queue_manager", _("Queue Manager"),
                     []() -> QWidget * { return new QueueManager; });
}

EXPORT void queue_manager_hide() { dock_hide_simple("queue_manager"); }

} // namespace audqt
