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
#include <QPointer>
#include <QRegularExpression>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/playlist.h>
#include <libaudcore/runtime.h>

#include <libaudqt/libaudqt.h>

namespace audqt
{

static aud::array<FileMode, QPointer<QFileDialog>> s_dialogs;

static void import_playlist(Playlist playlist, const char * filename)
{
    playlist.set_filename(filename);
    playlist.insert_entry(0, filename, Tuple(), false);
}

static void export_playlist(Playlist playlist, const char * filename)
{
    if (!playlist.exists())
        return;

    Playlist::GetMode mode = Playlist::Wait;
    if (aud_get_bool("metadata_on_play"))
        mode = Playlist::NoWait;

    playlist.set_filename(filename);
    playlist.save_to_file(filename, mode);
}

static void export_playlist(Playlist playlist, const char * filename,
                            const char * default_ext)
{
    if (uri_get_extension(filename))
        export_playlist(playlist, filename);
    else if (default_ext && default_ext[0])
        export_playlist(playlist, str_concat({filename, ".", default_ext}));
    else
        aud_ui_show_error(_("Please type a filename extension or select a "
                            "format from the drop-down list."));
}

static QStringList get_format_filters()
{
    QStringList filters;
    filters.append(QString(_("Select Format by Extension")).append(" (*)"));

    for (auto & format : Playlist::save_formats())
    {
        QStringList extensions;
        for (const char * ext : format.exts)
            extensions.append(QString("*.%1").arg(ext));

        filters.append(QString("%1 (%2)").arg(
            static_cast<const char *>(format.name), extensions.join(' ')));
    }

    return filters;
}

static QString get_extension_from_filter(QString filter)
{
    /*
     * Find first extension in filter string
     * "Audacious Playlists (*.audpl)" -> audpl
     * "M3U Playlists (*.m3u *.m3u8)"  -> m3u
     */
    static QRegularExpression regex(".*\\(\\*\\.(\\S*).*\\)$");
    return regex.match(filter).captured(1);
}

EXPORT void fileopener_show(FileMode mode)
{
    QPointer<QFileDialog> & dialog = s_dialogs[mode];

    if (!dialog)
    {
        static constexpr aud::array<FileMode, const char *> titles{
            N_("Open Files"), N_("Open Folder"),     N_("Add Files"),
            N_("Add Folder"), N_("Import Playlist"), N_("Export Playlist")};

        static constexpr aud::array<FileMode, const char *> labels{
            N_("Open"), N_("Open"),   N_("Add"),
            N_("Add"),  N_("Import"), N_("Export")};

        static constexpr aud::array<FileMode, QFileDialog::FileMode> modes{
            QFileDialog::ExistingFiles, QFileDialog::Directory,
            QFileDialog::ExistingFiles, QFileDialog::Directory,
            QFileDialog::ExistingFile,  QFileDialog::AnyFile};

        String path = aud_get_str("audgui", "filesel_path");
        dialog = new QFileDialog(nullptr, _(titles[mode]), QString(path));

        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setFileMode(modes[mode]);
        if (modes[mode] == QFileDialog::FileMode::Directory)
            dialog->setOption(QFileDialog::ShowDirsOnly);
        dialog->setLabelText(QFileDialog::Accept, _(labels[mode]));
        dialog->setLabelText(QFileDialog::Reject, _("Cancel"));
        dialog->setWindowRole("file-dialog");

        auto playlist = Playlist::active_playlist();

        if (mode == FileMode::ExportPlaylist)
        {
            dialog->setAcceptMode(QFileDialog::AcceptSave);
            dialog->setNameFilters(get_format_filters());

            String filename = playlist.get_filename();
            if (filename)
                dialog->selectUrl(QString(filename));
        }

        QObject::connect(dialog.data(), &QFileDialog::directoryEntered,
                         [](const QString & path) {
                             aud_set_str("audgui", "filesel_path",
                                         path.toUtf8().constData());
                         });

        QObject::connect(
            dialog.data(), &QFileDialog::accepted, [dialog, mode, playlist]() {
                Index<PlaylistAddItem> files;
                for (const QUrl & url : dialog->selectedUrls())
                    files.append(String(url.toEncoded().constData()));

                switch (mode)
                {
                case FileMode::Add:
                case FileMode::AddFolder:
                    aud_drct_pl_add_list(std::move(files), -1);
                    break;
                case FileMode::Open:
                case FileMode::OpenFolder:
                    aud_drct_pl_open_list(std::move(files));
                    break;
                case FileMode::ImportPlaylist:
                    if (files.len() == 1)
                    {
                        import_playlist(Playlist::blank_playlist(),
                                        files[0].filename);
                    }
                    break;
                case FileMode::ExportPlaylist:
                    if (files.len() == 1)
                    {
                        QString filter = dialog->selectedNameFilter();
                        QString extension = get_extension_from_filter(filter);
                        export_playlist(playlist, files[0].filename,
                                        extension.toUtf8());
                    }
                    break;
                default:
                    /* not reached */
                    break;
                }
            });
    }

    window_bring_to_front(dialog);
}

} // namespace audqt
