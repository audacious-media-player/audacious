/*
 * fileopener.cc
 * Copyright 2014 Michał Lipski
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

#include <QFileDialog>

#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include <libaudqt/libaudqt.h>

namespace audqt {

static aud::array<FileMode, QFileDialog *> s_dialogs;

EXPORT void fileopener_show (FileMode mode)
{
    QFileDialog * & dialog = s_dialogs[mode];

    if (! dialog)
    {
        static constexpr aud::array<FileMode, const char *> titles {
            N_("Open Files"),
            N_("Open Folder"),
            N_("Add Files"),
            N_("Add Folder")
        };

        static constexpr aud::array<FileMode, const char *> labels {
            N_("Open"),
            N_("Open"),
            N_("Add"),
            N_("Add")
        };

        static constexpr aud::array<FileMode, QFileDialog::FileMode> modes {
            QFileDialog::ExistingFiles,
            QFileDialog::Directory,
            QFileDialog::ExistingFiles,
            QFileDialog::Directory
        };

        String path = aud_get_str ("audgui", "filesel_path");
        dialog = new QFileDialog (nullptr, _(titles[mode]), QString (path));

        dialog->setAttribute (Qt::WA_DeleteOnClose);
        dialog->setFileMode (modes[mode]);
        dialog->setLabelText (QFileDialog::Accept, _(labels[mode]));

        QObject::connect (dialog, & QFileDialog::directoryEntered, [] (const QString & path)
            { aud_set_str ("audgui", "filesel_path", path.toUtf8 ().constData ()); });

        QObject::connect (dialog, & QFileDialog::accepted, [dialog, mode] ()
        {
            Index<PlaylistAddItem> files;
            for (const QUrl & url : dialog->selectedUrls ())
                files.append (String (url.toEncoded ().constData ()));

            if (mode == FileMode::Add || mode == FileMode::AddFolder)
                aud_drct_pl_add_list (std::move (files), -1);
            else
                aud_drct_pl_open_list (std::move (files));
        });

        QObject::connect (dialog, & QObject::destroyed, [& dialog] ()
            { dialog = nullptr; });
    }

    window_bring_to_front (dialog);
}

} // namespace audqt
