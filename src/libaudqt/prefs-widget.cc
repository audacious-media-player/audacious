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
#include <math.h>

#include <QButtonGroup>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>

namespace audqt {

HookableWidget::HookableWidget (const PreferencesWidget * parent, const char * domain) :
    m_parent (parent), m_domain (domain)
{
    if (m_parent->cfg.hook)
        hook.capture (new HookReceiver<HookableWidget>
         {m_parent->cfg.hook, this, & HookableWidget::update_from_cfg});
}

void HookableWidget::update_from_cfg ()
{
    m_updating = true;
    update ();
    m_updating = false;
}

/* button */
ButtonWidget::ButtonWidget (const PreferencesWidget * parent, const char * domain) :
    QPushButton (translate_str (parent->label, domain))
{
    QObject::connect (this, & QPushButton::clicked, parent->data.button.callback);
}

/* boolean widget (checkbox) */
BooleanWidget::BooleanWidget (const PreferencesWidget * parent, const char * domain) :
    QCheckBox (translate_str (parent->label, domain)),
    ParentWidget (parent, domain)
{
    update ();

    QObject::connect (this, & QCheckBox::stateChanged, [this] (int state) {
        if (m_updating)
            return;
        m_parent->cfg.set_bool (state != Qt::Unchecked);
        if (m_child_layout)
            enable_layout (m_child_layout, state != Qt::Unchecked);
    });
}

void BooleanWidget::update ()
{
    bool on = m_parent->cfg.get_bool ();
    setCheckState (on ? Qt::Checked : Qt::Unchecked);
    if (m_child_layout)
        enable_layout (m_child_layout, on);
}

/* integer (radio button) */
RadioButtonWidget::RadioButtonWidget (const PreferencesWidget * parent,
 const char * domain, QButtonGroup * btn_group) :
    QRadioButton (translate_str (parent->label, domain)),
    ParentWidget (parent, domain)
{
    if (btn_group)
        btn_group->addButton (this, parent->data.radio_btn.value);

    update ();

    QObject::connect (this, & QAbstractButton::toggled, [this] (bool checked) {
        if (m_updating)
            return;
        if (checked)
            m_parent->cfg.set_int (m_parent->data.radio_btn.value);
        if (m_child_layout)
            enable_layout (m_child_layout, checked);
    });
}

void RadioButtonWidget::update ()
{
    bool checked = (m_parent->cfg.get_int () == m_parent->data.radio_btn.value);
    if (checked)
        setChecked (true);
    if (m_child_layout)
        enable_layout (m_child_layout, checked);
}

/* integer (spinbox) */
IntegerWidget::IntegerWidget (const PreferencesWidget * parent, const char * domain) :
    HookableWidget (parent, domain),
    m_spinner (new QSpinBox)
{
    auto layout = make_hbox (this);

    if (parent->label)
        layout->addWidget (new QLabel (translate_str (parent->label, domain)));

    m_spinner->setRange ((int) m_parent->data.spin_btn.min, (int) m_parent->data.spin_btn.max);
    m_spinner->setSingleStep ((int) m_parent->data.spin_btn.step);
    layout->addWidget (m_spinner);

    if (parent->data.spin_btn.right_label)
        layout->addWidget (new QLabel (translate_str (parent->data.spin_btn.right_label, domain)));

    layout->addStretch (1);

    update ();

    /*
     * Qt has two different valueChanged signals for spin boxes.  So we have to do an explicit
     * cast to the type of the correct valueChanged signal.  --kaniini.
     */
    void (QSpinBox::* signal) (int) = & QSpinBox::valueChanged;
    QObject::connect (m_spinner, signal, [this] (int value) {
        if (! m_updating)
            m_parent->cfg.set_int (value);
    });
}

void IntegerWidget::update ()
{
    m_spinner->setValue (m_parent->cfg.get_int ());
}

/* double (spinbox) */
DoubleWidget::DoubleWidget (const PreferencesWidget * parent, const char * domain) :
    HookableWidget (parent, domain),
    m_spinner (new QDoubleSpinBox)
{
    auto layout = make_hbox (this);

    if (parent->label)
        layout->addWidget (new QLabel (translate_str (parent->label, domain)));

    auto decimals_for = [] (double step)
        { return aud::max (0, -(int) floor (log10 (step) + 0.01)); };

    m_spinner->setDecimals (decimals_for (m_parent->data.spin_btn.step));
    m_spinner->setRange (m_parent->data.spin_btn.min, m_parent->data.spin_btn.max);
    m_spinner->setSingleStep (m_parent->data.spin_btn.step);
    layout->addWidget (m_spinner);

    if (parent->data.spin_btn.right_label)
        layout->addWidget (new QLabel (translate_str (parent->data.spin_btn.right_label, domain)));

    layout->addStretch (1);

    update ();

    void (QDoubleSpinBox::* signal) (double) = & QDoubleSpinBox::valueChanged;
    QObject::connect (m_spinner, signal, [this] (double value) {
        if (! m_updating)
            m_parent->cfg.set_float (value);
    });
}

void DoubleWidget::update ()
{
    m_spinner->setValue (m_parent->cfg.get_float ());
}

/* string (lineedit) */
StringWidget::StringWidget (const PreferencesWidget * parent, const char * domain) :
    HookableWidget (parent, domain),
    m_lineedit (new QLineEdit)
{
    auto layout = make_hbox (this);

    if (parent->label)
        layout->addWidget (new QLabel (translate_str (parent->label, domain)));

    if (parent->type == PreferencesWidget::Entry && parent->data.entry.password)
        m_lineedit->setEchoMode (QLineEdit::Password);

    layout->addWidget (m_lineedit, 1);

    update ();

    QObject::connect (m_lineedit, & QLineEdit::textChanged, [this] (const QString & value) {
        if (! m_updating)
            m_parent->cfg.set_string (value.toUtf8 ());
    });
}

void StringWidget::update ()
{
    m_lineedit->setText ((const char *) m_parent->cfg.get_string ());
}

/* combo box widget (string or int) */
ComboBoxWidget::ComboBoxWidget (const PreferencesWidget * parent, const char * domain) :
    HookableWidget (parent, domain),
    m_combobox (new QComboBox)
{
    auto layout = make_hbox (this);

    if (parent->label)
        layout->addWidget (new QLabel (translate_str (parent->label, domain)));

    layout->addWidget (m_combobox);
    layout->addStretch (1);

    update ();

    void (QComboBox::* signal) (int) = & QComboBox::currentIndexChanged;
    QObject::connect (m_combobox, signal, [this] (int idx) {
        if (m_updating)
            return;

        QVariant data = m_combobox->itemData (idx);

        switch (m_parent->cfg.type)
        {
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
}

void ComboBoxWidget::update ()
{
    ArrayRef<ComboItem> items = m_parent->data.combo.elems;

    if (m_parent->data.combo.fill)
        items = m_parent->data.combo.fill ();

    m_combobox->clear ();

    /* add combobox items */
    switch (m_parent->cfg.type)
    {
    case WidgetConfig::Int:
        for (const ComboItem & item : items)
            m_combobox->addItem (dgettext (m_domain, item.label), item.num);
        break;
    case WidgetConfig::String:
        for (const ComboItem & item : items)
            m_combobox->addItem (dgettext (m_domain, item.label), item.str);
        break;
    default:
        break;
    }

    /* set selected index */
    switch (m_parent->cfg.type)
    {
    case WidgetConfig::Int:
    {
        int num = m_parent->cfg.get_int ();

        for (int i = 0; i < items.len; i ++)
        {
            if (items.data[i].num == num)
            {
                m_combobox->setCurrentIndex (i);
                break;
            }
        }

        break;
    }
    case WidgetConfig::String:
    {
        String str = m_parent->cfg.get_string ();

        for (int i = 0; i < items.len; i ++)
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
BoxWidget::BoxWidget (const PreferencesWidget * parent, const char * domain, bool horizontal_layout)
{
    QBoxLayout * layout;
    if (parent->data.box.horizontal)
        layout = make_hbox (this, sizes.TwoPt);
    else
        layout = make_vbox (this, sizes.TwoPt);

    prefs_populate (layout, parent->data.box.widgets, domain);

    /* only add stretch if the orientation does not match the enclosing layout */
    if (parent->data.box.horizontal != horizontal_layout)
        layout->addStretch (1);
}

TableWidget::TableWidget (const PreferencesWidget * parent, const char * domain)
{
    // TODO: proper table layout
    auto layout = make_vbox (this, sizes.TwoPt);
    prefs_populate (layout, parent->data.table.widgets, domain);
}

NotebookWidget::NotebookWidget (const PreferencesWidget * parent, const char * domain)
{
    for (const NotebookTab & tab : parent->data.notebook.tabs)
    {
        auto widget = new QWidget (this);
        widget->setContentsMargins (margins.FourPt);

        auto layout = make_vbox (widget, sizes.TwoPt);
        prefs_populate (layout, tab.widgets, domain);
        layout->addStretch (1);

        addTab (widget, translate_str (tab.name, domain));
    }
}

} // namespace audqt
