/*
 * font-entry.cc
 * Copyright 2015 John Lindgren
 * Copyright 2019 Ariadne Conill
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

#include <stdlib.h>

#include <QAction>
#include <QFontDialog>
#include <QLineEdit>
#include <QPointer>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>

namespace audqt
{

class FontEntry : public QLineEdit
{
public:
    FontEntry(QWidget * parent = nullptr, const char * font = nullptr)
        : QLineEdit(parent),
          m_action(QIcon::fromTheme("preferences-desktop-font"), _("Set Font"), nullptr)
    {
        addAction(&m_action, TrailingPosition);
        connect(&m_action, &QAction::triggered, this, &FontEntry::show_dialog);

        if (font)
            setText(font);

        end(false);
    }

private:
    void show_dialog();

    QAction m_action;
    QPointer<QFontDialog> m_dialog;
};

/* parse a subset of Pango font descriptions */
EXPORT QFont qfont_from_string(const char * name)
{
    auto family = str_copy(name);
    int size = 0;
    QFont::Weight weight = QFont::Normal;
    QFont::Style style = QFont::StyleNormal;
    QFont::Stretch stretch = QFont::Unstretched;

    while (1)
    {
        /* check for attributes */
        bool attr_found = false;
        const char * space = strrchr(family, ' ');

        if (space)
        {
            const char * attr = space + 1;

            char * endptr;
            long num = strtol(attr, &endptr, 10);

            attr_found = true;

            if (size == 0 && num > 0 && *endptr == '\0')
                size = num;
            else if (!strcmp(attr, "Light"))
                weight = QFont::Light;
            else if (!strcmp(attr, "Bold"))
                weight = QFont::Bold;
            else if (!strcmp(attr, "Oblique"))
                style = QFont::StyleOblique;
            else if (!strcmp(attr, "Italic"))
                style = QFont::StyleItalic;
            else if (!strcmp(attr, "Condensed"))
                stretch = QFont::Condensed;
            else if (!strcmp(attr, "Expanded"))
                stretch = QFont::Expanded;
            else
                attr_found = false;
        }

        if (!attr_found)
        {
            QFont font((const char *)family);

            if (size > 0)
                font.setPointSize(size);
            if (weight != QFont::Normal)
                font.setWeight(weight);
            if (style != QFont::StyleNormal)
                font.setStyle(style);
            if (stretch != QFont::Unstretched)
                font.setStretch(stretch);

            return font;
        }

        family.resize(space - family);
    }
}

EXPORT StringBuf qfont_to_string(const QFont & font)
{
    StringBuf font_str = str_copy(font.family().toUtf8());

    auto weight = font.weight();
    auto style = font.style();
    auto stretch = font.stretch();

    if (weight == QFont::Light)
        font_str.insert(-1, " Light");
    else if (weight == QFont::Bold)
        font_str.insert(-1, " Bold");

    if (style == QFont::StyleOblique)
        font_str.insert(-1, " Oblique");
    else if (style == QFont::StyleItalic)
        font_str.insert(-1, " Italic");

    if (stretch == QFont::Condensed)
        font_str.insert(-1, " Condensed");
    else if (stretch == QFont::Expanded)
        font_str.insert(-1, " Expanded");

    str_append_printf(font_str, " %d", font.pointSize());

    return font_str;
}

void FontEntry::show_dialog()
{
    if (!m_dialog)
    {
        m_dialog = new QFontDialog(this);

        QObject::connect(m_dialog.data(), &QFontDialog::fontSelected,
                         [this](const QFont & font) {
                             setText((const char *)qfont_to_string(font));
                             end(false);
                         });
    }

    m_dialog->setCurrentFont(qfont_from_string(text().toUtf8()));
    window_bring_to_front(m_dialog);
}

EXPORT QLineEdit * font_entry_new(QWidget * parent, const char * font)
{
    return new FontEntry(parent, font);
}

} // namespace audqt
