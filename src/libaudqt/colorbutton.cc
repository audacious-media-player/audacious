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

#include "colorbutton.h"

namespace audqt {

ColorButton::ColorButton (QWidget * parent) :
    QPushButton (parent)
{
    connect (this, &QPushButton::clicked, this, &ColorButton::onClicked);
}

void ColorButton::setColor (const QColor & color)
{
    if (color != m_color)
    {
        m_color = color;

        QString style = QStringLiteral ("QWidget { background-color: %1 }").arg (m_color.name ());
        setStyleSheet (style);

        onColorChanged ();
    }
}

void ColorButton::onClicked ()
{
    QColorDialog dialog (m_color);

    if (dialog.exec() == QDialog::Accepted)
        setColor (dialog.selectedColor ());
}

}
