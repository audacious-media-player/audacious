/*
 * volumebutton.h
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

#ifndef LIBAUDQT_VOLUMEBUTTON_H
#define LIBAUDQT_VOLUMEBUTTON_H

namespace audqt {

class EXPORT VolumeButton : public QToolButton {
    Q_OBJECT

public:
    VolumeButton (QWidget * parent = nullptr, int min = 0, int max = 100);
    void setValue (int value);

signals:
    void valueChanged (int value);

public slots:
    void showSlider ();
    void handleValueChange (int value);

private:
    QSlider * m_slider;
    QWidget * m_container;
    QVBoxLayout * m_layout;
};

}

#endif
