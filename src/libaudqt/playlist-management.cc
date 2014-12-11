/*
 * playlist-management.cc
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

#include "libaudqt.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <libaudcore/playlist.h>

namespace audqt {

static QDialog * buildDeleteDialog (int playlist)
{
    auto dialog = new QDialog;

    auto prompt = new QLabel ((const char *) str_printf
     (_("Do you want to permanently remove “%s”?"),
     (const char *) aud_playlist_get_title (playlist)), dialog);

    auto skip_prompt = new QCheckBox (translate_str (N_("_Don’t ask again")), dialog);

    auto remove = new QPushButton (translate_str (N_("_Remove")), dialog);
    auto cancel = new QPushButton (translate_str (N_("_Cancel")), dialog);

    auto buttonbox = new QDialogButtonBox (dialog);
    buttonbox->addButton (remove, QDialogButtonBox::AcceptRole);
    buttonbox->addButton (cancel, QDialogButtonBox::RejectRole);

    int id = aud_playlist_get_unique_id (playlist);
    QObject::connect (buttonbox, &QDialogButtonBox::accepted, [dialog, id] () {
        int list = aud_playlist_by_unique_id (id);
        if (list >= 0)
            aud_playlist_delete (list);
        dialog->close ();
    });

    QObject::connect (buttonbox, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    QObject::connect (skip_prompt, &QCheckBox::stateChanged, [] (int state) {
        aud_set_bool ("audgui", "no_confirm_playlist_delete", (state == Qt::Checked));
    });

    auto layout = new QVBoxLayout (dialog);
    layout->addWidget (prompt);
    layout->addWidget (skip_prompt);
    layout->addWidget (buttonbox);

    dialog->setWindowTitle (_("Remove Playlist"));

    return dialog;
}

EXPORT void playlist_confirm_delete (int playlist)
{
    if (aud_get_bool ("audgui", "no_confirm_playlist_delete"))
    {
        aud_playlist_delete (playlist);
        return;
    }

    auto dialog = buildDeleteDialog (playlist);
    dialog->setAttribute (Qt::WA_DeleteOnClose);
    dialog->show ();
}

} // namespace audqt
