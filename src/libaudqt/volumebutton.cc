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

#include "libaudqt.h"

#include <QFrame>
#include <QIcon>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/runtime.h>

namespace audqt {

class VolumeButton : public QToolButton
{
public:
    VolumeButton (QWidget * parent = nullptr);

private:
    void updateDelta ();
    void updateIcon (int val);
    void updateVolume ();
    void showSlider ();
    void setVolume (int val);
    QToolButton * newSliderButton (int dir);

    void wheelEvent (QWheelEvent * e);

    QSlider * m_slider;
    QFrame * m_container;
    int m_scroll_delta = 0;

    HookReceiver<VolumeButton> update_hook
        {"set volume_delta", this, & VolumeButton::updateDelta};
};

VolumeButton::VolumeButton (QWidget * parent) :
    QToolButton (parent)
{
    setFocusPolicy (Qt::NoFocus);

    m_container = new QFrame (this, Qt::Popup);
    m_container->setFrameShape (QFrame::StyledPanel);

    m_slider = new QSlider (Qt::Vertical, this);
    m_slider->setMinimumHeight (audqt::sizes.OneInch);
    m_slider->setRange (0, 100);

    updateDelta ();

    auto layout = make_vbox (m_container, sizes.TwoPt);
    layout->setContentsMargins (margins.TwoPt);

    layout->addWidget (newSliderButton (1));
    layout->addWidget (m_slider);
    layout->addWidget (newSliderButton (-1));

    int val = aud_drct_get_volume_main ();
    m_slider->setValue (val);
    updateIcon (val);

    connect (this, & QAbstractButton::clicked, this, & VolumeButton::showSlider);
    connect (m_slider, & QAbstractSlider::valueChanged, this, & VolumeButton::setVolume);

    auto timer = new Timer<VolumeButton> (TimerRate::Hz4, this, & VolumeButton::updateVolume);
    connect (this, & QObject::destroyed, [timer] () { delete timer; });

    timer->start ();
}

void VolumeButton::updateDelta ()
{
    int delta = aud_get_int ("volume_delta");
    m_slider->setSingleStep (delta);
    m_slider->setPageStep (delta);
}

void VolumeButton::updateIcon (int val)
{
    if (val == 0)
        setIcon (audqt::get_icon ("audio-volume-muted"));
    else if (val < 34)
        setIcon (audqt::get_icon ("audio-volume-low"));
    else if (val < 67)
        setIcon (audqt::get_icon ("audio-volume-medium"));
    else
        setIcon (audqt::get_icon ("audio-volume-high"));

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
    QSize button_size = sizeHint ();
    QSize container_size = m_container->sizeHint ();

    int dx = container_size.width () / 2 - button_size.width () / 2;
    int dy = container_size.height () / 2 - button_size.height () / 2;

    QPoint pos = mapToGlobal (QPoint (0, 0));
    pos -= QPoint (dx, dy);
    pos.setX(qMax(pos.x(), 0));
    pos.setY(qMax(pos.y(), 0));

    m_container->move (pos);
    window_bring_to_front (m_container);
}

void VolumeButton::setVolume (int val)
{
    aud_drct_set_volume_main (val);
    updateIcon (val);
}

QToolButton * VolumeButton::newSliderButton (int dir)
{
    auto button = new QToolButton (this);
    button->setText (dir < 0 ? "-" : "+");
    button->setAutoRaise (true);
    button->setSizePolicy (QSizePolicy::Ignored, QSizePolicy::Preferred);

    connect (button, & QAbstractButton::clicked, [this, dir] () {
        m_slider->setValue (m_slider->value () + dir * aud_get_int ("volume_delta"));
    });

    return button;
}

void VolumeButton::wheelEvent (QWheelEvent * e)
{
    m_scroll_delta += e->angleDelta ().y ();

    /* we want discrete steps here */
    int steps = m_scroll_delta / 120;
    if (steps != 0)
    {
        m_scroll_delta -= 120 * steps;
        m_slider->setValue (m_slider->value () + steps * aud_get_int ("volume_delta"));
    }
}

EXPORT QToolButton * volume_button_new (QWidget * parent)
{
    return new VolumeButton (parent);
}

} // namespace audqt
