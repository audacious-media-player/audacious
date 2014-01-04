/*
 * preferences.c
 * Copyright 2007-2012 Tomasz Moń, William Pitcock, and John Lindgren
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

#include "preferences.h"

#include <string.h>
#include <gtk/gtk.h>

#include "i18n.h"
#include "misc.h"

/* HELPERS */

static bool_t widget_get_bool (const PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg_type == VALUE_BOOLEAN, FALSE);

    if (widget->cfg)
        return * (bool_t *) widget->cfg;
    else if (widget->cname)
        return get_bool (widget->csect, widget->cname);
    else
        return FALSE;
}

static void widget_set_bool (const PreferencesWidget * widget, bool_t value)
{
    g_return_if_fail (widget->cfg_type == VALUE_BOOLEAN);

    if (widget->cfg)
        * (bool_t *) widget->cfg = value;
    else if (widget->cname)
        set_bool (widget->csect, widget->cname, value);

    if (widget->callback)
        widget->callback ();
}

static int widget_get_int (const PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg_type == VALUE_INT, 0);

    if (widget->cfg)
        return * (int *) widget->cfg;
    else if (widget->cname)
        return get_int (widget->csect, widget->cname);
    else
        return 0;
}

static void widget_set_int (const PreferencesWidget * widget, int value)
{
    g_return_if_fail (widget->cfg_type == VALUE_INT);

    if (widget->cfg)
        * (int *) widget->cfg = value;
    else if (widget->cname)
        set_int (widget->csect, widget->cname, value);

    if (widget->callback)
        widget->callback ();
}

static double widget_get_double (const PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg_type == VALUE_FLOAT, 0);

    if (widget->cfg)
        return * (float *) widget->cfg;
    else if (widget->cname)
        return get_double (widget->csect, widget->cname);
    else
        return 0;
}

static void widget_set_double (const PreferencesWidget * widget, double value)
{
    g_return_if_fail (widget->cfg_type == VALUE_FLOAT);

    if (widget->cfg)
        * (float *) widget->cfg = value;
    else if (widget->cname)
        set_double (widget->csect, widget->cname, value);

    if (widget->callback)
        widget->callback ();
}

static char * widget_get_string (const PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg_type == VALUE_STRING, NULL);

    if (widget->cfg)
        return str_get (* (char * *) widget->cfg);
    else if (widget->cname)
        return get_str (widget->csect, widget->cname);
    else
        return NULL;
}

static void widget_set_string (const PreferencesWidget * widget, const char * value)
{
    g_return_if_fail (widget->cfg_type == VALUE_STRING);

    if (widget->cfg)
    {
        g_free (* (char * *) widget->cfg);
        * (char * *) widget->cfg = g_strdup (value);
    }
    else if (widget->cname)
        set_str (widget->csect, widget->cname, value);

    if (widget->callback)
        widget->callback ();
}

/* WIDGET_CHK_BTN */

static void on_toggle_button_toggled (GtkToggleButton * button, const PreferencesWidget * widget)
{
    bool_t active = gtk_toggle_button_get_active (button);
    widget_set_bool (widget, active);

    GtkWidget * child = g_object_get_data ((GObject *) button, "child");
    if (child)
        gtk_widget_set_sensitive (child, active);
}

static void init_toggle_button (GtkWidget * button, const PreferencesWidget * widget)
{
    if (widget->cfg_type != VALUE_BOOLEAN)
        return;

    gtk_toggle_button_set_active ((GtkToggleButton *) button, widget_get_bool (widget));
    g_signal_connect (button, "toggled", (GCallback) on_toggle_button_toggled, (void *) widget);
}

/* WIDGET_LABEL */

static void create_label (const PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * icon, const char * domain)
{
    if (widget->data.label.stock_id)
        * icon = gtk_image_new_from_icon_name (widget->data.label.stock_id, GTK_ICON_SIZE_BUTTON);

    * label = gtk_label_new_with_mnemonic (dgettext (domain, widget->label));
    gtk_label_set_use_markup ((GtkLabel *) * label, TRUE);

    if (widget->data.label.single_line == FALSE)
        gtk_label_set_line_wrap ((GtkLabel *) * label, TRUE);

    gtk_misc_set_alignment ((GtkMisc *) * label, 0, 0.5);
}

