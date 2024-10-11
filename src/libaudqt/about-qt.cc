/*
 * about-qt.cc
 * Copyright 2014 Ariadne Conill
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
#include <QLabel>
#include <QPlainTextEdit>
#include <QPointer>
#include <QTabWidget>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <libaudcore/vfs.h>

#include "libaudqt.h"

static QTabWidget * buildCreditsNotebook(QWidget * parent)
{
    const char * data_dir = aud_get_path(AudPath::DataDir);
    const char * titles[2] = {N_("Credits"), N_("License")};
    const char * filenames[2] = {"AUTHORS", "COPYING"};

    auto tabs = new QTabWidget(parent);
    tabs->setDocumentMode(true);
    tabs->setMinimumSize(6 * audqt::sizes.OneInch, 2 * audqt::sizes.OneInch);

    for (int i = 0; i < 2; i++)
    {
        auto text = VFSFile::read_file(filename_build({data_dir, filenames[i]}),
                                       VFS_APPEND_NULL);
        auto edit = new QPlainTextEdit(text.begin(), parent);
        edit->setReadOnly(true);
        edit->setFrameStyle(QFrame::NoFrame);
        tabs->addTab(edit, _(titles[i]));
    }

    return tabs;
}

static QDialog * buildAboutWindow()
{
    const char * about_text =
        "<big><b>Audacious " VERSION "</b></big><br>" COPYRIGHT;
    const char * website = "https://audacious-media-player.org";

    auto window = new QDialog;
    window->setWindowTitle(_("About Audacious"));
    window->setWindowRole("about");

    auto logo = new QLabel(window);
    int logo_size = audqt::to_native_dpi(400);
    logo->setPixmap(QIcon(":/about-logo.svg").pixmap(logo_size, logo_size));
    logo->setAlignment(Qt::AlignHCenter);

    auto text = new QLabel(about_text, window);
    text->setAlignment(Qt::AlignHCenter);

    auto anchor = QString("<a href=\"%1\">%1</a>").arg(website);
    auto link_label = new QLabel(anchor, window);
    link_label->setAlignment(Qt::AlignHCenter);
    link_label->setOpenExternalLinks(true);

#ifdef Q_OS_MAC
    link_label->setContentsMargins(0, 0, 0, audqt::sizes.EightPt);
#endif

    auto layout = audqt::make_vbox(window);
    layout->addSpacing(audqt::sizes.EightPt);
    layout->addWidget(logo);
    layout->addWidget(text);
    layout->addWidget(link_label);
    layout->addWidget(buildCreditsNotebook(window));

    return window;
}

static QPointer<QDialog> s_aboutwin;

namespace audqt
{

EXPORT void aboutwindow_show()
{
    if (!s_aboutwin)
    {
        s_aboutwin = buildAboutWindow();
        s_aboutwin->setAttribute(Qt::WA_DeleteOnClose);
    }

    window_bring_to_front(s_aboutwin);
}

EXPORT void aboutwindow_hide() { delete s_aboutwin; }

} // namespace audqt
