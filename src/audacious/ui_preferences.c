/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2011  Audacious development team.
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

#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <libaudcore/hook.h>

#include "config.h"
#include "debug.h"
#include "gtk-compat.h"
#include "i18n.h"
#include "misc.h"
#include "output.h"
#include "playback.h"
#include "playlist.h"
#include "plugin.h"
#include "plugins.h"
#include "preferences.h"
#include "ui_preferences.h"

#ifdef USE_CHARDET
#include <libguess.h>
#endif

static void sw_volume_toggled (void);

enum CategoryViewCols {
    CATEGORY_VIEW_COL_ICON,
    CATEGORY_VIEW_COL_NAME,
    CATEGORY_VIEW_COL_ID,
    CATEGORY_VIEW_N_COLS
};

typedef struct {
    const char *icon_path;
    const char *name;
} Category;

typedef struct {
    const char *name;
    const char *tag;
} TitleFieldTag;

static /* GtkWidget * */ void * prefswin = NULL;
static GtkWidget *filepopup_settings = NULL;
static GtkWidget *category_treeview = NULL;
static GtkWidget *category_notebook = NULL;
GtkWidget *filepopupbutton = NULL;

/* filepopup settings widgets */
GtkWidget *filepopup_cover_name_include;
GtkWidget *filepopup_cover_name_exclude;
GtkWidget *filepopup_recurse;
GtkWidget *filepopup_recurse_depth;
GtkWidget *filepopup_recurse_depth_box;
GtkWidget *filepopup_use_file_cover;
GtkWidget *filepopup_showprogressbar;
GtkWidget *filepopup_delay;

/* prefswin widgets */
GtkWidget *titlestring_entry;
GtkWidget *filepopup_settings_button;

static Category categories[] = {
 {"audio.png", N_("Audio")},
 {"connectivity.png", N_("Network")},
 {"playlist.png", N_("Playlist")},
 {"plugins.png", N_("Plugins")},
};

static int n_categories = G_N_ELEMENTS(categories);

static TitleFieldTag title_field_tags[] = {
    { N_("Artist")     , "${artist}" },
    { N_("Album")      , "${album}" },
    { N_("Title")      , "${title}" },
    { N_("Tracknumber"), "${track-number}" },
    { N_("Genre")      , "${genre}" },
    { N_("Filename")   , "${file-name}" },
    { N_("Filepath")   , "${file-path}" },
    { N_("Date")       , "${date}" },
    { N_("Year")       , "${year}" },
    { N_("Comment")    , "${comment}" },
    { N_("Codec")      , "${codec}" },
    { N_("Quality")    , "${quality}" },
};
static const unsigned int n_title_field_tags = G_N_ELEMENTS(title_field_tags);

#ifdef USE_CHARDET
static ComboBoxElements chardet_detector_presets[] = {
 {"", N_("None")},
 {GUESS_REGION_AR, N_("Arabic")},
 {GUESS_REGION_BL, N_("Baltic")},
 {GUESS_REGION_CN, N_("Chinese")},
 {GUESS_REGION_GR, N_("Greek")},
 {GUESS_REGION_HW, N_("Hebrew")},
 {GUESS_REGION_JP, N_("Japanese")},
 {GUESS_REGION_KR, N_("Korean")},
 {GUESS_REGION_PL, N_("Polish")},
 {GUESS_REGION_RU, N_("Russian")},
 {GUESS_REGION_TW, N_("Taiwanese")},
 {GUESS_REGION_TR, N_("Turkish")}};
#endif

static ComboBoxElements bitdepth_elements[] = {
    { GINT_TO_POINTER(16), "16" },
    { GINT_TO_POINTER(24), "24" },
    { GINT_TO_POINTER(32), "32" },
    {GINT_TO_POINTER (0), "Floating point"},
};

typedef struct {
    void *next;
    GtkWidget *container;
    const char * pg_name;
    const char * img_url;
} CategoryQueueEntry;

CategoryQueueEntry *category_queue = NULL;

static void * create_output_plugin_box (void);

static PreferencesWidget rg_mode_widgets[] = {
 {WIDGET_CHK_BTN, N_("Album mode"), .cfg_type = VALUE_BOOLEAN, .cname = "replay_gain_album"}};

static PreferencesWidget audio_page_widgets[] = {
 {WIDGET_LABEL, N_("<b>Output Settings</b>")},
 {WIDGET_CUSTOM, .data = {.populate = create_output_plugin_box}},
 {WIDGET_COMBO_BOX, N_("Bit depth:"),
  .cfg_type = VALUE_INT, .cname = "output_bit_depth",
  .data = {.combo = {bitdepth_elements, G_N_ELEMENTS (bitdepth_elements), TRUE}}},
 {WIDGET_SPIN_BTN, N_("Buffer size:"),
  .cfg_type = VALUE_INT, .cname = "output_buffer_size",
  .data = {.spin_btn = {100, 10000, 1000, N_("ms")}}},
 {WIDGET_CHK_BTN, N_("Use software volume control (not recommended)"),
  .cfg_type = VALUE_BOOLEAN, .cname = "software_volume_control", .callback = sw_volume_toggled},
 {WIDGET_LABEL, N_("<b>Replay Gain</b>")},
 {WIDGET_CHK_BTN, N_("Enable Replay Gain"),
  .cfg_type = VALUE_BOOLEAN, .cname = "enable_replay_gain"},
 {WIDGET_BOX, .child = TRUE, .data = {.box = {rg_mode_widgets, G_N_ELEMENTS (rg_mode_widgets), TRUE}}},
 {WIDGET_CHK_BTN, N_("Prevent clipping (recommended)"), .child = TRUE,
  .cfg_type = VALUE_BOOLEAN, .cname = "enable_clipping_prevention"},
 {WIDGET_LABEL, N_("<b>Adjust Levels</b>"), .child = TRUE},
 {WIDGET_SPIN_BTN, N_("Amplify all files:"), .child = TRUE,
  .cfg_type = VALUE_FLOAT, .cname = "replay_gain_preamp",
  .data = {.spin_btn = {-15, 15, 0.1, N_("dB")}}},
 {WIDGET_SPIN_BTN, N_("Amplify untagged files:"), .child = TRUE,
  .cfg_type = VALUE_FLOAT, .cname = "default_gain",
  .data = {.spin_btn = {-15, 15, 0.1, N_("dB")}}}};

static PreferencesWidget proxy_host_port_elements[] = {
 {WIDGET_ENTRY, N_("Proxy hostname:"), .cfg_type = VALUE_STRING, .cname = "proxy_host"},
 {WIDGET_ENTRY, N_("Proxy port:"), .cfg_type = VALUE_STRING, .cname = "proxy_port"}};

static PreferencesWidget proxy_auth_elements[] = {
 {WIDGET_ENTRY, N_("Proxy username:"), .cfg_type = VALUE_STRING, .cname = "proxy_user"},
 {WIDGET_ENTRY, N_("Proxy password:"), .cfg_type = VALUE_STRING, .cname = "proxy_pass",
  .data = {.entry = {.password = TRUE}}}};

static PreferencesWidget connectivity_page_widgets[] = {
    {WIDGET_LABEL, N_("<b>Proxy Configuration</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Enable proxy usage"), .cfg_type = VALUE_BOOLEAN, .cname = "use_proxy"},
    {WIDGET_TABLE, .child = TRUE, .data = {.table = {proxy_host_port_elements,
     G_N_ELEMENTS (proxy_host_port_elements)}}},
    {WIDGET_CHK_BTN, N_("Use authentication with proxy"),
     .cfg_type = VALUE_BOOLEAN, .cname = "use_proxy_auth"},
    {WIDGET_TABLE, .child = TRUE, .data = {.table = {proxy_auth_elements,
     G_N_ELEMENTS (proxy_auth_elements)}}}
};

static PreferencesWidget chardet_elements[] = {
#ifdef USE_CHARDET
 {WIDGET_COMBO_BOX, N_("Auto character encoding detector for:"),
  .cfg_type = VALUE_STRING, .cname = "chardet_detector", .child = TRUE,
  .data = {.combo = {chardet_detector_presets,
  G_N_ELEMENTS (chardet_detector_presets), TRUE}}},
#endif
 {WIDGET_ENTRY, N_("Fallback character encodings:"), .cfg_type = VALUE_STRING,
  .cname = "chardet_fallback", .child = TRUE}};

