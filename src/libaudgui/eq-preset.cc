/*
 * eq-preset.c
 * Copyright 2015 John Lindgren
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

#include <string.h>

#include <libaudcore/equalizer.h>
#include <libaudcore/i18n.h>
#include <libaudcore/index.h>
#include <libaudcore/runtime.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"
#include "list.h"
#include "menu.h"
#include "preset-browser.h"

struct PresetItem {
    EqualizerPreset preset;
    bool selected;
};

static Index<PresetItem> preset_list;
static bool changes_made;

static GtkWidget * list, * entry, * add, * revert;

static void get_value (void *, int row, int column, GValue * value)
{
    g_return_if_fail (row >= 0 && row < preset_list.len ());
    g_value_set_string (value, preset_list[row].preset.name);
}

static bool get_selected (void *, int row)
{
    g_return_val_if_fail (row >= 0 && row < preset_list.len (), false);
    return preset_list[row].selected;
}

static void set_selected (void *, int row, bool selected)
{
    g_return_if_fail (row >= 0 && row < preset_list.len ());
    preset_list[row].selected = selected;
}

static void select_all (void *, bool selected)
{
    for (PresetItem & item : preset_list)
        item.selected = selected;
}

static void activate_row (void *, int row)
{
    g_return_if_fail (row >= 0 && row < preset_list.len ());
    aud_eq_apply_preset (preset_list[row].preset);
    aud_set_bool (nullptr, "equalizer_active", true);
}

static void focus_change (void *, int row)
{
    g_return_if_fail (row >= 0 && row < preset_list.len ());
    gtk_entry_set_text ((GtkEntry *) entry, preset_list[row].preset.name);
}

static const AudguiListCallbacks callbacks = {
    get_value,
    get_selected,
    set_selected,
    select_all,
    activate_row,
    nullptr, // right_click
    nullptr, // shift_rows
    nullptr, // data_type
    nullptr, // get_data
    nullptr, // receive_data
    nullptr, // mouse_motion
    nullptr, // mouse_leave
    focus_change
};

static void populate_list ()
{
    auto presets = aud_eq_read_presets ("eq.preset");
    for (const EqualizerPreset & preset : presets)
        preset_list.append (preset, false);
}

static void save_list ()
{
    Index<EqualizerPreset> presets;
    for (const PresetItem & item : preset_list)
        presets.append (item.preset);

    presets.sort ([] (const EqualizerPreset & a, const EqualizerPreset & b)
        { return strcmp (a.name, b.name); });

    aud_eq_write_presets (presets, "eq.preset");
}

static int find_by_name (const char * name)
{
    for (PresetItem & item : preset_list)
    {
        if (! strcmp (item.preset.name, name))
            return & item - preset_list.begin ();
    }

    return -1;
}

static void text_changed ()
{
    const char * name = gtk_entry_get_text ((GtkEntry *) entry);
    gtk_widget_set_sensitive (add, (bool) name[0]);
}

static void add_from_entry ()
{
    const char * name = gtk_entry_get_text ((GtkEntry *) entry);
    int idx = find_by_name (name);

    if (idx < 0)
    {
        idx = preset_list.len ();
        preset_list.append (EqualizerPreset {String (name)});
        audgui_list_insert_rows (list, idx, 1);
    }

    aud_eq_update_preset (preset_list[idx].preset);

    select_all (nullptr, false);
    preset_list[idx].selected = true;

    audgui_list_update_selection (list, 0, preset_list.len ());
    audgui_list_set_focus (list, idx);

    changes_made = true;
    gtk_widget_set_sensitive (revert, true);
}

static void delete_selected ()
{
    auto is_selected = [] (const PresetItem & item)
        { return item.selected; };

    int old_len = preset_list.len ();
    preset_list.remove_if (is_selected);
    int new_len = preset_list.len ();

    if (old_len != new_len)
    {
        audgui_list_delete_rows (list, 0, old_len);
        audgui_list_insert_rows (list, 0, new_len);

        changes_made = true;
        gtk_widget_set_sensitive (revert, true);
    }
}

static void revert_changes ()
{
    audgui_list_delete_rows (list, 0, preset_list.len ());

    preset_list.clear ();
    populate_list ();

    audgui_list_insert_rows (list, 0, preset_list.len ());

    changes_made = false;
    gtk_widget_set_sensitive (revert, false);
}

static void cleanup_eq_preset_window ()
{
    if (changes_made)
    {
        save_list ();
        changes_made = false;
    }

    preset_list.clear ();

    list = nullptr;
    entry = nullptr;
    add = nullptr;
    revert = nullptr;
}

static GtkWidget * create_menu_bar ()
{
    static const AudguiMenuItem import_items[] = {
        MenuCommand (N_("Preset File ..."), nullptr, 0, (GdkModifierType) 0, eq_preset_load_file),
        MenuCommand (N_("EQF File ..."), nullptr, 0, (GdkModifierType) 0, eq_preset_load_eqf),
        MenuSep (),
        MenuCommand (N_("Winamp Presets ..."), nullptr, 0, (GdkModifierType) 0, eq_preset_import_winamp)
    };

    static const AudguiMenuItem export_items[] = {
        MenuCommand (N_("Preset File ..."), nullptr, 0, (GdkModifierType) 0, eq_preset_save_file),
        MenuCommand (N_("EQF File ..."), nullptr, 0, (GdkModifierType) 0, eq_preset_save_eqf)
    };

    static const AudguiMenuItem menus[] = {
        MenuSub (N_("Import"), nullptr, {import_items}),
        MenuSub (N_("Export"), nullptr, {export_items})
    };

    GtkWidget * bar = gtk_menu_bar_new ();
    audgui_menu_init (bar, {menus}, nullptr);
    return bar;
}

static GtkWidget * create_eq_preset_window ()
{
    int dpi = audgui_get_dpi ();

    populate_list ();

    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) window, _("Equalizer Presets"));
    gtk_window_set_type_hint ((GtkWindow *) window, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_default_size ((GtkWindow *) window, 3 * dpi, 3 * dpi);
    audgui_destroy_on_escape (window);

    g_signal_connect (window, "destroy", (GCallback) cleanup_eq_preset_window, nullptr);

    GtkWidget * outer = gtk_vbox_new (false, 0);
    gtk_container_add ((GtkContainer *) window, outer);

    gtk_box_pack_start ((GtkBox *) outer, create_menu_bar (), false, false, 0);

    GtkWidget * vbox = gtk_vbox_new (false, 6);
    gtk_container_set_border_width ((GtkContainer *) vbox, 6);
    gtk_box_pack_start ((GtkBox *) outer, vbox, true, true, 0);

    GtkWidget * hbox = gtk_hbox_new (false, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, false, false, 0);

    entry = gtk_entry_new ();
    gtk_box_pack_start ((GtkBox *) hbox, entry, true, true, 0);

    add = audgui_button_new (_("Save Preset"), "document-save",
     (AudguiCallback) add_from_entry, nullptr);
    gtk_widget_set_sensitive (add, false);
    gtk_box_pack_start ((GtkBox *) hbox, add, false, false, 0);

    g_signal_connect (entry, "activate", (GCallback) add_from_entry, nullptr);
    g_signal_connect (entry, "changed", (GCallback) text_changed, nullptr);

    GtkWidget * scrolled = gtk_scrolled_window_new (nullptr, nullptr);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scrolled, GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled,
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start ((GtkBox *) vbox, scrolled, true, true, 0);

    list = audgui_list_new (& callbacks, nullptr, preset_list.len ());
    gtk_tree_view_set_headers_visible ((GtkTreeView *) list, false);
    audgui_list_add_column (list, nullptr, 0, G_TYPE_STRING, -1);
    gtk_container_add ((GtkContainer *) scrolled, list);

    GtkWidget * hbox2 = gtk_hbox_new (false, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox2, false, false, 0);

    GtkWidget * remove = audgui_button_new (_("Delete Selected"), "edit-delete",
     (AudguiCallback) delete_selected, nullptr);
    gtk_box_pack_start ((GtkBox *) hbox2, remove, false, false, 0);

    revert = audgui_button_new (_("Revert Changes"), "edit-undo",
     (AudguiCallback) revert_changes, nullptr);
    gtk_widget_set_sensitive (revert, false);
    gtk_box_pack_end ((GtkBox *) hbox2, revert, false, false, 0);

    return window;
}

EXPORT void audgui_show_eq_preset_window ()
{
    if (! audgui_reshow_unique_window (AUDGUI_EQ_PRESET_WINDOW))
        audgui_show_unique_window (AUDGUI_EQ_PRESET_WINDOW, create_eq_preset_window ());
}

static void merge_presets (const Index<EqualizerPreset> & presets)
{
    /* eliminate duplicates (could be optimized) */
    for (const EqualizerPreset & preset : presets)
    {
        auto is_duplicate = [& preset] (const PresetItem & item)
            { return item.preset.name == preset.name; };

        preset_list.remove_if (is_duplicate);
    }

    for (const EqualizerPreset & preset : presets)
        preset_list.append (preset, false);
}

EXPORT void audgui_import_eq_presets (const Index<EqualizerPreset> & presets)
{
    if (list)
    {
        /* import via presets window if it exists */
        audgui_list_delete_rows (list, 0, preset_list.len ());
        merge_presets (presets);
        audgui_list_insert_rows (list, 0, preset_list.len ());

        changes_made = true;
        gtk_widget_set_sensitive (revert, true);
    }
    else
    {
        /* otherwise import directly to ~/.config/audacious/eq.preset */
        populate_list ();
        merge_presets (presets);
        save_list ();
        preset_list.clear ();
    }
}
