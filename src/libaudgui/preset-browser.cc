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
#include <libaudcore/equalizer.h>
#include <libaudcore/i18n.h>
#include <libaudcore/vfs.h>

typedef void (* FilebrowserCallback) (const char * filename);

static void browser_response (GtkWidget * dialog, int response, void * data)
{
    if (response == GTK_RESPONSE_ACCEPT)
    {
        CharPtr filename (gtk_file_chooser_get_uri ((GtkFileChooser *) dialog));
        ((FilebrowserCallback) data) (filename);
    }

    gtk_widget_destroy (dialog);
}

static void show_preset_browser (const char * title, gboolean save,
 const char * default_filename, FilebrowserCallback callback)
{
    GtkWidget * browser = gtk_file_chooser_dialog_new (title, nullptr, save ?
     GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN, _("Cancel"),
     GTK_RESPONSE_CANCEL, save ? _("Save") : _("Load"), GTK_RESPONSE_ACCEPT,
     nullptr);

    gtk_file_chooser_set_local_only ((GtkFileChooser *) browser, false);

    if (default_filename)
        gtk_file_chooser_set_current_name ((GtkFileChooser *) browser, default_filename);

    g_signal_connect (browser, "response", (GCallback) browser_response, (void *) callback);

    audgui_show_unique_window (AUDGUI_PRESET_BROWSER_WINDOW, browser);
}

static void do_load_file (const char * filename)
{
    EqualizerPreset preset;

    VFSFile file (filename, "r");
    if (! file || ! aud_load_preset_file (preset, file))
        return;

    aud_eq_apply_preset (preset);
}

void eq_preset_load_file ()
{
    show_preset_browser (_("Load Preset File"), false, nullptr, do_load_file);
}

static void do_load_eqf (const char * filename)
{
    VFSFile file (filename, "r");
    if (! file)
        return;

    Index<EqualizerPreset> presets = aud_import_winamp_presets (file);

    if (presets.len ())
        aud_eq_apply_preset (presets[0]);
}

void eq_preset_load_eqf ()
{
    show_preset_browser (_("Load EQF File"), false, nullptr, do_load_eqf);
}

static void do_save_file (const char * filename)
{
    EqualizerPreset preset;
    aud_eq_update_preset (preset);

    VFSFile file (filename, "w");
    if (file)
        aud_save_preset_file (preset, file);
}

void eq_preset_save_file ()
{
    show_preset_browser (_("Save Preset File"), true, _("<name>.preset"), do_save_file);
}

static void do_save_eqf (const char * filename)
{
    VFSFile file (filename, "w");
    if (! file)
        return;

    EqualizerPreset preset = EqualizerPreset ();
    preset.name = String ("Preset1");

    aud_eq_update_preset (preset);
    aud_export_winamp_preset (preset, file);
}

void eq_preset_save_eqf ()
{
    show_preset_browser (_("Save EQF File"), true, _("<name>.eqf"), do_save_eqf);
}

static void do_import_winamp (const char * filename)
{
    VFSFile file (filename, "r");
    if (! file)
        return;

    audgui_import_eq_presets (aud_import_winamp_presets (file));
}

void eq_preset_import_winamp ()
{
    show_preset_browser (_("Import Winamp Presets"), false, nullptr, do_import_winamp);
}
