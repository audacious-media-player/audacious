/*
 * prefs-widget.c
 * Copyright 2007-2014 Tomasz Moń, William Pitcock, and John Lindgren
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

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
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

static void widget_changed (GtkWidget * widget, const PreferencesWidget * w)
{
    switch (w->type)
    {
    case PreferencesWidget::CheckButton:
    {
        gboolean set = gtk_toggle_button_get_active ((GtkToggleButton *) widget);
        widget_set_bool (w, set);

        auto child = (GtkWidget *) g_object_get_data ((GObject *) widget, "child");
        if (child)
            gtk_widget_set_sensitive (child, set);

        break;
    }

    case PreferencesWidget::RadioButton:
        if (gtk_toggle_button_get_active ((GtkToggleButton *) widget))
            widget_set_int (w, w->data.radio_btn.value);

        break;

    case PreferencesWidget::SpinButton:
        if (w->cfg.type == WidgetConfig::Int)
            widget_set_int (w, gtk_spin_button_get_value_as_int ((GtkSpinButton *) widget));
        else if (w->cfg.type == WidgetConfig::Float)
            widget_set_double (w, gtk_spin_button_get_value ((GtkSpinButton *) widget));

        break;

    case PreferencesWidget::FontButton:
        widget_set_string (w, gtk_font_button_get_font_name ((GtkFontButton *) widget));
        break;

    case PreferencesWidget::Entry:
        widget_set_string (w, gtk_entry_get_text ((GtkEntry *) widget));
        break;

    case PreferencesWidget::ComboBox:
    {
        auto items = (const ComboItem *) g_object_get_data ((GObject *) widget, "comboitems");
        int idx = gtk_combo_box_get_active ((GtkComboBox *) widget);

        if (w->cfg.type == WidgetConfig::Int)
            widget_set_int (w, items[idx].num);
        else if (w->cfg.type == WidgetConfig::String)
            widget_set_string (w, items[idx].str);

        break;
    }

    default:
        break;
    }
}

static void combobox_update (GtkWidget * combobox, const PreferencesWidget * widget);

static void widget_update (void * unused, void * widget)
{
    auto w = (const PreferencesWidget *) g_object_get_data ((GObject *) widget, "prefswidget");

    g_signal_handlers_block_by_func (widget, (void *) widget_changed, (void *) w);

    switch (w->type)
    {
    case PreferencesWidget::CheckButton:
        gtk_toggle_button_set_active ((GtkToggleButton *) widget, widget_get_bool (w));
        break;

    case PreferencesWidget::RadioButton:
        if (widget_get_int (w) == w->data.radio_btn.value)
            gtk_toggle_button_set_active ((GtkToggleButton *) widget, true);

        break;

    case PreferencesWidget::SpinButton:
        if (w->cfg.type == WidgetConfig::Int)
            gtk_spin_button_set_value ((GtkSpinButton *) widget, widget_get_int (w));
        else if (w->cfg.type == WidgetConfig::Float)
            gtk_spin_button_set_value ((GtkSpinButton *) widget, widget_get_double (w));

        break;

    case PreferencesWidget::FontButton:
        gtk_font_button_set_font_name ((GtkFontButton *) widget, widget_get_string (w));
        break;

    case PreferencesWidget::Entry:
        gtk_entry_set_text ((GtkEntry *) widget, widget_get_string (w));
        break;

    case PreferencesWidget::ComboBox:
        combobox_update ((GtkWidget *) widget, w);
        break;

    default:
        break;
    }

    g_signal_handlers_unblock_by_func (widget, (void *) widget_changed, (void *) w);
}

static void widget_unhook (GtkWidget * widget, const PreferencesWidget * w)
{
    hook_dissociate_full (w->cfg.hook, widget_update, widget);
}

static void widget_init (GtkWidget * widget, const PreferencesWidget * w)
{
    g_object_set_data ((GObject *) widget, "prefswidget", (void *) w);

    widget_update (nullptr, widget);

    switch (w->type)
    {
    case PreferencesWidget::CheckButton:
    case PreferencesWidget::RadioButton:
        g_signal_connect (widget, "toggled", (GCallback) widget_changed, (void *) w);
        break;

    case PreferencesWidget::SpinButton:
        g_signal_connect (widget, "value_changed", (GCallback) widget_changed, (void *) w);
        break;

    case PreferencesWidget::FontButton:
        g_signal_connect (widget, "font_set", (GCallback) widget_changed, (void *) w);
        break;

    case PreferencesWidget::Entry:
    case PreferencesWidget::ComboBox:
        g_signal_connect (widget, "changed", (GCallback) widget_changed, (void *) w);
        break;

    default:
        break;
    }

    if (w->cfg.hook)
    {
        hook_associate (w->cfg.hook, widget_update, widget);
        g_signal_connect (widget, "destroy", (GCallback) widget_unhook, (void *) w);
    }
}

/* WIDGET_LABEL */

