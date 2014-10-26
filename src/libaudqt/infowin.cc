/*
 * infowin.cc
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

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/playlist.h>
#include <libaudcore/probe.h>

#include "info-widget.h"
#include "libaudqt.h"

namespace audqt {

class InfoWindow : public QDialog {
public:
    InfoWindow (QWidget * parent = nullptr);
    ~InfoWindow ();

    void fillInfo (int playlist, int entry, const char * filename, const Tuple & tuple,
     PluginHandle * decoder, bool updating_enabled);
    void displayImage (const char * filename);

private:
    QHBoxLayout m_hbox;
    QVBoxLayout m_vbox;
    QLabel m_image;
    InfoWidget m_infowidget;
    QDialogButtonBox m_buttonbox;
};

static InfoWindow * m_infowin = nullptr;

static void infowin_display_image_async (const char * filename, InfoWindow * infowin)
{
    infowin->displayImage (filename);
}

InfoWindow::InfoWindow (QWidget * parent) : QDialog (parent)
{
    m_hbox.addWidget (& m_image);
    m_hbox.addWidget (& m_infowidget);
    m_vbox.addLayout (& m_hbox);
    m_vbox.addWidget (& m_buttonbox);

    setLayout (& m_vbox);
    setWindowTitle (_("Song Info"));

    hook_associate ("art ready", (HookFunction) infowin_display_image_async, this);

    m_buttonbox.setStandardButtons (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QObject::connect (& m_buttonbox, &QDialogButtonBox::accepted, [=] () {
        m_infowidget.updateFile ();
        hide ();
    });

    QObject::connect (& m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::close);
}

InfoWindow::~InfoWindow ()
{
    hook_dissociate ("art ready", (HookFunction) infowin_display_image_async);
}

void InfoWindow::fillInfo (int playlist, int entry, const char * filename, const Tuple & tuple,
 PluginHandle * decoder, bool updating_enabled)
{
    displayImage (filename);
    m_infowidget.fillInfo (playlist, entry, filename, tuple, decoder, updating_enabled);
}

void InfoWindow::displayImage (const char * filename)
{
    QImage img = art_request (filename).scaled (256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    m_image.setPixmap (QPixmap::fromImage (img));
}

EXPORT void infowin_show (int playlist, int entry)
{
    if (! m_infowin)
        m_infowin = new InfoWindow;

    String filename = aud_playlist_entry_get_filename (playlist, entry);
    if (! filename)
        return;

    PluginHandle * decoder = aud_playlist_entry_get_decoder (playlist, entry);
    if (! decoder)
        return;

    Tuple tuple = aud_playlist_entry_get_tuple (playlist, entry);

    if (tuple)
    {
        tuple.delete_fallbacks ();
        m_infowin->fillInfo (playlist, entry, filename, tuple, decoder,
            aud_file_can_write_tuple (filename, decoder));
    }
    else
        aud_ui_show_error (str_printf (_("No info available for %s.\n"),
            (const char *) filename));

    m_infowin->resize (700, 300);

    window_bring_to_front (m_infowin);
}

EXPORT void infowin_show_current ()
{
    int playlist = aud_playlist_get_playing ();
    int position;

    if (playlist == -1)
        playlist = aud_playlist_get_active ();

    position = aud_playlist_get_position (playlist);

    if (position == -1)
        return;

    infowin_show (playlist, position);
}

EXPORT void infowin_hide ()
{
    if (! m_infowin)
        return;

    m_infowin->hide ();
}

} // namespace audqt
