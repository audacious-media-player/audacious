/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2008  Audacious development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifndef AUDACIOUS_PREFERENCES_H
#define AUDACIOUS_PREFERENCES_H

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
    gpointer value;
    const char *label;
} ComboBoxElements;

struct _NotebookTab;

struct _PreferencesWidget {
    WidgetType type;         /* widget type */
    char *label;             /* widget title (for SPIN_BTN it's text left to widget) */
    gpointer cfg;            /* connected config value */
    void (*callback) (void); /* this func will be called after value change, can be NULL */
    char *tooltip;           /* widget tooltip, can be NULL */
    bool_t child;
    ValueType cfg_type;      /* connected value type */
    const char * csect;     /* config file section */
    const char * cname;     /* config file key name */

    union {
        struct {
            double min, max, step;
            char *right_label;      /* text right to widget */
        } spin_btn;

        struct {
            struct _PreferencesWidget *elem;
            int rows;
        } table;

        struct {
            char *stock_id;
            bool_t single_line; /* FALSE to enable line wrap */
        } label;

        struct {
            char *title;
        } font_btn;

        struct {
            bool_t password;
        } entry;

        struct {
            ComboBoxElements *elements;
            int n_elements;
            bool_t enabled;
        } combo;

        struct {
            struct _PreferencesWidget *elem;
            int n_elem;

            bool_t horizontal;  /* FALSE gives vertical, TRUE gives horizontal aligment of child widgets */
            bool_t frame;       /* whether to draw frame around box */
        } box;

        struct {
            struct _NotebookTab *tabs;
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
    char *name;
    PreferencesWidget *settings;
    int n_settings;
} NotebookTab;

typedef enum {
    PREFERENCES_WINDOW,  /* displayed in seperate window */
} PreferencesType;

struct _PluginPreferences {
    const char * domain;
    const char * title;
    const char * imgurl;

    PreferencesWidget *prefs;
    int n_prefs;

    PreferencesType type;

    void (*init)(void);
    void (*apply)(void);
    void (*cancel)(void);
    void (*cleanup)(void);

    gpointer data;    /* for internal interface use only */
};

#endif /* AUDACIOUS_PREFERENCES_H */
