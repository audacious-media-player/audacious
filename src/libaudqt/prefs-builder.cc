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

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <libaudcore/i18n.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"
#include "prefs-widget.h"

namespace audqt {

void prefs_populate (QLayout * layout, ArrayRef<const PreferencesWidget> widgets, const char * domain)
{
    if (! layout)
    {
        AUDDBG("prefs_populate was passed a null layout!\n");
        return;
    }

    for (const PreferencesWidget & w : widgets)
    {
        switch (w.type)
        {
        case PreferencesWidget::CheckButton: {
            BooleanWidget * bw = new BooleanWidget (& w);
            layout->addWidget (bw->widget ());
            break;
        }
        case PreferencesWidget::Label: {
            LabelWidget * lw = new LabelWidget (& w);
            layout->addWidget (lw->widget ());
            break;
        }
        case PreferencesWidget::SpinButton: {
            switch (w.cfg.type) {
            case WidgetConfig::Int: {
                IntegerWidget * iw = new IntegerWidget (& w);
                layout->addWidget (iw->widget ());
                break;
            }
            case WidgetConfig::Float: {
                DoubleWidget * dw = new DoubleWidget (& w);
                layout->addWidget (dw->widget ());
                break;
            }
            default:
                AUDDBG("encountered unhandled configuration type %d for PreferencesWidget::SpinButton\n", w.cfg.type);
                break;
            }
        }
        case PreferencesWidget::Entry: {
            StringWidget * sw = new StringWidget (& w);
            layout->addWidget (sw->widget ());
            break;
        }
        case PreferencesWidget::RadioButton: {
            /* XXX: unimplemented */
            AUDDBG("radio buttons are unimplemented\n");
            break;
        }
        case PreferencesWidget::FontButton: {
            /* XXX: unimplemented */
            AUDDBG("font buttons are unimplemented\n");
            break;
        }
        case PreferencesWidget::ComboBox: {
            /* XXX: unimplemented */
            AUDDBG("combo boxes are unimplemented\n");
            break;
        }
        case PreferencesWidget::Custom: {
            if (w.data.populate)
            {
                QWidget * wid = (QWidget *) w.data.populate ();
                layout->addWidget (wid);
            }
            break;
        }
        default:
            AUDDBG("invoked stub handler for PreferencesWidget type %d\n", w.type);
            break;
        }
    }
}

};