/* WIDGET_RADIO_BTN */

static void on_radio_button_toggled (GtkWidget * button, const PreferencesWidget * widget)
{
    if (gtk_toggle_button_get_active ((GtkToggleButton *) button))
        widget_set_int (widget, widget->data.radio_btn.value);
}

static void init_radio_button (GtkWidget * button, const PreferencesWidget * widget)
{
    if (widget->cfg_type != VALUE_INT)
        return;

    if (widget_get_int (widget) == widget->data.radio_btn.value)
        gtk_toggle_button_set_active ((GtkToggleButton *) button, TRUE);

    g_signal_connect (button, "toggled", (GCallback) on_radio_button_toggled, (void *) widget);
}

/* WIDGET_SPIN_BTN */

static void on_spin_btn_changed_int (GtkSpinButton * button, const PreferencesWidget * widget)
{
    widget_set_int (widget, gtk_spin_button_get_value_as_int (button));
}

static void on_spin_btn_changed_float (GtkSpinButton * button, const PreferencesWidget * widget)
{
    widget_set_double (widget, gtk_spin_button_get_value (button));
}

static void create_spin_button (const PreferencesWidget * widget,
 GtkWidget * * label_pre, GtkWidget * * spin_btn, GtkWidget * * label_past,
 const char * domain)
{
    * label_pre = gtk_label_new (dgettext (domain, widget->label));
    * spin_btn = gtk_spin_button_new_with_range (widget->data.spin_btn.min,
     widget->data.spin_btn.max, widget->data.spin_btn.step);

    if (widget->tooltip)
        gtk_widget_set_tooltip_text (* spin_btn, dgettext (domain, widget->tooltip));

    if (widget->data.spin_btn.right_label)
        * label_past = gtk_label_new (dgettext (domain, widget->data.spin_btn.right_label));

    switch (widget->cfg_type)
    {
    case VALUE_INT:
        gtk_spin_button_set_value ((GtkSpinButton *) * spin_btn, widget_get_int (widget));
        g_signal_connect (* spin_btn, "value_changed", (GCallback)
         on_spin_btn_changed_int, (void *) widget);
        break;

    case VALUE_FLOAT:
        gtk_spin_button_set_value ((GtkSpinButton *) * spin_btn, widget_get_double (widget));
        g_signal_connect (* spin_btn, "value_changed", (GCallback)
         on_spin_btn_changed_float, (void *) widget);
        break;

    default:
        break;
    }
}

/* WIDGET_FONT_BTN */

static void on_font_btn_font_set (GtkFontButton * button, const PreferencesWidget * widget)
{
    widget_set_string (widget, gtk_font_button_get_font_name (button));
}

void create_font_btn (const PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * font_btn, const char * domain)
{
    * font_btn = gtk_font_button_new ();
    gtk_font_button_set_use_font ((GtkFontButton *) * font_btn, TRUE);
    gtk_font_button_set_use_size ((GtkFontButton *) * font_btn, TRUE);
    gtk_widget_set_hexpand (* font_btn, TRUE);

    if (widget->label)
    {
        * label = gtk_label_new_with_mnemonic (dgettext (domain, widget->label));
        gtk_label_set_use_markup ((GtkLabel *) * label, TRUE);
        gtk_misc_set_alignment ((GtkMisc *) * label, 1, 0.5);
        gtk_label_set_justify ((GtkLabel *) * label, GTK_JUSTIFY_RIGHT);
        gtk_label_set_mnemonic_widget ((GtkLabel *) * label, * font_btn);
    }

    if (widget->data.font_btn.title)
        gtk_font_button_set_title ((GtkFontButton *) * font_btn,
         dgettext (domain, widget->data.font_btn.title));

    char * name = widget_get_string (widget);
    if (name)
    {
        gtk_font_button_set_font_name ((GtkFontButton *) * font_btn, name);
        str_unref (name);
    }

    g_signal_connect (* font_btn, "font_set", (GCallback) on_font_btn_font_set, (void *) widget);
}

