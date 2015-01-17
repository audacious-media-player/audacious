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

#include "prefs-widget.h"
#include "libaudqt.h"

#include <assert.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>

namespace audqt {

/* button */
QWidget * ButtonWidget::widget ()
{
    auto button = new QPushButton (translate_str (m_parent->label, m_domain));
    QObject::connect (button, & QPushButton::clicked, m_parent->data.button.callback);
    return button;
}

/* boolean widget (checkbox) */
QWidget * BooleanWidget::widget ()
{
    m_checkbox = new QCheckBox (translate_str (m_parent->label, m_domain));

    update ();

    QObject::connect (m_checkbox, & QCheckBox::stateChanged, [this] (int state) {
        m_parent->cfg.set_bool ((state != Qt::Unchecked));
    });

    return m_checkbox;
}

void BooleanWidget::update ()
{
    m_checkbox->setCheckState (m_parent->cfg.get_bool () ? Qt::Checked : Qt::Unchecked);
}

/* label */
QWidget * LabelWidget::widget ()
{
    return new QLabel (translate_str (m_parent->label, m_domain));
}

/* integer (radio button) */
QWidget * RadioButtonWidget::widget (QButtonGroup * btn_group)
{
    m_radio = new QRadioButton (translate_str (m_parent->label, m_domain));

    if (btn_group)
        btn_group->addButton (m_radio, m_parent->data.radio_btn.value);

    update ();

    QObject::connect (m_radio, & QAbstractButton::clicked, [this] (bool checked) {
        if (checked)
            m_parent->cfg.set_int (m_parent->data.radio_btn.value);
    });

    return m_radio;
}

void RadioButtonWidget::update ()
{
    if (m_parent->cfg.get_int () == m_parent->data.radio_btn.value)
        m_radio->setChecked (true);
}

/* integer (spinbox) */
QWidget * IntegerWidget::widget ()
{
    auto container = new QWidget;
    auto layout = new QHBoxLayout (container);

    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (4);

    if (m_parent->label)
        layout->addWidget (new QLabel (translate_str (m_parent->label, m_domain)));

    m_spinner = new QSpinBox;
    layout->addWidget (m_spinner);

    if (m_parent->data.spin_btn.right_label)
        layout->addWidget (new QLabel (translate_str
         (m_parent->data.spin_btn.right_label, m_domain)));

    layout->addStretch (1);

    update ();

    /*
     * Qt has two different valueChanged signals for spin boxes.  So we have to do an explicit
     * cast to the type of the correct valueChanged signal.  --kaniini.
     */
    void (QSpinBox::* signal) (int) = & QSpinBox::valueChanged;
    QObject::connect (m_spinner, signal, [this] (int value) {
        m_parent->cfg.set_int (value);
    });

    return container;
}

void IntegerWidget::update ()
{
    m_spinner->setRange ((int) m_parent->data.spin_btn.min, (int) m_parent->data.spin_btn.max);
    m_spinner->setSingleStep ((int) m_parent->data.spin_btn.step);
    m_spinner->setValue (m_parent->cfg.get_int ());
}

/* double (spinbox) */
QWidget * DoubleWidget::widget ()
{
    auto container = new QWidget;
    auto layout = new QHBoxLayout (container);

    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (4);

    if (m_parent->label)
        layout->addWidget (new QLabel (translate_str (m_parent->label, m_domain)));

    m_spinner = new QDoubleSpinBox;
    layout->addWidget (m_spinner);

    if (m_parent->data.spin_btn.right_label)
        layout->addWidget (new QLabel (translate_str
         (m_parent->data.spin_btn.right_label, m_domain)));

    layout->addStretch (1);

    update ();

    void (QDoubleSpinBox::* signal) (double) = & QDoubleSpinBox::valueChanged;
    QObject::connect (m_spinner, signal, [this] (double value) {
        m_parent->cfg.set_float (value);
    });

    return container;
}

void DoubleWidget::update ()
{
    m_spinner->setRange (m_parent->data.spin_btn.min, m_parent->data.spin_btn.max);
    m_spinner->setSingleStep (m_parent->data.spin_btn.step);
    m_spinner->setValue (m_parent->cfg.get_float ());
}

/* string (lineedit) */
QWidget * StringWidget::widget ()
{
    auto container = new QWidget;
    auto layout = new QHBoxLayout (container);

    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (4);

    if (m_parent->label)
        layout->addWidget (new QLabel (translate_str (m_parent->label, m_domain)));

    m_lineedit = new QLineEdit;
    if (m_parent->data.entry.password)
        m_lineedit->setEchoMode (QLineEdit::Password);

    layout->addWidget (m_lineedit, 1);

    update ();

    QObject::connect (m_lineedit, & QLineEdit::textChanged, [this] (const QString & value) {
        m_parent->cfg.set_string (value.toUtf8 ());
    });

    return container;
}

void StringWidget::update ()
{
    m_lineedit->setText ((const char *) m_parent->cfg.get_string ());
}

/* combo box widget (string or int) */
QWidget * ComboBoxWidget::widget ()
{
    auto container = new QWidget;
    auto layout = new QHBoxLayout (container);

    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (4);

    if (m_parent->label)
        layout->addWidget (new QLabel (translate_str (m_parent->label, m_domain)));

    m_combobox = new QComboBox;
    layout->addWidget (m_combobox);
    layout->addStretch (1);

    update ();

    void (QComboBox::* signal) (int) = & QComboBox::currentIndexChanged;
    QObject::connect (m_combobox, signal, [this] (int idx) {
        QVariant data = m_combobox->itemData (idx);

        switch (m_parent->cfg.type) {
        case WidgetConfig::Int:
            m_parent->cfg.set_int (data.toInt ());
            break;
        case WidgetConfig::String:
            m_parent->cfg.set_string (data.toString ().toUtf8 ());
            break;
        default:
            break;
        }
    });

    return container;
}

void ComboBoxWidget::update ()
{
    ArrayRef<ComboItem> items = m_parent->data.combo.elems;

    if (m_parent->data.combo.fill)
        items = m_parent->data.combo.fill ();

    m_combobox->clear ();

    /* add combobox items */
    switch (m_parent->cfg.type) {
    case WidgetConfig::Int:
        for (const ComboItem & item : items)
            m_combobox->addItem (translate_str (item.label, m_domain), item.num);
        break;
    case WidgetConfig::String:
        for (const ComboItem & item : items)
            m_combobox->addItem (translate_str (item.label, m_domain), item.str);
        break;
    default:
        break;
    }

    /* set selected index */
    switch (m_parent->cfg.type) {
    case WidgetConfig::Int: {
        int num = m_parent->cfg.get_int ();

        for (int i = 0; i < items.len; i++)
        {
            if (items.data[i].num == num)
            {
                m_combobox->setCurrentIndex (i);
                break;
            }
        }

        break;
    }
    case WidgetConfig::String: {
        String str = m_parent->cfg.get_string ();

        for (int i = 0; i < items.len; i++)
        {
            if (! strcmp_safe (items.data[i].str, str))
            {
                m_combobox->setCurrentIndex (i);
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
    auto container = new QWidget;

    QBoxLayout * layout;
    if (m_parent->data.box.horizontal)
        layout = new QHBoxLayout (container);
    else
        layout = new QVBoxLayout (container);

    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (4);
    prefs_populate (layout, m_parent->data.box.widgets, m_domain);

    return container;
}

QWidget * TableWidget::widget ()
{
    // TODO: proper table layout
    auto container = new QWidget;
    auto layout = new QVBoxLayout (container);

    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (4);
    prefs_populate (layout, m_parent->data.table.widgets, m_domain);

    return container;
}

QWidget * NotebookWidget::widget ()
{
    auto tabs = new QTabWidget;

    for (const NotebookTab & tab : m_parent->data.notebook.tabs)
    {
        auto widget = new QWidget (tabs);
        auto layout = new QVBoxLayout (widget);

        layout->setContentsMargins (0, 0, 0, 0);
        layout->setSpacing (4);
        prefs_populate (layout, tab.widgets, nullptr);

        tabs->addTab (widget, translate_str (tab.name, m_domain));
    }

    return tabs;
}

} // namespace audqt