static PreferencesWidget playlist_page_widgets[] = {
    {WIDGET_LABEL, N_("<b>Behavior</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Continue playback on startup"),
     .cfg_type = VALUE_BOOLEAN, .cname = "resume_playback_on_startup"},
    {WIDGET_CHK_BTN, N_("Advance when the current song is deleted"),
     .cfg_type = VALUE_BOOLEAN, .cname = "advance_on_delete"},
    {WIDGET_CHK_BTN, N_("Clear the playlist when opening files"),
     .cfg_type = VALUE_BOOLEAN, .cname = "clear_playlist"},
    {WIDGET_CHK_BTN, N_("Open files in a temporary playlist"),
     .cfg_type = VALUE_BOOLEAN, .cname = "open_to_temporary"},
    {WIDGET_LABEL, N_("<b>Metadata</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Do not load metadata for songs until played"),
     .cfg_type = VALUE_BOOLEAN, .cname = "metadata_on_play",
     .callback = playlist_trigger_scan},
    {WIDGET_TABLE, .data = {.table = {chardet_elements,
     G_N_ELEMENTS (chardet_elements)}}}
};

#define TITLESTRING_NPRESETS 6

static const char * const titlestring_presets[TITLESTRING_NPRESETS] = {
 "${title}",
 "${?artist:${artist} - }${title}",
 "${?artist:${artist} - }${?album:${album} - }${title}",
 "${?artist:${artist} - }${?album:${album} - }${?track-number:${track-number}. }${title}",
 "${?artist:${artist} }${?album:[ ${album} ] }${?artist:- }${?track-number:${track-number}. }${title}",
 "${?album:${album} - }${title}"};

static const char * const titlestring_preset_names[TITLESTRING_NPRESETS] = {
 N_("TITLE"),
 N_("ARTIST - TITLE"),
 N_("ARTIST - ALBUM - TITLE"),
 N_("ARTIST - ALBUM - TRACK. TITLE"),
 N_("ARTIST [ ALBUM ] - TRACK. TITLE"),
 N_("ALBUM - TITLE")};

static void prefswin_page_queue_destroy(CategoryQueueEntry *ent);

static void
change_category(GtkNotebook * notebook,
                GtkTreeSelection * selection)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    int index;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, CATEGORY_VIEW_COL_ID, &index, -1);
    gtk_notebook_set_current_page(notebook, index);
}

static void
editable_insert_text(GtkEditable * editable,
                     const char * text,
                     int * pos)
{
    gtk_editable_insert_text(editable, text, strlen(text), pos);
}

static void
titlestring_tag_menu_callback(GtkMenuItem * menuitem,
                              gpointer data)
{
    const char *separator = " - ";
    int item = GPOINTER_TO_INT(data);
    int pos;

    pos = gtk_editable_get_position(GTK_EDITABLE(titlestring_entry));

    /* insert separator as needed */
    if (g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(titlestring_entry)), -1) > 0)
        editable_insert_text(GTK_EDITABLE(titlestring_entry), separator, &pos);

    editable_insert_text(GTK_EDITABLE(titlestring_entry), _(title_field_tags[item].tag),
                         &pos);

    gtk_editable_set_position(GTK_EDITABLE(titlestring_entry), pos);
}

static void
on_titlestring_help_button_clicked(GtkButton * button,
                                   gpointer data)
{
    GtkMenu * menu = data;
    gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);
}

static void update_titlestring_cbox (GtkComboBox * cbox, const char * format)
{
    int preset;
    for (preset = 0; preset < TITLESTRING_NPRESETS; preset ++)
    {
        if (! strcmp (titlestring_presets[preset], format))
            break;
    }

    if (gtk_combo_box_get_active (cbox) != preset)
        gtk_combo_box_set_active (cbox, preset);
}

static void on_titlestring_entry_changed (GtkEntry * entry, GtkComboBox * cbox)
{
    const char * format = gtk_entry_get_text (entry);
    set_string (NULL, "generic_title_format", format);
    update_titlestring_cbox (cbox, format);
    playlist_reformat_titles ();
}

static void on_titlestring_cbox_changed (GtkComboBox * cbox, GtkEntry * entry)
{
    int preset = gtk_combo_box_get_active (cbox);
    if (preset < TITLESTRING_NPRESETS)
        gtk_entry_set_text (entry, titlestring_presets[preset]);
}

static void widget_set_bool (PreferencesWidget * widget, bool_t value)
{
    g_return_if_fail (widget->cfg_type == VALUE_BOOLEAN);

    if (widget->cfg)
        * (bool_t *) widget->cfg = value;
    else if (widget->cname)
        set_bool (widget->csect, widget->cname, value);

    if (widget->callback)
        widget->callback ();
}

static bool_t widget_get_bool (PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg_type == VALUE_BOOLEAN, FALSE);

    if (widget->cfg)
        return * (bool_t *) widget->cfg;
    else if (widget->cname)
        return get_bool (widget->csect, widget->cname);
    else
        return FALSE;
}

static void widget_set_int (PreferencesWidget * widget, int value)
{
    g_return_if_fail (widget->cfg_type == VALUE_INT);

    if (widget->cfg)
        * (int *) widget->cfg = value;
    else if (widget->cname)
        set_int (widget->csect, widget->cname, value);

    if (widget->callback)
        widget->callback ();
}

static int widget_get_int (PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg_type == VALUE_INT, 0);

    if (widget->cfg)
        return * (int *) widget->cfg;
    else if (widget->cname)
        return get_int (widget->csect, widget->cname);
    else
        return 0;
}

static void widget_set_double (PreferencesWidget * widget, double value)
{
    g_return_if_fail (widget->cfg_type == VALUE_FLOAT);

    if (widget->cfg)
        * (float *) widget->cfg = value;
    else if (widget->cname)
        set_double (widget->csect, widget->cname, value);

    if (widget->callback)
        widget->callback ();
}

static double widget_get_double (PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg_type == VALUE_FLOAT, 0);

    if (widget->cfg)
        return * (float *) widget->cfg;
    else if (widget->cname)
        return get_double (widget->csect, widget->cname);
    else
        return 0;
}

static void widget_set_string (PreferencesWidget * widget, const char * value)
{
    g_return_if_fail (widget->cfg_type == VALUE_STRING);

    if (widget->cfg)
    {
        g_free (* (char * *) widget->cfg);
        * (char * *) widget->cfg = g_strdup (value);
    }
    else if (widget->cname)
        set_string (widget->csect, widget->cname, value);

    if (widget->callback)
        widget->callback ();
}

static char * widget_get_string (PreferencesWidget * widget)
{
    g_return_val_if_fail (widget->cfg_type == VALUE_STRING, NULL);

    if (widget->cfg)
        return g_strdup (* (char * *) widget->cfg);
    else if (widget->cname)
        return get_string (widget->csect, widget->cname);
    else
        return NULL;
}

static void on_font_btn_font_set (GtkFontButton * button, PreferencesWidget * widget)
{
    widget_set_string (widget, gtk_font_button_get_font_name (button));
}

static void
plugin_preferences_ok(GtkWidget *widget, PluginPreferences *settings)
{
    if (settings->apply)
        settings->apply();

    gtk_widget_destroy(GTK_WIDGET(settings->data));
}

static void
plugin_preferences_apply(GtkWidget *widget, PluginPreferences *settings)
{
    if (settings->apply)
        settings->apply();
}

static void
plugin_preferences_cancel(GtkWidget *widget, PluginPreferences *settings)
{
    if (settings->cancel)
        settings->cancel();

    gtk_widget_destroy(GTK_WIDGET(settings->data));
}

static void plugin_preferences_destroy(GtkWidget *widget, PluginPreferences *settings)
{
    gtk_widget_destroy(widget);

    if (settings->cleanup)
        settings->cleanup();

    settings->data = NULL;
}

