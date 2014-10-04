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

#include <QtGui>
#include <QtWidgets>
#include <QAbstractTableModel>

#include <libaudcore/audstrings.h>
#include <libaudcore/playlist.h>
#include <libaudcore/probe.h>
#include <libaudcore/runtime.h>
#include <libaudcore/interface.h>
#include <libaudcore/tuple.h>
#include <libaudcore/hook.h>

#include <libaudqt/libaudqt.h>

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
            return QString ((const char *) aud_playlist_entry_get_title (list, entry, true));
    }

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
    ~QueueManagerDialog ();

private:
    QVBoxLayout m_layout;
    QTreeView m_treeview;
    QDialogButtonBox m_buttonbox;
    QPushButton m_btn_unqueue;
    QPushButton m_btn_close;
    QueueManagerModel m_model;

    static void update_hook (void *, void * user);

    void removeSelected ();
};

QueueManagerDialog::QueueManagerDialog (QWidget * parent) : QDialog (parent)
{
    m_btn_unqueue.setText (translate_str (_("_Unqueue")));
    m_btn_close.setText (translate_str (_("_Close")));

    connect (& m_btn_close, &QAbstractButton::clicked, this, &QWidget::hide);
    connect (& m_btn_unqueue, &QAbstractButton::clicked, this, &QueueManagerDialog::removeSelected);

    m_buttonbox.addButton (& m_btn_close, QDialogButtonBox::AcceptRole);
    m_buttonbox.addButton (& m_btn_unqueue, QDialogButtonBox::AcceptRole);

    m_layout.addWidget (& m_treeview);
    m_layout.addWidget (& m_buttonbox);

    m_treeview.setModel (& m_model);
    m_treeview.setSelectionMode (QAbstractItemView::ExtendedSelection);
    m_treeview.header ()->hide ();

    hook_associate ("playlist activate", QueueManagerDialog::update_hook, this);
    hook_associate ("playlist update", QueueManagerDialog::update_hook, this);

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

QueueManagerDialog::~QueueManagerDialog ()
{
    hook_dissociate ("playlist activate", QueueManagerDialog::update_hook);
    hook_dissociate ("playlist update", QueueManagerDialog::update_hook);
}

void QueueManagerDialog::update_hook (void *, void * user)
{
    QueueManagerDialog * s = static_cast<QueueManagerDialog *> (user);

    if (! s)
        return;

    s->m_model.reset ();
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

static QueueManagerDialog * m_queuemgr = nullptr;

EXPORT void queue_manager_show ()
{
    if (! m_queuemgr)
        m_queuemgr = new QueueManagerDialog;

    window_bring_to_front (m_queuemgr);
}

EXPORT void queue_manager_hide ()
{
    if (! m_queuemgr)
        return;

    m_queuemgr->hide ();
}

}
