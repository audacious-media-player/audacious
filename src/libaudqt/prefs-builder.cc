/*
 * prefs-builder.cc
 * Copyright 2014 William Pitcock and John Lindgren
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
#include "prefs-widget.h"

#include <QButtonGroup>
#include <QFrame>
#include <QLayout>

#include <libaudcore/i18n.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

namespace audqt {

void prefs_populate (QBoxLayout * layout, ArrayRef<PreferencesWidget> widgets, const char * domain)
{
    QButtonGroup * radio_btn_group = nullptr;

    for (const PreferencesWidget & w : widgets)
    {
        if (radio_btn_group && w.type != PreferencesWidget::RadioButton)
            radio_btn_group = nullptr;

        switch (w.type)
        {
        case PreferencesWidget::Button:
            layout->addWidget (new ButtonWidget (& w, domain));
            break;

        case PreferencesWidget::CheckButton:
            layout->addWidget (new BooleanWidget (& w, domain));
            break;

        case PreferencesWidget::Label:
            layout->addWidget (new QLabel (translate_str (w.label, domain)));
            break;

        case PreferencesWidget::SpinButton:
            switch (w.cfg.type)
            {
            case WidgetConfig::Int:
                layout->addWidget (new IntegerWidget (& w, domain));
                break;
            case WidgetConfig::Float:
                layout->addWidget (new DoubleWidget (& w, domain));
                break;
            default:
                AUDDBG ("encountered unhandled configuration type %d for PreferencesWidget::SpinButton\n", w.cfg.type);
                break;
            }
            break;

        case PreferencesWidget::Entry:
            layout->addWidget (new StringWidget (& w, domain));
            break;

        case PreferencesWidget::RadioButton:
            if (! radio_btn_group)
                radio_btn_group = new QButtonGroup;

            layout->addWidget (new RadioButtonWidget (& w, domain, radio_btn_group));
            break;

        case PreferencesWidget::FontButton:
            /* XXX: unimplemented */
            AUDDBG ("font buttons are unimplemented\n");
            break;

        case PreferencesWidget::ComboBox:
            layout->addWidget (new ComboBoxWidget (& w, domain));
            break;

        case PreferencesWidget::CustomQt:
            if (w.data.populate)
                layout->addWidget ((QWidget *) w.data.populate ());
            break;

        /* layout widgets follow */
        case PreferencesWidget::Box:
            layout->addWidget (new BoxWidget (& w, domain));
            break;

        case PreferencesWidget::Table:
            layout->addWidget (new TableWidget (& w, domain));
            break;

        case PreferencesWidget::Notebook:
            layout->addWidget (new NotebookWidget (& w, domain));
            break;

        case PreferencesWidget::Separator:
        {
            QFrame * f = new QFrame;
            f->setFrameShape (w.data.separator.horizontal ? QFrame::HLine : QFrame::VLine);
            layout->addWidget (f);
            break;
        }

        /* stub handler */
        default:
            AUDDBG ("invoked stub handler for PreferencesWidget type %d\n", w.type);
            break;
        }
    }

    layout->addStretch (1);
}

} // namespace audqt
