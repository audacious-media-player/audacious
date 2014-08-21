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

    m_layout.setContentsMargins (0, 0, 0, 0);
    m_layout.addWidget (& m_label_text);
    m_container.setLayout (& m_layout);

    return & m_container;
}

/* integer (radio button) */
QWidget * RadioButtonWidget::widget (QButtonGroup * btn_group)
{
    if (btn_group)
        btn_group->addButton (& m_widget, m_parent->data.radio_btn.value);

    m_widget.setText (m_parent->label);
    m_widget.setToolTip (m_parent->tooltip);

    QObject::connect (& m_widget, &QAbstractButton::clicked, [=] (bool checked) {
        if (! checked)
            return;

        set (m_parent->data.radio_btn.value);
    });

    return & m_widget;
}

int RadioButtonWidget::get ()
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

void RadioButtonWidget::set (int value)
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

/* integer (spinbox) */
QWidget * IntegerWidget::widget ()
{
    m_layout.setContentsMargins (0, 0, 0, 0);

    m_label_pre.setText (m_parent->label);
    m_layout.addWidget (& m_label_pre);

    m_spinner.setRange ((int) m_parent->data.spin_btn.min, (int) m_parent->data.spin_btn.max);
    m_spinner.setSingleStep ((int) m_parent->data.spin_btn.step);
    m_spinner.setValue (get ());
    m_layout.addWidget (& m_spinner);

    if (m_parent->data.spin_btn.right_label)
    {
        m_label_post.setText (m_parent->data.spin_btn.right_label);
        m_layout.addWidget (& m_label_post);
    }

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

    m_container.setLayout (& m_layout);

    return & m_container;
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
    m_layout.setContentsMargins (0, 0, 0, 0);

    m_label_pre.setText (m_parent->label);
    m_layout.addWidget (& m_label_pre);

    m_spinner.setRange (m_parent->data.spin_btn.min, m_parent->data.spin_btn.max);
    m_spinner.setSingleStep (m_parent->data.spin_btn.step);
    m_spinner.setValue (get ());
    m_layout.addWidget (& m_spinner);

    if (m_parent->data.spin_btn.right_label)
    {
        m_label_post.setText (m_parent->data.spin_btn.right_label);
        m_layout.addWidget (& m_label_post);
    }

    /* an explanation of this crime against humanity is above in IntegerWidget::widget().  --kaniini. */
    QObject::connect (& m_spinner,
                      static_cast <void (QDoubleSpinBox::*) (double)> (& QDoubleSpinBox::valueChanged),
                      [=] (double value) {
        set (value);
    });

    m_container.setLayout (& m_layout);

    return & m_container;
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

/* string (lineedit) */
QWidget * StringWidget::widget ()
{
    m_layout.setContentsMargins (0, 0, 0, 0);

    if (m_parent->label)
    {
        m_label.setText (m_parent->label);
        m_layout.addWidget (& m_label);
    }

    if (m_parent->data.entry.password)
        m_lineedit.setEchoMode (QLineEdit::Password);

    String value = get ();
    m_lineedit.setText ((const char *) value);
    m_layout.addWidget (&m_lineedit);

    m_container.setLayout (& m_layout);

    if (m_parent->tooltip)
        m_container.setToolTip (m_parent->tooltip);

    return & m_container;
}

String StringWidget::get ()
{
    if (! m_parent)
        return String ();

    if (m_parent->cfg.value)
        return * (String *) m_parent->cfg.value;
    else if (m_parent->cfg.name)
        return aud_get_str (m_parent->cfg.section, m_parent->cfg.name);
    else
        return String ();
}

void StringWidget::set (const char * value)
{
    if (! m_parent || m_parent->cfg.type != WidgetConfig::String)
        return;

    if (m_parent->cfg.value)
        * (String *) m_parent->cfg.value = String (value);
    else if (m_parent->cfg.name)
        aud_set_str (m_parent->cfg.section, m_parent->cfg.name, value);

    if (m_parent->cfg.callback)
        m_parent->cfg.callback ();
}

/* combo box widget (string or int) */
QWidget * ComboBoxWidget::widget ()
{
    m_layout.setContentsMargins (0, 0, 0, 0);

    if (m_parent->label)
    {
        m_label.setText (m_parent->label);
        m_layout.addWidget (& m_label);
    }

    m_layout.addWidget (&m_combobox);
    m_container.setLayout (& m_layout);

    if (m_parent->tooltip)
        m_container.setToolTip (m_parent->tooltip);

    fill ();

    QObject::connect (& m_combobox,
                      static_cast <void (QComboBox::*) (int)> (&QComboBox::currentIndexChanged),
                      [=] (int idx) {
        QVariant data = m_combobox.itemData (idx);

        switch (m_parent->cfg.type) {
        case WidgetConfig::Int:
            set_int (data.toInt ());
            break;
        case WidgetConfig::String:
            set_str (data.toString ().toUtf8 ());
            break;
        default:
            AUDDBG("unhandled configuration type %d in ComboBoxWidget::currentIndexChanged functor\n", m_parent->cfg.type);
            break;
        }
    });

    return & m_container;
}

String ComboBoxWidget::get_str ()
{
    if (! m_parent)
        return String ();

    if (m_parent->cfg.value)
        return * (String *) m_parent->cfg.value;
    else if (m_parent->cfg.name)
        return aud_get_str (m_parent->cfg.section, m_parent->cfg.name);
    else
        return String ();
}

void ComboBoxWidget::set_str (const char * value)
{
    if (! m_parent || m_parent->cfg.type != WidgetConfig::String)
        return;

    if (m_parent->cfg.value)
        * (String *) m_parent->cfg.value = String (value);
    else if (m_parent->cfg.name)
        aud_set_str (m_parent->cfg.section, m_parent->cfg.name, value);

    if (m_parent->cfg.callback)
        m_parent->cfg.callback ();
}

int ComboBoxWidget::get_int ()
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

void ComboBoxWidget::set_int (int value)
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

void ComboBoxWidget::fill ()
{
    ArrayRef<const ComboBoxElements> elems = m_parent->data.combo.elems;

    if (m_parent->data.combo.fill)
        elems = m_parent->data.combo.fill ();

    switch (m_parent->cfg.type) {
    case WidgetConfig::Int:
        for (const ComboBoxElements & elem : elems)
            m_combobox.addItem (elem.label, (int) ((intptr_t) elem.value));
        break;
    case WidgetConfig::String:
        for (const ComboBoxElements & elem : elems)
            m_combobox.addItem (elem.label, QString ((const char *) elem.value));
        break;
    default:
        AUDDBG("unhandled configuration type %d for ComboBoxWidget::fill()\n", m_parent->cfg.type);
        return;
    }

    /* set selected index */
    switch (m_parent->cfg.type) {
    case WidgetConfig::Int: {
        int ivalue = get_int ();

        for (int i = 0; i < elems.len; i++)
        {
            if ((int) ((intptr_t) elems.data[i].value) == ivalue)
            {
                m_combobox.setCurrentIndex (i);
                break;
            }
        }

        break;
    }
    case WidgetConfig::String: {
        String value = get_str ();

        for (int i = 0; i < elems.len; i++)
        {
            if (value && ! strcmp ((const char *) elems.data[i].value, value))
            {
                m_combobox.setCurrentIndex (i);
                break;
            }
        }

        break;
    }
    default:
        break;
    }
}

/* layout widgets */
QWidget * BoxWidget::widget ()
{
    QLayout * l = m_parent->data.box.horizontal ?
                  (QLayout *) & m_hbox_layout : (QLayout *) & m_vbox_layout;

    l->setContentsMargins (0, 0, 0, 0);
    prefs_populate (l, m_parent->data.box.widgets, nullptr);

    m_container.setLayout (l);

    return & m_container;
}

QWidget * TableWidget::widget ()
{
    AUDDBG("TableWidget::widget is a stub\n");

    prefs_populate (& m_layout, m_parent->data.table.widgets, nullptr);

    m_container.setLayout (& m_layout);

    return & m_container;
}

QWidget * NotebookWidget::widget ()
{
    for (const NotebookTab & tab : m_parent->data.notebook.tabs)
    {
        QWidget * w = new QWidget;
        QVBoxLayout * vbox = new QVBoxLayout;

        w->setLayout (vbox);

        prefs_populate (vbox, tab.widgets, nullptr);

        m_container.addTab (w, tab.name);
    }

    return & m_container;
}

};
