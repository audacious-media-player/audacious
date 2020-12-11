/*
 * volumebutton.cc
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

#include "libaudqt.h"

#include <QIcon>
#include <QMenu>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidgetAction>

#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/runtime.h>

namespace audqt
{

class VolumeButton : public QToolButton
{
public:
    VolumeButton(QWidget * parent = nullptr);

private:
    void updateDelta();
    void updateIcon(int val);
    void updateVolume();
    void setVolume(int val);
    void setUpButton(QToolButton * button, int dir);

    void wheelEvent(QWheelEvent * e);

    QMenu m_menu;
    QWidgetAction m_action;
    QWidget m_container;
    QToolButton m_buttons[2];
    QSlider m_slider;
    int m_scroll_delta = 0;

    HookReceiver<VolumeButton> update_hook{"set volume_delta", this,
                                           &VolumeButton::updateDelta};

    Timer<VolumeButton> m_timer{TimerRate::Hz4, this,
                                &VolumeButton::updateVolume};
};

VolumeButton::VolumeButton(QWidget * parent)
    : QToolButton(parent), m_action(this), m_slider(Qt::Vertical)
{
    m_slider.setMinimumHeight(audqt::sizes.OneInch);
    m_slider.setRange(0, 100);

    setUpButton(&m_buttons[0], 1);
    setUpButton(&m_buttons[1], -1);

    auto layout = make_vbox(&m_container, sizes.TwoPt);
    layout->setContentsMargins(margins.TwoPt);
    layout->addWidget(&m_buttons[0]);
    layout->addWidget(&m_slider);
    layout->addWidget(&m_buttons[1]);
    layout->setAlignment(&m_slider, Qt::AlignHCenter);

    m_action.setDefaultWidget(&m_container);
    m_menu.addAction(&m_action);

    setAutoRaise(true);
    setFocusPolicy(Qt::NoFocus);
    setMenu(&m_menu);
    setPopupMode(InstantPopup);
    setStyleSheet("QToolButton::menu-indicator { image: none; }");

    int val = aud_drct_get_volume_main();
    m_slider.setValue(val);
    m_slider.setFocus();

    updateIcon(val);
    updateDelta();

    connect(&m_slider, &QAbstractSlider::valueChanged, this,
            &VolumeButton::setVolume);

    m_timer.start();
}

void VolumeButton::updateDelta()
{
    int delta = aud_get_int("volume_delta");
    m_slider.setSingleStep(delta);
    m_slider.setPageStep(delta);
}

void VolumeButton::updateIcon(int val)
{
    if (val == 0)
        setIcon(audqt::get_icon("audio-volume-muted"));
    else if (val < 34)
        setIcon(audqt::get_icon("audio-volume-low"));
    else if (val < 67)
        setIcon(audqt::get_icon("audio-volume-medium"));
    else
        setIcon(audqt::get_icon("audio-volume-high"));

    setToolTip(QString("%1 %").arg(val));
}

void VolumeButton::updateVolume()
{
    if (m_slider.isSliderDown())
        return;

    int val = aud_drct_get_volume_main();
    if (val != m_slider.value())
    {
        disconnect(&m_slider, nullptr, this, nullptr);
        m_slider.setValue(val);
        updateIcon(val);
        connect(&m_slider, &QAbstractSlider::valueChanged, this,
                &VolumeButton::setVolume);
    }
}

void VolumeButton::setVolume(int val)
{
    aud_drct_set_volume_main(val);
    updateIcon(val);
}

void VolumeButton::setUpButton(QToolButton * button, int dir)
{
    button->setText(dir < 0 ? "-" : "+");
    button->setAutoRaise(true);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    connect(button, &QAbstractButton::clicked, [this, dir]() {
        m_slider.setValue(m_slider.value() + dir * aud_get_int("volume_delta"));
    });
}

void VolumeButton::wheelEvent(QWheelEvent * e)
{
    m_scroll_delta += e->angleDelta().y();

    /* we want discrete steps here */
    int steps = m_scroll_delta / 120;
    if (steps != 0)
    {
        m_scroll_delta -= 120 * steps;
        m_slider.setValue(m_slider.value() +
                          steps * aud_get_int("volume_delta"));
    }
}

EXPORT QToolButton * volume_button_new(QWidget * parent)
{
    return new VolumeButton(parent);
}

} // namespace audqt
