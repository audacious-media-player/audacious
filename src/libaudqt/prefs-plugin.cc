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

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/i18n.h>
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

    const char * name = header->info.name;
    const char * text = header->info.about;
    if (! text)
        return;

    if (header->info.domain)
    {
        name = dgettext (header->info.domain, name);
        text = dgettext (header->info.domain, text);
    }

    AUDDBG ("name = %s\n", name);

    simple_message (str_printf (_("About %s"), name), text, QMessageBox::Information);
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

    if (cw && cw->root)
    {
        window_bring_to_front (cw->root);
        return;
    }

    Plugin * header = (Plugin *) aud_plugin_get_header (ph);
    if (! header)
        return;

    const PluginPreferences * p = header->info.prefs;
    if (! p)
        return;

    if (! cw)
    {
        cw = new ConfigWindow {ph};
        config_windows.append (cw);
    }

    cw->root = new QDialog;
    cw->root->setAttribute (Qt::WA_DeleteOnClose);
    cw->root->setContentsMargins (margins.FourPt);

    if (p->init)
        p->init ();

    QObject::connect (cw->root, & QObject::destroyed, [p, cw] () {
        if (p->cleanup)
            p->cleanup ();

        cw->root = nullptr;
    });

    const char * name = header->info.name;
    if (header->info.domain)
        name = dgettext (header->info.domain, name);

    cw->root->setWindowTitle ((const char *) str_printf(_("%s Settings"), name));

    auto vbox = make_vbox (cw->root, sizes.TwoPt);
    prefs_populate (vbox, p->widgets, header->info.domain);
    vbox->addStretch (1);

    QDialogButtonBox * bbox = new QDialogButtonBox;

    if (p->apply)
    {
        bbox->setStandardButtons (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        bbox->button (QDialogButtonBox::Ok)->setText (translate_str (N_("_Set")));
        bbox->button (QDialogButtonBox::Cancel)->setText (translate_str (N_("_Cancel")));

        QObject::connect (bbox, & QDialogButtonBox::accepted, [p, cw] () {
            p->apply ();
            cw->root->deleteLater ();
        });
    }
    else
    {
        bbox->setStandardButtons (QDialogButtonBox::Close);
        bbox->button (QDialogButtonBox::Close)->setText (translate_str (N_("_Close")));
    }

    QObject::connect (bbox, & QDialogButtonBox::rejected, cw->root, & QObject::deleteLater);

    vbox->addWidget (bbox);

    window_bring_to_front (cw->root);
}

} // namespace audqt
