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

#ifndef LIBAUDQT_PREFS_WIDGET_H
#define LIBAUDQT_PREFS_WIDGET_H

#include <libaudcore/preferences.h>
#include <libaudcore/hook.h>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QRadioButton;
class QSpinBox;
class QWidget;

namespace audqt {

/*
 * basic idea is this.  create classes which wrap the PreferencesWidgets.
 * Each should have it's own get(), set() and widget() methods.  those are the
 * functions that we really care about.
 * get() and set() allow for introspection and manipulation of the underlying
 * objects.  they also handle pinging the plugin which owns the PreferencesWidget,
 * i.e. calling PreferencesWidget::callback().
 * widget() builds the actual Qt side of the widget, hooks up the relevant signals
 * to slots, etc.  the result of widget() is not const as it is linked into a
 * layout manager or shown or whatever.
 */

/* base class which provides plumbing for hooks. */
class HookableWidget {
protected:
    HookableWidget (const PreferencesWidget * parent, const char * domain) :
        m_parent (parent), m_domain (domain)
    {
        if (m_parent->cfg.hook)
            hook.capture (new HookReceiver<HookableWidget>
             {m_parent->cfg.hook, this, & HookableWidget::update});
    }

    virtual ~HookableWidget () {}
    virtual void update () {}

    const PreferencesWidget * const m_parent;
    const char * const m_domain;

private:
    SmartPtr<HookReceiver<HookableWidget>> hook;
};

/* button widget */
class ButtonWidget : HookableWidget {
public:
    ButtonWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QWidget * widget ();
};

/* boolean widget (checkbox) */
class BooleanWidget : HookableWidget {
public:
    BooleanWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QWidget * widget ();
    void update ();

private:
    QCheckBox * m_checkbox;
};

/* label, no get or set functions needed. */
class LabelWidget : HookableWidget {
public:
    LabelWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QWidget * widget ();
};

/* integer widget (spinner) */
class IntegerWidget : HookableWidget {
public:
    IntegerWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QWidget * widget ();
    void update ();

private:
    QSpinBox * m_spinner;
};

/* integer widget (radio button) */
class RadioButtonWidget : HookableWidget {
public:
    RadioButtonWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QWidget * widget (QButtonGroup * btn_group = nullptr);
    void update ();

private:
    QRadioButton * m_radio;
};

/* double widget (spinner) */
class DoubleWidget : HookableWidget {
public:
    DoubleWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        HookableWidget (parent, domain) {}

    QWidget * widget ();
    void update ();

private:
    QDoubleSpinBox * m_spinner;
};

/* string widget (lineedit) */
class StringWidget : HookableWidget {
public:
    StringWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QWidget * widget ();
    void update ();

private:
    QLineEdit * m_lineedit;
};

/* combo box (string or int) */
class ComboBoxWidget : HookableWidget {
public:
    ComboBoxWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QWidget * widget ();
    void update ();

private:
    QComboBox * m_combobox;
};

/* box container widget */
class BoxWidget : HookableWidget {
public:
    BoxWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QWidget * widget ();
};

/* table container widget */
class TableWidget : HookableWidget {
public:
    TableWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QWidget * widget ();
};

/* notebook widget */
class NotebookWidget : HookableWidget {
public:
    NotebookWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QWidget * widget ();
};

} // namespace audqt

#endif
