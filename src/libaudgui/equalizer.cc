/*
 * equalizer.c
 * Copyright 2010-2015 John Lindgren
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

#include <math.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/equalizer.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static void on_off_cb (GtkToggleButton * on_off)
{
    aud_set_bool (nullptr, "equalizer_active", gtk_toggle_button_get_active (on_off));
}

static void on_off_update (void *, GtkWidget * on_off)
{
    gtk_toggle_button_set_active ((GtkToggleButton *) on_off, aud_get_bool
     (nullptr, "equalizer_active"));
}

static GtkWidget * create_on_off ()
{
    GtkWidget * on_off = gtk_check_button_new_with_mnemonic (_("_Enable"));
    g_signal_connect (on_off, "toggled", (GCallback) on_off_cb, nullptr);
    hook_associate ("set equalizer_active", (HookFunction) on_off_update, on_off);

    on_off_update (nullptr, on_off);
    return on_off;
}

static void reset_to_zero ()
{
    aud_eq_apply_preset (EqualizerPreset ());
}

static void slider_moved (GtkRange * slider)
{
    int band = GPOINTER_TO_INT (g_object_get_data ((GObject *) slider, "band"));
    double value = round (gtk_range_get_value (slider));

    if (band == -1)
        aud_set_double (nullptr, "equalizer_preamp", value);
    else
        aud_eq_set_band (band, value);
}

static GtkWidget * create_slider (const char * name, int band, GtkWidget * hbox)
{
    GtkWidget * vbox = gtk_vbox_new (false, 6);

    GtkWidget * label = gtk_label_new (name);
    gtk_label_set_angle ((GtkLabel *) label, 90);
    gtk_box_pack_start ((GtkBox *) vbox, label, true, false, 0);

    GtkWidget * slider = gtk_vscale_new_with_range (-AUD_EQ_MAX_GAIN, AUD_EQ_MAX_GAIN, 1);
    gtk_scale_set_draw_value ((GtkScale *) slider, true);
    gtk_scale_set_value_pos ((GtkScale *) slider, GTK_POS_BOTTOM);
    gtk_range_set_inverted ((GtkRange *) slider, true);
    gtk_widget_set_size_request (slider, -1, audgui_get_dpi () * 5 / 4);

    g_object_set_data ((GObject *) slider, "band", GINT_TO_POINTER (band));
    g_signal_connect (slider, "value-changed", (GCallback) slider_moved, nullptr);

    gtk_box_pack_start ((GtkBox *) vbox, slider, false, false, 0);
    gtk_box_pack_start ((GtkBox *) hbox, vbox, false, false, 0);

    return slider;
}

static void set_slider (GtkWidget * slider, double value)
{
    g_signal_handlers_block_by_func (slider, (void *) slider_moved, nullptr);
    gtk_range_set_value ((GtkRange *) slider, round (value));
    g_signal_handlers_unblock_by_func (slider, (void *) slider_moved, nullptr);
}

static void update_sliders (void *, GtkWidget * window)
{
    GtkWidget * preamp = (GtkWidget *) g_object_get_data ((GObject *) window, "preamp");
    set_slider (preamp, aud_get_double (nullptr, "equalizer_preamp"));

    double values[AUD_EQ_NBANDS];
    aud_eq_get_bands (values);

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
    {
        StringBuf slider_id = str_printf ("slider%d", i);
        GtkWidget * slider = (GtkWidget *) g_object_get_data ((GObject *) window, slider_id);
        set_slider (slider, values[i]);
    }
}

static void destroy_cb ()
{
    hook_dissociate ("set equalizer_active", (HookFunction) on_off_update);
    hook_dissociate ("set equalizer_bands", (HookFunction) update_sliders);
    hook_dissociate ("set equalizer_preamp", (HookFunction) update_sliders);
}

static GtkWidget * create_window ()
{
    const char * const names[AUD_EQ_NBANDS] = {N_("31 Hz"), N_("63 Hz"),
     N_("125 Hz"), N_("250 Hz"), N_("500 Hz"), N_("1 kHz"), N_("2 kHz"),
     N_("4 kHz"), N_("8 kHz"), N_("16 kHz")};

    GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) window, _("Equalizer"));
    gtk_window_set_type_hint ((GtkWindow *) window, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_resizable ((GtkWindow *) window, false);
    gtk_container_set_border_width ((GtkContainer *) window, 6);
    audgui_destroy_on_escape (window);

    GtkWidget * vbox = gtk_vbox_new (false, 6);
    gtk_container_add ((GtkContainer *) window, vbox);

    GtkWidget * top_row = gtk_hbox_new (false, 6);
    gtk_box_pack_start ((GtkBox *) vbox, top_row, false, false, 0);

    gtk_box_pack_start ((GtkBox *) top_row, create_on_off (), false, false, 0);

    GtkWidget * presets = audgui_button_new (_("Presets ..."), nullptr,
     (AudguiCallback) audgui_show_eq_preset_window, nullptr);
    gtk_box_pack_end ((GtkBox *) top_row, presets, false, false, 0);

    GtkWidget * zero = audgui_button_new (_("Reset to Zero"), nullptr,
     (AudguiCallback) reset_to_zero, nullptr);
    gtk_box_pack_end ((GtkBox *) top_row, zero, false, false, 0);

    GtkWidget * hbox = gtk_hbox_new (false, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, false, false, 0);

    GtkWidget * preamp = create_slider (_("Preamp"), -1, hbox);
    g_object_set_data ((GObject *) window, "preamp", preamp);

    gtk_box_pack_start ((GtkBox *) hbox, gtk_vseparator_new (), false, false, 0);

    for (int i = 0; i < AUD_EQ_NBANDS; i ++)
    {
        StringBuf slider_id = str_printf ("slider%d", i);
        GtkWidget * slider = create_slider (_(names[i]), i, hbox);
        g_object_set_data ((GObject *) window, slider_id, slider);
    }

    update_sliders (nullptr, window);

    hook_associate ("set equalizer_preamp", (HookFunction) update_sliders, window);
    hook_associate ("set equalizer_bands", (HookFunction) update_sliders, window);

    g_signal_connect (window, "destroy", (GCallback) destroy_cb, nullptr);

    return window;
}

EXPORT void audgui_show_equalizer_window ()
{
    if (! audgui_reshow_unique_window (AUDGUI_EQUALIZER_WINDOW))
        audgui_show_unique_window (AUDGUI_EQUALIZER_WINDOW, create_window ());
}

EXPORT void audgui_hide_equalizer_window ()
{
    audgui_hide_unique_window (AUDGUI_EQUALIZER_WINDOW);
}
