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
    VALUE_CFG_BOOLEAN,   /* cfg holds config database key for bool option */
    VALUE_CFG_STRING,    /* cfg holds config database key for gchar* option */
    VALUE_NULL,
} ValueType;

typedef struct {
    gpointer value;
    const gchar *label;
} ComboBoxElements;

struct _NotebookTab;

typedef struct _PreferencesWidget {
    WidgetType type;         /* widget type */
    char *label;             /* widget title (for SPIN_BTN it's text left to widget) */
    gpointer cfg;            /* connected config value */
    void (*callback) (void); /* this func will be called after value change, can be NULL */
    char *tooltip;           /* widget tooltip, can be NULL */
    gboolean child;
    union {
        struct {
            gdouble min, max, step;
            char *right_label;      /* text right to widget */
        } spin_btn;

        struct {
            struct _PreferencesWidget *elem;
            gint rows;
        } table;

        struct {
            char *stock_id;
            gboolean single_line; /* FALSE to enable line wrap */
        } label;

        struct {
            char *title;
        } font_btn;

        struct {
            gboolean password;
        } entry;

        struct {
            ComboBoxElements *elements;
            gint n_elements;
            gboolean enabled;
        } combo;

        struct {
            struct _PreferencesWidget *elem;
            gint n_elem;

            gboolean horizontal;  /* FALSE gives vertical, TRUE gives horizontal aligment of child widgets */
            gboolean frame;       /* whether to draw frame around box */
        } box;

        struct {
            struct _NotebookTab *tabs;
            gint n_tabs;
        } notebook;

        struct {
            gboolean horizontal; /* FALSE gives vertical, TRUE gives horizontal separator */
        } separator;

        /* for WIDGET_CUSTOM --nenolod */
        /* GtkWidget * (* populate) (void); */
        void * (* populate) (void);
    } data;
    ValueType cfg_type;      /* connected value type */
} PreferencesWidget;

typedef struct _NotebookTab {
    gchar *name;
    PreferencesWidget *settings;
    gint n_settings;
} NotebookTab;

typedef enum {
    PREFERENCES_WINDOW,  /* displayed in seperate window */
    PREFERENCES_PAGE,    /* added as new page in main preferences window */
} PreferencesType;

typedef struct {
    gchar *title;
    gchar *imgurl;        /* Optional */

    PreferencesWidget *prefs;
    gint n_prefs;

    PreferencesType type;

    void (*init)(void);
    void (*apply)(void);
    void (*cancel)(void);
    void (*cleanup)(void);

    gpointer data;    /* for internal interface use only */
} PluginPreferences;

/* void create_widgets_with_domain (GtkBox * box, PreferencesWidget * widgets,
 gint amt, const gchar * domain); */
void create_widgets_with_domain (void * box, PreferencesWidget * widgets, gint
 amt, const gchar * domain);

#define create_widgets(b, w, a) create_widgets_with_domain (b, w, a, PACKAGE);

#endif /* AUDACIOUS_PREFERENCES_H */
