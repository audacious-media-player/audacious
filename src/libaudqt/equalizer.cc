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

#include <QtGui>
#include <QtWidgets>

#include <libaudcore/audstrings.h>
#include <libaudcore/equalizer.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"
#include "equalizer.h"
#include "equalizer.moc"

namespace audqt {

EqualizerWindow::EqualizerWindow (QWidget * parent) : QDialog (parent)
{
    const char * const names[AUD_EQ_NBANDS] = {N_("31 Hz"), N_("63 Hz"),
     N_("125 Hz"), N_("250 Hz"), N_("500 Hz"), N_("1 kHz"), N_("2 kHz"),
     N_("4 kHz"), N_("8 kHz"), N_("16 kHz")};

    m_preamp_slider = new EqualizerSlider (-1, _("Preamp"));
    m_slider_layout.addWidget (m_preamp_slider);

    m_line.setFrameShape (QFrame::VLine);
    m_line.setFrameShadow (QFrame::Sunken);
    m_slider_layout.addWidget (& m_line);

    for (int i = 0; i < AUD_EQ_NBANDS; i++)
        createSlider (i, names[i]);

    m_onoff_checkbox.setText (_("Enabled"));
    m_layout.addWidget (& m_onoff_checkbox);

    m_slider_container.setLayout (& m_slider_layout);
    m_layout.addWidget (& m_slider_container);

    setLayout (& m_layout);
    setWindowTitle (_("Equalizer"));

    connect (& m_onoff_checkbox, &QCheckBox::stateChanged, this, &EqualizerWindow::onoffStateChanged);
    onoffUpdate (nullptr, this);

    updateSliders ();

    hook_associate ("set equalizer_active", (HookFunction) onoffUpdate, this);
    hook_associate ("set equalizer_preamp", (HookFunction) updateSlidersProxy, this);
    hook_associate ("set equalizer_bands", (HookFunction) updateSlidersProxy, this);
}

EqualizerWindow::~EqualizerWindow ()
{
    for (int i = 0; i < AUD_EQ_NBANDS; i++)
        delete m_sliders[i];

    hook_dissociate ("set equalizer_active", (HookFunction) onoffUpdate);
    hook_dissociate ("set equalizer_preamp", (HookFunction) updateSlidersProxy);
    hook_dissociate ("set equalizer_bands", (HookFunction) updateSlidersProxy);
}

void EqualizerWindow::createSlider (int idx, const char * label)
{
    m_sliders[idx] = new EqualizerSlider (idx, label);
    m_slider_layout.addWidget (m_sliders[idx]);
}

void EqualizerWindow::updateSliders ()
{
    m_preamp_slider->setValue (aud_get_int (nullptr, "equalizer_preamp"));

    double values[AUD_EQ_NBANDS];
    aud_eq_get_bands (values);

    for (int i = 0; i < AUD_EQ_NBANDS; i++)
        m_sliders[i]->setValue (values[i]);
}

EqualizerSlider::EqualizerSlider (int band, const char * label, QWidget * parent) : QWidget (parent),
    m_band (band), m_slider (new QSlider (Qt::Vertical)), m_label (new QLabel)
{
    m_slider->setRange (-AUD_EQ_MAX_GAIN, AUD_EQ_MAX_GAIN);
    m_label->setText (label);

    m_layout.addWidget (m_slider);
    m_layout.addWidget (m_label);

    setLayout (& m_layout);

    connect (m_slider, &QSlider::valueChanged, this, &EqualizerSlider::valueChangedTrigger);
}

void EqualizerWindow::onoffStateChanged (int state)
{
    aud_set_bool (nullptr, "equalizer_active", (state == Qt::Checked));
}

EqualizerSlider::~EqualizerSlider ()
{
    delete m_slider;
    delete m_label;
}

void EqualizerSlider::valueChangedTrigger (int value)
{
    AUDDBG("value change, band %d, value %d\n", m_band, value);

    if (m_band == -1)
        aud_set_int (nullptr, "equalizer_preamp", value);
    else
        aud_eq_set_band (m_band, value);
}

void EqualizerSlider::setValue (int value)
{
    m_slider->setValue (value);
}

static EqualizerWindow * m_equalizer = nullptr;

EXPORT void equalizer_show (void)
{
    if (! m_equalizer)
        m_equalizer = new EqualizerWindow;

    window_bring_to_front (m_equalizer);
}

EXPORT void equalizer_hide (void)
{
    if (! m_equalizer)
        return;

    m_equalizer->hide ();
}

};
