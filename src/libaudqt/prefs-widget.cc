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

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"
#include "prefs-widget.h"

namespace audqt {

/* button */
QWidget * ButtonWidget::widget ()
{
    m_widget.setText (translate_str (m_parent->label, m_domain));

    QObject::connect (& m_widget, &QPushButton::clicked, [=] () {
        m_parent->data.button.callback ();
    });

    return & m_widget;
}

/* boolean widget (checkbox) */
QWidget * BooleanWidget::widget ()
{
    m_widget.setText (translate_str (m_parent->label, m_domain));

    QObject::connect (& m_widget, &QCheckBox::stateChanged, [=] (int state) {
        set ((state != Qt::Unchecked));
    });

    update ();

    return & m_widget;
}

void BooleanWidget::update ()
{
    m_widget.setCheckState (get () ? Qt::Checked : Qt::Unchecked);
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
    m_label_text.setText (translate_str (m_parent->label, m_domain));
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

    m_widget.setText (translate_str (m_parent->label, m_domain));

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

    m_label_pre.setText (translate_str (m_parent->label, m_domain));
    m_layout.addWidget (& m_label_pre);

    m_layout.addWidget (& m_spinner);

    if (m_parent->data.spin_btn.right_label)
    {
        m_label_post.setText (translate_str (m_parent->data.spin_btn.right_label, m_domain));
        m_layout.addWidget (& m_label_post);
    }

    update ();

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

void IntegerWidget::update ()
{
    m_spinner.setRange ((int) m_parent->data.spin_btn.min, (int) m_parent->data.spin_btn.max);
    m_spinner.setSingleStep ((int) m_parent->data.spin_btn.step);
    m_spinner.setValue (get ());
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

    m_label_pre.setText (translate_str (m_parent->label, m_domain));
    m_layout.addWidget (& m_label_pre);

    m_layout.addWidget (& m_spinner);

    if (m_parent->data.spin_btn.right_label)
    {
        m_label_post.setText (translate_str (m_parent->data.spin_btn.right_label, m_domain));
        m_layout.addWidget (& m_label_post);
    }

    update ();

    /* an explanation of this crime against humanity is above in IntegerWidget::widget().  --kaniini. */
    QObject::connect (& m_spinner,
                      static_cast <void (QDoubleSpinBox::*) (double)> (& QDoubleSpinBox::valueChanged),
                      [=] (double value) {
        set (value);
    });

    m_container.setLayout (& m_layout);

    return & m_container;
}

void DoubleWidget::update ()
{
    m_spinner.setRange (m_parent->data.spin_btn.min, m_parent->data.spin_btn.max);
    m_spinner.setSingleStep (m_parent->data.spin_btn.step);
    m_spinner.setValue (get ());
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
        m_label.setText (translate_str (m_parent->label, m_domain));
        m_layout.addWidget (& m_label);
    }

    m_layout.addWidget (&m_lineedit);
    m_container.setLayout (& m_layout);

    update ();

    return & m_container;
}

void StringWidget::update ()
{
    if (m_parent->data.entry.password)
        m_lineedit.setEchoMode (QLineEdit::Password);

    String value = get ();
    m_lineedit.setText ((const char *) value);
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
        m_label.setText (translate_str (m_parent->label, m_domain));
        m_layout.addWidget (& m_label);
    }

    m_layout.addWidget (&m_combobox);
    m_container.setLayout (& m_layout);

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
    ArrayRef<const ComboItem> items = m_parent->data.combo.elems;

    if (m_parent->data.combo.fill)
        items = m_parent->data.combo.fill ();

    switch (m_parent->cfg.type) {
    case WidgetConfig::Int:
        for (const ComboItem & item : items)
            m_combobox.addItem (item.label, item.num);
        break;
    case WidgetConfig::String:
        for (const ComboItem & item : items)
            m_combobox.addItem (item.label, item.str);
        break;
    default:
        AUDDBG("unhandled configuration type %d for ComboBoxWidget::fill()\n", m_parent->cfg.type);
        return;
    }

    update ();
}

void ComboBoxWidget::update ()
{
    ArrayRef<const ComboItem> items = m_parent->data.combo.elems;

    /* set selected index */
    switch (m_parent->cfg.type) {
    case WidgetConfig::Int: {
        int num = get_int ();

        for (int i = 0; i < items.len; i++)
        {
            if (items.data[i].num == num)
            {
                m_combobox.setCurrentIndex (i);
                break;
            }
        }

        break;
    }
    case WidgetConfig::String: {
        String str = get_str ();

        for (int i = 0; i < items.len; i++)
        {
            if (! strcmp_safe (items.data[i].str, str))
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
    prefs_populate (l, m_parent->data.box.widgets, m_domain);

    m_container.setLayout (l);

    return & m_container;
}

QWidget * TableWidget::widget ()
{
    AUDDBG("TableWidget::widget is a stub\n");

    prefs_populate (& m_layout, m_parent->data.table.widgets, m_domain);

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

        m_container.addTab (w, translate_str (tab.name, m_domain));
    }

    return & m_container;
}

};
