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
    /* layout prior to header label */
    QBoxLayout * orig_layout = layout;

    /* layouts prior to check box */
    QBoxLayout * parent_layout = nullptr;
    QBoxLayout * parent_orig_layout = nullptr;

    ParentWidget * parent_widget = nullptr;
    QButtonGroup * radio_btn_group[2] = {nullptr, nullptr};

    for (const PreferencesWidget & w : widgets)
    {
        if (w.child)
        {
            if (! parent_layout)
            {
                /* save prior layouts */
                parent_layout = layout;
                parent_orig_layout = orig_layout;

                /* create new layout for child widgets */
                if (dynamic_cast<QHBoxLayout *> (parent_layout))
                    layout = make_hbox (nullptr, sizes.TwoPt);
                else
                {
                    layout = make_vbox (nullptr, sizes.TwoPt);
                    layout->setContentsMargins (sizes.EightPt, 0, 0, 0);
                }

                parent_layout->addLayout (layout);

                orig_layout = layout;

                if (parent_widget)
                    parent_widget->set_child_layout (layout);
            }
        }
        else
        {
            if (parent_layout)
            {
                /* restore prior layouts */
                layout = parent_layout;
                orig_layout = parent_orig_layout;

                parent_layout = nullptr;
                parent_orig_layout = nullptr;
            }

            /* enable/disable child widgets */
            if (parent_widget)
                parent_widget->update_from_cfg ();

            parent_widget = nullptr;
        }

        if (w.type != PreferencesWidget::RadioButton)
            radio_btn_group[w.child] = nullptr;
        if (! w.child)
            radio_btn_group[true] = nullptr;

        switch (w.type)
        {
        case PreferencesWidget::Button:
            layout->addWidget (new ButtonWidget (& w, domain));
            break;

        case PreferencesWidget::CheckButton:
        {
            auto checkbox = new BooleanWidget (& w, domain);
            layout->addWidget (checkbox);

            if (! w.child)
                parent_widget = checkbox;

            break;
        }

        case PreferencesWidget::Label:
        {
            auto label = new QLabel (translate_str (w.label, domain));

            if (strstr (w.label, "<b>"))
            {
                /* extra spacing above a header */
                if (orig_layout->itemAt (0))
                    orig_layout->addSpacing (sizes.EightPt);

                orig_layout->addWidget (label);

                /* create indented layout below header */
                layout = make_vbox (nullptr, sizes.TwoPt);
                layout->setContentsMargins (sizes.EightPt, 0, 0, 0);
                orig_layout->addLayout (layout);
            }
            else
                layout->addWidget (label);

            break;
        }

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
        /* TODO: implement file chooser */
        case PreferencesWidget::FileEntry:
            layout->addWidget (new StringWidget (& w, domain));
            break;

        case PreferencesWidget::RadioButton:
        {
            if (! radio_btn_group[w.child])
                radio_btn_group[w.child] = new QButtonGroup;

            auto radio_btn = new RadioButtonWidget (& w, domain, radio_btn_group[w.child]);
            layout->addWidget (radio_btn);

            if (! w.child)
                parent_widget = radio_btn;

            break;
        }

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
            layout->addWidget (new BoxWidget (& w, domain,
             (bool) dynamic_cast<QHBoxLayout *> (layout)));
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
            f->setFrameShadow (QFrame::Sunken);

            layout->addSpacing (sizes.FourPt);
            layout->addWidget (f);
            layout->addSpacing (sizes.FourPt);

            break;
        }

        /* stub handler */
        default:
            AUDDBG ("invoked stub handler for PreferencesWidget type %d\n", w.type);
            break;
        }
    }

    /* enable/disable child widgets */
    if (parent_widget)
        parent_widget->update_from_cfg ();
}

} // namespace audqt
