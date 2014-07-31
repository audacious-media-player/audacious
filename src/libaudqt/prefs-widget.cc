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

};
