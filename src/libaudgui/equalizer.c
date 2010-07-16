/*
 * libaudgui/equalizer.c
 * Copyright 2010 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

/*
 * Note: Because some GTK developer had the bright idea to put the minimum at
 * the top of a GtkVScale and the maximum at the bottom, we have to reverse the
 * sign of the values we get.
 */

#include <math.h>

#include <audacious/audconfig.h>
#include <audacious/i18n.h>
#include <libaudcore/hook.h>

#include "config.h"
#include "libaudgui-gtk.h"

typedef struct
{
    GtkWidget * slider;
    gfloat * setting;
}
SliderPair;

static void on_off_cb (GtkToggleButton * on_off, void * unused)
{
    gboolean active = gtk_toggle_button_get_active (on_off);

    if (aud_cfg->equalizer_active != active)
    {
        aud_cfg->equalizer_active = active;
        hook_call ("equalizer changed", NULL);
    }
}

static void on_off_update (void * unused, GtkWidget * on_off)
{
    gboolean active = gtk_toggle_button_get_active ((GtkToggleButton *) on_off);

    if (active != aud_cfg->equalizer_active)
        gtk_toggle_button_set_active ((GtkToggleButton *) on_off,
         aud_cfg->equalizer_active);
}

static GtkWidget * create_on_off (void)
{
    GtkWidget * on_off;

    on_off = gtk_check_button_new_with_mnemonic (_("_Enable"));
    g_signal_connect ((GObject *) on_off, "toggled", (GCallback) on_off_cb, NULL);
    hook_associate ("equalizer changed", (HookFunction) on_off_update, on_off);

    on_off_update (NULL, on_off);
    return on_off;
}

static void slider_cb (GtkRange * slider, gfloat * setting)
{
    gint old = roundf (* setting);
    gint new = round (-gtk_range_get_value (slider));

    if (old != new)
    {
        * setting = new;
        hook_call ("equalizer changed", NULL);
    }
}

static void slider_update (void * unused, SliderPair * pair)
{
    gint old = round (-gtk_range_get_value ((GtkRange *) pair->slider));
    gint new = roundf (* pair->setting);

    if (old != new)
        gtk_range_set_value ((GtkRange *) pair->slider, -new);
}

static void set_slider_update (GtkWidget * slider, gfloat * setting)
{
    SliderPair * pair = g_slice_new (SliderPair);

    pair->slider = slider;
    pair->setting = setting;

    hook_associate ("equalizer changed", (HookFunction) slider_update, pair);
    slider_update (NULL, pair);
}

static gchar * format_value (GtkScale * slider, gdouble value, void * unused)
{
    return g_strdup_printf ("%d", (gint) -value);
}

static GtkWidget * create_slider (const gchar * name, gfloat * setting)
{
    GtkWidget * vbox, * slider, * label;

    vbox = gtk_vbox_new (FALSE, 6);

    label = gtk_label_new (name);
    gtk_label_set_angle ((GtkLabel *) label, 90);
    gtk_box_pack_start ((GtkBox *) vbox, label, TRUE, FALSE, 0);

    slider = gtk_vscale_new_with_range (-EQUALIZER_MAX_GAIN, EQUALIZER_MAX_GAIN,
     1);
    gtk_scale_set_draw_value ((GtkScale *) slider, TRUE);
    gtk_scale_set_value_pos ((GtkScale *) slider, GTK_POS_BOTTOM);
    gtk_widget_set_size_request (slider, -1, 120);
    g_signal_connect ((GObject *) slider, "format-value", (GCallback)
     format_value, NULL);
    g_signal_connect ((GObject *) slider, "value-changed", (GCallback)
     slider_cb, setting);
    set_slider_update (slider, setting);
    gtk_box_pack_start ((GtkBox *) vbox, slider, FALSE, FALSE, 0);

    return vbox;
}

static GtkWidget * create_window (void)
{
    static const gchar * names[AUD_EQUALIZER_NBANDS] = {N_("60 Hz"),
     N_("170 Hz"), N_("310 Hz"), N_("600 Hz"), N_("1 kHz"), N_("3 kHz"),
     N_("6 kHz"), N_("12 kHz"), N_("14 kHz"), N_("16 kHz")};
    GtkWidget * window, * vbox, * hbox;
    gint i;

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

    gtk_box_pack_start ((GtkBox *) hbox, create_slider (_("Preamp"),
     & aud_cfg->equalizer_preamp), FALSE, FALSE, 0);

    gtk_box_pack_start ((GtkBox *) hbox, gtk_vseparator_new (), FALSE, FALSE, 0);

    for (i = 0; i < AUD_EQUALIZER_NBANDS; i ++)
        gtk_box_pack_start ((GtkBox *) hbox, create_slider (_(names[i]),
         & aud_cfg->equalizer_bands[i]), FALSE, FALSE, 0);

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
    if (equalizer_window != NULL)
        gtk_widget_hide (equalizer_window);
}
