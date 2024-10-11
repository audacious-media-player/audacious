/*
 * infowin-qt.cc
 * Copyright 2006-2014 Ariadne Conill, Tomasz Moń, Eugene Zagidullin,
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

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/playlist.h>
#include <libaudcore/probe.h>

#include "info-widget.h"
#include "libaudqt.h"

namespace audqt
{

class InfoWindow : public QDialog
{
public:
    InfoWindow(QWidget * parent = nullptr);

    void fillInfo(Index<PlaylistAddItem> && items, bool updating_enabled);

private:
    String m_filename;
    QLabel m_image;
    QTextEdit m_uri_label;
    InfoWidget m_infowidget;
    QPushButton * m_save_btn;

    void displayImage(const char * filename);

    const HookReceiver<InfoWindow, const char *> art_hook{
        "art ready", this, &InfoWindow::displayImage};
};

InfoWindow::InfoWindow(QWidget * parent) : QDialog(parent)
{
    setWindowTitle(_("Song Info"));
    setWindowRole("song-info");
    setContentsMargins(margins.TwoPt);

    m_uri_label.setFixedWidth(2 * audqt::sizes.OneInch);
    m_uri_label.setFrameStyle(QFrame::NoFrame);
    m_uri_label.setReadOnly(true);
    m_uri_label.viewport()->setAutoFillBackground(false);

    auto left_vbox = make_vbox(nullptr);
    left_vbox->addWidget(&m_image, 1, Qt::AlignCenter);
    left_vbox->addWidget(&m_uri_label, 1);

    auto hbox = make_hbox(nullptr);
    hbox->addLayout(left_vbox);
    hbox->addWidget(&m_infowidget);

    auto vbox = make_vbox(this);
    vbox->addLayout(hbox);

    auto bbox =
        new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close |
                                 QDialogButtonBox::Reset,
                             this);

    m_save_btn = bbox->button(QDialogButtonBox::Save);
    auto close_btn = bbox->button(QDialogButtonBox::Close),
         revert_btn = bbox->button(QDialogButtonBox::Reset);

    close_btn->setText(translate_str(N_("_Close")));
    revert_btn->setText(translate_str(N_("_Revert")));

    m_infowidget.linkEnabled(m_save_btn);
    m_infowidget.linkEnabled(revert_btn);

    vbox->addWidget(bbox);

    connect(bbox, &QDialogButtonBox::accepted, [this]() {
        if (m_infowidget.updateFile())
            deleteLater();
        else
            aud_ui_show_error(str_printf(_("Error writing tag(s).")));
    });

    connect(bbox, &QDialogButtonBox::rejected, this, &QObject::deleteLater);
    connect(revert_btn, &QPushButton::clicked, &m_infowidget,
            &InfoWidget::revertInfo);
}

void InfoWindow::fillInfo(Index<PlaylistAddItem> && items,
                          bool updating_enabled)
{
    if (items.len() == 1)
    {
        m_filename = String(items[0].filename);
        m_uri_label.setPlainText((QString)uri_to_display(m_filename));
        displayImage(m_filename);
        m_save_btn->setText(translate_str(N_("_Save")));
    }
    else
    {
        m_filename = String();
        m_uri_label.setPlainText(
            translate_str(N_("%1 files selected")).arg(items.len()));
        m_image.setPixmap(
            QIcon::fromTheme("audio-x-generic").pixmap(to_native_dpi(48)));
        m_save_btn->setText(
            translate_str(N_("_Save %1 files")).arg(items.len()));
    }

    m_infowidget.fillInfo(std::move(items), updating_enabled);
}

void InfoWindow::displayImage(const char * filename)
{
    if (!strcmp_safe(filename, m_filename))
        m_image.setPixmap(
            art_request(filename, 2 * sizes.OneInch, 2 * sizes.OneInch));
}

static QPointer<InfoWindow> s_infowin;

static void show_infowin(Index<PlaylistAddItem> && items, bool can_write)
{
    if (!s_infowin)
    {
        s_infowin = new InfoWindow;
        s_infowin->setAttribute(Qt::WA_DeleteOnClose);
    }

    s_infowin->fillInfo(std::move(items), can_write);
    s_infowin->resize(6 * sizes.OneInch, 3 * sizes.OneInch);
    window_bring_to_front(s_infowin);
}

static void fetch_entry(Playlist playlist, int entry,
                        Index<PlaylistAddItem> & items, bool & can_write)
{
    String filename = playlist.entry_filename(entry);
    if (!filename)
        return;

    String error;
    PluginHandle * decoder =
        playlist.entry_decoder(entry, Playlist::Wait, &error);
    Tuple tuple =
        decoder ? playlist.entry_tuple(entry, Playlist::Wait, &error) : Tuple();

    if (decoder && tuple.valid())
    {
        /* cuesheet entries cannot be updated */
        can_write = (can_write && aud_file_can_write_tuple(filename, decoder) &&
                     !tuple.is_set(Tuple::StartTime));

        tuple.delete_fallbacks();
        items.append(filename, std::move(tuple), decoder);
    }

    if (error)
        aud_ui_show_error(str_printf(_("Error opening %s:\n%s"),
                                     (const char *)filename,
                                     (const char *)error));
}

EXPORT void infowin_show(Playlist playlist, int entry)
{
    Index<PlaylistAddItem> items;
    bool can_write = true;

    fetch_entry(playlist, entry, items, can_write);

    if (items.len())
        show_infowin(std::move(items), can_write);
    else
        infowin_hide();
}

EXPORT void infowin_show_selected(Playlist playlist)
{
    Index<PlaylistAddItem> items;
    bool can_write = true;

    int n_entries = playlist.n_entries();
    for (int entry = 0; entry < n_entries; entry++)
    {
        if (playlist.entry_selected(entry))
            fetch_entry(playlist, entry, items, can_write);
    }

    if (items.len())
        show_infowin(std::move(items), can_write);
    else
        infowin_hide();
}

EXPORT void infowin_show_current()
{
    auto playlist = Playlist::playing_playlist();
    if (playlist == Playlist())
        playlist = Playlist::active_playlist();

    int position = playlist.get_position();
    if (position < 0)
        return;

    infowin_show(playlist, position);
}

EXPORT void infowin_hide() { delete s_infowin; }

} // namespace audqt
