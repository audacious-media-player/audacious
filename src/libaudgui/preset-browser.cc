/*
 * preset-browser.c
 * Copyright 2014-2015 John Lindgren
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

#define AUD_GLIB_INTEGRATION
#include "internal.h"
#include "libaudgui.h"
#include "preset-browser.h"

#include <gtk/gtk.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/vfs.h>

typedef void (* PresetAction) (const char * filename, const EqualizerPreset * preset);

static void browser_response (GtkWidget * dialog, int response, void * data)
{
    if (response == GTK_RESPONSE_ACCEPT)
    {
        CharPtr filename (gtk_file_chooser_get_uri ((GtkFileChooser *) dialog));
        auto preset = (const EqualizerPreset *)
         g_object_get_data ((GObject *) dialog, "eq-preset");

        ((PresetAction) data) (filename, preset);
    }

    gtk_widget_destroy (dialog);
}

static void show_preset_browser (const char * title, gboolean save,
 const char * default_filename, PresetAction callback,
 const EqualizerPreset * preset)
{
    GtkWidget * browser = gtk_file_chooser_dialog_new (title, nullptr, save ?
     GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN, _("Cancel"),
     GTK_RESPONSE_CANCEL, save ? _("Save") : _("Load"), GTK_RESPONSE_ACCEPT,
     nullptr);

    gtk_file_chooser_set_local_only ((GtkFileChooser *) browser, false);

    if (default_filename)
        gtk_file_chooser_set_current_name ((GtkFileChooser *) browser, default_filename);

    if (preset)
        g_object_set_data_full ((GObject *) browser, "eq-preset",
         new EqualizerPreset (* preset), aud::delete_obj<EqualizerPreset>);

    g_signal_connect (browser, "response", (GCallback) browser_response, (void *) callback);

    audgui_show_unique_window (AUDGUI_PRESET_BROWSER_WINDOW, browser);
}

static void do_load_file (const char * filename, const EqualizerPreset *)
{
    Index<EqualizerPreset> presets;
    presets.append ();

    VFSFile file (filename, "r");
    if (! file || ! aud_load_preset_file (presets[0], file))
        return;

    audgui_import_eq_presets (presets);
}

void eq_preset_load_file ()
{
    show_preset_browser (_("Load Preset File"), false, nullptr, do_load_file, nullptr);
}

static void do_load_eqf (const char * filename, const EqualizerPreset *)
{
    VFSFile file (filename, "r");
    if (! file)
        return;

    audgui_import_eq_presets (aud_import_winamp_presets (file));
}

void eq_preset_load_eqf ()
{
    show_preset_browser (_("Load EQF File"), false, nullptr, do_load_eqf, nullptr);
}

static void do_save_file (const char * filename, const EqualizerPreset * preset)
{
    g_return_if_fail (preset);

    VFSFile file (filename, "w");
    if (file)
        aud_save_preset_file (* preset, file);
}

void eq_preset_save_file (const EqualizerPreset & preset)
{
    StringBuf name = str_concat ({preset.name, ".preset"});
    show_preset_browser (_("Save Preset File"), true, name, do_save_file, & preset);
}

static void do_save_eqf (const char * filename, const EqualizerPreset * preset)
{
    g_return_if_fail (preset);

    VFSFile file (filename, "w");
    if (! file)
        return;

    aud_export_winamp_preset (* preset, file);
}

void eq_preset_save_eqf (const EqualizerPreset & preset)
{
    StringBuf name = str_concat ({preset.name, ".eqf"});
    show_preset_browser (_("Save EQF File"), true, name, do_save_eqf, & preset);
}
