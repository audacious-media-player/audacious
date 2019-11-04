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

#include <QAction>
#include <QLineEdit>
#include <QPointer>
#include <QFontDialog>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>

namespace audqt {

class FontEntry : public QLineEdit {
public:
    FontEntry (QWidget * parent = nullptr, const char * font = nullptr) :
        QLineEdit (parent),
        m_action (get_icon ("dialog-text-and-font"), _("Set Font"), nullptr)
    {
        addAction (& m_action, TrailingPosition);
        connect (& m_action, & QAction::triggered, this, & FontEntry::show_dialog);

        if (font)
            setText (font);

        end (false);
    }

private:
    QFontDialog * create_dialog ();
    void show_dialog ();

    QAction m_action;
    QPointer<QFontDialog> m_dialog;
};

/* parse a subset of Pango font descriptions */
static QFont * qfont_from_string (const char * name)
{
    auto family = str_copy (name);
    int size = 0;
    QFont::Weight weight = QFont::Normal;
    QFont::Style style = QFont::StyleNormal;
    QFont::Stretch stretch = QFont::Unstretched;

    while (1)
    {
        /* check for attributes */
        bool attr_found = false;
        const char * space = strrchr (family, ' ');

        if (space)
        {
            const char * attr = space + 1;
            int num = str_to_int (attr);

            attr_found = true;

            if (num > 0)
                size = num;
            else if (! strcmp (attr, "Light"))
                weight = QFont::Light;
            else if (! strcmp (attr, "Bold"))
                weight = QFont::Bold;
            else if (! strcmp (attr, "Oblique"))
                style = QFont::StyleOblique;
            else if (! strcmp (attr, "Italic"))
                style = QFont::StyleItalic;
            else if (! strcmp (attr, "Condensed"))
                stretch = QFont::Condensed;
            else if (! strcmp (attr, "Expanded"))
                stretch = QFont::Expanded;
            else
                attr_found = false;
        }

        if (! attr_found)
        {
            auto font = new QFont ((const char *) family);

            /* check for a recognized font family */
            if (! space || font->exactMatch ())
            {
                if (size > 0)
                    font->setPointSize (size);
                if (weight != QFont::Normal)
                    font->setWeight (weight);
                if (style != QFont::StyleNormal)
                    font->setStyle (style);
                if (stretch != QFont::Unstretched)
                    font->setStretch (stretch);

                return font;
            }

            delete font;
        }

        family.resize (space - family);
    }
}

QFontDialog * FontEntry::create_dialog ()
{
    auto dialog = new QFontDialog (this);

    QObject::connect (dialog, & QFontDialog::fontSelected, [this] (const QFont & font) {
        auto family = font.family ().toUtf8 ();

        // build the description string
        StringBuf font_str = str_copy ((const char *) family);

        auto weight = font.weight ();
        auto style = font.style ();
        auto stretch = font.stretch ();

        if (weight == QFont::Light)
            font_str = str_concat({font_str, " Light"});
        else if (weight == QFont::Bold)
            font_str = str_concat({font_str, " Bold"});

        if (style == QFont::StyleOblique)
            font_str = str_concat({font_str, " Oblique"});
        else if (style == QFont::StyleItalic)
            font_str = str_concat({font_str, " Italic"});

        if (stretch == QFont::Condensed)
            font_str = str_concat ({font_str, " Condensed"});
        else if (stretch == QFont::Expanded)
            font_str = str_concat ({font_str, " Expanded"});

        font_str = str_concat({font_str, " ", int_to_str (font.pointSize ())});

        font_entry_set_font (this, font_str);
    });

    return dialog;
}

void FontEntry::show_dialog ()
{
    if (! m_dialog)
        m_dialog = create_dialog ();

    auto font = qfont_from_string (font_entry_get_font (this));
    if (! font)
    {
        window_bring_to_front (m_dialog);
        return;
    }

    m_dialog->setCurrentFont (*font);
    delete font;

    window_bring_to_front (m_dialog);
}

EXPORT QLineEdit * font_entry_new (QWidget * parent, const char * font)
{
    return new FontEntry (parent, font);
}

EXPORT String font_entry_get_font (QLineEdit * entry)
{
    QByteArray text = entry->text ().toUtf8 ();

    return String (text);
}

EXPORT void font_entry_set_font (QLineEdit * entry, const char * font)
{
    entry->setText (font);
    entry->end (false);
}

} // namespace audqt
