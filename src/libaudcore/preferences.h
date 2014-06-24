/*
 * preferences.h
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

#ifndef LIBAUDCORE_PREFERENCES_H
#define LIBAUDCORE_PREFERENCES_H

#include <libaudcore/core.h>

enum WidgetType {
    WIDGET_NONE,
    WIDGET_CHK_BTN,
    WIDGET_LABEL,
    WIDGET_RADIO_BTN,
    WIDGET_SPIN_BTN,
    WIDGET_CUSTOM,           /* 'custom' widget, you hand back the widget you want to add --nenolod */
    WIDGET_FONT_BTN,
    WIDGET_TABLE,
    WIDGET_ENTRY,
    WIDGET_COMBO_BOX,
    WIDGET_BOX,
    WIDGET_NOTEBOOK,
    WIDGET_SEPARATOR,
};

enum ValueType {
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_BOOLEAN,
    VALUE_STRING,
    VALUE_NULL,
};

struct ComboBoxElements {
    const void * value;
    const char * label;
};

struct WidgetVRadio {
    int value;
};

struct WidgetVSpin {
    double min, max, step;
    const char * right_label; /* text right to widget */
};

struct WidgetVTable {
    const PreferencesWidget * elem;
    int rows;
};

struct WidgetVLabel {
    const char * stock_id;
    bool single_line; /* false to enable line wrap */
};

struct WidgetVFonts {
    const char * title;
};

struct WidgetVEntry {
    bool password;
};

struct WidgetVCombo {
    /* static init */
    const ComboBoxElements * elements;
    int n_elements;

    /* runtime init */
    const ComboBoxElements * (* fill) (int * n_elements);
};

struct WidgetVBox {
    const PreferencesWidget * elem;
    int n_elem;

    bool horizontal;  /* false gives vertical, true gives horizontal aligment of child widgets */
    bool frame;       /* whether to draw frame around box */
};

struct NotebookTab {
    const char * name;
    const PreferencesWidget * widgets;
    int n_widgets;
};

struct WidgetVNotebook {
    const NotebookTab * tabs;
    int n_tabs;
};

struct WidgetVSeparator {
    bool horizontal;  /* false gives vertical, true gives horizontal separator */
};

union WidgetVariant {
    struct WidgetVRadio radio_btn;
    struct WidgetVSpin spin_btn;
    struct WidgetVTable table;
    struct WidgetVLabel label;
    struct WidgetVFonts font_btn;
    struct WidgetVEntry entry;
    struct WidgetVCombo combo;
    struct WidgetVBox box;
    struct WidgetVNotebook notebook;
    struct WidgetVSeparator separator;

    /* for WIDGET_CUSTOM */
    /* GtkWidget * (* populate) (void); */
    void * (* populate) (void);

    constexpr WidgetVariant (WidgetVRadio radio) : radio_btn (radio) {}
    constexpr WidgetVariant (WidgetVSpin spin) : spin_btn (spin) {}
    constexpr WidgetVariant (WidgetVTable table) : table (table) {}
    constexpr WidgetVariant (WidgetVLabel label) : label (label) {}
    constexpr WidgetVariant (WidgetVFonts fonts) : font_btn (fonts) {}
    constexpr WidgetVariant (WidgetVEntry entry) : entry (entry) {}
    constexpr WidgetVariant (WidgetVCombo combo) : combo (combo) {}
    constexpr WidgetVariant (WidgetVBox box) : box (box) {}
    constexpr WidgetVariant (WidgetVNotebook notebook) : notebook (notebook) {}
    constexpr WidgetVariant (WidgetVSeparator separator) : separator (separator) {}

    /* also serves as default constructor */
    constexpr WidgetVariant (void * (* populate) (void) = 0) : populate (populate) {}
};

struct PreferencesWidget {
    WidgetType type;          /* widget type */
    const char * label;       /* widget title (for SPIN_BTN it's text left to widget) */
    void * cfg;               /* connected config value */
    void (* callback) (void); /* this func will be called after value change, can be nullptr */
    const char * tooltip;     /* widget tooltip, can be nullptr */
    bool child;
    ValueType cfg_type;       /* connected value type */
    const char * csect;       /* config file section */
    const char * cname;       /* config file key name */

    WidgetVariant data;
};

struct WidgetConfig {
    ValueType type;
    void * value;
    const char * section, * name;
    void (* callback) (void);
};

enum WidgetIsChild {
    WIDGET_NOT_CHILD,
    WIDGET_CHILD
};

constexpr const PreferencesWidget WidgetLabel (const char * label, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_LABEL, label, 0, 0, 0, (child == WIDGET_CHILD)}; }

constexpr const PreferencesWidget WidgetCheck (const char * label, WidgetConfig cfg,
 WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_CHK_BTN, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name}; }

constexpr const PreferencesWidget WidgetRadio (const char * label, WidgetConfig cfg,
 WidgetVRadio radio, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_RADIO_BTN, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name, radio}; }

constexpr const PreferencesWidget WidgetSpin (const char * label, WidgetConfig cfg,
 WidgetVSpin spin, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_SPIN_BTN, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name, spin}; }

constexpr const PreferencesWidget WidgetEntry (const char * label, WidgetConfig cfg,
 WidgetVEntry entry = WidgetVEntry(), WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_ENTRY, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name, entry}; }

constexpr const PreferencesWidget WidgetCombo (const char * label, WidgetConfig cfg,
 WidgetVCombo combo, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_COMBO_BOX, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name, combo}; }

constexpr const PreferencesWidget WidgetFonts (const char * label, WidgetConfig cfg,
 WidgetVFonts fonts, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_FONT_BTN, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name, fonts}; }

constexpr const PreferencesWidget WidgetBox (WidgetVBox box, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_BOX, 0, 0, 0, 0, (child == WIDGET_CHILD), VALUE_NULL, 0, 0, box}; }

constexpr const PreferencesWidget WidgetTable (WidgetVTable table, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_TABLE, 0, 0, 0, 0, (child == WIDGET_CHILD), VALUE_NULL, 0, 0, table}; }

constexpr const PreferencesWidget WidgetNotebook (WidgetVNotebook notebook)
    { return {WIDGET_NOTEBOOK, 0, 0, 0, 0, 0, VALUE_NULL, 0, 0, notebook}; }

constexpr const PreferencesWidget WidgetSeparator (WidgetVSeparator separator = WidgetVSeparator ())
    { return {WIDGET_SEPARATOR, 0, 0, 0, 0, 0, VALUE_NULL, 0, 0, separator}; }

constexpr const PreferencesWidget WidgetCustom (void * (* populate) (void))
    { return {WIDGET_CUSTOM, 0, 0, 0, 0, 0, VALUE_NULL, 0, 0, populate}; }

struct PluginPreferences {
    const PreferencesWidget * widgets;
    int n_widgets;

    void (* init) (void);
    void (* apply) (void);
    void (* cleanup) (void);
};

#endif /* LIBAUDCORE_PREFERENCES_H */
