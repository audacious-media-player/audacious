/*
 * playlist-management.h
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

#ifndef LIBAUDQT_PLAYLIST_MANAGEMENT_H
#define LIBAUDQT_PLAYLIST_MANAGEMENT_H

namespace audqt {

class PlaylistDeleteDialog : public QDialog {
    Q_OBJECT

public:
    PlaylistDeleteDialog(int playlist, QWidget * parent = 0);
    ~PlaylistDeleteDialog();

public slots:
    void acceptedTrigger ();

private:
    int m_playlist_uniqid;

    QLabel m_prompt;

    QPushButton m_remove;
    QPushButton m_cancel;

    QCheckBox m_skip_prompt;

    QVBoxLayout m_layout;
    QDialogButtonBox m_buttonbox;
};

}

#endif
