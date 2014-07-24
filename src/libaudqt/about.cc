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

#include <QtGui>
#include <QtWidgets>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"
#include "about.h"
#include "about.moc"

namespace audqt {

static AboutWindow *m_aboutwin = nullptr;

void AboutWindow::buildCreditsNotebook ()
{
    const char * data_dir = aud_get_path (AudPath::DataDir);

    char * credits_path = strdup (filename_build ({data_dir, "AUTHORS"}));
    char * license_path = strdup (filename_build ({data_dir, "COPYING"}));

    const char * titles[2] = {_("Credits"), _("License")};
    const char * paths[2] = {credits_path, license_path};

    for (int i = 0; i < 2; i++)
    {
        QFile f(paths[i]);
        if (!f.open (QIODevice::ReadOnly))
            continue;

        QTextStream in(&f);

        m_textedits[i] = new QPlainTextEdit (in.readAll());
        m_textedits[i]->setReadOnly (true);
        m_tabs.addTab (m_textedits[i], titles[i]);

        f.close();
    }

    free (credits_path);
    free (license_path);
}

AboutWindow::AboutWindow (QWidget * parent) : QDialog (parent)
{
    const char * data_dir = aud_get_path (AudPath::DataDir);
    const char * logo_path = filename_build ({data_dir, "images", "about-logo.png"});
    const char about_text[] =
     "<big><b>Audacious " VERSION "</b></big><br>\n"
     "Copyright © 2001-2014 Audacious developers and others";

    QPixmap pm (logo_path);
    m_logo.setPixmap (pm);
    m_logo.setAlignment (Qt::AlignHCenter);

    m_about_text.setText (about_text);
    m_about_text.setAlignment (Qt::AlignHCenter);

    buildCreditsNotebook ();

    m_layout.addWidget (& m_logo);
    m_layout.addWidget (& m_about_text);
    m_layout.addWidget (& m_tabs);

    setLayout (& m_layout);
    setWindowTitle (_("About Audacious"));

    resize (640, 480);
}

AboutWindow::~AboutWindow ()
{
    for (int i = 0; i < 2; i++)
    {
        if (m_textedits[i])
            delete m_textedits[i];
    }
}

EXPORT void aboutwindow_show ()
{
    if (!m_aboutwin)
        m_aboutwin = new AboutWindow;

    m_aboutwin->show ();
}

EXPORT void aboutwindow_hide ()
{
    if (!m_aboutwin)
        return;

    m_aboutwin->hide ();
}

};