/* WIDGET_ENTRY */

static void on_entry_changed (GtkEntry * entry, const PreferencesWidget * widget)
{
    widget_set_string (widget, gtk_entry_get_text (entry));
}

static void create_entry (const PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * entry, const char * domain)
{
    * entry = gtk_entry_new ();
    gtk_entry_set_visibility ((GtkEntry *) * entry, ! widget->data.entry.password);
    gtk_widget_set_hexpand (* entry, TRUE);

    if (widget->label)
        * label = gtk_label_new (dgettext (domain, widget->label));

    if (widget->tooltip)
        gtk_widget_set_tooltip_text (* entry, dgettext (domain, widget->tooltip));

    if (widget->cfg_type == VALUE_STRING)
    {
        char * value = widget_get_string (widget);
        if (value)
        {
            gtk_entry_set_text ((GtkEntry *) * entry, value);
            str_unref (value);
        }

        g_signal_connect (* entry, "changed", (GCallback) on_entry_changed, (void *) widget);
    }
}

/* WIDGET_COMBO_BOX */

static void on_cbox_changed_int (GtkComboBox * combobox, const PreferencesWidget * widget)
{
    int position = gtk_combo_box_get_active (combobox);
    const ComboBoxElements * elements = g_object_get_data ((GObject *) combobox, "comboboxelements");
    widget_set_int (widget, GPOINTER_TO_INT (elements[position].value));
}

static void on_cbox_changed_string (GtkComboBox * combobox, const PreferencesWidget * widget)
{
    int position = gtk_combo_box_get_active (combobox);
    const ComboBoxElements * elements = g_object_get_data ((GObject *) combobox, "comboboxelements");
    widget_set_string (widget, elements[position].value);
}

static void fill_cbox (GtkWidget * combobox, const PreferencesWidget * widget, const char * domain)
{
    const ComboBoxElements * elements = widget->data.combo.elements;
    int n_elements = widget->data.combo.n_elements;

    if (widget->data.combo.fill)
        elements = widget->data.combo.fill (& n_elements);

    g_object_set_data ((GObject *) combobox, "comboboxelements", (void *) elements);

    for (int i = 0; i < n_elements; i ++)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) combobox,
         dgettext (domain, elements[i].label));

    switch (widget->cfg_type)
    {
    case VALUE_INT:;
        int ivalue = widget_get_int (widget);

        for (int i = 0; i < n_elements; i++)
        {
            if (GPOINTER_TO_INT (elements[i].value) == ivalue)
            {
                gtk_combo_box_set_active ((GtkComboBox *) combobox, i);
                break;
            }
        }

        g_signal_connect (combobox, "changed", (GCallback) on_cbox_changed_int, (void *) widget);
        break;

    case VALUE_STRING:;
        char * value = widget_get_string (widget);

        for(int i = 0; i < n_elements; i++)
        {
            if (value && ! strcmp (elements[i].value, value))
            {
                gtk_combo_box_set_active ((GtkComboBox *) combobox, i);
                break;
            }
        }

        str_unref (value);

        g_signal_connect (combobox, "changed", (GCallback) on_cbox_changed_string, (void *) widget);
        break;

    default:
        break;
    }
}

static void create_cbox (const PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * combobox, const char * domain)
{
    * combobox = gtk_combo_box_text_new ();

    if (widget->label)
        * label = gtk_label_new (dgettext (domain, widget->label));

    fill_cbox (* combobox, widget, domain);
}

/* WIDGET_TABLE */

