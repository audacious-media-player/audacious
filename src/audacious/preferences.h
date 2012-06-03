/*
 * preferences.h
 * Copyright 2007-2011 Tomasz Moń, William Pitcock, and John Lindgren
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#ifndef AUDACIOUS_PREFERENCES_H
#define AUDACIOUS_PREFERENCES_H

#include <audacious/types.h>

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
    void * value;
    const char * label;
} ComboBoxElements;

struct _NotebookTab;

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

    union {
        struct {
            int value;
        } radio_btn;

        struct {
            double min, max, step;
            const char * right_label; /* text right to widget */
        } spin_btn;

        struct {
            struct _PreferencesWidget *elem;
            int rows;
        } table;

        struct {
            const char * stock_id;
            bool_t single_line; /* FALSE to enable line wrap */
        } label;

        struct {
            const char * title;
        } font_btn;

        struct {
            bool_t password;
        } entry;

        struct {
            const ComboBoxElements * elements;
            int n_elements;
        } combo;

        struct {
            const struct _PreferencesWidget * elem;
            int n_elem;

            bool_t horizontal;  /* FALSE gives vertical, TRUE gives horizontal aligment of child widgets */
            bool_t frame;       /* whether to draw frame around box */
        } box;

        struct {
            const struct _NotebookTab * tabs;
            int n_tabs;
        } notebook;

        struct {
            bool_t horizontal; /* FALSE gives vertical, TRUE gives horizontal separator */
        } separator;

        /* for WIDGET_CUSTOM --nenolod */
        /* GtkWidget * (* populate) (void); */
        void * (* populate) (void);
    } data;
};

typedef struct _NotebookTab {
    const char * name;
    const PreferencesWidget * widgets;
    int n_widgets;
} NotebookTab;

struct _PluginPreferences {
    const PreferencesWidget * widgets;
    int n_widgets;

    void (*init)(void);
    void (*apply)(void);
    void (*cleanup)(void);
};

#endif /* AUDACIOUS_PREFERENCES_H */
