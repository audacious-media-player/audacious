/*
 * song-window-qt.cc
 * Copyright 2021 Steve Storey
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

#include <QAbstractButton>
#include <QAbstractListModel>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QVector>

#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>

namespace audqt
{

static void do_jump(int entry)
{
    if (entry < 0)
        return;

    auto playlist = Playlist::active_playlist ();
    playlist.set_position (entry);
    playlist.start_playback ();
}

static void do_queue(int entry)
{
    if (entry < 0)
        return;

    auto playlist = Playlist::active_playlist ();
    int queued = playlist.queue_find_entry (entry);
    if (queued >= 0)
        playlist.queue_remove (queued);
    else
        playlist.queue_insert (-1, entry);
}

static bool is_queued(int entry)
{
    if (entry < 0)
        return false;

    auto playlist = Playlist::active_playlist ();
    int queued = playlist.queue_find_entry (entry);
    return queued >= 0;
}

struct PlaylistEntry
{
    int index;
    QString title;
};

class SongListModel : public QAbstractListModel
{
public:
    enum
    {
        ColumnEntry,
        ColumnTitle,
        NColumns
    };

    static void selectFirstRow(QTreeView* treeView, SongListModel* treeModel)
    {
        QModelIndex rootIndex = treeView->rootIndex();
        QModelIndex rootChildIndex = treeModel->index(0, 0, rootIndex);
        if (rootChildIndex.isValid()) {
            treeView->selectionModel()->select(rootChildIndex,
                                               QItemSelectionModel::Rows | QItemSelectionModel::Select);
        }
    }

    void update(QItemSelectionModel * sel, QString * filter);
    void toggleQueueSelected();
    void jumpToSelected();
    bool isSelectedQueued();
    void selectionChanged(const QItemSelection & selected);

protected:
    int rowCount(const QModelIndex & parent) const override
    {
        return parent.isValid() ? 0 : m_rows;
    }

    int columnCount(const QModelIndex &) const override { return NColumns; }

    QVariant data(const QModelIndex & index, int role) const override;

private:
    int m_rows = 0;
    bool m_in_update = false;
    int m_selected_row_index = -1;
    QVector<PlaylistEntry> * m_filteredTuples;
};

void SongListModel::toggleQueueSelected()
{
    do_queue(m_filteredTuples->value(m_selected_row_index).index - 1);
}

void SongListModel::jumpToSelected()
{
    do_jump(m_filteredTuples->value(m_selected_row_index).index - 1);
}

bool SongListModel::isSelectedQueued()
{
    return is_queued(m_filteredTuples->value(m_selected_row_index).index - 1);
}

QVariant SongListModel::data(const QModelIndex & index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        int entry = index.row();

        PlaylistEntry tuple = m_filteredTuples->value(entry);
        if (index.column() == ColumnEntry)
            return tuple.index;
        else if (index.column() == ColumnTitle)
        {
            return tuple.title;
        }
    }
    else if (role == Qt::TextAlignmentRole && index.column() == ColumnEntry)
        return int(Qt::AlignRight | Qt::AlignVCenter);

    return QVariant();
}

static bool includeEntry(int index, QString* title , QString * filter) {
    if (filter == nullptr)
        return true;
    // Split the filter into different words, where all sub-words must
    // be contained within the title to keep it in the list
    QStringList parts = filter->split(" ");
    for (int i = 0; i < parts.size(); i++)
        if (parts[i].length() > 0 && !title->contains(parts[i], Qt::CaseInsensitive))
            return false;
    return true;
}

void SongListModel::update(QItemSelectionModel * sel, QString * filter)
{
    QVector<PlaylistEntry> * filteredTuples = new QVector<PlaylistEntry>;
    auto playlist = Playlist::active_playlist();
    // Copy the playlist, filtering the rows
    int playlist_size = playlist.n_entries();
    for (int i = 0; i < playlist_size; i++)
    {
        Tuple playlistTuple = playlist.entry_tuple(i, Playlist::NoWait);
        QString* title = new QString(playlistTuple.get_str(Tuple::FormattedTitle));
        if (includeEntry(i, title, filter))
        {
            PlaylistEntry* localEntry = new PlaylistEntry;
            localEntry->index = i + 1;
            localEntry->title = *title;
            filteredTuples->append(*localEntry);
        }
    }
    m_filteredTuples = filteredTuples;

    int rows = m_filteredTuples->size();
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

    m_in_update = false;
}

void SongListModel::selectionChanged(const QItemSelection & selected)
{
    if (m_in_update)
        return;

    for (auto & index : selected.indexes())
        // Should just be a single entry, but we'll take the last
        // one
        m_selected_row_index = index.row();
}


class SongsWindow : public QDialog
{
public:
    static SongsWindow * get_instance()
    {
        if (!instance)
            (void)new SongsWindow;
        return instance;
    }

    static void destroy_instance()
    {
        if (instance)
            delete instance;
    }

private:
    static SongsWindow * instance;
    SongListModel m_songListModel;
    QTreeView m_treeview;
    QLineEdit m_filterEdit;
    QCheckBox m_closeAfterJump;
    QPushButton m_queueAndUnqueueButton;

    SongsWindow();
    ~SongsWindow() { instance = nullptr; }

    void update()
    {
        m_songListModel.update(m_treeview.selectionModel(), nullptr);
    }

    void jumpToSelected()
    {
        m_songListModel.jumpToSelected();
        if (m_closeAfterJump.isChecked())
            destroy_instance();
    }

    void onFilterChanged()
    {
        QString currentText = m_filterEdit.text();
        m_songListModel.update(m_treeview.selectionModel(), &currentText);
        SongListModel::selectFirstRow(&m_treeview, &m_songListModel);
    }

    void updateQueueButton()
    {
        m_queueAndUnqueueButton.setText(m_songListModel.isSelectedQueued()
                ? translate_str(N_("Un_queue")) : translate_str(N_("_Queue")));
    }
};

/* static data */
SongsWindow * SongsWindow::instance = nullptr;

