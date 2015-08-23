/*
 * about.cc
 * Copyright 2014 William Pitcock
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
#include <QFile>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"

static QTabWidget * buildCreditsNotebook (QWidget * parent)
{
    const char * data_dir = aud_get_path (AudPath::DataDir);
    const char * titles[2] = {N_("Credits"), N_("License")};
    const char * filenames[2] = {"AUTHORS", "COPYING"};

    auto tabs = new QTabWidget (parent);

    for (int i = 0; i < 2; i ++)
    {
        QFile f (QString (filename_build ({data_dir, filenames[i]})));
        if (! f.open (QIODevice::ReadOnly))
            continue;

        QTextStream in (& f);

        auto edit = new QPlainTextEdit (in.readAll ().trimmed (), parent);
        edit->setReadOnly (true);
        tabs->addTab (edit, _(titles[i]));

        f.close ();
    }

    return tabs;
}

static QDialog * buildAboutWindow ()
{
    const char * data_dir = aud_get_path (AudPath::DataDir);
    const char * logo_path = filename_build ({data_dir, "images", "about-logo.png"});
    const char * about_text = "<big><b>Audacious " VERSION "</b></big><br>" COPYRIGHT;
    const char * website = "http://audacious-media-player.org";

    auto window = new QDialog;

    auto logo = new QLabel (window);
    logo->setPixmap (QPixmap (logo_path));
    logo->setAlignment (Qt::AlignHCenter);

    auto text = new QLabel (about_text, window);
    text->setAlignment (Qt::AlignHCenter);

    auto anchor = QString ("<a href='%1'>%1</a>").arg (website);
    auto link_label = new QLabel (anchor, window);
    link_label->setAlignment (Qt::AlignHCenter);
    link_label->setContentsMargins (0, 5, 0, 0);
    link_label->setOpenExternalLinks (true);

    auto layout = new QVBoxLayout (window);
    layout->addWidget (logo);
    layout->addWidget (text);
    layout->addWidget (link_label);
    layout->addWidget (buildCreditsNotebook (window));

    window->setWindowTitle (_("About Audacious"));
    window->setFixedSize (590, 450);

    return window;
}

static QDialog * s_aboutwin = nullptr;

namespace audqt {

EXPORT void aboutwindow_show ()
{
    if (! s_aboutwin)
    {
        s_aboutwin = buildAboutWindow ();
        s_aboutwin->setAttribute (Qt::WA_DeleteOnClose);

        QObject::connect (s_aboutwin, & QObject::destroyed, [] () {
            s_aboutwin = nullptr;
        });
    }

    window_bring_to_front (s_aboutwin);
}

EXPORT void aboutwindow_hide ()
{
    if (s_aboutwin)
        delete s_aboutwin;
}

} // namespace audqt
