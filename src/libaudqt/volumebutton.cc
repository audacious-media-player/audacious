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

#include <QIcon>
#include <QSlider>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <libaudcore/drct.h>
#include <libaudcore/hook.h>

namespace audqt {

EXPORT VolumeButton::VolumeButton (QWidget * parent) :
    QToolButton (parent)
{
    setFocusPolicy (Qt::NoFocus);

    m_container = new QWidget (this);
    m_container->setWindowFlags (Qt::Popup);

    auto layout = new QVBoxLayout (m_container);
    layout->setContentsMargins (6, 6, 6, 6);

    m_slider = new QSlider (Qt::Vertical, this);
    m_slider->setRange (0, 100);
    m_slider->setSingleStep (2);
    m_slider->setPageStep (20);

    layout->addWidget (m_slider);

    int val = aud_drct_get_volume_main ();
    m_slider->setValue (val);
    updateIcon (val);

    connect (this, & QAbstractButton::clicked, this, & VolumeButton::showSlider);
    connect (m_slider, & QAbstractSlider::valueChanged, this, & VolumeButton::setVolume);

    auto timer = new Timer<VolumeButton> (TimerRate::Hz4, this, & VolumeButton::updateVolume);
    connect (this, & QObject::destroyed, [timer] () { delete timer; });

    timer->start ();
}

void VolumeButton::updateIcon (int val)
{
    if (val == 0)
        setIcon (QIcon::fromTheme ("audio-volume-muted"));
    else if (val > 0 && val < 35)
        setIcon (QIcon::fromTheme ("audio-volume-low"));
    else if (val >= 35 && val < 70)
        setIcon (QIcon::fromTheme ("audio-volume-medium"));
    else if (val >= 70)
        setIcon (QIcon::fromTheme ("audio-volume-high"));

    setToolTip (QString ("%1 %").arg (val));
}

void VolumeButton::updateVolume ()
{
    if (m_slider->isSliderDown ())
        return;

    int val = aud_drct_get_volume_main ();
    if (val != m_slider->value ())
    {
        disconnect (m_slider, nullptr, this, nullptr);
        m_slider->setValue (val);
        updateIcon (val);
        connect (m_slider, & QAbstractSlider::valueChanged, this, & VolumeButton::setVolume);
    }
}

void VolumeButton::showSlider ()
{
    m_container->move (mapToGlobal (QPoint (0, 0)));
    window_bring_to_front (m_container);
}

void VolumeButton::setVolume (int val)
{
    aud_drct_set_volume_main (val);
    updateIcon (val);
}

void VolumeButton::wheelEvent (QWheelEvent * e)
{
    int val = m_slider->value ();
    int y = e->angleDelta ().y ();

    if (y < 0)
        m_slider->setValue (-- val);
    else
        m_slider->setValue (++ val);
}

} // namespace audqt
