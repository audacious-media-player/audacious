/*
 * prefs-builder.cc
 * Copyright 2014 William Pitcock and John Lindgren
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

void prefs_populate (QLayout * layout, ArrayRef<const PreferencesWidget> widgets, const char * domain)
{
    if (! layout)
    {
        AUDDBG("prefs_populate was passed a null layout!\n");
        return;
    }

    for (const PreferencesWidget & w : widgets)
    {
        switch (w.type)
        {
        default:
            AUDDBG("invoked stub handler for PreferencesWidget type %d\n", w.type);
            break;
        }
    }
}

};

