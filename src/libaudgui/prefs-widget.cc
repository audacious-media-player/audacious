/*
 * prefs-widget.c
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

#include <string.h>
#include <gtk/gtk.h>

#include <libaudcore/i18n.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include "libaudgui-gtk.h"

/* HELPERS */

static bool widget_get_bool (const PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg.type == WidgetConfig::Bool, false);

    if (widget->cfg.value)
        return * (bool *) widget->cfg.value;
    else if (widget->cfg.name)
        return aud_get_bool (widget->cfg.section, widget->cfg.name);
    else
        return false;
}

static void widget_set_bool (const PreferencesWidget * widget, bool value)
{
    g_return_if_fail (widget->cfg.type == WidgetConfig::Bool);

    if (widget->cfg.value)
        * (bool *) widget->cfg.value = value;
    else if (widget->cfg.name)
        aud_set_bool (widget->cfg.section, widget->cfg.name, value);

    if (widget->cfg.callback)
        widget->cfg.callback ();
}

static int widget_get_int (const PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg.type == WidgetConfig::Int, 0);

    if (widget->cfg.value)
        return * (int *) widget->cfg.value;
    else if (widget->cfg.name)
        return aud_get_int (widget->cfg.section, widget->cfg.name);
    else
        return 0;
}

static void widget_set_int (const PreferencesWidget * widget, int value)
{
    g_return_if_fail (widget->cfg.type == WidgetConfig::Int);

    if (widget->cfg.value)
        * (int *) widget->cfg.value = value;
    else if (widget->cfg.name)
        aud_set_int (widget->cfg.section, widget->cfg.name, value);

    if (widget->cfg.callback)
        widget->cfg.callback ();
}

static double widget_get_double (const PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg.type == WidgetConfig::Float, 0);

    if (widget->cfg.value)
        return * (double *) widget->cfg.value;
    else if (widget->cfg.name)
        return aud_get_double (widget->cfg.section, widget->cfg.name);
    else
        return 0;
}

static void widget_set_double (const PreferencesWidget * widget, double value)
{
    g_return_if_fail (widget->cfg.type == WidgetConfig::Float);

    if (widget->cfg.value)
        * (double *) widget->cfg.value = value;
    else if (widget->cfg.name)
        aud_set_double (widget->cfg.section, widget->cfg.name, value);

    if (widget->cfg.callback)
        widget->cfg.callback ();
}

static String widget_get_string (const PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg.type == WidgetConfig::String, String ());

    if (widget->cfg.value)
        return * (String *) widget->cfg.value;
    else if (widget->cfg.name)
        return aud_get_str (widget->cfg.section, widget->cfg.name);
    else
        return String ();
}

static void widget_set_string (const PreferencesWidget * widget, const char * value)
{
    g_return_if_fail (widget->cfg.type == WidgetConfig::String);

    if (widget->cfg.value)
        * (String *) widget->cfg.value = String (value);
    else if (widget->cfg.name)
        aud_set_str (widget->cfg.section, widget->cfg.name, value);

    if (widget->cfg.callback)
        widget->cfg.callback ();
}

/* WIDGET_CHK_BTN */

static void on_toggle_button_toggled (GtkToggleButton * button, const PreferencesWidget * widget)
{
    gboolean active = gtk_toggle_button_get_active (button);
    widget_set_bool (widget, active);

    GtkWidget * child = (GtkWidget *) g_object_get_data ((GObject *) button, "child");
    if (child)
        gtk_widget_set_sensitive (child, active);
}

