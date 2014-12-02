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

#ifndef LIBAUDQT_VOLUMEBUTTON_H
#define LIBAUDQT_VOLUMEBUTTON_H

#include <QToolButton>

class QSlider;

namespace audqt {

class VolumeButton : public QToolButton
{
public:
    VolumeButton (QWidget * parent = nullptr);

private:
    void updateIcon (int val);
    void updateVolume ();
    void showSlider ();
    void setVolume (int val);

    void wheelEvent (QWheelEvent * event);

    QSlider * m_slider;
    QWidget * m_container;
};

} // namespace audqt

#endif
