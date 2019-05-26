/*
 * colorbutton.h
 * Copyright 2019 William Pitcock
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

#ifndef LIBAUDQT_COLORBUTTON_H
#define LIBAUDQT_COLORBUTTON_H

#include "libaudqt/export.h"

#include <QPushButton>
#include <QColorDialog>
#include <QColor>

namespace audqt {

class LIBAUDQT_PUBLIC ColorButton : public QPushButton {
public:
    ColorButton (QWidget * parent = nullptr);
    ~ColorButton () {};

    void setColor (const QColor &);

    QColor color() const {
        return m_color;
    }

protected:
    virtual void onColorChanged () {};

private slots:
    void onClicked ();

private:
    QColor m_color;
};

}

#endif
