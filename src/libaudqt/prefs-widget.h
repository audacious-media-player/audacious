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

#include <QtGui>
#include <QtWidgets>

#include <libaudcore/i18n.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#ifndef LIBAUDQT_PREFS_WIDGET_H
#define LIBAUDQT_PREFS_WIDGET_H

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

/* button widget */
class ButtonWidget {
public:
    ButtonWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;
    QPushButton m_widget;
};

/* boolean widget (checkbox) */
class BooleanWidget {
public:
    BooleanWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;
    QCheckBox m_widget;

    bool get ();
    void set (bool);
};

/* label, no get or set functions needed. */
class LabelWidget {
public:
    LabelWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;

    QWidget m_container;
    QHBoxLayout m_layout;
    QLabel m_label_pixmap;
    QLabel m_label_text;
};

/* integer widget (spinner) */
class IntegerWidget {
public:
    IntegerWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;

    QSpinBox m_spinner;
    QLabel m_label_post;
    QLabel m_label_pre;
    QWidget m_container;
    QHBoxLayout m_layout;

    int get ();
    void set (int);
};

/* integer widget (radio button) */
class RadioButtonWidget {
public:
    RadioButtonWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    QWidget * widget (QButtonGroup * btn_group = nullptr);

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;

    QRadioButton m_widget;

    int get ();
    void set (int);
};

/* double widget (spinner) */
class DoubleWidget {
public:
    DoubleWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;

    QDoubleSpinBox m_spinner;
    QLabel m_label_post;
    QLabel m_label_pre;
    QWidget m_container;
    QHBoxLayout m_layout;

    double get ();
    void set (double);
};

/* string widget (lineedit) */
class StringWidget {
public:
    StringWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    virtual QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;

    QWidget m_container;
    QLineEdit m_lineedit;
    QLabel m_label;
    QHBoxLayout m_layout;

    String get ();
    void set (const char *);
};

/* combo box (string or int) */
class ComboBoxWidget {
public:
    ComboBoxWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    virtual QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;

    QWidget m_container;
    QComboBox m_combobox;
    QLabel m_label;
    QHBoxLayout m_layout;

    String get_str ();
    int get_int ();

    void set_str (const char *);
    void set_int (int);

    void fill ();
};

/* box container widget */
class BoxWidget {
public:
    BoxWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;

    QWidget m_container;
    QHBoxLayout m_hbox_layout;
    QVBoxLayout m_vbox_layout;
};

/* table container widget */
class TableWidget {
public:
    TableWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;

    QWidget m_container;
    QVBoxLayout m_layout;
};

/* notebook widget */
class NotebookWidget {
public:
    NotebookWidget (const PreferencesWidget * parent, const char * domain = nullptr) :
        m_parent (parent), m_domain (domain)
    {
    }

    QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    const char * m_domain;

    QTabWidget m_container;
};

}

#endif
