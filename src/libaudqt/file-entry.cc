/*
 * file-entry.cc
 * Copyright 2018 John Lindgren
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

#include <QAction>
#include <QLineEdit>
#include <QPointer>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>

namespace audqt
{

class FileEntry : public QLineEdit
{
public:
    FileEntry(QWidget * parent, const char * title,
              QFileDialog::FileMode file_mode,
              QFileDialog::AcceptMode accept_mode)
        : QLineEdit(parent), m_title(title), m_file_mode(file_mode),
          m_accept_mode(accept_mode),
          m_action(get_icon("document-open"), _("Browse"), nullptr)
    {
        addAction(&m_action, TrailingPosition);
        connect(&m_action, &QAction::triggered, this, &FileEntry::show_dialog);
    }

private:
    QFileDialog * create_dialog();
    void show_dialog();

    const QString m_title;
    const QFileDialog::FileMode m_file_mode;
    const QFileDialog::AcceptMode m_accept_mode;

    QAction m_action;
    QPointer<QFileDialog> m_dialog;
};

QFileDialog * FileEntry::create_dialog()
{
    auto dialog = new QFileDialog(this, m_title);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setFileMode(m_file_mode);
    dialog->setAcceptMode(m_accept_mode);

    String uri = file_entry_get_uri(this);
    if (uri)
    {
        // As of Qt 5.11, selectUrl() works fine with the built-in Qt dialog
        // but has no effect on the GTK-based dialog.  As a workaround, try to
        // get a local file path and use selectFile() instead.
        StringBuf local = uri_to_filename(uri);
        if (local)
            dialog->selectFile((const char *)local);
        else
            dialog->selectUrl(QUrl((const char *)uri));
    }

    QObject::connect(dialog, &QFileDialog::accepted, [this, dialog]() {
        auto urls = dialog->selectedUrls();
        if (urls.length() == 1)
            file_entry_set_uri(this, urls[0].toEncoded().constData());
    });

    return dialog;
}

void FileEntry::show_dialog()
{
    if (!m_dialog)
        m_dialog = create_dialog();

    window_bring_to_front(m_dialog);
}

EXPORT QLineEdit * file_entry_new(QWidget * parent, const char * title,
                                  QFileDialog::FileMode file_mode,
                                  QFileDialog::AcceptMode accept_mode)
{
    return new FileEntry(parent, title, file_mode, accept_mode);
}

EXPORT String file_entry_get_uri(QLineEdit * entry)
{
    QByteArray text = entry->text().toUtf8();

    if (text.isEmpty())
        return String();
    else if (strstr(text, "://"))
        return String(text);
    else
        return String(filename_to_uri(
            filename_normalize(filename_expand(str_copy(text)))));
}

EXPORT void file_entry_set_uri(QLineEdit * entry, const char * uri)
{
    if (!uri || !uri[0])
    {
        entry->clear();
        return;
    }

    StringBuf path = uri_to_filename(uri, false);
    entry->setText(path ? filename_contract(std::move(path)) : uri);
    entry->end(false);
}

} // namespace audqt