static void create_label (const PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * icon, const char * domain)
{
    * label = gtk_label_new_with_mnemonic (dgettext (domain, widget->label));
    gtk_label_set_use_markup ((GtkLabel *) * label, true);
    gtk_label_set_line_wrap ((GtkLabel *) * label, true);
    gtk_misc_set_alignment ((GtkMisc *) * label, 0, 0.5);
}

/* WIDGET_SPIN_BTN */

static void create_spin_button (const PreferencesWidget * widget,
 GtkWidget * * label_pre, GtkWidget * * spin_btn, GtkWidget * * label_past,
 const char * domain)
{
    * label_pre = gtk_label_new (dgettext (domain, widget->label));
    * spin_btn = gtk_spin_button_new_with_range (widget->data.spin_btn.min,
     widget->data.spin_btn.max, widget->data.spin_btn.step);

    if (widget->data.spin_btn.right_label)
        * label_past = gtk_label_new (dgettext (domain, widget->data.spin_btn.right_label));

    widget_init (* spin_btn, widget);
}

/* WIDGET_FONT_BTN */

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

    widget_init (* font_btn, widget);
}

/* WIDGET_ENTRY */

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

    widget_init (* entry, widget);
}

/* WIDGET_COMBO_BOX */

static void combobox_update (GtkWidget * combobox, const PreferencesWidget * widget)
{
    auto domain = (const char *) g_object_get_data ((GObject *) combobox, "combodomain");

    ArrayRef<const ComboItem> items = widget->data.combo.elems;
    if (widget->data.combo.fill)
        items = widget->data.combo.fill ();

    g_object_set_data ((GObject *) combobox, "comboitems", (void *) items.data);

    /* no gtk_combo_box_text_clear()? */
    gtk_list_store_clear ((GtkListStore *) gtk_combo_box_get_model ((GtkComboBox *) combobox));

    for (const ComboItem & item : items)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) combobox,
         dgettext (domain, item.label));

    if (widget->cfg.type == WidgetConfig::Int)
    {
        int num = widget_get_int (widget);

        for (int i = 0; i < items.len; i ++)
        {
            if (items.data[i].num == num)
            {
                gtk_combo_box_set_active ((GtkComboBox *) combobox, i);
                break;
            }
        }
    }
    else if (widget->cfg.type == WidgetConfig::String)
    {
        String str = widget_get_string (widget);

        for (int i = 0; i < items.len; i ++)
        {
            if (! strcmp_safe (items.data[i].str, str))
            {
                gtk_combo_box_set_active ((GtkComboBox *) combobox, i);
                break;
            }
        }
    }
}

static void create_cbox (const PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * combobox, const char * domain)
{
    * combobox = gtk_combo_box_text_new ();

    if (widget->label)
        * label = gtk_label_new (dgettext (domain, widget->label));

    g_object_set_data ((GObject *) * combobox, "combodomain", (void *) domain);
    widget_init (* combobox, widget);
}

/* WIDGET_TABLE */

