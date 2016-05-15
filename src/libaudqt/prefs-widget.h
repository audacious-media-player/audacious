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

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QTabWidget>

class QButtonGroup;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;

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
public:
    void update_from_cfg ();

protected:
    HookableWidget (const PreferencesWidget * parent, const char * domain);

    virtual ~HookableWidget () {}
    virtual void update () {}

    const PreferencesWidget * const m_parent;
    const char * const m_domain;
    bool m_updating = false;

private:
    SmartPtr<HookReceiver<HookableWidget>> hook;
};

/* shared class which allows disabling child widgets */
class ParentWidget : public HookableWidget {
public:
    void set_child_layout (QLayout * layout)
        { m_child_layout = layout; }

protected:
    ParentWidget (const PreferencesWidget * parent, const char * domain) :
        HookableWidget (parent, domain) {}

    QLayout * m_child_layout = nullptr;
};

/* button widget */
class ButtonWidget : public QPushButton {
public:
    ButtonWidget (const PreferencesWidget * parent, const char * domain);
};

/* boolean widget (checkbox) */
class BooleanWidget : public QCheckBox, public ParentWidget {
public:
    BooleanWidget (const PreferencesWidget * parent, const char * domain);
private:
    void update ();
};

/* integer widget (spinner) */
class IntegerWidget : public QWidget, HookableWidget {
public:
    IntegerWidget (const PreferencesWidget * parent, const char * domain);
private:
    void update ();
    QSpinBox * m_spinner;
};

/* integer widget (radio button) */
class RadioButtonWidget : public QRadioButton, public ParentWidget {
public:
    RadioButtonWidget (const PreferencesWidget * parent, const char * domain,
     QButtonGroup * btn_group);
private:
    void update ();
};

/* double widget (spinner) */
class DoubleWidget : public QWidget, HookableWidget {
public:
    DoubleWidget (const PreferencesWidget * parent, const char * domain);
private:
    void update ();
    QDoubleSpinBox * m_spinner;
};

/* string widget (lineedit) */
class StringWidget : public QWidget, HookableWidget {
public:
    StringWidget (const PreferencesWidget * parent, const char * domain);
private:
    void update ();
    QLineEdit * m_lineedit;
};

/* combo box (string or int) */
class ComboBoxWidget : public QWidget, HookableWidget {
public:
    ComboBoxWidget (const PreferencesWidget * parent, const char * domain);
private:
    void update ();
    QComboBox * m_combobox;
};

/* box container widget */
class BoxWidget : public QWidget {
public:
    BoxWidget (const PreferencesWidget * parent, const char * domain, bool horizontal_layout);
};

/* table container widget */
class TableWidget : public QWidget {
public:
    TableWidget (const PreferencesWidget * parent, const char * domain);
};

/* notebook widget */
class NotebookWidget : public QTabWidget {
public:
    NotebookWidget (const PreferencesWidget * parent, const char * domain);
};

} // namespace audqt

#endif
