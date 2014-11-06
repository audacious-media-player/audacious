/*
 * volumebutton.cc
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

#include "volumebutton.h"
#include "libaudqt.h"

#include <QApplication>
#include <QIcon>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>

#include <libaudcore/drct.h>

namespace audqt {

EXPORT VolumeButton::VolumeButton (QWidget * parent) :
    QToolButton (parent)
{
    setFocusPolicy (Qt::NoFocus);

    auto layout = new QVBoxLayout (this);
    layout->setContentsMargins (6, 6, 6, 6);

    m_slider = new QSlider (Qt::Vertical, this);
    m_slider->setRange (0, 100);

    layout->addWidget (m_slider);

    m_container = new QWidget;
    m_container->setLayout (layout);

    updateVolume ();

    connect (this, & QAbstractButton::clicked, this, & VolumeButton::showSlider);
    connect (m_slider, & QAbstractSlider::valueChanged, this, & VolumeButton::setVolume);
}

EXPORT void VolumeButton::updateIcon (int val)
{
    if (val == 0)
        setIcon (QIcon::fromTheme ("audio-volume-off"));
    else if (val > 0 && val < 35)
        setIcon (QIcon::fromTheme ("audio-volume-low"));
    else if (val >= 35 && val < 70)
        setIcon (QIcon::fromTheme ("audio-volume-medium"));
    else if (val >= 70)
        setIcon (QIcon::fromTheme ("audio-volume-high"));
}

EXPORT void VolumeButton::updateVolume ()
{
    int val = aud_drct_get_volume_main ();
    updateIcon (val);
    m_slider->setValue (val);
}

EXPORT void VolumeButton::showSlider ()
{
    updateVolume ();

    m_container->setWindowFlags (Qt::Popup);
    m_container->move (mapToGlobal (QPoint (0, 0)));

    window_bring_to_front (m_container);
}

EXPORT void VolumeButton::setVolume (int val)
{
    aud_drct_set_volume_main (val);
    updateIcon (val);
}

} // namespace audqt
