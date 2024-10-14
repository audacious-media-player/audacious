/*
 * playlist-management.cc
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

#include "libaudqt.h"

#include <QCheckBox>
#include <QInputDialog>
#include <QPushButton>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>
#include <libaudcore/runtime.h>

namespace audqt
{

static QDialog * buildRenameDialog(Playlist playlist)
{
    auto dialog = new QInputDialog;
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(_("Rename Playlist"));
    dialog->setWindowRole("rename-playlist");
    dialog->setLabelText(_("What would you like to call this playlist?"));
    dialog->setOkButtonText(translate_str(N_("_Rename")));
    dialog->setCancelButtonText(translate_str(N_("_Cancel")));
    dialog->setTextValue((const char *)playlist.get_title());

    QObject::connect(dialog, &QInputDialog::textValueSelected,
                     [dialog, playlist](const QString & text) {
                         playlist.set_title(text.toUtf8());
                         dialog->close();
                     });

    return dialog;
}

static QDialog * buildDeleteDialog(Playlist playlist)
{
    auto dialog = new QMessageBox;
    auto skip_prompt =
        new QCheckBox(translate_str(N_("_Don’t ask again")), dialog);
    auto remove = new QPushButton(translate_str(N_("_Remove")), dialog);
    auto cancel = new QPushButton(translate_str(N_("_Cancel")), dialog);

    dialog->setIcon(QMessageBox::Question);
    dialog->setWindowTitle(_("Remove Playlist"));
    dialog->setWindowRole("remove-playlist");
    dialog->setText(
        (const char *)str_printf(_("Do you want to permanently remove “%s”?"),
                                 (const char *)playlist.get_title()));
    dialog->setCheckBox(skip_prompt);
    dialog->addButton(remove, QMessageBox::AcceptRole);
    dialog->addButton(cancel, QMessageBox::RejectRole);
    dialog->setDefaultButton(remove);

    remove->setIcon(QIcon::fromTheme("edit-delete"));
    cancel->setIcon(QIcon::fromTheme("process-stop"));

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    QObject::connect(skip_prompt, &QCheckBox::checkStateChanged, [](Qt::CheckState state) {
#else
    QObject::connect(skip_prompt, &QCheckBox::stateChanged, [](int state) {
#endif
        aud_set_bool("audgui", "no_confirm_playlist_delete",
                     (state == Qt::Checked));
    });

    QObject::connect(remove, &QPushButton::clicked, [dialog, playlist]() {
        playlist.remove_playlist();
        dialog->close();
    });

    return dialog;
}

EXPORT void playlist_show_rename(Playlist playlist)
{
    auto dialog = buildRenameDialog(playlist);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

EXPORT void playlist_confirm_delete(Playlist playlist)
{
    if (aud_get_bool("audgui", "no_confirm_playlist_delete"))
    {
        playlist.remove_playlist();
        return;
    }

    auto dialog = buildDeleteDialog(playlist);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

} // namespace audqt
