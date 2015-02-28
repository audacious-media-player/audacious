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
#include <QPushButton>
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

    void fillInfo (int playlist, int entry, const char * filename, const Tuple & tuple,
     PluginHandle * decoder, bool updating_enabled);

private:
    String m_filename;
    QLabel m_image;
    InfoWidget m_infowidget;

    void displayImage (const char * filename);

    const HookReceiver<InfoWindow, const char *>
     art_hook {"art ready", this, & InfoWindow::displayImage};
};

InfoWindow::InfoWindow (QWidget * parent) : QDialog (parent)
{
    setWindowTitle (_("Song Info"));

    auto hbox = new QHBoxLayout;
    hbox->addWidget (& m_image);
    hbox->addWidget (& m_infowidget);

    auto vbox = new QVBoxLayout (this);
    vbox->addLayout (hbox);

    auto bbox = new QDialogButtonBox (QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    bbox->button (QDialogButtonBox::Save)->setText (translate_str (N_("_Save")));
    bbox->button (QDialogButtonBox::Close)->setText (translate_str (N_("_Close")));
    vbox->addWidget (bbox);

    QObject::connect (bbox, & QDialogButtonBox::accepted, [this] () {
        m_infowidget.updateFile ();
        deleteLater ();
    });

    QObject::connect (bbox, & QDialogButtonBox::rejected, this, & QObject::deleteLater);
}

void InfoWindow::fillInfo (int playlist, int entry, const char * filename, const Tuple & tuple,
 PluginHandle * decoder, bool updating_enabled)
{
    m_filename = String (filename);
    displayImage (filename);
    m_infowidget.fillInfo (playlist, entry, filename, tuple, decoder, updating_enabled);
}

void InfoWindow::displayImage (const char * filename)
{
    if (! strcmp_safe (filename, m_filename))
        m_image.setPixmap (art_request (filename));
}

static InfoWindow * s_infowin = nullptr;

EXPORT void infowin_show (int playlist, int entry)
{
    if (! s_infowin)
    {
        s_infowin = new InfoWindow;
        s_infowin->setAttribute (Qt::WA_DeleteOnClose);

        QObject::connect (s_infowin, & QObject::destroyed, [] () {
            s_infowin = nullptr;
        });
    }

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
        s_infowin->fillInfo (playlist, entry, filename, tuple, decoder,
            aud_file_can_write_tuple (filename, decoder));
    }
    else
        aud_ui_show_error (str_printf (_("No info available for %s.\n"),
            (const char *) filename));

    s_infowin->resize (700, 300);

    window_bring_to_front (s_infowin);
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
    delete s_infowin;
}

} // namespace audqt
