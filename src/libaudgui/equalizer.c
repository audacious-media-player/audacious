/*
 * equalizer.c
 * Copyright 2010-2011 John Lindgren
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

/*
 * Note: Because some GTK developer had the bright idea to put the minimum at
 * the top of a GtkVScale and the maximum at the bottom, we have to reverse the
 * sign of the values we get.
 */

#include <math.h>

#include <audacious/i18n.h>
#include <audacious/misc.h>
#include <audacious/types.h>
#include <libaudcore/hook.h>

#include "config.h"
#include "libaudgui-gtk.h"

static void on_off_cb (GtkToggleButton * on_off, void * unused)
{
    aud_set_bool (NULL, "equalizer_active", gtk_toggle_button_get_active (on_off));
}

static void on_off_update (void * unused, GtkWidget * on_off)
{
    gtk_toggle_button_set_active ((GtkToggleButton *) on_off, aud_get_bool
     (NULL, "equalizer_active"));
}

static GtkWidget * create_on_off (void)
{
    GtkWidget * on_off;

    on_off = gtk_check_button_new_with_mnemonic (_("_Enable"));
    g_signal_connect ((GObject *) on_off, "toggled", (GCallback) on_off_cb, NULL);
    hook_associate ("set equalizer_active", (HookFunction) on_off_update, on_off);

    on_off_update (NULL, on_off);
    return on_off;
}

static void slider_moved (GtkRange * slider, void * unused)
{
    int band = GPOINTER_TO_INT (g_object_get_data ((GObject *) slider, "band"));
    double value = round (-gtk_range_get_value (slider));

    if (band == -1)
        aud_set_double (NULL, "equalizer_preamp", value);
    else
        aud_eq_set_band (band, value);
}

static void slider_update (void * unused, GtkRange * slider)
{
    int band = GPOINTER_TO_INT (g_object_get_data ((GObject *) slider, "band"));
    double value;

    if (band == -1)
        value = round (aud_get_double (NULL, "equalizer_preamp"));
    else
        value = round (aud_eq_get_band (band));

    g_signal_handlers_block_by_func (slider, (void *) slider_moved, NULL);
    gtk_range_set_value (slider, -value);
    g_signal_handlers_unblock_by_func (slider, (void *) slider_moved, NULL);
}

static char * format_value (GtkScale * slider, double value, void * unused)
{
    return g_strdup_printf ("%d", (int) -value);
}

static GtkWidget * create_slider (const char * name, int band)
{
    GtkWidget * vbox, * slider, * label;

    vbox = gtk_vbox_new (FALSE, 6);

    label = gtk_label_new (name);
    gtk_label_set_angle ((GtkLabel *) label, 90);
    gtk_box_pack_start ((GtkBox *) vbox, label, TRUE, FALSE, 0);

    slider = gtk_vscale_new_with_range (-EQUALIZER_MAX_GAIN, EQUALIZER_MAX_GAIN, 1);
    gtk_scale_set_draw_value ((GtkScale *) slider, TRUE);
    gtk_scale_set_value_pos ((GtkScale *) slider, GTK_POS_BOTTOM);
    gtk_widget_set_size_request (slider, -1, 120);

    g_object_set_data ((GObject *) slider, "band", GINT_TO_POINTER (band));
    g_signal_connect ((GObject *) slider, "format-value", (GCallback) format_value, NULL);
    g_signal_connect ((GObject *) slider, "value-changed", (GCallback) slider_moved, NULL);

    slider_update (NULL, (GtkRange *) slider);

    if (band == -1)
        hook_associate ("set equalizer_preamp", (HookFunction) slider_update, slider);
    else
        hook_associate ("set equalizer_bands", (HookFunction) slider_update, slider);

    gtk_box_pack_start ((GtkBox *) vbox, slider, FALSE, FALSE, 0);

    return vbox;
}

static GtkWidget * create_window (void)
{
    const char * const names[AUD_EQUALIZER_NBANDS] = {N_("31 Hz"), N_("63 Hz"),
     N_("125 Hz"), N_("250 Hz"), N_("500 Hz"), N_("1 kHz"), N_("2 kHz"),
     N_("4 kHz"), N_("8 kHz"), N_("16 kHz")};
    GtkWidget * window, * vbox, * hbox;
    int i;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title ((GtkWindow *) window, _("Equalizer"));
    gtk_window_set_type_hint ((GtkWindow *) window, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_resizable ((GtkWindow *) window, FALSE);
    gtk_container_set_border_width ((GtkContainer *) window, 6);
    g_signal_connect ((GObject *) window, "delete-event", (GCallback)
     gtk_widget_hide_on_delete, NULL);
    audgui_hide_on_escape (window);

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add ((GtkContainer *) window, vbox);

    gtk_box_pack_start ((GtkBox *) vbox, create_on_off (), FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, FALSE, FALSE, 0);

    gtk_box_pack_start ((GtkBox *) hbox, create_slider (_("Preamp"), -1), FALSE, FALSE, 0);
    gtk_box_pack_start ((GtkBox *) hbox, gtk_vseparator_new (), FALSE, FALSE, 0);

    for (i = 0; i < AUD_EQUALIZER_NBANDS; i ++)
        gtk_box_pack_start ((GtkBox *) hbox, create_slider (_(names[i]), i), FALSE, FALSE, 0);

    gtk_widget_show_all (vbox);
    return window;
}

static GtkWidget * equalizer_window = NULL;

void audgui_show_equalizer_window (void)
{
    if (equalizer_window == NULL)
        equalizer_window = create_window ();

    gtk_window_present ((GtkWindow *) equalizer_window);
}

void audgui_hide_equalizer_window (void)
{
    if (! equalizer_window)
        return;

    hook_dissociate ("set equalizer_active", (HookFunction) on_off_update);
    hook_dissociate ("set equalizer_bands", (HookFunction) slider_update);
    hook_dissociate ("set equalizer_preamp", (HookFunction) slider_update);

    gtk_widget_destroy (equalizer_window);
    equalizer_window = NULL;
}