static void fill_grid (GtkWidget * grid, const PreferencesWidget * elements,
 int n_elements, const char * domain)
{
    for (int i = 0; i < n_elements; i ++)
    {
        GtkWidget * widget_left = NULL, * widget_middle = NULL, * widget_right = NULL;

        switch (elements[i].type)
        {
            case WIDGET_SPIN_BTN:
                create_spin_button (& elements[i], & widget_left,
                 & widget_middle, & widget_right, domain);
                break;

            case WIDGET_LABEL:
                create_label (& elements[i], & widget_middle, & widget_left, domain);
                break;

            case WIDGET_FONT_BTN:
                create_font_btn (& elements[i], & widget_left, & widget_middle, domain);
                break;

            case WIDGET_ENTRY:
                create_entry (& elements[i], & widget_left, & widget_middle, domain);
                break;

            case WIDGET_COMBO_BOX:
                create_cbox (& elements[i], & widget_left, & widget_middle, domain);
                break;

            default:
                break;
        }

        if (widget_left)
            gtk_grid_attach ((GtkGrid *) grid, widget_left, 0, i, 1, 1);

        if (widget_middle)
            gtk_grid_attach ((GtkGrid *) grid, widget_middle, 1, i, 1, 1);

        if (widget_right)
            gtk_grid_attach ((GtkGrid *) grid, widget_right, 2, i, 1, 1);
    }
}

/* ALL WIDGETS */