SongsWindow::SongsWindow()
{
    /* initialize static data */
    instance = this;

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(_("Jump to Song"));
    setWindowRole("jump-to-song");
    setContentsMargins(0, 0, 0, 0);

    auto vbox_parent = make_vbox(this);

    QWidget * content = new QWidget;
    content->setContentsMargins(margins.FourPt);
    vbox_parent->addWidget(content);

    auto vbox_content = make_vbox(content);

    // **** START Filter box ****
    QWidget * filter_box = new QWidget;
    filter_box->setContentsMargins(margins.FourPt);
    vbox_content->addWidget(filter_box);

    auto hbox_filter = make_hbox(filter_box);
    auto label_filter = new QLabel(_("Filter: "), this);
    hbox_filter->addWidget(label_filter);
    m_filterEdit.setClearButtonEnabled(true);
    m_filterEdit.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    hbox_filter->addWidget(&m_filterEdit);
    QObject::connect(&m_filterEdit, &QLineEdit::textChanged, [this]() {
        this->onFilterChanged();
    });
    // **** END Filter box ****

    // **** START Track list ****
    m_treeview.setAllColumnsShowFocus(true);
    m_treeview.setFrameShape(QFrame::NoFrame);
    m_treeview.setHeaderHidden(true);
    m_treeview.setIndentation(0);
    m_treeview.setModel(&m_songListModel);
    m_treeview.setSelectionMode(QAbstractItemView::SingleSelection);
    auto header = m_treeview.header();
    header->setSectionResizeMode(SongListModel::ColumnEntry,
                                 QHeaderView::Interactive);
    header->resizeSection(SongListModel::ColumnEntry,
                          audqt::to_native_dpi(50));
    QObject::connect(m_treeview.selectionModel(), &QItemSelectionModel::selectionChanged,
                     [this](const QItemSelection & selected) {
                         this->m_songListModel.selectionChanged(selected);
                         this->updateQueueButton();
                     });
    QObject::connect(&m_treeview, &QAbstractItemView::doubleClicked, [this]() {
        this->jumpToSelected();
    });

    update();
    vbox_content->addWidget(&m_treeview);
    // **** END Track list ****

    // **** START Bottom button bar ****
    QWidget * footerWidget = new QWidget;
    vbox_content->addWidget(footerWidget);
    auto hbox_footer = make_hbox(footerWidget);

    m_closeAfterJump.setChecked(true);
    m_closeAfterJump.setText(translate_str(N_("C_lose on jump")));
    hbox_footer->addWidget(&m_closeAfterJump);

    QDialogButtonBox * bbox = new QDialogButtonBox(QDialogButtonBox::Close);
    bbox->addButton(&m_queueAndUnqueueButton, QDialogButtonBox::ApplyRole);
    m_queueAndUnqueueButton.setText(translate_str(N_("_Queue")));
    QPushButton* btn_Jump = bbox->addButton(translate_str(N_("_Jump")), QDialogButtonBox::AcceptRole);
    QPushButton* btn_Close = bbox->button(QDialogButtonBox::Close);
    btn_Close->setText(translate_str(N_("_Close")));
    btn_Close->setIcon(QIcon::fromTheme("window-close"));
    btn_Jump->setIcon(QIcon::fromTheme("go-jump"));
    hbox_footer->addWidget(bbox);

    QObject::connect(&m_queueAndUnqueueButton, &QAbstractButton::clicked, [this]() {
        this->m_songListModel.toggleQueueSelected();
        this->updateQueueButton();
    });

    QObject::connect(btn_Jump, &QAbstractButton::clicked, [this]() {
        this->jumpToSelected();
    });

    QObject::connect(bbox, &QDialogButtonBox::rejected, destroy_instance);
    // **** END Bottom button bar ****

    resize(500, 500);
}

EXPORT void songwin_show()
{
    window_bring_to_front(SongsWindow::get_instance());
}

EXPORT void songwin_hide()
{
    SongsWindow::destroy_instance();
}

} // namespace audqt