void plugin_preferences_show (PluginPreferences * settings)
{
    GtkWidget *window;
    GtkWidget *vbox, *bbox, *ok, *apply, *cancel;

    if (settings->data != NULL) {
        gtk_widget_show(GTK_WIDGET(settings->data));
        return;
    }

    if (settings->init)
        settings->init();

    const char * d = settings->domain;
    if (! d)
    {
        printf ("WARNING: PluginPreferences window with title \"%s\" did not "
         "declare its gettext domain.  Text may not be translated correctly.\n",
         settings->title);
        d = "audacious-plugins";
    }

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);

    if (settings->title)
        gtk_window_set_title ((GtkWindow *) window, dgettext (d, settings->title));

    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(plugin_preferences_destroy), settings);

    vbox = gtk_vbox_new(FALSE, 10);
    create_widgets_with_domain ((GtkBox *) vbox, settings->prefs,
     settings->n_prefs, d);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    ok = gtk_button_new_from_stock(GTK_STOCK_OK);
    g_signal_connect(G_OBJECT(ok), "clicked",
                     G_CALLBACK(plugin_preferences_ok), settings);
    gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
    gtk_widget_set_can_default (ok, TRUE);
    gtk_widget_grab_default(ok);

    apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    g_signal_connect(G_OBJECT(apply), "clicked",
                     G_CALLBACK(plugin_preferences_apply), settings);
    gtk_box_pack_start(GTK_BOX(bbox), apply, TRUE, TRUE, 0);

    cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(G_OBJECT(cancel), "clicked",
                     G_CALLBACK(plugin_preferences_cancel), settings);
    gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(prefswin));
    gtk_widget_show_all(window);
    settings->data = (gpointer)window;
}

void plugin_preferences_cleanup (PluginPreferences * p)
{
    if (p->data != NULL)
    {
        gtk_widget_destroy (p->data);
        p->data = NULL;
    }
}

static void on_spin_btn_changed_int (GtkSpinButton * button, PreferencesWidget * widget)
{
    widget_set_int (widget, gtk_spin_button_get_value_as_int (button));
}

static void on_spin_btn_changed_float (GtkSpinButton * button, PreferencesWidget * widget)
{
    widget_set_double (widget, gtk_spin_button_get_value (button));
}

static void fill_category_list (GtkTreeView * treeview, GtkNotebook * notebook)
{
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GdkPixbuf *img;
    CategoryQueueEntry *qlist;
    int i;

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Category"));
    gtk_tree_view_append_column(treeview, column);
    gtk_tree_view_column_set_spacing(column, 2);

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "pixbuf", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", 1, NULL);

    g_object_set ((GObject *) renderer, "wrap-width", 96, "wrap-mode",
     PANGO_WRAP_WORD_CHAR, NULL);

    store = gtk_list_store_new(CATEGORY_VIEW_N_COLS,
                               GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));

    for (i = 0; i < n_categories; i ++)
    {
        char * path = g_strdup_printf ("%s/images/%s",
         get_path (AUD_PATH_DATA_DIR), categories[i].icon_path);
        img = gdk_pixbuf_new_from_file (path, NULL);
        g_free (path);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           CATEGORY_VIEW_COL_ICON, img,
                           CATEGORY_VIEW_COL_NAME,
                           gettext(categories[i].name), CATEGORY_VIEW_COL_ID,
                           i, -1);
        g_object_unref(img);
    }

    selection = gtk_tree_view_get_selection(treeview);

    g_signal_connect_swapped(selection, "changed",
                             G_CALLBACK(change_category), notebook);

    /* mark the treeview widget as available to third party plugins */
    category_treeview = GTK_WIDGET(treeview);

    /* prefswin_page_queue_destroy already pops the queue forward for us. */
    for (qlist = category_queue; qlist != NULL; qlist = category_queue)
    {
        CategoryQueueEntry *ent = (CategoryQueueEntry *) qlist;

        prefswin_page_new(ent->container, ent->pg_name, ent->img_url);
        prefswin_page_queue_destroy(ent);
    }
}

static void on_show_filepopup_toggled (GtkToggleButton * button)
{
    bool_t active = gtk_toggle_button_get_active (button);
    set_bool (NULL, "show_filepopup_for_tuple", active);
    gtk_widget_set_sensitive (filepopup_settings_button, active);
}

static void on_filepopup_settings_clicked (void)
{
    char * string = get_string (NULL, "cover_name_include");
    gtk_entry_set_text ((GtkEntry *) filepopup_cover_name_include, string);
    g_free (string);

    string = get_string (NULL, "cover_name_exclude");
    gtk_entry_set_text ((GtkEntry *) filepopup_cover_name_exclude, string);
    g_free (string);

    gtk_toggle_button_set_active ((GtkToggleButton *) filepopup_recurse,
     get_bool (NULL, "recurse_for_cover"));
    gtk_spin_button_set_value ((GtkSpinButton *) filepopup_recurse_depth,
     get_int (NULL, "recurse_for_cover_depth"));
    gtk_toggle_button_set_active ((GtkToggleButton *) filepopup_use_file_cover,
     get_bool (NULL, "use_file_cover"));

    gtk_toggle_button_set_active ((GtkToggleButton *) filepopup_showprogressbar,
     get_bool (NULL, "filepopup_showprogressbar"));
    gtk_spin_button_set_value ((GtkSpinButton *) filepopup_delay,
     get_int (NULL, "filepopup_delay"));

    gtk_widget_show (filepopup_settings);
}

static void on_filepopup_ok_clicked (void)
{
    set_string (NULL, "cover_name_include",
     gtk_entry_get_text ((GtkEntry *) filepopup_cover_name_include));
    set_string (NULL, "cover_name_exclude",
     gtk_entry_get_text ((GtkEntry *) filepopup_cover_name_exclude));

    set_bool (NULL, "recurse_for_cover",
     gtk_toggle_button_get_active ((GtkToggleButton *) filepopup_recurse));
    set_int (NULL, "recurse_for_cover_depth",
     gtk_spin_button_get_value_as_int ((GtkSpinButton *) filepopup_recurse_depth));
    set_bool (NULL, "use_file_cover",
     gtk_toggle_button_get_active ((GtkToggleButton *) filepopup_use_file_cover));

    set_bool (NULL, "filepopup_showprogressbar",
     gtk_toggle_button_get_active ((GtkToggleButton *) filepopup_showprogressbar));
    set_int (NULL, "filepopup_delay",
     gtk_spin_button_get_value_as_int ((GtkSpinButton *) filepopup_delay));

    gtk_widget_hide (filepopup_settings);
}

static void
on_filepopup_cancel_clicked(GtkButton *button, gpointer data)
{
    gtk_widget_hide(filepopup_settings);
}

static void on_toggle_button_toggled (GtkToggleButton * button, PreferencesWidget * widget)
{
    bool_t active = gtk_toggle_button_get_active (button);
    widget_set_bool (widget, active);

    GtkWidget * child = g_object_get_data ((GObject *) button, "child");
    if (child)
        gtk_widget_set_sensitive (child, active);
}

static void init_toggle_button (GtkWidget * button, PreferencesWidget * widget)
{
    if (widget->cfg_type != VALUE_BOOLEAN)
        return;

    gtk_toggle_button_set_active ((GtkToggleButton *) button, widget_get_bool (widget));
    g_signal_connect (button, "toggled", (GCallback) on_toggle_button_toggled, widget);
}

static void on_entry_changed (GtkEntry * entry, PreferencesWidget * widget)
{
    widget_set_string (widget, gtk_entry_get_text (entry));
}

static void on_cbox_changed_int (GtkComboBox * combobox, PreferencesWidget * widget)
{
    int position = gtk_combo_box_get_active (combobox);
    widget_set_int (widget, GPOINTER_TO_INT (widget->data.combo.elements[position].value));
}

static void on_cbox_changed_string (GtkComboBox * combobox, PreferencesWidget * widget)
{
    int position = gtk_combo_box_get_active (combobox);
    widget_set_string (widget, widget->data.combo.elements[position].value);
}

static void fill_cbox (GtkWidget * combobox, PreferencesWidget * widget)
{
    unsigned int i=0,index=0;

    for (i = 0; i < widget->data.combo.n_elements; i ++)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) combobox,
         _(widget->data.combo.elements[i].label));

    if (widget->data.combo.enabled) {
        switch (widget->cfg_type) {
            case VALUE_INT:
                g_signal_connect(combobox, "changed",
                                 G_CALLBACK(on_cbox_changed_int), widget);

                int ivalue = widget_get_int (widget);

                for(i=0; i<widget->data.combo.n_elements; i++) {
                    if (GPOINTER_TO_INT (widget->data.combo.elements[i].value) == ivalue)
                    {
                        index = i;
                        break;
                    }
                }
                break;
            case VALUE_STRING:
                g_signal_connect(combobox, "changed",
                                 G_CALLBACK(on_cbox_changed_string), widget);

                char * value = widget_get_string (widget);

                for(i=0; i<widget->data.combo.n_elements; i++) {
                    if (value && ! strcmp ((char *) widget->data.combo.elements[i].value, value))
                    {
                        index = i;
                        break;
                    }
                }

                g_free (value);
                break;
            default:
                break;
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), index);
    } else {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), -1);
        gtk_widget_set_sensitive(GTK_WIDGET(combobox), 0);
    }
}

