/*
 * url-opener.cc
 * Copyright 2015 Thomas Lange
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

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"

namespace audqt {

static QDialog * buildUrlDialog (bool open)
{
    static const PreferencesWidget widgets[] = {
        WidgetCheck (N_("_Save to history"),
            WidgetBool (0, "save_url_history"))
    };

    const char * title, * verb, * icon;

    if (open)
    {
        title = _("Open URL");
        verb = _("_Open");
        icon = "document-open";
    }
    else
    {
        title = _("Add URL");
        verb = _("_Add");
        icon = "list-add";
    }

    auto dialog = new QDialog;
    dialog->setWindowTitle (title);
    dialog->setContentsMargins (margins.EightPt);

    auto label = new QLabel (_("Enter URL:"), dialog);

    auto combobox = new QComboBox (dialog);
    combobox->setEditable (true);
    combobox->setMinimumContentsLength (50);

    auto clear_button = new QPushButton (translate_str (N_("C_lear history")), dialog);
    clear_button->setIcon (QIcon::fromTheme ("edit-clear"));

    auto hbox = make_hbox (nullptr);
    prefs_populate (hbox, widgets, PACKAGE);
    hbox->addStretch (1);
    hbox->addWidget (clear_button);

    auto button1 = new QPushButton (translate_str (verb), dialog);
    button1->setIcon (QIcon::fromTheme (icon));

    auto button2 = new QPushButton (translate_str (N_("_Cancel")), dialog);
    button2->setIcon (QIcon::fromTheme ("process-stop"));

    auto buttonbox = new QDialogButtonBox (dialog);
    buttonbox->addButton (button1, QDialogButtonBox::AcceptRole);
    buttonbox->addButton (button2, QDialogButtonBox::RejectRole);

    auto layout = make_vbox (dialog);
    layout->addWidget (label);
    layout->addWidget (combobox);
    layout->addLayout (hbox);
    layout->addStretch (1);
    layout->addWidget (buttonbox);

    for (int i = 0;; i ++)
    {
        String item = aud_history_get (i);
        if (! item)
            break;

        combobox->addItem (QString (item));
    }
    combobox->setCurrentIndex (-1);

    QObject::connect (clear_button, & QPushButton::pressed, [combobox] () {
        combobox->clear ();
        aud_history_clear ();
    });

    QObject::connect (buttonbox, & QDialogButtonBox::rejected, dialog, & QDialog::close);

    QObject::connect (buttonbox, & QDialogButtonBox::accepted, [dialog, combobox, open] () {
        QByteArray url = combobox->currentText ().toUtf8 ();

        if (open)
            aud_drct_pl_open (url);
        else
            aud_drct_pl_add (url, -1);

        if (aud_get_bool (nullptr, "save_url_history"))
            aud_history_add (url);

        dialog->close ();
    });

    return dialog;
}

static QDialog * s_dialog = nullptr;

EXPORT void urlopener_show (bool open)
{
    if (! s_dialog)
    {
        s_dialog = buildUrlDialog (open);
        s_dialog->setAttribute (Qt::WA_DeleteOnClose);

        QObject::connect (s_dialog, & QObject::destroyed, [] () {
            s_dialog = nullptr;
        });
    }

    window_bring_to_front (s_dialog);
}

} // namespace audqt
