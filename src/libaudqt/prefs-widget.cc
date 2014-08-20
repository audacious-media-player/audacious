/*
 * prefs-widget.cc
 * Copyright 2007-2014 Tomasz Moń, William Pitcock, and John Lindgren
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

/* boolean widget (checkbox) */
QWidget * BooleanWidget::widget ()
{
    m_widget.setText (m_parent->label);
    m_widget.setToolTip (m_parent->tooltip);
    m_widget.setCheckState (get () ? Qt::Checked : Qt::Unchecked);

    QObject::connect (& m_widget, &QCheckBox::stateChanged, [=] (int state) {
        set ((state != Qt::Unchecked));
    });

    return & m_widget;
}

bool BooleanWidget::get ()
{
    if (! m_parent)
        return false;

    if (m_parent->cfg.value)
        return * (bool *) m_parent->cfg.value;
    else if (m_parent->cfg.name)
        return aud_get_bool (m_parent->cfg.section, m_parent->cfg.name);
    else
        return false;
}

void BooleanWidget::set (bool value)
{
    if (! m_parent || m_parent->cfg.type != WidgetConfig::Bool)
        return;

    if (m_parent->cfg.value)
        * (bool *) m_parent->cfg.value = value;
    else if (m_parent->cfg.name)
        aud_set_bool (m_parent->cfg.section, m_parent->cfg.name, value);

    if (m_parent->cfg.callback)
        m_parent->cfg.callback ();
}

/* label */
QWidget * LabelWidget::widget ()
{
    m_label_text.setText (m_parent->label);
    m_label_text.setToolTip (m_parent->tooltip);

    if (m_parent->data.label.stock_id)
    {
        AUDDBG ("stock icons are not yet implemented on qt\n");
    }

    m_layout.addWidget (& m_label_text);
    m_container.setLayout (& m_layout);

    return & m_container;
}

/* integer (spinbox) */
QWidget * IntegerWidget::widget ()
{
    m_spinner.setPrefix (m_parent->label);
    m_spinner.setRange ((int) m_parent->data.spin_btn.min, (int) m_parent->data.spin_btn.max);
    m_spinner.setSingleStep ((int) m_parent->data.spin_btn.step);
    m_spinner.setValue (get ());

    if (m_parent->data.spin_btn.right_label)
        m_spinner.setSuffix (m_parent->data.spin_btn.right_label);

    /*
     * Qt has two different valueChanged signals for spin boxes.  So we have to do an explicit
     * cast to the type of the correct valueChanged signal.  Unfortunately Qt wants us to use
     * a C++ style cast here.  And really, I wouldn't know how to do this type of cast using a C-style
     * cast anyway.   --kaniini.
     */
    QObject::connect (& m_spinner,
                      static_cast <void (QSpinBox::*) (int)> (& QSpinBox::valueChanged),
                      [=] (int value) {
        set (value);
    });

    return & m_spinner;
}

int IntegerWidget::get ()
{
    if (! m_parent)
        return 0;

    if (m_parent->cfg.value)
        return * (int *) m_parent->cfg.value;
    else if (m_parent->cfg.name)
        return aud_get_int (m_parent->cfg.section, m_parent->cfg.name);
    else
        return 0;
}

void IntegerWidget::set (int value)
{
    if (! m_parent || m_parent->cfg.type != WidgetConfig::Int)
        return;

    if (m_parent->cfg.value)
        * (int *) m_parent->cfg.value = value;
    else if (m_parent->cfg.name)
        aud_set_int (m_parent->cfg.section, m_parent->cfg.name, value);

    if (m_parent->cfg.callback)
        m_parent->cfg.callback ();
}

/* double (spinbox) */
QWidget * DoubleWidget::widget ()
{
    m_spinner.setPrefix (m_parent->label);
    m_spinner.setRange (m_parent->data.spin_btn.min, m_parent->data.spin_btn.max);
    m_spinner.setSingleStep (m_parent->data.spin_btn.step);
    m_spinner.setValue (get ());

    if (m_parent->data.spin_btn.right_label)
        m_spinner.setSuffix (m_parent->data.spin_btn.right_label);

    /* an explanation of this crime against humanity is above in IntegerWidget::widget().  --kaniini. */
    QObject::connect (& m_spinner,
                      static_cast <void (QDoubleSpinBox::*) (double)> (& QDoubleSpinBox::valueChanged),
                      [=] (double value) {
        set (value);
    });

    return & m_spinner;
}

double DoubleWidget::get ()
{
    if (! m_parent)
        return 0;

    if (m_parent->cfg.value)
        return * (double *) m_parent->cfg.value;
    else if (m_parent->cfg.name)
        return aud_get_double (m_parent->cfg.section, m_parent->cfg.name);
    else
        return 0.0;
}

void DoubleWidget::set (double value)
{
    if (! m_parent || m_parent->cfg.type != WidgetConfig::Float)
        return;

    if (m_parent->cfg.value)
        * (double *) m_parent->cfg.value = value;
    else if (m_parent->cfg.name)
        aud_set_double (m_parent->cfg.section, m_parent->cfg.name, value);

    if (m_parent->cfg.callback)
        m_parent->cfg.callback ();
}

};