void
create_filepopup_settings(void)
{
    GtkWidget *vbox;
    GtkWidget *table;

    GtkWidget *label_cover_retrieve;
    GtkWidget *label_cover_search;
    GtkWidget *label_exclude;
    GtkWidget *label_include;
    GtkWidget *label_search_depth;
    GtkWidget *label_misc;
    GtkWidget *label_delay;

    GtkAdjustment *recurse_for_cover_depth_adj;
    GtkAdjustment *delay_adj;
    GtkWidget *alignment;

    GtkWidget *hbox;
    GtkWidget *hbuttonbox;
    GtkWidget *btn_cancel;
    GtkWidget *btn_ok;

    filepopup_settings = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(filepopup_settings), 12);
    gtk_window_set_title(GTK_WINDOW(filepopup_settings), _("Popup Information Settings"));
    gtk_window_set_position(GTK_WINDOW(filepopup_settings), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(filepopup_settings), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(filepopup_settings), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_transient_for(GTK_WINDOW(filepopup_settings), GTK_WINDOW(prefswin));

    vbox = gtk_vbox_new(FALSE, 12);
    gtk_container_add(GTK_CONTAINER(filepopup_settings), vbox);

    label_cover_retrieve = gtk_label_new(_("<b>Cover image retrieve</b>"));
    gtk_box_pack_start(GTK_BOX(vbox), label_cover_retrieve, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_cover_retrieve), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_cover_retrieve), 0, 0.5);

    label_cover_search = gtk_label_new(_("While searching for the album's cover, Audacious looks for certain words in the filename. You can specify those words in the lists below, separated using commas."));
    gtk_box_pack_start(GTK_BOX(vbox), label_cover_search, FALSE, FALSE, 0);
    gtk_label_set_line_wrap(GTK_LABEL(label_cover_search), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_cover_search), 0, 0);
    gtk_misc_set_padding(GTK_MISC(label_cover_search), 12, 0);

    table = gtk_table_new(2, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 4);
    gtk_table_set_col_spacings(GTK_TABLE(table), 4);

    filepopup_cover_name_include = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), filepopup_cover_name_include, 1, 2, 0, 1,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_activates_default(GTK_ENTRY(filepopup_cover_name_include), TRUE);

    label_exclude = gtk_label_new(_("Exclude:"));
    gtk_table_attach(GTK_TABLE(table), label_exclude, 0, 1, 1, 2,
                     (GtkAttachOptions) (0),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_exclude), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label_exclude), 12, 0);

    label_include = gtk_label_new(_("Include:"));
    gtk_table_attach(GTK_TABLE(table), label_include, 0, 1, 0, 1,
                     (GtkAttachOptions) (0),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_include), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label_include), 12, 0);

    filepopup_cover_name_exclude = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), filepopup_cover_name_exclude, 1, 2, 1, 2,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_activates_default(GTK_ENTRY(filepopup_cover_name_exclude), TRUE);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);

    filepopup_recurse = gtk_check_button_new_with_mnemonic(_("Recursively search for cover"));
    gtk_container_add(GTK_CONTAINER(alignment), filepopup_recurse);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 45, 0);

    filepopup_recurse_depth_box = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), filepopup_recurse_depth_box);

    label_search_depth = gtk_label_new(_("Search depth: "));
    gtk_box_pack_start(GTK_BOX(filepopup_recurse_depth_box), label_search_depth, TRUE, TRUE, 0);
    gtk_misc_set_padding(GTK_MISC(label_search_depth), 4, 0);

    recurse_for_cover_depth_adj = (GtkAdjustment *) gtk_adjustment_new (0, 0,
     100, 1, 10, 0);
    filepopup_recurse_depth = gtk_spin_button_new(GTK_ADJUSTMENT(recurse_for_cover_depth_adj), 1, 0);
    gtk_box_pack_start(GTK_BOX(filepopup_recurse_depth_box), filepopup_recurse_depth, TRUE, TRUE, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(filepopup_recurse_depth), TRUE);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);

    filepopup_use_file_cover = gtk_check_button_new_with_mnemonic(_("Use per-file cover"));
    gtk_container_add(GTK_CONTAINER(alignment), filepopup_use_file_cover);

    label_misc = gtk_label_new(_("<b>Miscellaneous</b>"));
    gtk_box_pack_start(GTK_BOX(vbox), label_misc, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_misc), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_misc), 0, 0.5);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);

    filepopup_showprogressbar = gtk_check_button_new_with_mnemonic(_("Show Progress bar for the current track"));
    gtk_container_add(GTK_CONTAINER(alignment), filepopup_showprogressbar);

    alignment = gtk_alignment_new(0, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);

    label_delay = gtk_label_new(_("Delay until filepopup comes up: "));
    gtk_box_pack_start(GTK_BOX(hbox), label_delay, TRUE, TRUE, 0);
    gtk_misc_set_alignment(GTK_MISC(label_delay), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label_delay), 12, 0);

    delay_adj = (GtkAdjustment *) gtk_adjustment_new (0, 0, 100, 1, 10, 0);
    filepopup_delay = gtk_spin_button_new(GTK_ADJUSTMENT(delay_adj), 1, 0);
    gtk_box_pack_start(GTK_BOX(hbox), filepopup_delay, TRUE, TRUE, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(filepopup_delay), TRUE);

    hbuttonbox = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(vbox), hbuttonbox, FALSE, FALSE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(hbuttonbox), 6);

    btn_cancel = gtk_button_new_from_stock("gtk-cancel");
    gtk_container_add(GTK_CONTAINER(hbuttonbox), btn_cancel);

    btn_ok = gtk_button_new_from_stock("gtk-ok");
    gtk_container_add(GTK_CONTAINER(hbuttonbox), btn_ok);
    gtk_widget_set_can_default(btn_ok, TRUE);

    g_signal_connect(G_OBJECT(filepopup_settings), "delete_event",
                     G_CALLBACK(gtk_widget_hide_on_delete),
                     NULL);
    g_signal_connect(G_OBJECT(btn_cancel), "clicked",
                     G_CALLBACK(on_filepopup_cancel_clicked),
                     NULL);
    g_signal_connect(G_OBJECT(btn_ok), "clicked",
                     G_CALLBACK(on_filepopup_ok_clicked),
                     NULL);

    gtk_widget_grab_default(btn_ok);
    gtk_widget_show_all(vbox);
}

static void create_spin_button (PreferencesWidget * widget, GtkWidget * *
 label_pre, GtkWidget * * spin_btn, GtkWidget * * label_past, const char *
 domain)
{
     g_return_if_fail(widget->type == WIDGET_SPIN_BTN);

     * label_pre = gtk_label_new (dgettext (domain, widget->label));

     *spin_btn = gtk_spin_button_new_with_range(widget->data.spin_btn.min,
                                                widget->data.spin_btn.max,
                                                widget->data.spin_btn.step);


     if (widget->tooltip)
         gtk_widget_set_tooltip_text (* spin_btn, dgettext (domain,
          widget->tooltip));

     if (widget->data.spin_btn.right_label) {
         * label_past = gtk_label_new (dgettext (domain,
          widget->data.spin_btn.right_label));
     }

    switch (widget->cfg_type)
    {
    case VALUE_INT:
        gtk_spin_button_set_value ((GtkSpinButton *) * spin_btn, widget_get_int (widget));
        g_signal_connect (* spin_btn, "value_changed", (GCallback) on_spin_btn_changed_int, widget);
        break;
    case VALUE_FLOAT:
        gtk_spin_button_set_value ((GtkSpinButton *) * spin_btn, widget_get_double (widget));
        g_signal_connect (* spin_btn, "value_changed", (GCallback)
         on_spin_btn_changed_float, widget);
        break;
    default:
        break;
    }
}