static void init_toggle_button (GtkWidget * button, const PreferencesWidget * widget)
{
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
    gtk_label_set_use_markup ((GtkLabel *) * label, true);

    if (widget->data.label.single_line == false)
        gtk_label_set_line_wrap ((GtkLabel *) * label, true);

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
    if (widget_get_int (widget) == widget->data.radio_btn.value)
        gtk_toggle_button_set_active ((GtkToggleButton *) button, true);

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

    switch (widget->cfg.type)
    {
    case WidgetConfig::Int:
        gtk_spin_button_set_value ((GtkSpinButton *) * spin_btn, widget_get_int (widget));
        g_signal_connect (* spin_btn, "value_changed", (GCallback)
         on_spin_btn_changed_int, (void *) widget);
        break;

    case WidgetConfig::Float:
        gtk_spin_button_set_value ((GtkSpinButton *) * spin_btn, widget_get_double (widget));
        g_signal_connect (* spin_btn, "value_changed", (GCallback)
         on_spin_btn_changed_float, (void *) widget);
        break;

    default:
        g_warn_if_reached ();
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
    gtk_font_button_set_use_font ((GtkFontButton *) * font_btn, true);
    gtk_font_button_set_use_size ((GtkFontButton *) * font_btn, true);

    if (widget->label)
    {
        * label = gtk_label_new (dgettext (domain, widget->label));
        gtk_misc_set_alignment ((GtkMisc *) * label, 1, 0.5);
    }

    if (widget->data.font_btn.title)
        gtk_font_button_set_title ((GtkFontButton *) * font_btn,
         dgettext (domain, widget->data.font_btn.title));

    String name = widget_get_string (widget);
    if (name)
        gtk_font_button_set_font_name ((GtkFontButton *) * font_btn, name);

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

    if (widget->label)
    {
        * label = gtk_label_new (dgettext (domain, widget->label));
        gtk_misc_set_alignment ((GtkMisc *) * label, 1, 0.5);
    }

    if (widget->tooltip)
        gtk_widget_set_tooltip_text (* entry, dgettext (domain, widget->tooltip));

    String value = widget_get_string (widget);
    if (value)
        gtk_entry_set_text ((GtkEntry *) * entry, value);

    g_signal_connect (* entry, "changed", (GCallback) on_entry_changed, (void *) widget);
}

/* WIDGET_COMBO_BOX */

static void on_cbox_changed_int (GtkComboBox * combobox, const PreferencesWidget * widget)
{
    int position = gtk_combo_box_get_active (combobox);
    const ComboBoxElements * elements = (const ComboBoxElements *)
     g_object_get_data ((GObject *) combobox, "comboboxelements");
    widget_set_int (widget, GPOINTER_TO_INT (elements[position].value));
}

static void on_cbox_changed_string (GtkComboBox * combobox, const PreferencesWidget * widget)
{
    int position = gtk_combo_box_get_active (combobox);
    const ComboBoxElements * elements = (const ComboBoxElements *)
     g_object_get_data ((GObject *) combobox, "comboboxelements");
    widget_set_string (widget, (const char *) elements[position].value);
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

    switch (widget->cfg.type)
    {
    case WidgetConfig::Int:
    {
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
    }

    case WidgetConfig::String:
    {
        String value = widget_get_string (widget);

        for(int i = 0; i < n_elements; i++)
        {
            if (value && ! strcmp ((const char *) elements[i].value, value))
            {
                gtk_combo_box_set_active ((GtkComboBox *) combobox, i);
                break;
            }
        }

        g_signal_connect (combobox, "changed", (GCallback) on_cbox_changed_string, (void *) widget);
        break;
    }

    default:
        g_warn_if_reached ();
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
        GtkWidget * widget_left = nullptr, * widget_middle = nullptr, * widget_right = nullptr;

        switch (elements[i].type)
        {
            case PreferencesWidget::SpinButton:
                create_spin_button (& elements[i], & widget_left,
                 & widget_middle, & widget_right, domain);
                break;

            case PreferencesWidget::Label:
                create_label (& elements[i], & widget_middle, & widget_left, domain);
                break;

            case PreferencesWidget::FontButton:
                create_font_btn (& elements[i], & widget_left, & widget_middle, domain);
                break;

            case PreferencesWidget::Entry:
                create_entry (& elements[i], & widget_left, & widget_middle, domain);
                break;

            case PreferencesWidget::ComboBox:
                create_cbox (& elements[i], & widget_left, & widget_middle, domain);
                break;

            default:
                break;
        }

        if (widget_left)
            gtk_table_attach ((GtkTable *) grid, widget_left, 0, 1, i, i + 1,
             GTK_FILL, GTK_FILL, 0, 0);

        if (widget_middle)
            gtk_table_attach ((GtkTable *) grid, widget_middle, 1, 2, i, i + 1,
             GTK_FILL, GTK_FILL, 0, 0);

        if (widget_right)
            gtk_table_attach ((GtkTable *) grid, widget_right, 2, 3, i, i + 1,
             GTK_FILL, GTK_FILL, 0, 0);
    }
}

/* ALL WIDGETS */

/* box: a GtkBox */
void audgui_create_widgets_with_domain (GtkWidget * box,
 const PreferencesWidget * widgets, int n_widgets, const char * domain)
{
    GtkWidget * widget = nullptr, * child_box = nullptr;
    GSList * radio_btn_group = nullptr;

    for (int i = 0; i < n_widgets; i ++)
    {
        GtkWidget * label = nullptr;

        if (widget && widgets[i].child)
        {
            if (! child_box)
            {
                child_box = gtk_vbox_new (false, 0);
                g_object_set_data ((GObject *) widget, "child", child_box);

                GtkWidget * alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
                gtk_box_pack_start ((GtkBox *) box, alignment, false, false, 0);
                gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 0, 12, 0);
                gtk_container_add ((GtkContainer *) alignment, child_box);

                if (GTK_IS_TOGGLE_BUTTON (widget))
                    gtk_widget_set_sensitive (child_box,
                     gtk_toggle_button_get_active ((GtkToggleButton *) widget));
            }
        }
        else
            child_box = nullptr;

        GtkWidget * alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
        gtk_alignment_set_padding ((GtkAlignment *) alignment, 6, 0, 12, 0);
        gtk_box_pack_start ((GtkBox *) (child_box ? child_box : box), alignment, false, false, 0);

        widget = nullptr;

        if (radio_btn_group && widgets[i].type != PreferencesWidget::RadioButton)
            radio_btn_group = nullptr;

        switch (widgets[i].type)
        {
            case PreferencesWidget::CheckButton:
                widget = gtk_check_button_new_with_mnemonic (dgettext (domain, widgets[i].label));
                init_toggle_button (widget, & widgets[i]);
                break;

            case PreferencesWidget::Label:
            {
                if (strstr (widgets[i].label, "<b>"))
                    gtk_alignment_set_padding ((GtkAlignment *) alignment,
                     (i == 0) ? 0 : 12, 0, 0, 0);

                GtkWidget * icon = nullptr;
                create_label (& widgets[i], & label, & icon, domain);

                if (icon)
                {
                    widget = gtk_hbox_new (false, 6);
                    gtk_box_pack_start ((GtkBox *) widget, icon, false, false, 0);
                    gtk_box_pack_start ((GtkBox *) widget, label, false, false, 0);
                }
                else
                    widget = label;

                break;
            }

            case PreferencesWidget::RadioButton:
                widget = gtk_radio_button_new_with_mnemonic (radio_btn_group,
                 dgettext (domain, widgets[i].label));
                radio_btn_group = gtk_radio_button_get_group ((GtkRadioButton *) widget);
                init_radio_button (widget, & widgets[i]);
                break;

            case PreferencesWidget::SpinButton:
            {
                widget = gtk_hbox_new (false, 6);

                GtkWidget * label_pre = nullptr, * spin_btn = nullptr, * label_past = nullptr;
                create_spin_button (& widgets[i], & label_pre, & spin_btn, & label_past, domain);

                if (label_pre)
                    gtk_box_pack_start ((GtkBox *) widget, label_pre, false, false, 0);
                if (spin_btn)
                    gtk_box_pack_start ((GtkBox *) widget, spin_btn, false, false, 0);
                if (label_past)
                    gtk_box_pack_start ((GtkBox *) widget, label_past, false, false, 0);

                break;
            }

            case PreferencesWidget::Custom:
                if (widgets[i].data.populate)
                    widget = (GtkWidget *) widgets[i].data.populate ();

                break;

            case PreferencesWidget::FontButton:
            {
                widget = gtk_hbox_new (false, 6);

                GtkWidget * font_btn = nullptr;
                create_font_btn (& widgets[i], & label, & font_btn, domain);

                if (label)
                    gtk_box_pack_start ((GtkBox *) widget, label, false, false, 0);
                if (font_btn)
                    gtk_box_pack_start ((GtkBox *) widget, font_btn, false, false, 0);

                break;
            }

            case PreferencesWidget::Table:
                widget = gtk_table_new (0, 0, false);
                gtk_table_set_col_spacings ((GtkTable *) widget, 6);
                gtk_table_set_row_spacings ((GtkTable *) widget, 6);

                fill_grid (widget, widgets[i].data.table.elem, widgets[i].data.table.rows, domain);

                break;

            case PreferencesWidget::Entry:
            {
                widget = gtk_hbox_new (false, 6);

                GtkWidget * entry = nullptr;
                create_entry (& widgets[i], & label, & entry, domain);

                if (label)
                    gtk_box_pack_start ((GtkBox *) widget, label, false, false, 0);
                if (entry)
                    gtk_box_pack_start ((GtkBox *) widget, entry, false, false, 0);

                break;
            }

            case PreferencesWidget::ComboBox:
            {
                widget = gtk_hbox_new (false, 6);

                GtkWidget * combo = nullptr;
                create_cbox (& widgets[i], & label, & combo, domain);

                if (label)
                    gtk_box_pack_start ((GtkBox *) widget, label, false, false, 0);
                if (combo)
                    gtk_box_pack_start ((GtkBox *) widget, combo, false, false, 0);

                break;
            }

            case PreferencesWidget::Box:
                if (widgets[i].data.box.horizontal)
                    widget = gtk_hbox_new (false, 6);
                else
                    widget = gtk_vbox_new (false, 0);

                audgui_create_widgets_with_domain (widget,
                 widgets[i].data.box.elem, widgets[i].data.box.n_elem, domain);

                if (widgets[i].data.box.frame)
                {
                    GtkWidget * frame = gtk_frame_new (dgettext (domain, widgets[i].label));
                    gtk_container_add ((GtkContainer *) frame, widget);
                    widget = frame;
                }

                break;

            case PreferencesWidget::Notebook:
                gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 0, 0, 0);

                widget = gtk_notebook_new ();

                for (int j = 0; j < widgets[i].data.notebook.n_tabs; j ++)
                {
                    GtkWidget * vbox = gtk_vbox_new (false, 0);
                    gtk_container_set_border_width ((GtkContainer *) vbox, 6);

                    audgui_create_widgets_with_domain (vbox,
                     widgets[i].data.notebook.tabs[j].widgets,
                     widgets[i].data.notebook.tabs[j].n_widgets, domain);

                    gtk_notebook_append_page ((GtkNotebook *) widget, vbox,
                     gtk_label_new (dgettext (domain,
                     widgets[i].data.notebook.tabs[j].name)));
                }

                break;

            case PreferencesWidget::Separator:
                gtk_alignment_set_padding ((GtkAlignment *) alignment, 6, 6, 0, 0);

                widget = widgets[i].data.separator.horizontal ?
                 gtk_hseparator_new () : gtk_vseparator_new ();
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

            if (widgets[i].tooltip && widgets[i].type != PreferencesWidget::SpinButton)
                gtk_widget_set_tooltip_text (widget, dgettext (domain,
                 widgets[i].tooltip));
        }
    }
}
