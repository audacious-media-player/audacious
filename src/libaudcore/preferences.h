/*
 * preferences.h
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

#ifndef LIBAUDCORE_PREFERENCES_H
#define LIBAUDCORE_PREFERENCES_H

#include <libaudcore/objects.h>

struct PreferencesWidget;

struct ComboItem {
    const char * label;
    const char * str;
    int num;

    constexpr ComboItem (const char * label, const char * str) :
        label (label),
        str (str),
        num (-1) {}

    constexpr ComboItem (const char * label, int num) :
        label (label),
        str (nullptr),
        num (num) {}
};

struct WidgetVRadio {
    int value;
};

struct WidgetVSpin {
    double min, max, step;
    const char * right_label; /* text right to widget */
};

struct WidgetVTable {
    ArrayRef<const PreferencesWidget> widgets;
};

struct WidgetVFonts {
    const char * title;
};

struct WidgetVEntry {
    bool password;
};

struct WidgetVCombo {
    /* static init */
    ArrayRef<const ComboItem> elems;

    /* runtime init */
    ArrayRef<const ComboItem> (* fill) ();
};

struct WidgetVBox {
    ArrayRef<const PreferencesWidget> widgets;

    bool horizontal;  /* false gives vertical, true gives horizontal aligment of child widgets */
    bool frame;       /* whether to draw frame around box */
};

struct NotebookTab {
    const char * name;
    ArrayRef<const PreferencesWidget> widgets;
};

struct WidgetVNotebook {
    ArrayRef<const NotebookTab> tabs;
};

struct WidgetVSeparator {
    bool horizontal;  /* false gives vertical, true gives horizontal separator */
};

union WidgetVariant {
    struct WidgetVRadio radio_btn;
    struct WidgetVSpin spin_btn;
    struct WidgetVTable table;
    struct WidgetVFonts font_btn;
    struct WidgetVEntry entry;
    struct WidgetVCombo combo;
    struct WidgetVBox box;
    struct WidgetVNotebook notebook;
    struct WidgetVSeparator separator;

    /* for custom GTK or Qt widgets */
    /* GtkWidget * (* populate) (void); */
    /* QWidget * (* populate) (void); */
    void * (* populate) (void);

    constexpr WidgetVariant (WidgetVRadio radio) : radio_btn (radio) {}
    constexpr WidgetVariant (WidgetVSpin spin) : spin_btn (spin) {}
    constexpr WidgetVariant (WidgetVTable table) : table (table) {}
    constexpr WidgetVariant (WidgetVFonts fonts) : font_btn (fonts) {}
    constexpr WidgetVariant (WidgetVEntry entry) : entry (entry) {}
    constexpr WidgetVariant (WidgetVCombo combo) : combo (combo) {}
    constexpr WidgetVariant (WidgetVBox box) : box (box) {}
    constexpr WidgetVariant (WidgetVNotebook notebook) : notebook (notebook) {}
    constexpr WidgetVariant (WidgetVSeparator separator) : separator (separator) {}

    /* also serves as default constructor */
    constexpr WidgetVariant (void * (* populate) (void) = 0) : populate (populate) {}
};

struct WidgetConfig
{
    enum Type {
        None,
        Bool,
        Int,
        Float,
        String
    };

    Type type;

    /* pointer to immediate value */
    void * value;
    /* identifier for configuration value */
    const char * section, * name;
    /* called when value is changed  */
    void (* callback) (void);
    /* widget updates when this hook is called */
    const char * hook;

    constexpr WidgetConfig () :
        type (None),
        value (nullptr),
        section (nullptr),
        name (nullptr),
        callback (nullptr),
        hook (nullptr) {}

    constexpr WidgetConfig (Type type, void * value, const char * section,
     const char * name, void (* callback) (void), const char * hook) :
        type (type),
        value (value),
        section (section),
        name (name),
        callback (callback),
        hook (hook) {}
};

constexpr WidgetConfig WidgetBool (bool & value,
 void (* callback) (void) = nullptr, const char * hook = nullptr)
    { return WidgetConfig (WidgetConfig::Bool, (void *) & value, 0, 0, callback, hook); }
constexpr WidgetConfig WidgetInt (int & value,
 void (* callback) (void) = nullptr, const char * hook = nullptr)
    { return WidgetConfig (WidgetConfig::Int, (void *) & value, 0, 0, callback, hook); }
constexpr WidgetConfig WidgetFloat (double & value,
 void (* callback) (void) = nullptr, const char * hook = nullptr)
    { return WidgetConfig (WidgetConfig::Float, (void *) & value, 0, 0, callback, hook); }