void create_font_btn (PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * font_btn, const char * domain)
{
    *font_btn = gtk_font_button_new();
    gtk_font_button_set_use_font(GTK_FONT_BUTTON(*font_btn), TRUE);
    gtk_font_button_set_use_size(GTK_FONT_BUTTON(*font_btn), TRUE);
    if (widget->label) {
        * label = gtk_label_new_with_mnemonic (dgettext (domain, widget->label));
        gtk_label_set_use_markup(GTK_LABEL(*label), TRUE);
        gtk_misc_set_alignment(GTK_MISC(*label), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(*label), GTK_JUSTIFY_RIGHT);
        gtk_label_set_mnemonic_widget(GTK_LABEL(*label), *font_btn);
    }

    if (widget->data.font_btn.title)
        gtk_font_button_set_title (GTK_FONT_BUTTON (* font_btn),
         dgettext (domain, widget->data.font_btn.title));

    char * name = widget_get_string (widget);
    if (name)
    {
        gtk_font_button_set_font_name ((GtkFontButton *) * font_btn, name);
        g_free (name);
    }

    g_signal_connect (* font_btn, "font_set", (GCallback) on_font_btn_font_set, widget);
}

static void create_entry (PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * entry, const char * domain)
{
    *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(*entry), !widget->data.entry.password);

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
            g_free (value);
        }

        g_signal_connect (* entry, "changed", (GCallback) on_entry_changed, widget);
    }
}

static void create_label (PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * icon, const char * domain)
{
    if (widget->data.label.stock_id)
        *icon = gtk_image_new_from_stock(widget->data.label.stock_id, GTK_ICON_SIZE_BUTTON);

    * label = gtk_label_new_with_mnemonic (dgettext (domain, widget->label));
    gtk_label_set_use_markup(GTK_LABEL(*label), TRUE);

    if (widget->data.label.single_line == FALSE)
        gtk_label_set_line_wrap(GTK_LABEL(*label), TRUE);

    gtk_misc_set_alignment(GTK_MISC(*label), 0, 0.5);
}

static void create_cbox (PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * combobox, const char * domain)
{
    * combobox = gtk_combo_box_text_new ();

    if (widget->label) {
        * label = gtk_label_new (dgettext (domain, widget->label));
    }

    fill_cbox (* combobox, widget);
}

static void fill_table (GtkWidget * table, PreferencesWidget * elements, int
 amt, const char * domain)
{
    int x;
    GtkWidget *widget_left, *widget_middle, *widget_right;
    GtkAttachOptions middle_policy = (GtkAttachOptions) (0);

    for (x = 0; x < amt; ++x) {
        widget_left = widget_middle = widget_right = NULL;
        switch (elements[x].type) {
            case WIDGET_SPIN_BTN:
                create_spin_button (& elements[x], & widget_left,
                 & widget_middle, & widget_right, domain);
                middle_policy = (GtkAttachOptions) (GTK_FILL);
                break;
            case WIDGET_LABEL:
                create_label (& elements[x], & widget_middle, & widget_left,
                 domain);
                middle_policy = (GtkAttachOptions) (GTK_FILL);
                break;
            case WIDGET_FONT_BTN:
                create_font_btn (& elements[x], & widget_left, & widget_middle,
                 domain);
                middle_policy = (GtkAttachOptions) (GTK_EXPAND | GTK_FILL);
                break;
            case WIDGET_ENTRY:
                create_entry (& elements[x], & widget_left, & widget_middle,
                 domain);
                middle_policy = (GtkAttachOptions) (GTK_EXPAND | GTK_FILL);
                break;
            case WIDGET_COMBO_BOX:
                create_cbox (& elements[x], & widget_left, & widget_middle,
                 domain);
                middle_policy = (GtkAttachOptions) (GTK_EXPAND | GTK_FILL);
                break;
            default:
                g_warning("Unsupported widget type %d in table", elements[x].type);
        }

        if (widget_left)
            gtk_table_attach(GTK_TABLE (table), widget_left, 0, 1, x, x+1,
                             (GtkAttachOptions) (0),
                             (GtkAttachOptions) (0), 0, 0);

        if (widget_middle)
            gtk_table_attach(GTK_TABLE(table), widget_middle, 1, widget_right ? 2 : 3, x, x+1,
                             middle_policy,
                             (GtkAttachOptions) (0), 4, 0);

        if (widget_right)
            gtk_table_attach(GTK_TABLE(table), widget_right, 2, 3, x, x+1,
                             (GtkAttachOptions) (0),
                             (GtkAttachOptions) (0), 0, 0);
    }
}

/* void create_widgets_with_domain (GtkBox * box, PreferencesWidget * widgets,
 int amt, const char * domain) */
void create_widgets_with_domain (void * box, PreferencesWidget * widgets, int
 amt, const char * domain)
{
    int x;
    GtkWidget *alignment = NULL, *widget = NULL;
    GtkWidget *child_box = NULL;
    GSList *radio_btn_group = NULL;

    for (x = 0; x < amt; ++x) {
        if (widget && widgets[x].child)
        {
            if (!child_box) {
                child_box = gtk_vbox_new(FALSE, 0);
                g_object_set_data(G_OBJECT(widget), "child", child_box);
                alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
                gtk_box_pack_start(box, alignment, FALSE, FALSE, 0);
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
                gtk_container_add (GTK_CONTAINER (alignment), child_box);

                if (GTK_IS_TOGGLE_BUTTON (widget))
                    gtk_widget_set_sensitive (child_box, gtk_toggle_button_get_active ((GtkToggleButton *) widget));
            }
        } else
            child_box = NULL;

        alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
        gtk_alignment_set_padding ((GtkAlignment *) alignment, 6, 0, 12, 0);
        gtk_box_pack_start(child_box ? GTK_BOX(child_box) : box, alignment, FALSE, FALSE, 0);

        if (radio_btn_group && widgets[x].type != WIDGET_RADIO_BTN)
            radio_btn_group = NULL;

        switch(widgets[x].type) {
            case WIDGET_CHK_BTN:
                widget = gtk_check_button_new_with_mnemonic (dgettext (domain, widgets[x].label));
                init_toggle_button (widget, & widgets[x]);
                break;
            case WIDGET_LABEL:
                gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 12, 0, 0, 0);

                GtkWidget *label = NULL, *icon = NULL;
                create_label (& widgets[x], & label, & icon, domain);

                if (icon == NULL)
                    widget = label;
                else {
                    widget = gtk_hbox_new(FALSE, 6);
                    gtk_box_pack_start(GTK_BOX(widget), icon, FALSE, FALSE, 0);
                    gtk_box_pack_start(GTK_BOX(widget), label, FALSE, FALSE, 0);
                }
                break;
            case WIDGET_RADIO_BTN:
                widget = gtk_radio_button_new_with_mnemonic (radio_btn_group,
                 dgettext (domain, widgets[x].label));
                radio_btn_group = gtk_radio_button_get_group ((GtkRadioButton *) widget);
                init_toggle_button (widget, & widgets[x]);
                break;
            case WIDGET_SPIN_BTN:
                widget = gtk_hbox_new(FALSE, 6);

                GtkWidget *label_pre = NULL, *spin_btn = NULL, *label_past = NULL;
                create_spin_button (& widgets[x], & label_pre, & spin_btn,
                 & label_past, domain);

                if (label_pre)
                    gtk_box_pack_start(GTK_BOX(widget), label_pre, FALSE, FALSE, 0);
                if (spin_btn)
                    gtk_box_pack_start(GTK_BOX(widget), spin_btn, FALSE, FALSE, 0);
                if (label_past)
                    gtk_box_pack_start(GTK_BOX(widget), label_past, FALSE, FALSE, 0);

                break;
            case WIDGET_CUSTOM:  /* custom widget. --nenolod */
                if (widgets[x].data.populate)
                    widget = widgets[x].data.populate();
                else
                    widget = NULL;

                break;
            case WIDGET_FONT_BTN:
                widget = gtk_hbox_new(FALSE, 6);

                GtkWidget *font_btn = NULL;
                create_font_btn (& widgets[x], & label, & font_btn, domain);

                if (label)
                    gtk_box_pack_start(GTK_BOX(widget), label, FALSE, FALSE, 0);
                if (font_btn)
                    gtk_box_pack_start(GTK_BOX(widget), font_btn, FALSE, FALSE, 0);
                break;
            case WIDGET_TABLE:
                widget = gtk_table_new(widgets[x].data.table.rows, 3, FALSE);
                fill_table (widget, widgets[x].data.table.elem,
                 widgets[x].data.table.rows, domain);
                gtk_table_set_row_spacings(GTK_TABLE(widget), 6);
                break;
            case WIDGET_ENTRY:
                widget = gtk_hbox_new(FALSE, 6);

                GtkWidget *entry = NULL;
                create_entry (& widgets[x], & label, & entry, domain);

                if (label)
                    gtk_box_pack_start(GTK_BOX(widget), label, FALSE, FALSE, 0);
                if (entry)
                    gtk_box_pack_start(GTK_BOX(widget), entry, TRUE, TRUE, 0);
                break;
            case WIDGET_COMBO_BOX:
                widget = gtk_hbox_new(FALSE, 6);

                GtkWidget *combo = NULL;
                create_cbox (& widgets[x], & label, & combo, domain);

                if (label)
                    gtk_box_pack_start(GTK_BOX(widget), label, FALSE, FALSE, 0);
                if (combo)
                    gtk_box_pack_start(GTK_BOX(widget), combo, FALSE, FALSE, 0);
                break;
            case WIDGET_BOX:
                gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 0, 0);

                if (widgets[x].data.box.horizontal) {
                    widget = gtk_hbox_new(FALSE, 0);
                } else {
                    widget = gtk_vbox_new(FALSE, 0);
                }

                create_widgets_with_domain ((GtkBox *) widget,
                 widgets[x].data.box.elem, widgets[x].data.box.n_elem, domain);

                if (widgets[x].data.box.frame) {
                    GtkWidget *tmp;
                    tmp = widget;

                    widget = gtk_frame_new (dgettext (domain, widgets[x].label));
                    gtk_container_add(GTK_CONTAINER(widget), tmp);
                }
                break;
            case WIDGET_NOTEBOOK:
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 0, 0);

                widget = gtk_notebook_new();

                int i;
                for (i = 0; i<widgets[x].data.notebook.n_tabs; i++) {
                    GtkWidget *vbox;
                    vbox = gtk_vbox_new(FALSE, 5);
                    create_widgets_with_domain ((GtkBox *) vbox,
                     widgets[x].data.notebook.tabs[i].settings,
                     widgets[x].data.notebook.tabs[i].n_settings, domain);

                    gtk_notebook_append_page (GTK_NOTEBOOK (widget), vbox,
                     gtk_label_new (dgettext (domain,
                     widgets[x].data.notebook.tabs[i].name)));
                }
                break;
            case WIDGET_SEPARATOR:
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 6, 0, 0);

                if (widgets[x].data.separator.horizontal == TRUE) {
                    widget = gtk_hseparator_new();
                } else {
                    widget = gtk_vseparator_new();
                }
                break;
            default:
                break;
        }

        if (widget && !gtk_widget_get_parent(widget))
            gtk_container_add(GTK_CONTAINER(alignment), widget);
        if (widget && widgets[x].tooltip && widgets[x].type != WIDGET_SPIN_BTN)
            gtk_widget_set_tooltip_text (widget, dgettext (domain,
             widgets[x].tooltip));
    }

}

