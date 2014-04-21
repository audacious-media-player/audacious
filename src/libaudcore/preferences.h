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

typedef enum {
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
} WidgetType;

typedef enum {
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_BOOLEAN,
    VALUE_STRING,
    VALUE_NULL,
} ValueType;

typedef struct {
    const void * value;
    const char * label;
} ComboBoxElements;

struct _NotebookTab;

struct WidgetVRadio {
    int value;
};

struct WidgetVSpin {
    double min, max, step;
    const char * right_label; /* text right to widget */
};

struct WidgetVTable {
    const struct _PreferencesWidget * elem;
    int rows;
};

struct WidgetVLabel {
    const char * stock_id;
    bool_t single_line; /* FALSE to enable line wrap */
};

struct WidgetVFonts {
    const char * title;
};

struct WidgetVEntry {
    bool_t password;
};

struct WidgetVCombo {
    /* static init */
    const ComboBoxElements * elements;
    int n_elements;

    /* runtime init */
    const ComboBoxElements * (* fill) (int * n_elements);
};

struct WidgetVBox {
    const struct _PreferencesWidget * elem;
    int n_elem;

    bool_t horizontal;  /* FALSE gives vertical, TRUE gives horizontal aligment of child widgets */
    bool_t frame;       /* whether to draw frame around box */
};

struct WidgetVNotebook {
    const struct _NotebookTab * tabs;
    int n_tabs;
};

struct WidgetVSeparator {
    bool_t horizontal;  /* FALSE gives vertical, TRUE gives horizontal separator */
};

typedef union _WidgetVariant {
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

#ifdef __cplusplus
    constexpr _WidgetVariant (WidgetVRadio radio) : radio_btn (radio) {}
    constexpr _WidgetVariant (WidgetVSpin spin) : spin_btn (spin) {}
    constexpr _WidgetVariant (WidgetVTable table) : table (table) {}
    constexpr _WidgetVariant (WidgetVLabel label) : label (label) {}
    constexpr _WidgetVariant (WidgetVFonts fonts) : font_btn (fonts) {}
    constexpr _WidgetVariant (WidgetVEntry entry) : entry (entry) {}
    constexpr _WidgetVariant (WidgetVCombo combo) : combo (combo) {}
    constexpr _WidgetVariant (WidgetVBox box) : box (box) {}
    constexpr _WidgetVariant (WidgetVNotebook notebook) : notebook (notebook) {}
    constexpr _WidgetVariant (WidgetVSeparator separator) : separator (separator) {}

    /* also serves as default constructor */
    constexpr _WidgetVariant (void * (* populate) (void) = 0) : populate (populate) {}
#endif
} WidgetVariant;

struct _PreferencesWidget {
    WidgetType type;          /* widget type */
    const char * label;       /* widget title (for SPIN_BTN it's text left to widget) */
    void * cfg;               /* connected config value */
    void (* callback) (void); /* this func will be called after value change, can be NULL */
    const char * tooltip;     /* widget tooltip, can be NULL */
    bool_t child;
    ValueType cfg_type;       /* connected value type */
    const char * csect;       /* config file section */
    const char * cname;       /* config file key name */

    WidgetVariant data;
};

#ifdef __cplusplus

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

constexpr PreferencesWidget WidgetLabel (const char * label, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_LABEL, label, 0, 0, 0, (child == WIDGET_CHILD)}; }

constexpr PreferencesWidget WidgetCheck (const char * label, WidgetConfig cfg,
 WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_CHK_BTN, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name}; }

constexpr PreferencesWidget WidgetRadio (const char * label, WidgetConfig cfg,
 WidgetVRadio radio, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_RADIO_BTN, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name, radio}; }

constexpr PreferencesWidget WidgetSpin (const char * label, WidgetConfig cfg,
 WidgetVSpin spin, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_SPIN_BTN, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name, spin}; }

constexpr PreferencesWidget WidgetEntry (const char * label, WidgetConfig cfg,
 WidgetVEntry entry = WidgetVEntry(), WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_ENTRY, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name, entry}; }

constexpr PreferencesWidget WidgetCombo (const char * label, WidgetConfig cfg,
 WidgetVCombo combo, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_COMBO_BOX, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name, combo}; }

constexpr PreferencesWidget WidgetFonts (const char * label, WidgetConfig cfg,
 WidgetVFonts fonts, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_FONT_BTN, label, cfg.value, cfg.callback, 0,
       (child == WIDGET_CHILD), cfg.type, cfg.section, cfg.name, fonts}; }

constexpr PreferencesWidget WidgetBox (WidgetVBox box, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_BOX, 0, 0, 0, 0, (child == WIDGET_CHILD), VALUE_NULL, 0, 0, box}; }

constexpr PreferencesWidget WidgetTable (WidgetVTable table, WidgetIsChild child = WIDGET_NOT_CHILD)
    { return {WIDGET_TABLE, 0, 0, 0, 0, (child == WIDGET_CHILD), VALUE_NULL, 0, 0, table}; }

constexpr PreferencesWidget WidgetNotebook (WidgetVNotebook notebook)
    { return {WIDGET_NOTEBOOK, 0, 0, 0, 0, 0, VALUE_NULL, 0, 0, notebook}; }

constexpr PreferencesWidget WidgetSeparator (WidgetVSeparator separator = WidgetVSeparator ())
    { return {WIDGET_SEPARATOR, 0, 0, 0, 0, 0, VALUE_NULL, 0, 0, separator}; }

constexpr PreferencesWidget WidgetCustom (void * (* populate) (void))
    { return {WIDGET_CUSTOM, 0, 0, 0, 0, 0, VALUE_NULL, 0, 0, populate}; }

#endif

typedef struct _NotebookTab {
    const char * name;
    const PreferencesWidget * widgets;
    int n_widgets;
} NotebookTab;

struct _PluginPreferences {
    const PreferencesWidget * widgets;
    int n_widgets;

    void (* init) (void);
    void (* apply) (void);
    void (* cleanup) (void);
};

#endif /* LIBAUDCORE_PREFERENCES_H */
