/*
 * equalizer.cc
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

#include <QCheckBox>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QSlider>
#include <QStyle>
#include <QVBoxLayout>

#include <libaudcore/equalizer.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"

class VLabel : public QLabel
{
public:
    VLabel (const QString & text, QWidget * parent = nullptr) :
        QLabel (text, parent) {}

    QSize minimumSizeHint () const
    {
        QSize s = QLabel::minimumSizeHint ();
        return QSize (s.height (), s.width ());
    }

    QSize sizeHint () const
    {
        QSize s = QLabel::sizeHint ();
        return QSize (s.height (), s.width ());
    }

    void paintEvent (QPaintEvent *)
    {
        QPainter p (this);
        p.rotate (270);

        QRect box (-height (), 0, height (), width ());
        style ()->drawItemText (& p, box, (int) alignment (), palette (),
         isEnabled (), text (), QPalette::Foreground);
    }
};

class EqualizerSlider : public QWidget
{
public:
    QSlider slider;

    EqualizerSlider (const char * label, QWidget * parent) :
        QWidget (parent),
        slider (Qt::Vertical)
    {
        slider.setMinimumHeight (audqt::sizes.OneInch);
        slider.setRange (-AUD_EQ_MAX_GAIN, AUD_EQ_MAX_GAIN);
        slider.setTickInterval (AUD_EQ_MAX_GAIN >> 1);
        slider.setTickPosition (QSlider::TicksBothSides);

        auto layout = audqt::make_vbox (this);
        auto value_label = new QLabel ("0");

        layout->addWidget (new VLabel (label, this), 1, Qt::AlignCenter);
        layout->addWidget (& slider, 0, Qt::AlignCenter);
        layout->addWidget (value_label, 0, Qt::AlignCenter);

        connect (& slider, & QSlider::valueChanged, [value_label] (int value) {
            value_label->setText (QString::number (value));
        });
    }
};

class EqualizerWindow : public QDialog
{
public:
    EqualizerWindow ();

private:
    QCheckBox m_onoff_checkbox;
    EqualizerSlider * m_preamp_slider;
    EqualizerSlider * m_sliders[AUD_EQ_NBANDS];

    void updateActive ();
    void updatePreamp ();
    void updateBands ();

    const HookReceiver<EqualizerWindow>
     hook1 {"set equalizer_active", this, & EqualizerWindow::updateActive},
     hook2 {"set equalizer_preamp", this, & EqualizerWindow::updatePreamp},
     hook3 {"set equalizer_bands", this, & EqualizerWindow::updateBands};
};

EqualizerWindow::EqualizerWindow () :
    m_onoff_checkbox (audqt::translate_str (N_("_Enable")))
{
    const char * const names[AUD_EQ_NBANDS] = {N_("31 Hz"), N_("63 Hz"),
     N_("125 Hz"), N_("250 Hz"), N_("500 Hz"), N_("1 kHz"), N_("2 kHz"),
     N_("4 kHz"), N_("8 kHz"), N_("16 kHz")};

    auto slider_container = new QWidget (this);
    auto slider_layout = audqt::make_hbox (slider_container, audqt::sizes.TwoPt);

    m_preamp_slider = new EqualizerSlider (_("Preamp"), this);
    slider_layout->addWidget (m_preamp_slider);

    auto line = new QFrame (this);
    line->setFrameShape (QFrame::VLine);
    line->setFrameShadow (QFrame::Sunken);
    slider_layout->addWidget (line);

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
    {
        m_sliders[i] = new EqualizerSlider (_(names[i]), this);
        slider_layout->addWidget (m_sliders[i]);
    }

    auto layout = audqt::make_vbox (this);
    layout->setSizeConstraint (QLayout::SetFixedSize);
    layout->addWidget (& m_onoff_checkbox);
    layout->addWidget (slider_container);

    setWindowTitle (_("Equalizer"));
    setContentsMargins (audqt::margins.EightPt);

    m_onoff_checkbox.setFocus ();

    updateActive ();
    updatePreamp ();
    updateBands ();

    connect (& m_onoff_checkbox, & QCheckBox::stateChanged, [] (int state) {
        aud_set_bool (nullptr, "equalizer_active", (state == Qt::Checked));
    });

    connect (& m_preamp_slider->slider, & QSlider::valueChanged, [] (int value) {
        aud_set_int (nullptr, "equalizer_preamp", value);
    });

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
    {
        connect (& m_sliders[i]->slider, & QSlider::valueChanged, [i] (int value) {
            aud_eq_set_band (i, value);
        });
    }
}

void EqualizerWindow::updateActive ()
{
    bool active = aud_get_bool (nullptr, "equalizer_active");
    m_onoff_checkbox.setCheckState (active ? Qt::Checked : Qt::Unchecked);
}

void EqualizerWindow::updatePreamp ()
{
    m_preamp_slider->slider.setValue (aud_get_int (nullptr, "equalizer_preamp"));
}

void EqualizerWindow::updateBands ()
{
    double values[AUD_EQ_NBANDS];
    aud_eq_get_bands (values);

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
        m_sliders[i]->slider.setValue (values[i]);
}

static EqualizerWindow * s_equalizer = nullptr;

namespace audqt {

EXPORT void equalizer_show ()
{
    if (! s_equalizer)
    {
        s_equalizer = new EqualizerWindow;
        s_equalizer->setAttribute (Qt::WA_DeleteOnClose);

        QObject::connect (s_equalizer, & QObject::destroyed, [] () {
            s_equalizer = nullptr;
        });
    }

    window_bring_to_front (s_equalizer);
}

EXPORT void equalizer_hide ()
{
    delete s_equalizer;
}

} // namespace audqt