static GtkWidget *
create_titlestring_tag_menu(void)
{
    GtkWidget *titlestring_tag_menu, *menu_item;
    unsigned int i;

    titlestring_tag_menu = gtk_menu_new();
    for(i = 0; i < n_title_field_tags; i++) {
        menu_item = gtk_menu_item_new_with_label(_(title_field_tags[i].name));
        gtk_menu_shell_append(GTK_MENU_SHELL(titlestring_tag_menu), menu_item);
        g_signal_connect(menu_item, "activate",
                         G_CALLBACK(titlestring_tag_menu_callback),
                         GINT_TO_POINTER(i));
    };
    gtk_widget_show_all(titlestring_tag_menu);

    return titlestring_tag_menu;
}

static void show_numbers_cb (GtkToggleButton * numbers, void * unused)
{
    set_bool (NULL, "show_numbers_in_pl", gtk_toggle_button_get_active (numbers));
    playlist_reformat_titles ();
    hook_call ("title change", NULL);
}

static void leading_zero_cb (GtkToggleButton * leading)
{
    set_bool (NULL, "leading_zero", gtk_toggle_button_get_active (leading));
    playlist_reformat_titles ();
    hook_call ("title change", NULL);
}

static void create_titlestring_widgets (GtkWidget * * cbox, GtkWidget * * entry)
{
    * cbox = gtk_combo_box_text_new ();
    for (int i = 0; i < TITLESTRING_NPRESETS; i ++)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) * cbox, _(titlestring_preset_names[i]));
    gtk_combo_box_text_append_text ((GtkComboBoxText *) * cbox, _("Custom"));

    * entry = gtk_entry_new ();

    char * format = get_string (NULL, "generic_title_format");
    update_titlestring_cbox ((GtkComboBox *) * cbox, format);
    gtk_entry_set_text ((GtkEntry *) * entry, format);
    g_free (format);

    g_signal_connect (* cbox, "changed", (GCallback) on_titlestring_cbox_changed, * entry);
    g_signal_connect (* entry, "changed", (GCallback) on_titlestring_entry_changed, * cbox);
}

