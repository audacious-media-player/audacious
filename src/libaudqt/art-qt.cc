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
#include <QIcon>
#include <QImage>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/probe.h>
#include <libaudcore/runtime.h>
#include <libaudqt/libaudqt.h>

namespace audqt {

EXPORT QPixmap art_request (const char * filename, unsigned int w, unsigned int h, bool want_hidpi)
{
    AudArtPtr art = aud_art_request (filename, AUD_ART_DATA);

    auto data = art.data ();
    auto img = data ? QImage::fromData ((const uchar *) data->begin (), data->len ()) : QImage ();

    if (img.isNull ())
    {
        unsigned size = to_native_dpi (48);
        return get_icon ("audio-x-generic").pixmap (aud::min (w, size), aud::min (h, size));
    }

    // return original image if requested size is zero,
    // or original size is smaller than requested size
    if ((w == 0 && h == 0) || ((unsigned) img.width () <= w && (unsigned) img.height () <= h))
        return QPixmap::fromImage (img);

    if (! want_hidpi)
        return QPixmap::fromImage (img.scaled (w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    qreal r = qApp->devicePixelRatio ();

    QPixmap pm = QPixmap::fromImage (img.scaled (w * r, h * r, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    pm.setDevicePixelRatio (r);

    return pm;
}

EXPORT QPixmap art_request_current (unsigned int w, unsigned int h, bool want_hidpi)
{
    String filename = aud_drct_get_filename ();
    if (! filename)
        return QPixmap ();

    return art_request (filename, w, h, want_hidpi);
}

} // namespace audqt