constexpr WidgetConfig WidgetString (String & value,
 void (* callback) (void) = nullptr, const char * hook = nullptr)
    { return WidgetConfig (WidgetConfig::String, (void *) & value, 0, 0, callback, hook); }

constexpr WidgetConfig WidgetBool (const char * section, const char * name,
 void (* callback) (void) = nullptr, const char * hook = nullptr)
    { return WidgetConfig (WidgetConfig::Bool, 0, section, name, callback, hook); }
constexpr WidgetConfig WidgetInt (const char * section, const char * name,
 void (* callback) (void) = nullptr, const char * hook = nullptr)
    { return WidgetConfig (WidgetConfig::Int, 0, section, name, callback, hook); }
constexpr WidgetConfig WidgetFloat (const char * section, const char * name,
 void (* callback) (void) = nullptr, const char * hook = nullptr)
    { return WidgetConfig (WidgetConfig::Float, 0, section, name, callback, hook); }
constexpr WidgetConfig WidgetString (const char * section, const char * name,
 void (* callback) (void) = nullptr, const char * hook = nullptr)
    { return WidgetConfig (WidgetConfig::String, 0, section, name, callback, hook); }

struct PreferencesWidget
{
    enum Type {
        Label,
        CheckButton,
        RadioButton,
        SpinButton,
        Entry,
        ComboBox,
        FontButton,
        Box,
        Table,
        Notebook,
        Separator,
        CustomGTK,
        CustomQt
    };

    Type type;
    const char * label;       /* widget title (for SPIN_BTN it's text left to widget) */
    bool child;

    WidgetConfig cfg;
    WidgetVariant data;
};

enum WidgetIsChild {
    WIDGET_NOT_CHILD,
    WIDGET_CHILD
};

constexpr PreferencesWidget WidgetLabel (const char * label,
 WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {PreferencesWidget::Label, label, (child == WIDGET_CHILD)}; }

constexpr PreferencesWidget WidgetCheck (const char * label,
 WidgetConfig cfg, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {PreferencesWidget::CheckButton, label,
       (child == WIDGET_CHILD), cfg}; }

constexpr PreferencesWidget WidgetRadio (const char * label,
 WidgetConfig cfg, WidgetVRadio radio, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {PreferencesWidget::RadioButton, label,
       (child == WIDGET_CHILD), cfg, radio}; }

constexpr PreferencesWidget WidgetSpin (const char * label,
 WidgetConfig cfg, WidgetVSpin spin, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {PreferencesWidget::SpinButton, label,
       (child == WIDGET_CHILD), cfg, spin}; }

constexpr PreferencesWidget WidgetEntry (const char * label,
 WidgetConfig cfg, WidgetVEntry entry = WidgetVEntry(),
 WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {PreferencesWidget::Entry, label,
       (child == WIDGET_CHILD), cfg, entry}; }

constexpr PreferencesWidget WidgetCombo (const char * label,
 WidgetConfig cfg, WidgetVCombo combo, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {PreferencesWidget::ComboBox, label,
       (child == WIDGET_CHILD), cfg, combo}; }

constexpr PreferencesWidget WidgetFonts (const char * label,
 WidgetConfig cfg, WidgetVFonts fonts, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {PreferencesWidget::FontButton, label,
       (child == WIDGET_CHILD), cfg, fonts}; }

constexpr PreferencesWidget WidgetBox (WidgetVBox box, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {PreferencesWidget::Box, 0, (child == WIDGET_CHILD), WidgetConfig (), box}; }

constexpr PreferencesWidget WidgetTable (WidgetVTable table,
 WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {PreferencesWidget::Table, 0, (child == WIDGET_CHILD), WidgetConfig (), table}; }

constexpr PreferencesWidget WidgetNotebook (WidgetVNotebook notebook)
    { return {PreferencesWidget::Notebook, 0, 0, WidgetConfig (), notebook}; }

constexpr PreferencesWidget WidgetSeparator (WidgetVSeparator separator = WidgetVSeparator ())
    { return {PreferencesWidget::Separator, 0, 0, WidgetConfig (), separator}; }

constexpr PreferencesWidget WidgetCustomGTK (void * (* populate) (void))
    { return {PreferencesWidget::CustomGTK, 0, 0, WidgetConfig (), populate}; }

constexpr PreferencesWidget WidgetCustomQt (void * (* populate) (void))
    { return {PreferencesWidget::CustomQt, 0, 0, WidgetConfig (), populate}; }

struct PluginPreferences {
    ArrayRef<const PreferencesWidget> widgets;

    void (* init) (void);
    void (* apply) (void);
    void (* cleanup) (void);
};

#endif /* LIBAUDCORE_PREFERENCES_H */
