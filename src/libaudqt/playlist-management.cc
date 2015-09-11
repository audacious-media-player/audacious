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
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <libaudcore/playlist.h>

namespace audqt {

static QDialog * buildRenameDialog (int playlist)
{
    auto dialog = new QDialog;
    auto prompt = new QLabel (_("What would you like to call this playlist?"), dialog);
    auto entry = new QLineEdit ((const char *) aud_playlist_get_title (playlist), dialog);
    auto rename = new QPushButton (translate_str (N_("_Rename")), dialog);
    auto cancel = new QPushButton (translate_str (N_("_Cancel")), dialog);

    auto buttonbox = new QDialogButtonBox (dialog);
    buttonbox->addButton (rename, QDialogButtonBox::AcceptRole);
    buttonbox->addButton (cancel, QDialogButtonBox::RejectRole);

    int id = aud_playlist_get_unique_id (playlist);
    QObject::connect (buttonbox, & QDialogButtonBox::accepted, [dialog, entry, id] () {
        int list = aud_playlist_by_unique_id (id);
        if (list >= 0)
            aud_playlist_set_title (list, entry->text ().toUtf8 ());
        dialog->close ();
    });

    QObject::connect (buttonbox, & QDialogButtonBox::rejected, dialog, & QDialog::close);

    auto layout = new QVBoxLayout (dialog);
    layout->addWidget (prompt);
    layout->addWidget (entry);
    layout->addWidget (buttonbox);

    dialog->setWindowTitle (_("Rename Playlist"));
    entry->selectAll ();

    return dialog;
}

static QDialog * buildDeleteDialog (int playlist)
{
    auto dialog = new QMessageBox;
    auto skip_prompt = new QCheckBox (translate_str (N_("_Don’t ask again")), dialog);
    auto remove = new QPushButton (translate_str (N_("_Remove")), dialog);
    auto cancel = new QPushButton (translate_str (N_("_Cancel")), dialog);

    dialog->setIcon (QMessageBox::Question);
    dialog->setWindowTitle (_("Remove Playlist"));
    dialog->setText ((const char *) str_printf (_("Do you want to permanently remove “%s”?"),
     (const char *) aud_playlist_get_title (playlist)));
    dialog->setCheckBox (skip_prompt);
    dialog->addButton (remove, QMessageBox::AcceptRole);
    dialog->addButton (cancel, QMessageBox::RejectRole);

    remove->setIcon (QIcon::fromTheme ("edit-delete"));
    cancel->setIcon (QIcon::fromTheme ("process-stop"));
    cancel->setDefault (true);

    QObject::connect (skip_prompt, & QCheckBox::stateChanged, [] (int state) {
        aud_set_bool ("audgui", "no_confirm_playlist_delete", (state == Qt::Checked));
    });

    QObject::connect (remove, & QPushButton::clicked, [dialog, playlist] () {
        int id = aud_playlist_get_unique_id (playlist);
        int list = aud_playlist_by_unique_id (id);
        if (list >= 0)
            aud_playlist_delete (list);
        dialog->close ();
    });

    return dialog;
}

EXPORT void playlist_show_rename (int playlist)
{
    auto dialog = buildRenameDialog (playlist);
    dialog->setAttribute (Qt::WA_DeleteOnClose);
    dialog->show ();
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
