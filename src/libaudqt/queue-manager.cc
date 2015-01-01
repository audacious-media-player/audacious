/*
 * queue-manager.cc
 * Copyright 2014 William Pitcock, John Lindgren
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

#include "libaudqt.h"

#include <QAbstractListModel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QTreeView>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/playlist.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>

/*
 * TODO:
 *     - shifting of selection entries
 */

namespace audqt {

class QueueManagerModel : public QAbstractListModel {
public:
    QueueManagerModel (QObject * parent = nullptr) : QAbstractListModel (parent) {}

    int rowCount (const QModelIndex & parent = QModelIndex()) const;
    int columnCount (const QModelIndex & parent = QModelIndex()) const;
    QVariant data (const QModelIndex & index, int role = Qt::DisplayRole) const;

    void reset ();
};

int QueueManagerModel::rowCount (const QModelIndex & parent) const
{
    return aud_playlist_queue_count (aud_playlist_get_active ());
}

int QueueManagerModel::columnCount (const QModelIndex & parent) const
{
    return 2;
}

QVariant QueueManagerModel::data (const QModelIndex & index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        int list = aud_playlist_get_active ();
        int entry = aud_playlist_queue_get_entry (list, index.row ());

        if (index.column () == 0)
            return entry;
        else
        {
            Tuple tuple = aud_playlist_entry_get_tuple (list, entry, Playlist::Guess);
            return QString ((const char *) tuple.get_str (Tuple::FormattedTitle));
        }
    }
    else if (role == Qt::TextAlignmentRole && index.column () == 0)
        return Qt::AlignRight;

    return QVariant ();
}

void QueueManagerModel::reset ()
{
    beginResetModel ();
    endResetModel ();
}

class QueueManagerDialog : public QDialog {
public:
    QueueManagerDialog (QWidget * parent = nullptr);

private:
    QVBoxLayout m_layout;
    QTreeView m_treeview;
    QDialogButtonBox m_buttonbox;
    QPushButton m_btn_unqueue;
    QPushButton m_btn_close;
    QueueManagerModel m_model;

    void update (Playlist::UpdateLevel level);
    void removeSelected ();

    const HookReceiver<QueueManagerDialog, Playlist::UpdateLevel>
     update_hook {"playlist update", this, & QueueManagerDialog::update};
    const HookReceiver<QueueManagerModel>
     activate_hook {"playlist activate", & m_model, & QueueManagerModel::reset};
};

QueueManagerDialog::QueueManagerDialog (QWidget * parent) :
    QDialog (parent)
{
    m_btn_unqueue.setText (translate_str (N_("_Unqueue")));
    m_btn_close.setText (translate_str (N_("_Close")));

    connect (& m_btn_close, &QAbstractButton::clicked, this, &QWidget::hide);
    connect (& m_btn_unqueue, &QAbstractButton::clicked, this, &QueueManagerDialog::removeSelected);

    m_buttonbox.addButton (& m_btn_close, QDialogButtonBox::AcceptRole);
    m_buttonbox.addButton (& m_btn_unqueue, QDialogButtonBox::AcceptRole);

    m_layout.addWidget (& m_treeview);
    m_layout.addWidget (& m_buttonbox);

    m_treeview.setIndentation (0);
    m_treeview.setModel (& m_model);
    m_treeview.setSelectionMode (QAbstractItemView::ExtendedSelection);
    m_treeview.header ()->hide ();

    setLayout (& m_layout);
    setWindowTitle (_("Queue Manager"));

    QItemSelectionModel * model = m_treeview.selectionModel ();
    connect (model, &QItemSelectionModel::selectionChanged, [=] (const QItemSelection & selected, const QItemSelection & deselected) {
        int list = aud_playlist_get_active ();

        for (auto & index : selected.indexes ())
            aud_playlist_entry_set_selected (list, aud_playlist_queue_get_entry (list, index.row ()), true);

        for (auto & index : deselected.indexes ())
            aud_playlist_entry_set_selected (list, aud_playlist_queue_get_entry (list, index.row ()), false);
    });

    resize (500, 250);
}

void QueueManagerDialog::update (Playlist::UpdateLevel level)
{
    /* resetting the model due to selection updates causes breakage, so don't do it. */
    if (level != Playlist::Selection)
        m_model.reset ();
}

void QueueManagerDialog::removeSelected ()
{
    int list = aud_playlist_get_active ();
    int count = aud_playlist_queue_count (list);

    for (int i = 0; i < count; )
    {
        int entry = aud_playlist_queue_get_entry (list, i);

        if (aud_playlist_entry_get_selected (list, entry))
        {
            aud_playlist_queue_delete (list, i, 1);
            aud_playlist_entry_set_selected (list, entry, false);
            count --;
        }
        else
            i ++;
    }
}

static QueueManagerDialog * s_queuemgr = nullptr;

EXPORT void queue_manager_show ()
{
    if (! s_queuemgr)
    {
        s_queuemgr = new QueueManagerDialog;
        s_queuemgr->setAttribute (Qt::WA_DeleteOnClose);

        QObject::connect (s_queuemgr, & QObject::destroyed, [] () {
            s_queuemgr = nullptr;
        });
    }

    window_bring_to_front (s_queuemgr);
}

EXPORT void queue_manager_hide ()
{
    delete s_queuemgr;
}

} // namespace audqt
