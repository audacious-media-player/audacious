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

#include "libaudcore/audstrings.h"
#include "libaudcore/equalizer.h"
#include "libaudcore/i18n.h"
#include "libaudcore/interface.h"
#include "libaudcore/runtime.h"
#include "libaudcore/vfs.h"

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

    QModelIndex add_preset (const EqualizerPreset & preset);
    QModelIndex add_preset (const char * name);
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

QModelIndex PresetModel::add_preset (const EqualizerPreset & preset)
{
    int insert_idx = rowCount ();
    for (int row = 0; row < rowCount (); row ++)
    {
        if (preset_at (row)->name == preset.name)
        {
            insert_idx = row;
            break;
        }
    }

    setItem (insert_idx, new PresetItem (preset));
    m_changed = true;

    return index (insert_idx, 0);
}

QModelIndex PresetModel::add_preset (const char * name)
{
    EqualizerPreset preset {String (name)};
    aud_eq_update_preset (preset);
    return add_preset (preset);
}

void PresetModel::apply_preset (int row)
{
    auto preset = preset_at (row);
    if (! preset)
        return;

    aud_eq_apply_preset (* preset);
    aud_set_bool ("equalizer_active", true);
}

class PresetView : public TreeView
{
public:
    PresetView (QPushButton * export_btn) :
        m_export_btn (export_btn)
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

    void add_imported (const Index<EqualizerPreset> & presets);

    const EqualizerPreset * preset_for_export () const
    {
        auto idxs = selectionModel ()->selectedIndexes ();
        if (idxs.size () != 1)
            return nullptr;

        return pmodel ()->preset_at (idxs[0].row ());
    }

protected:
    void activate (const QModelIndex & index) override
        { pmodel ()->apply_preset (index.row ()); }

    void selectionChanged (const QItemSelection & selected,
                           const QItemSelection & deselected) override
    {
        TreeView::selectionChanged (selected, deselected);

        auto idxs = selectionModel ()->selectedIndexes ();
        m_export_btn->setEnabled (idxs.size () == 1);
    }

private:
    QPushButton * m_export_btn;
};

void PresetView::add_imported (const Index<EqualizerPreset> & presets)
{
    QItemSelection sel;
    for (auto & preset : presets)
    {
        auto idx = pmodel ()->add_preset (preset);
        sel.select (idx, idx);
    }

    selectionModel ()->select (sel, QItemSelectionModel::Clear |
                                    QItemSelectionModel::SelectCurrent);

    if (presets.len () == 1)
    {
        aud_eq_apply_preset (presets[0]);
        aud_set_bool ("equalizer_active", true);
    }
}

static Index<EqualizerPreset> import_file (const char * filename)
{
    VFSFile file (filename, "r");
    if (! file)
        return Index<EqualizerPreset> ();

    if (str_has_suffix_nocase (filename, ".eqf") ||
        str_has_suffix_nocase (filename, ".q1"))
    {
        return aud_import_winamp_presets (file);
    }

    /* assume everything else is a native preset file */
    Index<EqualizerPreset> presets;
    presets.append ();

    if (! aud_load_preset_file (presets[0], file))
        presets.clear ();

    return presets;
}

static bool export_file (const char * filename, const EqualizerPreset & preset)
{
    VFSFile file (filename, "w");
    if (! file)
        return false;

    if (str_has_suffix_nocase (filename, ".eqf") ||
        str_has_suffix_nocase (filename, ".q1"))
    {
        return aud_export_winamp_preset (preset, file);
    }

    /* assume everything else is a native preset file */
    return aud_save_preset_file (preset, file);
}

static const char * name_filter = N_("Preset files (*.preset *.eqf *.q1)");

static void show_import_dialog (QDialog * parent, PresetView * view)
{
    auto dialog = new QFileDialog (parent, _("Load Preset File"));

    dialog->setAttribute (Qt::WA_DeleteOnClose);
    dialog->setFileMode (QFileDialog::ExistingFile);
    dialog->setLabelText (QFileDialog::Accept, _("Load"));
    dialog->setNameFilter (_(name_filter));

    QObject::connect (dialog, & QFileDialog::accepted, [dialog, view] () {
        auto urls = dialog->selectedUrls ();
        if (urls.size () != 1)
            return;

        auto filename = urls[0].toEncoded ();
        auto presets = import_file (filename);

        if (presets.len ())
        {
            view->add_imported (presets);
            dialog->deleteLater ();
        }
        else
        {
            aud_ui_show_error (str_printf (_("Error loading %s."), filename.constData ()));
        }
    });

    window_bring_to_front (dialog);
}

static void show_export_dialog (QDialog * parent, const EqualizerPreset & preset)
{
    auto dialog = new QFileDialog (parent, _("Save Preset File"));

    dialog->setAttribute (Qt::WA_DeleteOnClose);
    dialog->setAcceptMode (QFileDialog::AcceptSave);
    dialog->setFileMode (QFileDialog::AnyFile);
    dialog->setLabelText (QFileDialog::Accept, _("Save"));
    dialog->setNameFilter (_(name_filter));

    /* TODO: replace other illegal characters on Win32 */
    auto safe = QString (preset.name).replace (QLatin1Char ('/'), QLatin1Char ('_'));
    dialog->selectFile (safe + ".preset");

    QObject::connect (dialog, & QFileDialog::accepted, [dialog, preset] () {
        auto urls = dialog->selectedUrls ();
        if (urls.size () != 1)
            return;

        auto filename = urls[0].toEncoded ();

        if (export_file (filename, preset))
            dialog->deleteLater ();
        else
            aud_ui_show_error (str_printf (_("Error saving %s."), filename.constData ()));
    });

    window_bring_to_front (dialog);
}

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

    auto import_btn = new QPushButton (_("Import"));
    import_btn->setIcon (get_icon ("document-open"));

    auto export_btn = new QPushButton (_("Export"));
    export_btn->setIcon (get_icon ("document-save"));

    auto view = new PresetView (export_btn);

    auto hbox2 = make_hbox (nullptr);
    hbox2->addWidget (import_btn);
    hbox2->addWidget (export_btn);
    hbox2->addStretch (1);

    auto revert_btn = new QPushButton (_("Revert Changes"));
    revert_btn->setIcon (get_icon ("edit-undo"));
    revert_btn->setDisabled (true);

    auto close_btn = new QPushButton (_("Close"));
    close_btn->setIcon (get_icon ("window-close"));

    auto hbox3 = make_hbox (nullptr);
    hbox3->addWidget (revert_btn);
    hbox3->addStretch (1);
    hbox3->addWidget (close_btn);

    auto vbox = make_vbox (win);
    vbox->addLayout (hbox);
    vbox->addWidget (view);
    vbox->addLayout (hbox2);
    vbox->addLayout (hbox3);

    auto pmodel = view->pmodel ();

    QObject::connect (edit, & QLineEdit::textChanged, [save_btn] (const QString & text) {
        save_btn->setEnabled (! text.isEmpty ());
    });

    QObject::connect (save_btn, & QPushButton::clicked, [view, pmodel, edit, revert_btn] () {
        auto added = pmodel->add_preset (edit->text ().toUtf8 ());
        view->setCurrentIndex (added);
        revert_btn->setDisabled (false);
    });

    QObject::connect (import_btn, & QPushButton::clicked, [win, view] () {
        show_import_dialog (win, view);
    });

    QObject::connect (export_btn, & QPushButton::clicked, [win, view] () {
        auto preset = view->preset_for_export ();
        if (preset != nullptr)
            show_export_dialog (win, * preset);
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
