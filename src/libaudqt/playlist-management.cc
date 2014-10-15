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

#include <QtGui>
#include <QtWidgets>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <libaudcore/playlist.h>

#include "libaudqt.h"
#include "playlist-management.h"
#include "playlist-management.moc"

namespace audqt {

PlaylistDeleteDialog::PlaylistDeleteDialog (int playlist, QWidget * parent) : QDialog (parent)
{
    m_playlist_uniqid = aud_playlist_get_unique_id (playlist);

    m_prompt.setText (QString (translate_str (str_printf (_("Do you want to permanently remove “%s”?"),
     (const char *) aud_playlist_get_title (playlist)))));
    m_skip_prompt.setText (translate_str (_("_Don’t ask again")));

    m_remove.setText (translate_str (_("_Remove")));
    m_cancel.setText (translate_str (_("_Cancel")));

    m_buttonbox.addButton (& m_remove, QDialogButtonBox::AcceptRole);
    m_buttonbox.addButton (& m_cancel, QDialogButtonBox::RejectRole);

    connect (& m_buttonbox, &QDialogButtonBox::accepted, this, &PlaylistDeleteDialog::acceptedTrigger);
    connect (& m_buttonbox, &QDialogButtonBox::rejected, this, &QDialog::close);

    connect (& m_skip_prompt, &QCheckBox::stateChanged, [=] (int state) {
        aud_set_bool ("audgui", "no_confirm_playlist_delete", (state == Qt::Checked));
    });

    m_layout.addWidget (& m_prompt);
    m_layout.addWidget (& m_buttonbox);
    m_layout.addWidget (& m_skip_prompt);

    setLayout (& m_layout);
    setWindowTitle (_("Remove Playlist"));
}

PlaylistDeleteDialog::~PlaylistDeleteDialog ()
{
}

void PlaylistDeleteDialog::acceptedTrigger ()
{
    int list = aud_playlist_by_unique_id (m_playlist_uniqid);

    if (list >= 0)
        aud_playlist_delete (list);

    close ();
}

EXPORT void playlist_confirm_delete (int playlist)
{
    if (aud_get_bool ("audgui", "no_confirm_playlist_delete"))
    {
        aud_playlist_delete (playlist);
        return;
    }

    PlaylistDeleteDialog *d = new PlaylistDeleteDialog (playlist);
    d->show ();
}

}
