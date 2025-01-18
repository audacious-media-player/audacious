/*
 * song-window-qt.cc
 * Copyright 2021 Steve Storey
 * Copyright 2025 Thomas Lange
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
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QVector>

#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>
#include <libaudcore/runtime.h>

namespace audqt
{

static void do_jump(int entry)
{
    if (entry < 0)
        return;

    auto playlist = Playlist::active_playlist();
    playlist.set_position(entry);
    playlist.start_playback();
}

static void do_queue(int entry)
{
    if (entry < 0)
        return;

    auto playlist = Playlist::active_playlist();
    int queued = playlist.queue_find_entry(entry);
    if (queued >= 0)
        playlist.queue_remove(queued);
    else
        playlist.queue_insert(-1, entry);
}

static bool is_queued(int entry)
{
    if (entry < 0)
        return false;

    auto playlist = Playlist::active_playlist();
    int queued = playlist.queue_find_entry(entry);
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

    void update(const QString & filter);
    void toggleQueueSelected();
    void jumpToSelected();
    bool isSelectedQueued();
    void selectFirstRow(const QTreeView & treeView);
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
    QVector<PlaylistEntry> m_filteredTuples;
};

void SongListModel::toggleQueueSelected()
{
    do_queue(m_filteredTuples.value(m_selected_row_index).index - 1);
}

void SongListModel::jumpToSelected()
{
    do_jump(m_filteredTuples.value(m_selected_row_index).index - 1);
}

bool SongListModel::isSelectedQueued()
{
    return is_queued(m_filteredTuples.value(m_selected_row_index).index - 1);
}

QVariant SongListModel::data(const QModelIndex & index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        PlaylistEntry entry = m_filteredTuples.value(index.row());

        if (index.column() == ColumnEntry)
            return entry.index;
        if (index.column() == ColumnTitle)
            return entry.title;
    }
    else if (role == Qt::TextAlignmentRole && index.column() == ColumnEntry)
        return int(Qt::AlignRight | Qt::AlignVCenter);

    return QVariant();
}

static bool includeEntry(const QString & title, const QString & filter)
{
    if (filter.isEmpty())
        return true;

    // Split the filter into different words, where all sub-words must
    // be contained within the title to keep it in the list
    QStringList parts = filter.split(" ");
    int n_parts = parts.size();
    for (int i = 0; i < n_parts; i++)
    {
        if (parts[i].length() > 0 && !title.contains(parts[i], Qt::CaseInsensitive))
            return false;
    }

    return true;
}

void SongListModel::update(const QString & filter)
{
    QVector<PlaylistEntry> filteredTuples;
    auto playlist = Playlist::active_playlist();
    int playlist_size = playlist.n_entries();

    // Copy the playlist, filtering the rows
    for (int i = 0; i < playlist_size; i++)
    {
        Tuple playlistTuple = playlist.entry_tuple(i, Playlist::NoWait);
        QString title = QString(playlistTuple.get_str(Tuple::FormattedTitle));
        if (includeEntry(title, filter))
        {
            PlaylistEntry localEntry = {
                .index = i + 1,
                .title = title
            };
            filteredTuples.append(localEntry);
        }
    }
    m_filteredTuples = filteredTuples;

    int rows = m_filteredTuples.size();
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

void SongListModel::selectFirstRow(const QTreeView & treeView)
{
    QModelIndex rootIndex = treeView.rootIndex();
    QModelIndex rootChildIndex = index(0, 0, rootIndex);

    if (rootChildIndex.isValid())
    {
        treeView.selectionModel()->select(
            rootChildIndex,
            QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
    }
}

void SongListModel::selectionChanged(const QItemSelection & selected)
{
    if (m_in_update)
        return;

    QModelIndexList indexes = selected.indexes();
    m_selected_row_index = indexes.size() > 0 ? indexes.last().row() : -1;
}

class SongsWindow : public QDialog
{
public:
    SongsWindow(QWidget * parent = nullptr);

    QSize sizeHint() const override
    {
        return {5 * sizes.OneInch, 5 * sizes.OneInch};
    }

protected:
    void keyPressEvent(QKeyEvent * event) override;

private:
    SongListModel m_songListModel;
    QTreeView m_treeview;
    QLineEdit m_filterEdit;
    QCheckBox m_closeAfterJump;
    QPushButton m_queueAndUnqueueButton, m_jumpButton;

    const HookReceiver<SongsWindow, Playlist::UpdateLevel> //
        update_hook{"playlist update", this, &SongsWindow::updatePlaylist};
    const HookReceiver<SongsWindow> //
        activate_hook{"playlist activate", this, &SongsWindow::updateModel};

    void updateModel()
    {
        QString filter = m_filterEdit.text();
        m_songListModel.update(filter);
        m_songListModel.selectFirstRow(m_treeview);
    }

    void updatePlaylist(Playlist::UpdateLevel level)
    {
        if (level >= Playlist::Metadata)
            updateModel();
    }

    void jumpToSelected()
    {
        m_songListModel.jumpToSelected();
        if (m_closeAfterJump.isChecked())
            QDialog::close();
    }

    void updateQueueButton()
    {
        m_queueAndUnqueueButton.setText(m_songListModel.isSelectedQueued()
                ? translate_str(N_("Un_queue")) : translate_str(N_("_Queue")));
    }

    void updateButtonStates()
    {
        bool enable = m_treeview.selectionModel()->hasSelection();
        m_queueAndUnqueueButton.setEnabled(enable);
        m_jumpButton.setEnabled(enable);
    }
};

static QPointer<SongsWindow> s_songs_window;

SongsWindow::SongsWindow(QWidget * parent) : QDialog(parent)
{
    setWindowTitle(_("Jump to Song"));
    setWindowRole("jump-to-song");
    setContentsMargins(margins.FourPt);

    auto vbox_parent = make_vbox(this);

    QWidget * content = new QWidget;
    vbox_parent->addWidget(content);

    auto vbox_content = make_vbox(content);

    // **** START Filter box ****
    QWidget * filter_box = new QWidget;
    filter_box->setContentsMargins(margins.FourPt);
    vbox_content->addWidget(filter_box);

    auto hbox_filter = make_hbox(filter_box);
    auto label_filter = new QLabel(translate_str(N_("_Filter:")), this);
    label_filter->setBuddy(&m_filterEdit);
    hbox_filter->addWidget(label_filter);
    m_filterEdit.setClearButtonEnabled(true);
    m_filterEdit.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    hbox_filter->addWidget(&m_filterEdit);
    QObject::connect(&m_filterEdit, &QLineEdit::textChanged, [this]() {
        this->updateModel();
    });
    // **** END Filter box ****

    // **** START Track list ****
    m_treeview.setAllColumnsShowFocus(true);
    m_treeview.setFrameShape(QFrame::NoFrame);
    m_treeview.setHeaderHidden(true);
    m_treeview.setIndentation(0);
    m_treeview.setModel(&m_songListModel);
    m_treeview.setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeview.setUniformRowHeights(true);
    auto header = m_treeview.header();
    header->setSectionResizeMode(SongListModel::ColumnEntry,
                                 QHeaderView::Interactive);
    header->resizeSection(SongListModel::ColumnEntry, audqt::to_native_dpi(50));
    QObject::connect(m_treeview.selectionModel(),
                     &QItemSelectionModel::selectionChanged,
                     [this](const QItemSelection & selected) {
                         this->m_songListModel.selectionChanged(selected);
                         this->updateQueueButton();
                         this->updateButtonStates();
                     });
    QObject::connect(&m_treeview, &QAbstractItemView::doubleClicked, [this]() {
        this->jumpToSelected();
    });

    updateModel();
    vbox_content->addWidget(&m_treeview);
    // **** END Track list ****

    // **** START Bottom button bar ****
    QWidget * footerWidget = new QWidget;
    vbox_content->addWidget(footerWidget);
    auto hbox_footer = make_hbox(footerWidget);

    m_closeAfterJump.setChecked(aud_get_bool("audqt", "close_jtf_dialog"));
    m_closeAfterJump.setText(translate_str(N_("C_lose on jump")));
    hbox_footer->addWidget(&m_closeAfterJump);

    QDialogButtonBox * bbox = new QDialogButtonBox(QDialogButtonBox::Close);
    bbox->addButton(&m_queueAndUnqueueButton, QDialogButtonBox::ApplyRole);
    m_queueAndUnqueueButton.setText(translate_str(N_("_Queue")));
    bbox->addButton(&m_jumpButton, QDialogButtonBox::AcceptRole);
    m_jumpButton.setText(translate_str(N_("_Jump")));
    m_jumpButton.setIcon(QIcon::fromTheme("go-jump"));
    QPushButton * btn_Close = bbox->button(QDialogButtonBox::Close);
    btn_Close->setText(translate_str(N_("_Close")));
    btn_Close->setIcon(QIcon::fromTheme("window-close"));
    hbox_footer->addWidget(bbox);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(&m_closeAfterJump, &QCheckBox::checkStateChanged, [](Qt::CheckState state) {
#else
    connect(&m_closeAfterJump, &QCheckBox::stateChanged, [](int state) {
#endif
        aud_set_bool("audqt", "close_jtf_dialog", (state == Qt::Checked));
    });

    QObject::connect(&m_queueAndUnqueueButton, &QPushButton::clicked, [this]() {
        this->m_songListModel.toggleQueueSelected();
        this->updateQueueButton();
    });

    QObject::connect(&m_jumpButton, &QPushButton::clicked, [this]() {
        this->jumpToSelected();
    });

    QObject::connect(btn_Close, &QPushButton::clicked, this, &QDialog::close);
    // **** END Bottom button bar ****
}

void SongsWindow::keyPressEvent(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Return && event->modifiers() == Qt::ShiftModifier)
    {
        // Let Shift+Enter act as another shortcut for the Queue button
        m_queueAndUnqueueButton.animateClick();
    }

    QDialog::keyPressEvent(event);
}

EXPORT void songwin_show()
{
    if (!s_songs_window)
    {
        s_songs_window = new SongsWindow;
        s_songs_window->setAttribute(Qt::WA_DeleteOnClose);
    }

    window_bring_to_front(s_songs_window);
}

EXPORT void songwin_hide()
{
    delete s_songs_window;
}

} // namespace audqt
