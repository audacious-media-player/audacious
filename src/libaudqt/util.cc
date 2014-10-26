/*
 * util.cc
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

namespace audqt {

/* the goal is to force a window to come to the front on any qt platform */
EXPORT void window_bring_to_front (QWidget * window)
{
    window->show();

    Qt::WindowStates state = window->windowState();

    state &= ~Qt::WindowMinimized;
    state |= Qt::WindowActive;

    window->setWindowState (state);
    window->raise ();
    window->activateWindow ();
}

EXPORT void simple_message (const char * title, const char * text, const char * domain)
{
    QDialog msgbox;
    QVBoxLayout vbox;
    QLabel label;
    QDialogButtonBox bbox;

    label.setText (translate_str (text, domain));
    bbox.setStandardButtons (QDialogButtonBox::Ok);

    QObject::connect (& bbox, &QDialogButtonBox::accepted, & msgbox, &QDialog::accept);

    vbox.addWidget (& label);
    vbox.addWidget (& bbox);

    msgbox.setWindowTitle (title);

    msgbox.setLayout (& vbox);
    msgbox.exec ();
}

/* translate gtk+ accelerators and also handle dgettext() */
EXPORT QString translate_str (const char * str, const char * domain)
{
    /* translate the gtk+ accelerator (_) into a qt accelerator (&) */
    return QString (dgettext (domain ? domain : PACKAGE, str)).replace ('_', '&');
}

} // namespace audqt
