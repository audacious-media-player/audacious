/*
 * art-qt.cc
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

#include <QApplication>
#include <QIcon>
#include <QImage>
#include <QPixmap>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/probe.h>
#include <libaudcore/runtime.h>
#include <libaudqt/libaudqt.h>

namespace audqt
{

EXPORT QImage art_request(const char * filename, bool * queued)
{
    AudArtPtr art = aud_art_request(filename, AUD_ART_DATA, queued);

    auto data = art.data();
    return data ? QImage::fromData((const uchar *)data->begin(), data->len())
                : QImage();
}

EXPORT QPixmap art_scale(const QImage & image, unsigned int w, unsigned int h,
                         bool want_hidpi)
{
    // return original image if requested size is zero,
    // or original size is smaller than requested size
    if ((w == 0 && h == 0) ||
        ((unsigned)image.width() <= w && (unsigned)image.height() <= h))
        return QPixmap::fromImage(image);

    qreal r = want_hidpi ? qApp->devicePixelRatio() : 1;
    auto pixmap = QPixmap::fromImage(image.scaled(
        w * r, h * r, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    pixmap.setDevicePixelRatio(r);
    return pixmap;
}

EXPORT QPixmap art_request(const char * filename, unsigned int w,
                           unsigned int h, bool want_hidpi)
{
    auto img = art_request(filename);
    if (!img.isNull())
        return art_scale(img, w, h, want_hidpi);

    unsigned size = to_native_dpi(48);
    return QIcon::fromTheme("audio-x-generic")
        .pixmap(aud::min(w, size), aud::min(h, size));
}

EXPORT QPixmap art_request_current(unsigned int w, unsigned int h,
                                   bool want_hidpi)
{
    String filename = aud_drct_get_filename();
    if (!filename)
        return QPixmap();

    return art_request(filename, w, h, want_hidpi);
}

} // namespace audqt