static void
create_playlist_category(void)
{
    GtkWidget *vbox5;
    GtkWidget *alignment55;
    GtkWidget *label60;
    GtkWidget *alignment56;
    GtkWidget *table6;
    GtkWidget *titlestring_help_button;
    GtkWidget *image1;
    GtkWidget *label62;
    GtkWidget *label61;
    GtkWidget *alignment85;
    GtkWidget *label84;
    GtkWidget *alignment86;
    GtkWidget *hbox9;
    GtkWidget *vbox34;
    GtkWidget *image8;
    GtkWidget *titlestring_tag_menu = create_titlestring_tag_menu();
    GtkWidget * numbers_alignment, * numbers;

    vbox5 = gtk_vbox_new (FALSE, 0);
    gtk_container_add ((GtkContainer *) category_notebook, vbox5);

    create_widgets(GTK_BOX(vbox5), playlist_page_widgets, G_N_ELEMENTS(playlist_page_widgets));

    alignment55 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox5), alignment55, FALSE, FALSE, 0);
    gtk_alignment_set_padding ((GtkAlignment *) alignment55, 12, 3, 0, 0);

    label60 = gtk_label_new (_("<b>Song Display</b>"));
    gtk_container_add (GTK_CONTAINER (alignment55), label60);
    gtk_label_set_use_markup (GTK_LABEL (label60), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label60), 0, 0.5);

    numbers_alignment = gtk_alignment_new (0, 0, 0, 0);
    gtk_alignment_set_padding ((GtkAlignment *) numbers_alignment, 0, 0, 12, 0);
    gtk_box_pack_start ((GtkBox *) vbox5, numbers_alignment, 0, 0, 3);

    numbers = gtk_check_button_new_with_label (_("Show song numbers"));
    gtk_toggle_button_set_active ((GtkToggleButton *) numbers,
     get_bool (NULL, "show_numbers_in_pl"));
    g_signal_connect ((GObject *) numbers, "toggled", (GCallback)
     show_numbers_cb, 0);
    gtk_container_add ((GtkContainer *) numbers_alignment, numbers);

    numbers_alignment = gtk_alignment_new (0, 0, 0, 0);
    gtk_alignment_set_padding ((GtkAlignment *) numbers_alignment, 0, 0, 12, 0);
    gtk_box_pack_start ((GtkBox *) vbox5, numbers_alignment, 0, 0, 3);

    numbers = gtk_check_button_new_with_label (_("Show leading zeroes (02:00 "
     "instead of 2:00)"));
    gtk_toggle_button_set_active ((GtkToggleButton *) numbers, get_bool (NULL, "leading_zero"));
    g_signal_connect ((GObject *) numbers, "toggled", (GCallback)
     leading_zero_cb, 0);
    gtk_container_add ((GtkContainer *) numbers_alignment, numbers);

    alignment56 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox5), alignment56, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment56), 0, 0, 12, 0);

    table6 = gtk_table_new (2, 3, FALSE);
    gtk_container_add (GTK_CONTAINER (alignment56), table6);
    gtk_table_set_row_spacings (GTK_TABLE (table6), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table6), 12);

    titlestring_help_button = gtk_button_new ();
    gtk_table_attach (GTK_TABLE (table6), titlestring_help_button, 2, 3, 1, 2,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);

    gtk_widget_set_can_focus (titlestring_help_button, FALSE);
    gtk_widget_set_tooltip_text (titlestring_help_button, _("Show information about titlestring format"));
    gtk_button_set_relief (GTK_BUTTON (titlestring_help_button), GTK_RELIEF_HALF);
    gtk_button_set_focus_on_click (GTK_BUTTON (titlestring_help_button), FALSE);

    image1 = gtk_image_new_from_stock ("gtk-index", GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (titlestring_help_button), image1);

    GtkWidget * titlestring_cbox;
    create_titlestring_widgets (& titlestring_cbox, & titlestring_entry);
    gtk_table_attach (GTK_TABLE (table6), titlestring_cbox, 1, 3, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_table_attach (GTK_TABLE (table6), titlestring_entry, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    label62 = gtk_label_new (_("Custom string:"));
    gtk_table_attach (GTK_TABLE (table6), label62, 0, 1, 1, 2,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify (GTK_LABEL (label62), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment (GTK_MISC (label62), 1, 0.5);

    label61 = gtk_label_new (_("Title format:"));
    gtk_table_attach (GTK_TABLE (table6), label61, 0, 1, 0, 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify (GTK_LABEL (label61), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment (GTK_MISC (label61), 1, 0.5);

    alignment85 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox5), alignment85, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment85), 12, 12, 0, 0);

    label84 = gtk_label_new (_("<b>Popup Information</b>"));
    gtk_container_add (GTK_CONTAINER (alignment85), label84);
    gtk_label_set_use_markup (GTK_LABEL (label84), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label84), 0, 0.5);

    alignment86 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox5), alignment86, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment86), 0, 0, 12, 0);

    hbox9 = gtk_hbox_new (FALSE, 12);
    gtk_container_add (GTK_CONTAINER (alignment86), hbox9);

    vbox34 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox9), vbox34, TRUE, TRUE, 0);

    filepopupbutton = gtk_check_button_new_with_mnemonic (_("Show popup information for playlist entries"));
    gtk_widget_set_tooltip_text (filepopupbutton, _("Toggles popup information window for the pointed entry in the playlist. The window shows title of song, name of album, genre, year of publish, track number, track length, and artwork."));
    gtk_toggle_button_set_active ((GtkToggleButton *) filepopupbutton,
     get_bool (NULL, "show_filepopup_for_tuple"));
    gtk_box_pack_start ((GtkBox *) vbox34, filepopupbutton, TRUE, FALSE, 0);

    filepopup_settings_button = gtk_button_new ();
    gtk_widget_set_sensitive (filepopup_settings_button,
     get_bool (NULL, "show_filepopup_for_tuple"));
    gtk_box_pack_start (GTK_BOX (hbox9), filepopup_settings_button, FALSE, FALSE, 0);

    gtk_widget_set_can_focus (filepopup_settings_button, FALSE);
    gtk_widget_set_tooltip_text (filepopup_settings_button, _("Edit settings for popup information"));
    gtk_button_set_relief (GTK_BUTTON (filepopup_settings_button), GTK_RELIEF_HALF);

    image8 = gtk_image_new_from_stock ("gtk-properties", GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (filepopup_settings_button), image8);



    g_signal_connect (filepopupbutton, "toggled",
                     G_CALLBACK(on_show_filepopup_toggled),
                     NULL);
    g_signal_connect(G_OBJECT(filepopup_settings_button), "clicked",
                     G_CALLBACK(on_filepopup_settings_clicked),
                     NULL);

    g_signal_connect(titlestring_help_button, "clicked",
                     G_CALLBACK(on_titlestring_help_button_clicked),
                     titlestring_tag_menu);

    /* Create window for filepopup settings */
    create_filepopup_settings();
}

static GtkWidget * output_config_button, * output_about_button;

static bool_t output_enum_cb (PluginHandle * plugin, GList * * list)
{
    * list = g_list_prepend (* list, plugin);
    return TRUE;
}

static GList * output_get_list (void)
{
    static GList * list = NULL;

    if (list == NULL)
    {
        plugin_for_each (PLUGIN_TYPE_OUTPUT, (PluginForEachFunc) output_enum_cb,
         & list);
        list = g_list_reverse (list);
    }

    return list;
}

static void output_combo_update (GtkComboBox * combo)
{
    PluginHandle * plugin = plugin_get_current (PLUGIN_TYPE_OUTPUT);
    gtk_combo_box_set_active (combo, g_list_index (output_get_list (), plugin));
    gtk_widget_set_sensitive (output_config_button, plugin_has_configure (plugin));
    gtk_widget_set_sensitive (output_about_button, plugin_has_about (plugin));
}

static void output_combo_changed (GtkComboBox * combo)
{
    PluginHandle * plugin = g_list_nth_data (output_get_list (),
     gtk_combo_box_get_active (combo));
    g_return_if_fail (plugin != NULL);

    plugin_enable (plugin, TRUE);
    output_combo_update (combo);
}

static void output_combo_fill (GtkComboBox * combo)
{
    for (GList * node = output_get_list (); node != NULL; node = node->next)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) combo,
         plugin_get_name (node->data));
}

static void output_do_config (void)
{
    OutputPlugin * op = plugin_get_header (output_plugin_get_current ());
    g_return_if_fail (op != NULL);
    if (op->configure != NULL)
        op->configure ();
}

static void output_do_about (void)
{
    OutputPlugin * op = plugin_get_header (output_plugin_get_current ());
    g_return_if_fail (op != NULL);
    if (op->about != NULL)
        op->about ();
}

static void * create_output_plugin_box (void)
{
    GtkWidget * hbox1 = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) hbox1, gtk_label_new (_("Output plugin:")), FALSE, FALSE, 0);

    GtkWidget * vbox = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) hbox1, vbox, FALSE, FALSE, 0);

    GtkWidget * hbox2 = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox2, FALSE, FALSE, 0);

    GtkWidget * output_plugin_cbox = gtk_combo_box_text_new ();
    gtk_box_pack_start ((GtkBox *) hbox2, output_plugin_cbox, FALSE, FALSE, 0);

    GtkWidget * hbox3 = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox3, FALSE, FALSE, 0);

    output_config_button = gtk_button_new_from_stock (GTK_STOCK_PREFERENCES);
    gtk_box_pack_start ((GtkBox *) hbox3, output_config_button, FALSE, FALSE, 0);

    output_about_button = gtk_button_new_from_stock (GTK_STOCK_ABOUT);
    gtk_box_pack_start ((GtkBox *) hbox3, output_about_button, FALSE, FALSE, 0);

    output_combo_fill ((GtkComboBox *) output_plugin_cbox);
    output_combo_update ((GtkComboBox *) output_plugin_cbox);

    g_signal_connect (output_plugin_cbox, "changed", (GCallback) output_combo_changed, NULL);
    g_signal_connect (output_config_button, "clicked", (GCallback) output_do_config, NULL);
    g_signal_connect (output_about_button, "clicked", (GCallback) output_do_about, NULL);

    return hbox1;
}

static void create_audio_category (void)
{
    GtkWidget * audio_page_vbox = gtk_vbox_new (FALSE, 0);
    create_widgets ((GtkBox *) audio_page_vbox, audio_page_widgets, G_N_ELEMENTS (audio_page_widgets));
    gtk_container_add ((GtkContainer *) category_notebook, audio_page_vbox);
}

static void
create_connectivity_category(void)
{
    GtkWidget *connectivity_page_vbox;
    GtkWidget *vbox29;

    connectivity_page_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (category_notebook), connectivity_page_vbox);

    vbox29 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (connectivity_page_vbox), vbox29, TRUE, TRUE, 0);

    create_widgets(GTK_BOX(vbox29), connectivity_page_widgets, G_N_ELEMENTS(connectivity_page_widgets));
}

static void create_plugin_category (void)
{
    GtkWidget * notebook = gtk_notebook_new ();
    gtk_container_add ((GtkContainer *) category_notebook, notebook);

    int types[] = {PLUGIN_TYPE_TRANSPORT, PLUGIN_TYPE_PLAYLIST,
     PLUGIN_TYPE_INPUT, PLUGIN_TYPE_EFFECT, PLUGIN_TYPE_VIS, PLUGIN_TYPE_GENERAL};
    const char * names[] = {N_("Transport"), N_("Playlist"), N_("Input"),
     N_("Effect"), N_("Visualization"), N_("General")};

    for (int i = 0; i < G_N_ELEMENTS (types); i ++)
        gtk_notebook_append_page ((GtkNotebook *) notebook, plugin_view_new
         (types[i]), gtk_label_new (_(names[i])));
}

