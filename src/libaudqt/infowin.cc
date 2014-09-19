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

#include <QtGui>
#include <QtWidgets>

#include <libaudcore/audstrings.h>
#include <libaudcore/playlist.h>
#include <libaudcore/probe.h>
#include <libaudcore/runtime.h>
#include <libaudcore/interface.h>
#include <libaudcore/tuple.h>

#include <libaudqt/libaudqt.h>

namespace audqt {

class InfoWindow : public QDialog {
public:
    InfoWindow (QWidget * parent = nullptr);
    ~InfoWindow ();

    void fillInfo (int playlist, int entry, const char * filename, const Tuple & tuple,
     PluginHandle * decoder, bool updating_enabled);
    void displayImage (const char * filename);

private:
    QHBoxLayout m_layout;
    QLabel m_image;
};

InfoWindow::InfoWindow (QWidget * parent) : QDialog (parent)
{
    m_layout.addWidget (& m_image);
    setLayout (& m_layout);
    setWindowTitle (_("Track Information"));
}

InfoWindow::~InfoWindow ()
{
}

void InfoWindow::fillInfo (int playlist, int entry, const char * filename, const Tuple & tuple,
 PluginHandle * decoder, bool updating_enabled)
{
    displayImage (filename);
}

void InfoWindow::displayImage (const char * filename)
{
    QImage img = art_request (filename).scaled (128, 128, Qt::KeepAspectRatio);

    m_image.setPixmap (QPixmap::fromImage (img));
}

static InfoWindow * m_infowin = nullptr;

EXPORT void infowin_show (int playlist, int entry)
{
    if (! m_infowin)
        m_infowin = new InfoWindow;

    String filename = aud_playlist_entry_get_filename (playlist, entry);
    if (! filename)
        return;

    PluginHandle * decoder = aud_playlist_entry_get_decoder (playlist, entry, false);
    if (! decoder)
        return;

    Tuple tuple = aud_playlist_entry_get_tuple (playlist, entry, false);
    if (tuple)
        m_infowin->fillInfo (playlist, entry, filename, tuple, decoder,
            aud_file_can_write_tuple (filename, decoder));
    else
        aud_ui_show_error (str_printf (_("No info available for %s.\n"),
            (const char *) filename));

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

}
