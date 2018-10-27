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
#include "treeview.h"

#include <QBoxLayout>
#include <QDialog>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QStandardItemModel>

#include "libaudcore/equalizer.h"
#include "libaudcore/i18n.h"
#include "libaudcore/runtime.h"

namespace audqt {

class PresetItem : public QStandardItem
{
public:
    explicit PresetItem (const EqualizerPreset & preset) :
        QStandardItem ((const char *) preset.name),
        preset (preset) {}

    const EqualizerPreset preset;
};

class PresetModel : public QStandardItemModel
{
public:
    explicit PresetModel (QObject * parent) :
        QStandardItemModel (0, 1, parent) {}

    void load_all ();
    void save_all ();
    void add_preset (const char * name);
    void apply_preset (int row);

    const EqualizerPreset * preset_at (int row) const
    {
        auto pitem = static_cast<PresetItem *> (item (row));
        return pitem ? & pitem->preset : nullptr;
    }

    bool removeRows (int row, int count, const QModelIndex & parent = QModelIndex ()) override
    {
        bool removed = QStandardItemModel::removeRows (row, count, parent);
        m_changed = m_changed || removed;
        return removed;
    }

private:
    bool m_changed = false;
};

void PresetModel::load_all ()
{
    clear ();

    auto presets = aud_eq_read_presets ("eq.preset");
    for (const EqualizerPreset & preset : presets)
        appendRow (new PresetItem (preset));

    m_changed = false;
}

void PresetModel::save_all ()
{
    if (! m_changed)
        return;

    Index<EqualizerPreset> presets;
    for (int row = 0; row < rowCount (); row ++)
        presets.append (* preset_at (row));

    presets.sort ([] (const EqualizerPreset & a, const EqualizerPreset & b)
        { return strcmp (a.name, b.name); });

    aud_eq_write_presets (presets, "eq.preset");
    m_changed = false;
}

void PresetModel::add_preset (const char * name)
{
    int insert_idx = rowCount ();
    for (int row = 0; row < rowCount (); row ++)
    {
        if (! strcmp (preset_at (row)->name, name))
        {
            insert_idx = row;
            break;
        }
    }

    EqualizerPreset preset {String (name)};
    aud_eq_update_preset (preset);
    setItem (insert_idx, new PresetItem (preset));
    m_changed = true;
}

void PresetModel::apply_preset (int row)
{
    auto preset = preset_at (row);
    if (! preset)
        return;

    aud_eq_apply_preset (* preset);
    aud_set_bool (nullptr, "equalizer_active", true);
}

class PresetView : public TreeView
{
public:
    PresetView ()
    {
        setEditTriggers (QTreeView::NoEditTriggers);
        setHeaderHidden (true);
        setIndentation (0);
        setSelectionMode (QTreeView::ExtendedSelection);
        setUniformRowHeights (true);

        auto pmodel = new PresetModel (this);
        pmodel->load_all ();
        setModel (pmodel);
    }

    PresetModel * pmodel () const
        { return static_cast<PresetModel *> (model ()); }

protected:
    void activate (const QModelIndex & index) override
        { pmodel ()->apply_preset (index.row ()); }
};

static QDialog * create_preset_win ()
{
    auto win = new QDialog;
    win->setAttribute (Qt::WA_DeleteOnClose);
    win->setWindowTitle (_("Equalizer Presets"));
    win->setContentsMargins (margins.TwoPt);

    auto edit = new QLineEdit;
    auto save_btn = new QPushButton (_("Save Preset"));
    save_btn->setIcon (get_icon ("document-save"));
    save_btn->setDisabled (true);

    auto hbox = make_hbox (nullptr);
    hbox->addWidget (edit);
    hbox->addWidget (save_btn);

    auto view = new PresetView;

    auto revert_btn = new QPushButton (_("Revert Changes"));
    revert_btn->setIcon (get_icon ("edit-undo"));
    revert_btn->setDisabled (true);

    auto close_btn = new QPushButton (_("Close"));
    close_btn->setIcon (get_icon ("window-close"));

    auto hbox2 = make_hbox (nullptr);
    hbox2->addWidget (revert_btn);
    hbox2->addStretch (1);
    hbox2->addWidget (close_btn);

    auto vbox = make_vbox (win);
    vbox->addLayout (hbox);
    vbox->addWidget (view);
    vbox->addLayout (hbox2);

    auto pmodel = view->pmodel ();

    QObject::connect (edit, & QLineEdit::textChanged, [save_btn] (const QString & text) {
        save_btn->setEnabled (! text.isEmpty ());
    });

    QObject::connect (save_btn, & QPushButton::clicked, [pmodel, edit, revert_btn] () {
        pmodel->add_preset (edit->text ().toUtf8 ());
        revert_btn->setDisabled (false);
    });

    QObject::connect (pmodel, & PresetModel::rowsRemoved, [revert_btn] () {
        revert_btn->setDisabled (false);
    });

    QObject::connect (revert_btn, & QPushButton::clicked, [pmodel, revert_btn] () {
        pmodel->load_all ();
        revert_btn->setDisabled (true);
    });

    QObject::connect (close_btn, & QPushButton::clicked, win, & QObject::deleteLater);

    QObject::connect (win, & QObject::destroyed, [pmodel] () {
        pmodel->save_all ();
    });

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