static bool_t
prefswin_destroy(GtkWidget *window, GdkEvent *event, gpointer data)
{
    prefswin = NULL;
    category_notebook = NULL;
    gtk_widget_destroy(filepopup_settings);
    filepopup_settings = NULL;
    gtk_widget_destroy(window);
    return TRUE;
}

/* GtkWidget * * create_prefs_window (void) */
void * * create_prefs_window (void)
{
    char *aud_version_string;

    GtkWidget *vbox;
    GtkWidget *hbox1;
    GtkWidget *scrolledwindow6;
    GtkWidget *hseparator1;
    GtkWidget *hbox4;
    GtkWidget *audversionlabel;
    GtkWidget *prefswin_button_box;
    GtkWidget *hbox11;
    GtkWidget *image10;
    GtkWidget *close;
    GtkAccelGroup *accel_group;

    accel_group = gtk_accel_group_new ();

    prefswin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint (GTK_WINDOW (prefswin), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_container_set_border_width (GTK_CONTAINER (prefswin), 12);
    gtk_window_set_title (GTK_WINDOW (prefswin), _("Audacious Preferences"));
    gtk_window_set_position (GTK_WINDOW (prefswin), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size (GTK_WINDOW (prefswin), 680, 400);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (prefswin), vbox);

    hbox1 = gtk_hbox_new (FALSE, 8);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, TRUE, TRUE, 0);

    scrolledwindow6 = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX (hbox1), scrolledwindow6, FALSE, FALSE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow6), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow6), GTK_SHADOW_IN);

    category_treeview = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolledwindow6), category_treeview);
    gtk_widget_set_size_request (scrolledwindow6, 168, -1);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (category_treeview), FALSE);

    category_notebook = gtk_notebook_new ();
    gtk_box_pack_start (GTK_BOX (hbox1), category_notebook, TRUE, TRUE, 0);

    gtk_widget_set_can_focus (category_notebook, FALSE);
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (category_notebook), FALSE);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (category_notebook), FALSE);
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (category_notebook), TRUE);

    create_audio_category();
    create_connectivity_category();
    create_playlist_category();
    create_plugin_category();

    hseparator1 = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), hseparator1, FALSE, FALSE, 6);

    hbox4 = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox4, FALSE, FALSE, 0);

    audversionlabel = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (hbox4), audversionlabel, FALSE, FALSE, 0);
    gtk_label_set_use_markup (GTK_LABEL (audversionlabel), TRUE);

    prefswin_button_box = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (hbox4), prefswin_button_box, TRUE, TRUE, 0);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (prefswin_button_box), GTK_BUTTONBOX_END);
    gtk_box_set_spacing (GTK_BOX (prefswin_button_box), 6);

    hbox11 = gtk_hbox_new (FALSE, 2);

    image10 = gtk_image_new_from_stock ("gtk-refresh", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start (GTK_BOX (hbox11), image10, FALSE, FALSE, 0);

    close = gtk_button_new_from_stock ("gtk-close");
    gtk_container_add (GTK_CONTAINER (prefswin_button_box), close);
    gtk_widget_set_can_default(close, TRUE);
    gtk_widget_add_accelerator (close, "clicked", accel_group,
                                GDK_Escape, (GdkModifierType) 0,
                                GTK_ACCEL_VISIBLE);


    gtk_window_add_accel_group (GTK_WINDOW (prefswin), accel_group);

    /* connect signals */
    g_signal_connect(G_OBJECT(prefswin), "delete_event",
                     G_CALLBACK(prefswin_destroy),
                     NULL);
    g_signal_connect_swapped(G_OBJECT(close), "clicked",
                             G_CALLBACK(prefswin_destroy),
                             prefswin);

    /* create category view */
    fill_category_list ((GtkTreeView *) category_treeview, (GtkNotebook *) category_notebook);

    /* audacious version label */

    aud_version_string = g_strdup_printf
     ("<span size='small'>%s (%s)</span>", "Audacious " VERSION, BUILDSTAMP);

    gtk_label_set_markup( GTK_LABEL(audversionlabel) , aud_version_string );
    g_free(aud_version_string);
    gtk_widget_show_all(vbox);

    return & prefswin;
}

void
destroy_prefs_window(void)
{
    prefswin_destroy(prefswin, NULL, NULL);
}

void show_prefs_window (void)
{
    if (! prefswin)
        create_prefs_window ();

    gtk_window_present ((GtkWindow *) prefswin);
}

void
hide_prefs_window(void)
{
    g_return_if_fail(prefswin);
    gtk_widget_hide(GTK_WIDGET(prefswin));
}

static void prefswin_page_queue_new (GtkWidget * container, const char * name,
 const char * imgurl)
{
    CategoryQueueEntry *ent = g_new0(CategoryQueueEntry, 1);

    ent->container = container;
    ent->pg_name = name;
    ent->img_url = imgurl;

    if (category_queue)
        ent->next = category_queue;

    category_queue = ent;
}

static void
prefswin_page_queue_destroy(CategoryQueueEntry *ent)
{
    category_queue = ent->next;
    g_free(ent);
}

/*
 * Public APIs for adding new pages to the prefs window.
 *
 * Basically, the concept here is that third party components can register themselves in the root
 * preferences window.
 *
 * From a usability standpoint this makes the application look more "united", instead of cluttered
 * and malorganised. Hopefully this option will be used further in the future.
 *
 *    - nenolod
 */
/* int prefswin_page_new (GtkWidget * container, const char * name,
 const char * imgurl) */
int prefswin_page_new (void * container, const char * name, const char *
 imgurl)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GdkPixbuf *img = NULL;
    GtkTreeView *treeview = GTK_TREE_VIEW(category_treeview);
    int id;

    if (treeview == NULL || category_notebook == NULL)
    {
        prefswin_page_queue_new(container, name, imgurl);
        return -1;
    }

    model = gtk_tree_view_get_model(treeview);

    if (model == NULL)
    {
        prefswin_page_queue_new(container, name, imgurl);
        return -1;
    }

    /* Make sure the widgets are visible. */
    gtk_widget_show(container);
    id = gtk_notebook_append_page(GTK_NOTEBOOK(category_notebook), container, NULL);

    if (id == -1)
        return -1;

    if (imgurl != NULL)
        img = gdk_pixbuf_new_from_file(imgurl, NULL);

    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       CATEGORY_VIEW_COL_ICON, img,
                       CATEGORY_VIEW_COL_NAME,
                       name, CATEGORY_VIEW_COL_ID, id, -1);

    if (img != NULL)
        g_object_unref(img);

    return id;
}

void
prefswin_page_destroy(GtkWidget *container)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeView *treeview = GTK_TREE_VIEW(category_treeview);
    bool_t ret;
    int id;
    int index = -1;

    if (category_notebook == NULL || treeview == NULL || container == NULL)
        return;

    id = gtk_notebook_page_num(GTK_NOTEBOOK(category_notebook), container);

    if (id == -1)
        return;

    gtk_notebook_remove_page(GTK_NOTEBOOK(category_notebook), id);

    model = gtk_tree_view_get_model(treeview);

    if (model == NULL)
        return;

    ret = gtk_tree_model_get_iter_first(model, &iter);

    while (ret == TRUE)
    {
        gtk_tree_model_get(model, &iter, CATEGORY_VIEW_COL_ID, &index, -1);

        if (index == id)
        {
            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            ret = gtk_tree_model_get_iter_first(model, &iter);
            continue;
        }

        if (index > id)
        {
            index--;
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, CATEGORY_VIEW_COL_ID, index, -1);
        }

        ret = gtk_tree_model_iter_next(model, &iter);
    }
}

static void sw_volume_toggled (void)
{
    int vol[2];

    if (get_bool (NULL, "software_volume_control"))
    {
        vol[0] = get_int (NULL, "sw_volume_left");
        vol[1] = get_int (NULL, "sw_volume_right");
    }
    else
        playback_get_volume (& vol[0], & vol[1]);

    hook_call ("volume set", vol);
}