static void fill_grid (GtkWidget * grid,
 ArrayRef<const PreferencesWidget> widgets, const char * domain)
{
    for (const PreferencesWidget & w : widgets)
    {
        GtkWidget * widget_left = nullptr, * widget_middle = nullptr, * widget_right = nullptr;

        switch (w.type)
        {
            case PreferencesWidget::SpinButton:
                create_spin_button (& w, & widget_left,
                 & widget_middle, & widget_right, domain);
                break;

            case PreferencesWidget::Label:
                create_label (& w, & widget_middle, & widget_left, domain);
                break;

            case PreferencesWidget::FontButton:
                create_font_btn (& w, & widget_left, & widget_middle, domain);
                break;

            case PreferencesWidget::Entry:
                create_entry (& w, & widget_left, & widget_middle, domain);
                break;

            case PreferencesWidget::ComboBox:
                create_cbox (& w, & widget_left, & widget_middle, domain);
                break;

            default:
                break;
        }

        int i = & w - widgets.data;

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
 ArrayRef<const PreferencesWidget> widgets, const char * domain)
{
    GtkWidget * widget = nullptr, * child_box = nullptr;
    GSList * radio_btn_group = nullptr;

    for (const PreferencesWidget & w : widgets)
    {
        GtkWidget * label = nullptr;

        if (widget && w.child)
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

        if (radio_btn_group && w.type != PreferencesWidget::RadioButton)
            radio_btn_group = nullptr;

        switch (w.type)
        {
            case PreferencesWidget::CheckButton:
                widget = gtk_check_button_new_with_mnemonic (dgettext (domain, w.label));
                widget_init (widget, & w);
                break;

            case PreferencesWidget::Label:
            {
                if (strstr (w.label, "<b>"))
                    gtk_alignment_set_padding ((GtkAlignment *) alignment,
                     (& w == widgets.data) ? 0 : 12, 0, 0, 0);

                GtkWidget * icon = nullptr;
                create_label (& w, & label, & icon, domain);

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
                 dgettext (domain, w.label));
                radio_btn_group = gtk_radio_button_get_group ((GtkRadioButton *) widget);
                widget_init (widget, & w);
                break;

            case PreferencesWidget::SpinButton:
            {
                widget = gtk_hbox_new (false, 6);

                GtkWidget * label_pre = nullptr, * spin_btn = nullptr, * label_past = nullptr;
                create_spin_button (& w, & label_pre, & spin_btn, & label_past, domain);

                if (label_pre)
                    gtk_box_pack_start ((GtkBox *) widget, label_pre, false, false, 0);
                if (spin_btn)
                    gtk_box_pack_start ((GtkBox *) widget, spin_btn, false, false, 0);
                if (label_past)
                    gtk_box_pack_start ((GtkBox *) widget, label_past, false, false, 0);

                break;
            }

            case PreferencesWidget::CustomGTK:
                if (w.data.populate)
                    widget = (GtkWidget *) w.data.populate ();

                break;

            case PreferencesWidget::FontButton:
            {
                widget = gtk_hbox_new (false, 6);

                GtkWidget * font_btn = nullptr;
                create_font_btn (& w, & label, & font_btn, domain);

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

                fill_grid (widget, w.data.table.widgets, domain);

                break;

            case PreferencesWidget::Entry:
            {
                widget = gtk_hbox_new (false, 6);

                GtkWidget * entry = nullptr;
                create_entry (& w, & label, & entry, domain);

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
                create_cbox (& w, & label, & combo, domain);

                if (label)
                    gtk_box_pack_start ((GtkBox *) widget, label, false, false, 0);
                if (combo)
                    gtk_box_pack_start ((GtkBox *) widget, combo, false, false, 0);

                break;
            }

            case PreferencesWidget::Box:
                if (w.data.box.horizontal)
                    widget = gtk_hbox_new (false, 6);
                else
                    widget = gtk_vbox_new (false, 0);

                audgui_create_widgets_with_domain (widget, w.data.box.widgets, domain);

                if (w.data.box.frame)
                {
                    GtkWidget * frame = gtk_frame_new (dgettext (domain, w.label));
                    gtk_container_add ((GtkContainer *) frame, widget);
                    widget = frame;
                }

                break;

            case PreferencesWidget::Notebook:
                gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 0, 0, 0);

                widget = gtk_notebook_new ();

                for (const NotebookTab & tab : w.data.notebook.tabs)
                {
                    GtkWidget * vbox = gtk_vbox_new (false, 0);
                    gtk_container_set_border_width ((GtkContainer *) vbox, 6);

                    audgui_create_widgets_with_domain (vbox, tab.widgets, domain);

                    gtk_notebook_append_page ((GtkNotebook *) widget, vbox,
                     gtk_label_new (dgettext (domain, tab.name)));
                }

                break;

            case PreferencesWidget::Separator:
                gtk_alignment_set_padding ((GtkAlignment *) alignment, 6, 6, 0, 0);

                widget = w.data.separator.horizontal ?
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
        }
    }
}
