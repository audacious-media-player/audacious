/*
 * eq-preset-qt.cc
 * Copyright 2018 John Lindgren
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

#include "libaudqt.h"

#include <QBoxLayout>
#include <QDialog>
#include <QPointer>
#include <QStandardItemModel>
#include <QTreeView>

#include "libaudcore/equalizer.h"
#include "libaudcore/i18n.h"

namespace audqt {

class PresetItem : public QStandardItem
{
public:
    explicit PresetItem (const EqualizerPreset & preset) :
        QStandardItem ((const char *) preset.name),
        preset (preset)
    {
    }

    const EqualizerPreset preset;
};

static QStandardItemModel * create_model (QObject * parent)
{
    auto model = new QStandardItemModel (0, 1, parent);
    auto presets = aud_eq_read_presets ("eq.preset");

    for (const EqualizerPreset & preset : presets)
        model->appendRow (new PresetItem (preset));

    return model;
}

static QDialog * create_preset_win ()
{
    auto win = new QDialog;
    win->setAttribute (Qt::WA_DeleteOnClose);
    win->setWindowTitle (_("Equalizer Presets"));
    win->setContentsMargins (margins.TwoPt);

    auto view = new QTreeView;
    view->setModel (create_model (view));
    view->setEditTriggers (QTreeView::NoEditTriggers);
    view->setHeaderHidden (true);
    view->setIndentation (0);
    view->setUniformRowHeights (true);

    auto vbox = make_vbox (win);
    vbox->addWidget (view);

    return win;
}

static QPointer<QDialog> s_preset_win;

EXPORT void eq_presets_show ()
{
    if (! s_preset_win)
        s_preset_win = create_preset_win ();

    window_bring_to_front (s_preset_win);
}

EXPORT void eq_presets_hide ()
{
    delete s_preset_win;
}

} // namespace audqt
