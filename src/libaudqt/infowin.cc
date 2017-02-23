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

class InfoWindow : public QDialog
{
public:
    InfoWindow (QWidget * parent = nullptr);

    void fillInfo (const char * filename, const Tuple & tuple,
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
    setContentsMargins (margins.TwoPt);

    auto hbox = make_hbox (nullptr);
    hbox->addWidget (& m_image);
    hbox->addWidget (& m_infowidget);

    auto vbox = make_vbox (this);
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

void InfoWindow::fillInfo (const char * filename, const Tuple & tuple,
 PluginHandle * decoder, bool updating_enabled)
{
    m_filename = String (filename);
    displayImage (filename);
    m_infowidget.fillInfo (filename, tuple, decoder, updating_enabled);
}

void InfoWindow::displayImage (const char * filename)
{
    if (! strcmp_safe (filename, m_filename))
        m_image.setPixmap (art_request (filename, 2 * sizes.OneInch, 2 * sizes.OneInch));
}

static InfoWindow * s_infowin = nullptr;

static void show_infowin (const char * filename,
 const Tuple & tuple, PluginHandle * decoder, bool can_write)
{
    if (! s_infowin)
    {
        s_infowin = new InfoWindow;
        s_infowin->setAttribute (Qt::WA_DeleteOnClose);

        QObject::connect (s_infowin, & QObject::destroyed, [] () {
            s_infowin = nullptr;
        });
    }

    s_infowin->fillInfo (filename, tuple, decoder, can_write);
    s_infowin->resize (6 * sizes.OneInch, 3 * sizes.OneInch);
    window_bring_to_front (s_infowin);
}

EXPORT void infowin_show (Playlist playlist, int entry)
{
    String filename = playlist.entry_filename (entry);
    if (! filename)
        return;

    String error;
    PluginHandle * decoder = playlist.entry_decoder (entry, Playlist::Wait, & error);
    Tuple tuple = decoder ? playlist.entry_tuple (entry, Playlist::Wait, & error) : Tuple ();

    if (decoder && tuple.valid () && ! aud_custom_infowin (filename, decoder))
    {
        /* cuesheet entries cannot be updated */
        bool can_write = aud_file_can_write_tuple (filename, decoder) &&
         ! tuple.is_set (Tuple::StartTime);

        tuple.delete_fallbacks ();
        show_infowin (filename, tuple, decoder, can_write);
    }
    else
        infowin_hide ();

    if (error)
        aud_ui_show_error (str_printf (_("Error opening %s:\n%s"),
         (const char *) filename, (const char *) error));
}

EXPORT void infowin_show_current ()
{
    auto playlist = Playlist::playing_playlist ();
    if (playlist == Playlist ())
        playlist = Playlist::active_playlist ();

    int position = playlist.get_position ();
    if (position < 0)
        return;

    infowin_show (playlist, position);
}

EXPORT void infowin_hide ()
{
    delete s_infowin;
}

} // namespace audqt
