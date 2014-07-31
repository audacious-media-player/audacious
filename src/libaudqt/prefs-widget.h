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

/* boolean widget (checkbox) */
class BooleanWidget {
public:
    BooleanWidget (const PreferencesWidget * parent) :
        m_parent (parent)
    {
    }

    QWidget * widget ();

private:
    const PreferencesWidget * m_parent;
    QCheckBox m_widget;

    bool get ();
    void set (bool);
};

};

#endif
