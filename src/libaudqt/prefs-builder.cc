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
        case PreferencesWidget::Button: {
            ButtonWidget * bw = new ButtonWidget (& w, domain);
            layout->addWidget (bw->widget ());
            break;
        }
        case PreferencesWidget::CheckButton: {
            BooleanWidget * bw = new BooleanWidget (& w, domain);
            layout->addWidget (bw->widget ());
            break;
        }
        case PreferencesWidget::Label: {
            LabelWidget * lw = new LabelWidget (& w, domain);
            layout->addWidget (lw->widget ());
            break;
        }
        case PreferencesWidget::SpinButton: {
            switch (w.cfg.type) {
            case WidgetConfig::Int: {
                IntegerWidget * iw = new IntegerWidget (& w, domain);
                layout->addWidget (iw->widget ());
                break;
            }
            case WidgetConfig::Float: {
                DoubleWidget * dw = new DoubleWidget (& w, domain);
                layout->addWidget (dw->widget ());
                break;
            }
            default:
                AUDDBG("encountered unhandled configuration type %d for PreferencesWidget::SpinButton\n", w.cfg.type);
                break;
            }
            break;
        }
        case PreferencesWidget::Entry: {
            StringWidget * sw = new StringWidget (& w, domain);
            layout->addWidget (sw->widget ());
            break;
        }
        case PreferencesWidget::RadioButton: {
            if (! radio_btn_group)
                radio_btn_group = new QButtonGroup;

            RadioButtonWidget * rw = new RadioButtonWidget (& w, domain);
            layout->addWidget (rw->widget (radio_btn_group));
            break;
        }
        case PreferencesWidget::FontButton: {
            /* XXX: unimplemented */
            AUDDBG("font buttons are unimplemented\n");
            break;
        }
        case PreferencesWidget::ComboBox: {
            ComboBoxWidget * cw = new ComboBoxWidget (& w, domain);
            layout->addWidget (cw->widget ());
            break;
        }
        case PreferencesWidget::CustomQt: {
            if (w.data.populate)
            {
                QWidget * wid = (QWidget *) w.data.populate ();
                layout->addWidget (wid);
            }
            break;
        }

        /* layout widgets follow */
        case PreferencesWidget::Box: {
            BoxWidget * bw = new BoxWidget (& w, domain);
            layout->addWidget (bw->widget ());
            break;
        }
        case PreferencesWidget::Table: {
            TableWidget * tw = new TableWidget (& w, domain);
            layout->addWidget (tw->widget ());
            break;
        }
        case PreferencesWidget::Notebook: {
            NotebookWidget * nw = new NotebookWidget (& w, domain);
            layout->addWidget (nw->widget ());
            break;
        }
        case PreferencesWidget::Separator: {
            QFrame * f = new QFrame;
            f->setFrameShape (w.data.separator.horizontal ? QFrame::HLine : QFrame::VLine);
            layout->addWidget (f);
            break;
        }

        /* stub handler */
        default:
            AUDDBG("invoked stub handler for PreferencesWidget type %d\n", w.type);
            break;
        }
    }

    layout->addStretch (1);
}

} // namespace audqt