/* box: a GtkBox */
void create_widgets_with_domain (void * box, const PreferencesWidget * widgets,
 int n_widgets, const char * domain)
{
    GtkWidget * widget = NULL, * child_box = NULL;
    GSList * radio_btn_group = NULL;

    for (int i = 0; i < n_widgets; i ++)
    {
        GtkWidget * label = NULL;

        if (widget && widgets[i].child)
        {
            if (! child_box)
            {
                child_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
                g_object_set_data ((GObject *) widget, "child", child_box);

                GtkWidget * alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
                gtk_box_pack_start (box, alignment, FALSE, FALSE, 0);
                gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 0, 12, 0);
                gtk_container_add ((GtkContainer *) alignment, child_box);

                if (GTK_IS_TOGGLE_BUTTON (widget))
                    gtk_widget_set_sensitive (child_box,
                     gtk_toggle_button_get_active ((GtkToggleButton *) widget));
            }
        }
        else
            child_box = NULL;

        GtkWidget * alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
        gtk_alignment_set_padding ((GtkAlignment *) alignment, 6, 0, 12, 0);
        gtk_box_pack_start (child_box ? (GtkBox *) child_box : box, alignment, FALSE, FALSE, 0);

        widget = NULL;

        if (radio_btn_group && widgets[i].type != WIDGET_RADIO_BTN)
            radio_btn_group = NULL;

        switch (widgets[i].type)
        {
            case WIDGET_CHK_BTN:
                widget = gtk_check_button_new_with_mnemonic (dgettext (domain, widgets[i].label));
                init_toggle_button (widget, & widgets[i]);
                break;

            case WIDGET_LABEL:
                if (strstr (widgets[i].label, "<b>"))
                    gtk_alignment_set_padding ((GtkAlignment *) alignment,
                     (i == 0) ? 0 : 12, 0, 0, 0);

                GtkWidget * icon = NULL;
                create_label (& widgets[i], & label, & icon, domain);

                if (icon)
                {
                    widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
                    gtk_box_pack_start ((GtkBox *) widget, icon, FALSE, FALSE, 0);
                    gtk_box_pack_start ((GtkBox *) widget, label, FALSE, FALSE, 0);
                }
                else
                    widget = label;

                break;

            case WIDGET_RADIO_BTN:
                widget = gtk_radio_button_new_with_mnemonic (radio_btn_group,
                 dgettext (domain, widgets[i].label));
                radio_btn_group = gtk_radio_button_get_group ((GtkRadioButton *) widget);
                init_radio_button (widget, & widgets[i]);
                break;

            case WIDGET_SPIN_BTN:
                widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

                GtkWidget * label_pre = NULL, * spin_btn = NULL, * label_past = NULL;
                create_spin_button (& widgets[i], & label_pre, & spin_btn, & label_past, domain);

                if (label_pre)
                    gtk_box_pack_start ((GtkBox *) widget, label_pre, FALSE, FALSE, 0);
                if (spin_btn)
                    gtk_box_pack_start ((GtkBox *) widget, spin_btn, FALSE, FALSE, 0);
                if (label_past)
                    gtk_box_pack_start ((GtkBox *) widget, label_past, FALSE, FALSE, 0);

                break;

            case WIDGET_CUSTOM:
                if (widgets[i].data.populate)
                    widget = widgets[i].data.populate ();

                break;

            case WIDGET_FONT_BTN:
                widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

                GtkWidget * font_btn = NULL;
                create_font_btn (& widgets[i], & label, & font_btn, domain);

                if (label)
                    gtk_box_pack_start ((GtkBox *) widget, label, FALSE, FALSE, 0);
                if (font_btn)
                    gtk_box_pack_start ((GtkBox *) widget, font_btn, FALSE, FALSE, 0);

                break;

            case WIDGET_TABLE:
                widget = gtk_grid_new ();
                gtk_grid_set_column_spacing ((GtkGrid *) widget, 6);
                gtk_grid_set_row_spacing ((GtkGrid *) widget, 6);

                fill_grid (widget, widgets[i].data.table.elem, widgets[i].data.table.rows, domain);

                break;

            case WIDGET_ENTRY:
                widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

                GtkWidget * entry = NULL;
                create_entry (& widgets[i], & label, & entry, domain);

                if (label)
                    gtk_box_pack_start ((GtkBox *) widget, label, FALSE, FALSE, 0);
                if (entry)
                    gtk_box_pack_start ((GtkBox *) widget, entry, TRUE, TRUE, 0);

                break;

            case WIDGET_COMBO_BOX:
                widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

                GtkWidget * combo = NULL;
                create_cbox (& widgets[i], & label, & combo, domain);

                if (label)
                    gtk_box_pack_start ((GtkBox *) widget, label, FALSE, FALSE, 0);
                if (combo)
                    gtk_box_pack_start ((GtkBox *) widget, combo, FALSE, FALSE, 0);

                break;

            case WIDGET_BOX:
                if (widgets[i].data.box.horizontal)
                    widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
                else
                    widget = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

                create_widgets_with_domain ((GtkBox *) widget,
                 widgets[i].data.box.elem, widgets[i].data.box.n_elem, domain);

                if (widgets[i].data.box.frame)
                {
                    GtkWidget * frame = gtk_frame_new (dgettext (domain, widgets[i].label));
                    gtk_container_add ((GtkContainer *) frame, widget);
                    widget = frame;
                }

                break;

            case WIDGET_NOTEBOOK:
                gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 0, 0, 0);

                widget = gtk_notebook_new ();

                for (int j = 0; j < widgets[i].data.notebook.n_tabs; j ++)
                {
                    GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
                    gtk_container_set_border_width ((GtkContainer *) vbox, 6);

                    create_widgets_with_domain ((GtkBox *) vbox,
                     widgets[i].data.notebook.tabs[j].widgets,
                     widgets[i].data.notebook.tabs[j].n_widgets, domain);

                    gtk_notebook_append_page ((GtkNotebook *) widget, vbox,
                     gtk_label_new (dgettext (domain,
                     widgets[i].data.notebook.tabs[j].name)));
                }

                break;

            case WIDGET_SEPARATOR:
                gtk_alignment_set_padding ((GtkAlignment *) alignment, 6, 6, 0, 0);

                widget = gtk_separator_new (widgets[i].data.separator.horizontal
                 ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
                break;

            default:
                break;
        }

        if (widget)
        {
            /* use uniform spacing for horizontal boxes */
            if (gtk_orientable_get_orientation ((GtkOrientable *) box) ==
             GTK_ORIENTATION_HORIZONTAL)
                gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 0, 0, 0);

            gtk_container_add ((GtkContainer *) alignment, widget);

            if (widgets[i].tooltip && widgets[i].type != WIDGET_SPIN_BTN)
                gtk_widget_set_tooltip_text (widget, dgettext (domain,
                 widgets[i].tooltip));
        }
    }
}
