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

#include <QtGui>
#include <QtWidgets>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"
#include "volumebutton.h"
#include "volumebutton.moc"

namespace audqt {

VolumeButton::VolumeButton (QWidget * parent, int min, int max) :
    QToolButton (parent)
{
    setIcon (QIcon::fromTheme ("audio-volume-medium"));

    m_slider = new QSlider (Qt::Vertical);
    m_slider->setRange (min, max);

    m_layout = new QVBoxLayout (this);
    m_layout->setContentsMargins (6, 6, 6, 6);
    m_layout->addWidget (m_slider);

    m_container = new QWidget;
    m_container->setLayout (m_layout);

    connect (this, &QAbstractButton::clicked, this, &VolumeButton::showSlider);

    connect (m_slider, &QAbstractSlider::valueChanged, this, &VolumeButton::handleValueChange);
}

void VolumeButton::showSlider ()
{
    m_container->setWindowFlags (Qt::Popup);
    m_container->move (mapToGlobal (QPoint (0, 0)));

    window_bring_to_front (m_container);
}

void VolumeButton::handleValueChange (int value)
{
    AUDDBG ("volume changed, %d\n", value);

    emit valueChanged (value);
}

void VolumeButton::setValue (int value)
{
    m_slider->setValue (value);
}

}
