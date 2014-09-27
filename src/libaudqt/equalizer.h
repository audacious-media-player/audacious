/*
 * equalizer.h
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
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"

#ifndef LIBAUDQT_EQUALIZER_H
#define LIBAUDQT_EQUALIZER_H

namespace audqt {

class EqualizerSlider : public QWidget {
    Q_OBJECT

public:
    EqualizerSlider (int band, const char *label, QWidget * parent = 0);
    ~EqualizerSlider ();

    void setValue (int value);

public slots:
    void valueChangedTrigger (int value);

private:
    int m_band;
    QVBoxLayout m_layout;

    QSlider *m_slider;
    QLabel *m_label;
};

class EqualizerWindow : public QDialog {
    Q_OBJECT

public:
    EqualizerWindow (QWidget * parent = 0);
    ~EqualizerWindow ();

public slots:
    void onoffStateChanged (int state);

private:
    EqualizerSlider *m_preamp_slider;
    EqualizerSlider *m_sliders[AUD_EQ_NBANDS];
    QWidget m_slider_container;
    QHBoxLayout m_slider_layout;

    QVBoxLayout m_layout;
    QFrame m_line;
    QCheckBox m_onoff_checkbox;

    void createSlider (int idx, const char *label);
    void updateSliders ();

    static void onoffUpdate (void * unused, EqualizerWindow * window)
    {
        window->m_onoff_checkbox.setCheckState(aud_get_bool(nullptr, "equalizer_active") ? Qt::Checked : Qt::Unchecked);
    }

    static void updateSlidersProxy (void * unused, EqualizerWindow * window)
    {
        window->updateSliders();
    }
};

}

#endif
