/*
 * art.cc
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

#include <QApplication>
#include <QPixmap>
#include <QImage>

#include <libaudcore/playlist.h>
#include <libaudcore/probe.h>
#include <libaudcore/runtime.h>

namespace audqt {

EXPORT QPixmap art_request (const char * filename, unsigned int w, unsigned int h, bool want_hidpi)
{
    const Index<char> * data = aud_art_request_data (filename);

    if (! data)
    {
        AUDINFO ("no album art for %s.\n", filename);
        return QPixmap ();
    }

    QImage img = QImage::fromData ((const uchar *) data->begin (), data->len ()).scaled (w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    aud_art_unref (filename);

    QPixmap pm = QPixmap::fromImage (img);

    if (want_hidpi)
        pm.setDevicePixelRatio (qApp->devicePixelRatio ());

    return pm;
}

EXPORT QPixmap art_request_current (unsigned int w, unsigned int h, bool want_hidpi)
{
    int list = aud_playlist_get_playing ();
    int entry = aud_playlist_get_position (list);
    if (entry < 0)
        return QPixmap ();

    String filename = aud_playlist_entry_get_filename (list, entry);
    return art_request (filename, w, h, want_hidpi);
}

} // namespace audqt
