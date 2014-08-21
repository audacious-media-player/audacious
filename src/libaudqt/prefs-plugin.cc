/*
 * prefs-plugin.cc
 * Copyright 2014 William Pitcock
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

#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugin.h>
#include <libaudcore/plugins.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"

namespace audqt {

EXPORT void plugin_about (PluginHandle * ph)
{
    Plugin * header = (Plugin *) aud_plugin_get_header (ph);

    if (! header)
        return;

    const char * name = header->name;
    const char * text = header->about_text;

    /* Plugin::about() requires gtk+ and is deprecated anyway so we don't bother with it here. */
    if (! text)
        return;

    if (header->domain)
    {
        name = dgettext (header->domain, name);
        text = dgettext (header->domain, text);
    }

    AUDDBG("name = %s\n", name);

    simple_message(str_printf (_("About %s"), name), text);
}

struct ConfigWindow {
    PluginHandle * ph;
    QDialog * root;
};

static Index<ConfigWindow *> config_windows;

static ConfigWindow * find_config_window (PluginHandle * ph)
{
    for (ConfigWindow * cw : config_windows)
    {
        if (cw && cw->ph == ph)
            return cw;
    }

    return nullptr;
}

EXPORT void plugin_prefs (PluginHandle * ph)
{
    ConfigWindow * cw = find_config_window (ph);

    if (cw)
    {
        window_bring_to_front (cw->root);
        return;
    }

    Plugin * header = (Plugin *) aud_plugin_get_header (ph);
    if (! header)
        return;

    const PluginPreferences * p = header->prefs;
    if (! p)
        return;

    cw = new ConfigWindow;
    config_windows.append (cw);

    cw->ph = ph;
    cw->root = new QDialog;

    if (p->init)
        p->init ();

    const char * name = header->name;
    if (header->domain)
        name = dgettext (header->domain, header->name);

    cw->root->setWindowTitle ((const char *) str_printf(_("%s Settings"), name));

    QVBoxLayout * vbox = new QVBoxLayout;

    prefs_populate (vbox, p->widgets, header->domain);

    QDialogButtonBox * bbox = new QDialogButtonBox;

    if (p->apply)
    {
        bbox->setStandardButtons (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        QObject::connect (bbox, &QDialogButtonBox::accepted, [=] () {
            if (p->apply)
                p->apply ();

            cw->root->hide ();
        });

        QObject::connect (bbox, &QDialogButtonBox::rejected, cw->root, &QWidget::hide);
    }
    else
    {
        bbox->setStandardButtons (QDialogButtonBox::Close);

        QObject::connect (bbox, &QDialogButtonBox::rejected, cw->root, &QWidget::hide);
    }

    vbox->addWidget (bbox);
    cw->root->setLayout (vbox);

    window_bring_to_front (cw->root);
}

};
