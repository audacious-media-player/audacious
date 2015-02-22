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

struct PresetItem {
    EqualizerPreset preset;
    bool selected;
};

static Index<PresetItem> preset_list;
static bool changes_made;

static GtkWidget * list, * entry;

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
    if (changes_made)
    {
        auto sort_cb = [] (const EqualizerPreset & a, const EqualizerPreset & b, void *)
            { return strcmp (a.name, b.name); };

        Index<EqualizerPreset> presets;
        for (const PresetItem & item : preset_list)
            presets.append (item.preset);

        presets.sort (sort_cb, nullptr);
        aud_eq_write_presets (presets, "eq.preset");
        changes_made = false;
    }

    preset_list.clear ();
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
    changes_made = true;

    select_all (nullptr, false);
    preset_list[idx].selected = true;

    audgui_list_update_selection (list, 0, preset_list.len ());
    audgui_list_set_focus (list, idx);
}

static GtkWidget * create_eq_preset_window ()
{
    populate_list ();

    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) window, _("Equalizer Presets"));
    gtk_window_set_type_hint ((GtkWindow *) window, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_default_size ((GtkWindow *) window, 300, 300);
    gtk_container_set_border_width ((GtkContainer *) window, 6);
    audgui_destroy_on_escape (window);

    g_signal_connect (window, "destroy", (GCallback) save_list, nullptr);

    GtkWidget * vbox = gtk_vbox_new (false, 6);
    gtk_container_add ((GtkContainer *) window, vbox);

    GtkWidget * hbox = gtk_hbox_new (false, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, false, false, 0);

    entry = gtk_entry_new ();
    gtk_box_pack_start ((GtkBox *) hbox, entry, true, true, 0);

    g_signal_connect (entry, "activate", (GCallback) add_from_entry, nullptr);

    GtkWidget * add = audgui_button_new (_("Save Preset"), "document-save",
     (AudguiCallback) add_from_entry, nullptr);
    gtk_box_pack_start ((GtkBox *) hbox, add, false, false, 0);

    GtkWidget * scrolled = gtk_scrolled_window_new (nullptr, nullptr);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scrolled, GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled,
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start ((GtkBox *) vbox, scrolled, true, true, 0);

    list = audgui_list_new (& callbacks, nullptr, preset_list.len ());
    gtk_tree_view_set_headers_visible ((GtkTreeView *) list, false);
    audgui_list_add_column (list, nullptr, 0, G_TYPE_STRING, -1);
    gtk_container_add ((GtkContainer *) scrolled, list);

    return window;
}

EXPORT void audgui_show_eq_preset_window ()
{
    if (! audgui_reshow_unique_window (AUDGUI_EQ_PRESET_WINDOW))
        audgui_show_unique_window (AUDGUI_EQ_PRESET_WINDOW, create_eq_preset_window ());
}
