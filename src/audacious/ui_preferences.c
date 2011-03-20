/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2010  Audacious development team.
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

#include "audconfig.h"
#include "config.h"
#include "configdb.h"
#include "debug.h"
#include "glib-compat.h"
#include "gtk-compat.h"
#include "i18n.h"
#include "misc.h"
#include "output.h"
#include "playback.h"
#include "playlist.h"
#include "playlist-utils.h"
#include "plugin.h"
#include "plugins.h"
#include "preferences.h"
#include "ui_preferences.h"

#define TITLESTRING_UPDATE_TIMEOUT 3

static void sw_volume_toggled (void);

enum CategoryViewCols {
    CATEGORY_VIEW_COL_ICON,
    CATEGORY_VIEW_COL_NAME,
    CATEGORY_VIEW_COL_ID,
    CATEGORY_VIEW_N_COLS
};

typedef struct {
    const gchar *icon_path;
    const gchar *name;
} Category;

typedef struct {
    const gchar *name;
    const gchar *tag;
} TitleFieldTag;

static /* GtkWidget * */ void * prefswin = NULL;
static GtkWidget *filepopup_settings = NULL;
static GtkWidget *category_treeview = NULL;
static GtkWidget *category_notebook = NULL;
GtkWidget *filepopupbutton = NULL;

/* filepopup settings widgets */
GtkWidget *filepopup_settings_cover_name_include;
GtkWidget *filepopup_settings_cover_name_exclude;
GtkWidget *filepopup_settings_recurse_for_cover;
GtkWidget *filepopup_settings_recurse_for_cover_depth;
GtkWidget *filepopup_settings_recurse_for_cover_depth_box;
GtkWidget *filepopup_settings_use_file_cover;
GtkWidget *filepopup_settings_showprogressbar;
GtkWidget *filepopup_settings_delay;

/* prefswin widgets */
GtkWidget *titlestring_entry;
GtkWidget *filepopup_for_tuple_settings_button;
static gint titlestring_timeout_counter = 0;

static Category categories[] = {
 {"audio.png", N_("Audio")},
 {"replay_gain.png", N_("Replay Gain")},
 {"connectivity.png", N_("Network")},
 {"playlist.png", N_("Playlist")},
 {"plugins.png", N_("Plugins")},
};

static gint n_categories = G_N_ELEMENTS(categories);

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
static const guint n_title_field_tags = G_N_ELEMENTS(title_field_tags);

static ComboBoxElements chardet_detector_presets[] = {
    { N_("None")     , N_("None") },
    { N_("Japanese") , N_("Japanese") },
    { N_("Taiwanese"), N_("Taiwanese") },
    { N_("Chinese")  , N_("Chinese") },
    { N_("Korean")   , N_("Korean") },
    { N_("Russian")  , N_("Russian") },
    { N_("Greek")    , N_("Greek") },
    { N_("Hebrew")   , N_("Hebrew") },
    { N_("Turkish")  , N_("Turkish") },
    { N_("Arabic")   , N_("Arabic") },
    { N_("Polish")   , N_("Polish") },
    { N_("Baltic")   , N_("Baltic") },
    { N_("Universal"), N_("Universal") }
};

static ComboBoxElements bitdepth_elements[] = {
    { GINT_TO_POINTER(16), "16" },
    { GINT_TO_POINTER(24), "24" },
    { GINT_TO_POINTER(32), "32" },
    {GINT_TO_POINTER (0), "Floating point"},
};

typedef struct {
    void *next;
    GtkWidget *container;
    const gchar * pg_name;
    const gchar * img_url;
} CategoryQueueEntry;

CategoryQueueEntry *category_queue = NULL;

static PreferencesWidget audio_page_widgets[] = {
    {WIDGET_LABEL, N_("<b>Bit Depth</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_COMBO_BOX, N_("Output bit depth:"), &cfg.output_bit_depth, NULL,
                       N_("All streams will be converted to this bit depth.\n"
                          "This should be the max supported bit depth of\nthe sound card or output plugin."), FALSE,
                       {.combo = {bitdepth_elements, G_N_ELEMENTS(bitdepth_elements), TRUE}}, VALUE_INT},
    {WIDGET_LABEL, N_("<b>Volume Control</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Use software volume control"),
     & cfg.software_volume_control, sw_volume_toggled,
                     N_("Use software volume control. This may be useful for situations where your audio system does not support controlling the playback volume."), FALSE},
};

static PreferencesWidget rg_params_elements[] =
{{WIDGET_SPIN_BTN, N_("Amplify all files:"), & cfg.replay_gain_preamp, NULL,
 NULL, FALSE, {.spin_btn = {-15, 15, 0.01, N_("dB")}}, VALUE_FLOAT},
{WIDGET_SPIN_BTN, N_("Amplify untagged files:"), & cfg.default_gain, NULL,
 NULL, FALSE, {.spin_btn = {-15, 15, 0.01, N_("dB")}}, VALUE_FLOAT}};

static PreferencesWidget replay_gain_page_widgets[] =
 {{WIDGET_LABEL, N_("<b>Replay Gain</b>"), NULL, NULL, NULL, FALSE},
 {WIDGET_CHK_BTN, N_("Enable Replay Gain"), &cfg.enable_replay_gain, NULL,
  NULL, FALSE},
 {WIDGET_LABEL, N_("<b>Mode</b>"), NULL, NULL, NULL, TRUE},
 {WIDGET_RADIO_BTN, N_("Single track mode"), &cfg.replay_gain_track, NULL,
  NULL, TRUE},
 {WIDGET_RADIO_BTN, N_("Album mode"), &cfg.replay_gain_album, NULL, NULL,
  TRUE},
 {WIDGET_LABEL, N_("<b>Adjust Levels</b>"), NULL, NULL, NULL, TRUE},
 {WIDGET_TABLE, NULL, NULL, NULL, NULL, TRUE, {.table = {rg_params_elements,
  G_N_ELEMENTS (rg_params_elements)}}},
 {WIDGET_LABEL, N_("<b>Clipping Prevention</b>"), NULL, NULL, NULL, TRUE},
 {WIDGET_CHK_BTN, N_("Enable clipping prevention"),
  & cfg.enable_clipping_prevention, NULL, NULL, TRUE}};

static PreferencesWidget proxy_host_port_elements[] = {
    {WIDGET_ENTRY, N_("Proxy hostname:"), "proxy_host", NULL, NULL, FALSE, {.entry = {FALSE}}, VALUE_CFG_STRING},
    {WIDGET_ENTRY, N_("Proxy port:"), "proxy_port", NULL, NULL, FALSE, {.entry = {FALSE}}, VALUE_CFG_STRING},
};

static PreferencesWidget proxy_auth_elements[] = {
    {WIDGET_ENTRY, N_("Proxy username:"), "proxy_user", NULL, NULL, FALSE, {.entry = {FALSE}}, VALUE_CFG_STRING},
    {WIDGET_ENTRY, N_("Proxy password:"), "proxy_pass", NULL, NULL, FALSE, {.entry = {TRUE}}, VALUE_CFG_STRING},
};

static PreferencesWidget connectivity_page_widgets[] = {
    {WIDGET_LABEL, N_("<b>Proxy Configuration</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Enable proxy usage"), "use_proxy", NULL, NULL, FALSE,
     .cfg_type = VALUE_CFG_BOOLEAN},
    {WIDGET_TABLE, NULL, NULL, NULL, NULL, TRUE, {.table = {proxy_host_port_elements, G_N_ELEMENTS(proxy_host_port_elements)}}},
    {WIDGET_CHK_BTN, N_("Use authentication with proxy"), "proxy_use_auth",
     NULL, NULL, FALSE, .cfg_type = VALUE_CFG_BOOLEAN},
    {WIDGET_TABLE, NULL, NULL, NULL, NULL, TRUE, {.table = {proxy_auth_elements, G_N_ELEMENTS(proxy_auth_elements)}}},
    {WIDGET_LABEL, N_("<span size=\"small\">Changing these settings will require a restart of Audacious.</span>"), NULL, NULL, NULL, FALSE, {.label = {"gtk-dialog-warning"}}},
};

static PreferencesWidget chardet_elements[] = {
    {WIDGET_COMBO_BOX, N_("Auto character encoding detector for:"), &cfg.chardet_detector, NULL, NULL, TRUE,
        {.combo = {chardet_detector_presets, G_N_ELEMENTS(chardet_detector_presets),
                   #ifdef USE_CHARDET
                   TRUE
                   #else
                   FALSE
                   #endif
                   }}, VALUE_STRING},
    {WIDGET_ENTRY, N_("Fallback character encodings:"), &cfg.chardet_fallback, aud_config_chardet_update, N_("List of character encodings used for fall back conversion of metadata. If automatic character encoding detector failed or has been disabled, encodings in this list would be treated as candidates of the encoding of metadata, and fall back conversion from these encodings to UTF-8 would be attempted."), TRUE, {.entry = {FALSE}}, VALUE_STRING},
};

static PreferencesWidget playlist_page_widgets[] = {
    {WIDGET_LABEL, N_("<b>Behavior</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Continue playback on startup"),
     & cfg.resume_playback_on_startup, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Advance when the current song is deleted"),
     & cfg.advance_on_delete, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Clear the playlist when opening files"),
     & cfg.clear_playlist, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Open files in a temporary playlist"),
     & cfg.open_to_temporary, NULL, NULL, FALSE},
    {WIDGET_LABEL, N_("<b>Metadata</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_TABLE, NULL, NULL, NULL, NULL, TRUE, {.table = {chardet_elements, G_N_ELEMENTS(chardet_elements)}}},
};

static void prefswin_page_queue_destroy(CategoryQueueEntry *ent);

static void
change_category(GtkNotebook * notebook,
                GtkTreeSelection * selection)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint index;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, CATEGORY_VIEW_COL_ID, &index, -1);
    gtk_notebook_set_current_page(notebook, index);
}

static void
editable_insert_text(GtkEditable * editable,
                     const gchar * text,
                     gint * pos)
{
    gtk_editable_insert_text(editable, text, strlen(text), pos);
}

static void
titlestring_tag_menu_callback(GtkMenuItem * menuitem,
                              gpointer data)
{
    const gchar *separator = " - ";
    gint item = GPOINTER_TO_INT(data);
    gint pos;

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


static void
on_titlestring_entry_realize(GtkWidget * entry,
                             gpointer data)
{
    gtk_entry_set_text(GTK_ENTRY(entry), cfg.gentitle_format);
}

static gboolean
titlestring_timeout_proc (gpointer data)
{
    titlestring_timeout_counter--;

    if(titlestring_timeout_counter <= 0) {
        titlestring_timeout_counter = 0;
        playlist_reformat_titles ();
        return FALSE;
    } else {
        return TRUE;
    }
}

static void
on_titlestring_entry_changed(GtkWidget * entry,
                             gpointer data)
{
    g_free(cfg.gentitle_format);
    cfg.gentitle_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    if(titlestring_timeout_counter == 0) {
        g_timeout_add_seconds (1, (GSourceFunc) titlestring_timeout_proc, NULL);
    }

    titlestring_timeout_counter = TITLESTRING_UPDATE_TIMEOUT;
}

static void
on_titlestring_cbox_realize(GtkWidget * cbox,
                            gpointer data)
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(cbox), cfg.titlestring_preset);
    gtk_widget_set_sensitive(GTK_WIDGET(data),
                             (cfg.titlestring_preset == (gint)n_titlestring_presets));
}

static void
on_titlestring_cbox_changed(GtkWidget * cbox,
                            gpointer data)
{
    gint position = gtk_combo_box_get_active(GTK_COMBO_BOX(cbox));

    cfg.titlestring_preset = position;
    gtk_widget_set_sensitive(GTK_WIDGET(data), (position == 6));

    playlist_reformat_titles ();
}

static void
on_font_btn_realize(GtkFontButton * button, gchar **cfg)
{
    gtk_font_button_set_font_name(button, *cfg);
}

static void
on_font_btn_font_set(GtkFontButton * button, gchar **config)
{
    g_free(*config);
    *config = g_strdup(gtk_font_button_get_font_name(button));
    AUDDBG("Returned font name: \"%s\"\n", *config);
    void (*callback) (void) = g_object_get_data(G_OBJECT(button), "callback");
    if (callback != NULL) callback();
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

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title(GTK_WINDOW(window), _(settings->title));
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(plugin_preferences_destroy), settings);

    vbox = gtk_vbox_new(FALSE, 10);
    create_widgets(GTK_BOX(vbox), settings->prefs, settings->n_prefs);
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

static void
on_output_plugin_bufsize_realize(GtkSpinButton *button,
                                 gpointer data)
{
    gtk_spin_button_set_value(button, cfg.output_buffer_size);
}

static void
on_output_plugin_bufsize_value_changed(GtkSpinButton *button,
                                       gpointer data)
{
    cfg.output_buffer_size = gtk_spin_button_get_value_as_int(button);
}

static void
on_spin_btn_realize_gint(GtkSpinButton *button, gint *cfg)
{
    gtk_spin_button_set_value(button, *cfg);
}

static void
on_spin_btn_changed_gint(GtkSpinButton *button, gint *cfg)
{
    *cfg = gtk_spin_button_get_value_as_int(button);
}

static void
on_spin_btn_realize_gfloat(GtkSpinButton *button, gfloat *cfg)
{
     gtk_spin_button_set_value(button, (gdouble) *cfg);
}

static void
on_spin_btn_changed_gfloat(GtkSpinButton *button, gfloat *cfg)
{
    *cfg = (gfloat) gtk_spin_button_get_value(button);
}


static void
on_category_treeview_realize(GtkTreeView * treeview,
                             GtkNotebook * notebook)
{
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GdkPixbuf *img;
    CategoryQueueEntry *qlist;
    gint i;

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

    gint width, height;
    gtk_widget_get_size_request(GTK_WIDGET(treeview), &width, &height);
    g_object_set(G_OBJECT(renderer), "wrap-width", width - 64 - 24, "wrap-mode",
     PANGO_WRAP_WORD_CHAR, NULL);

    store = gtk_list_store_new(CATEGORY_VIEW_N_COLS,
                               GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));

    for (i = 0; i < n_categories; i ++)
    {
        gchar * path = g_strdup_printf ("%s/images/%s",
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

static void
on_show_filepopup_for_tuple_realize(GtkToggleButton * button, gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.show_filepopup_for_tuple);
    filepopupbutton = GTK_WIDGET(button);

    gtk_widget_set_sensitive(filepopup_for_tuple_settings_button, cfg.show_filepopup_for_tuple);
}

static void
on_show_filepopup_for_tuple_toggled(GtkToggleButton * button, gpointer data)
{
    cfg.show_filepopup_for_tuple = gtk_toggle_button_get_active(button);

    gtk_widget_set_sensitive(filepopup_for_tuple_settings_button, cfg.show_filepopup_for_tuple);
}

static void
on_recurse_for_cover_toggled(GtkToggleButton *button, gpointer data)
{
    gtk_widget_set_sensitive(GTK_WIDGET(data),
                             gtk_toggle_button_get_active(button));
}

static void
on_filepopup_for_tuple_settings_clicked(GtkButton *button, gpointer data)
{
    gtk_entry_set_text(GTK_ENTRY(filepopup_settings_cover_name_include), cfg.cover_name_include);
    gtk_entry_set_text(GTK_ENTRY(filepopup_settings_cover_name_exclude), cfg.cover_name_exclude);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filepopup_settings_recurse_for_cover), cfg.recurse_for_cover);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(filepopup_settings_recurse_for_cover_depth), cfg.recurse_for_cover_depth);
    on_recurse_for_cover_toggled(GTK_TOGGLE_BUTTON(filepopup_settings_recurse_for_cover), filepopup_settings_recurse_for_cover_depth_box);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filepopup_settings_use_file_cover), cfg.use_file_cover);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(filepopup_settings_showprogressbar), cfg.filepopup_showprogressbar);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(filepopup_settings_delay), cfg.filepopup_delay);

    gtk_widget_show(filepopup_settings);
}

static void
on_filepopup_settings_ok_clicked(GtkButton *button, gpointer data)
{
    g_free(cfg.cover_name_include);
    cfg.cover_name_include = g_strdup(gtk_entry_get_text(GTK_ENTRY(filepopup_settings_cover_name_include)));

    g_free(cfg.cover_name_exclude);
    cfg.cover_name_exclude = g_strdup(gtk_entry_get_text(GTK_ENTRY(filepopup_settings_cover_name_exclude)));

    cfg.recurse_for_cover = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filepopup_settings_recurse_for_cover));
    cfg.recurse_for_cover_depth = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(filepopup_settings_recurse_for_cover_depth));
    cfg.use_file_cover = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filepopup_settings_use_file_cover));
    cfg.filepopup_showprogressbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(filepopup_settings_showprogressbar));
    cfg.filepopup_delay = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(filepopup_settings_delay));

    gtk_widget_hide(filepopup_settings);
}

static void
on_filepopup_settings_cancel_clicked(GtkButton *button, gpointer data)
{
    gtk_widget_hide(filepopup_settings);
}

static void
on_toggle_button_toggled(GtkToggleButton * button, gboolean *cfg)
{
    *cfg = gtk_toggle_button_get_active(button);
    void (*callback) (void) = g_object_get_data(G_OBJECT(button), "callback");
    if (callback != NULL) callback();
    GtkWidget *child = g_object_get_data(G_OBJECT(button), "child");
    if (child) gtk_widget_set_sensitive(GTK_WIDGET(child), *cfg);
}

static void
on_toggle_button_realize(GtkToggleButton * button, gboolean *cfg)
{
    gtk_toggle_button_set_active(button, cfg ? *cfg : FALSE);
    GtkWidget *child = g_object_get_data(G_OBJECT(button), "child");
    if (child) gtk_widget_set_sensitive(GTK_WIDGET(child), cfg ? *cfg : FALSE);
}

static void
on_toggle_button_cfg_toggled(GtkToggleButton *button, gchar *cfg)
{
    g_return_if_fail(cfg != NULL);

    mcs_handle_t *db;
    gboolean ret = gtk_toggle_button_get_active(button);

    db = cfg_db_open();
    cfg_db_set_bool(db, NULL, cfg, ret);
    cfg_db_close(db);
}

static void
on_toggle_button_cfg_realize(GtkToggleButton *button, gchar *cfg)
{
    mcs_handle_t *db;
    gboolean ret;

    g_return_if_fail(cfg != NULL);

    db = cfg_db_open();

    if (cfg_db_get_bool(db, NULL, cfg, &ret) != FALSE)
        gtk_toggle_button_set_active(button, ret);

    cfg_db_close(db);
}

static void
on_entry_realize(GtkEntry *entry, gchar **cfg)
{
    g_return_if_fail(cfg != NULL);

    if (*cfg)
        gtk_entry_set_text(entry, *cfg);
}

static void
on_entry_changed(GtkEntry *entry, gchar **cfg)
{
    void (*callback) (void) = g_object_get_data(G_OBJECT(entry), "callback");
    const gchar *ret;

    g_return_if_fail(cfg != NULL);

    g_free(*cfg);

    ret = gtk_entry_get_text(entry);

    if (ret == NULL)
        *cfg = g_strdup("");
    else
        *cfg = g_strdup(ret);

    if (callback != NULL) callback();
}

static void
on_entry_cfg_realize(GtkEntry *entry, gchar *cfg)
{
    mcs_handle_t *db;
    gchar *ret;

    g_return_if_fail(cfg != NULL);

    db = cfg_db_open();

    if (cfg_db_get_string(db, NULL, cfg, &ret) != FALSE)
        gtk_entry_set_text(entry, ret);

    cfg_db_close(db);
}

static void
on_entry_cfg_changed(GtkEntry *entry, gchar *cfg)
{
    mcs_handle_t *db;
    gchar *ret = g_strdup(gtk_entry_get_text(entry));

    g_return_if_fail(cfg != NULL);

    db = cfg_db_open();
    cfg_db_set_string(db, NULL, cfg, ret);
    cfg_db_close(db);

    g_free(ret);
}

static void
on_cbox_changed_int(GtkComboBox * combobox, PreferencesWidget *widget)
{
    gint position = 0;

    position = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    *((gint *)widget->cfg) = GPOINTER_TO_INT(widget->data.combo.elements[position].value);
}

static void
on_cbox_changed_string(GtkComboBox * combobox, PreferencesWidget *widget)
{
    gint position = 0;

    position = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));

    g_free(*((gchar **)widget->cfg));

    *((gchar **)widget->cfg) = g_strdup(widget->data.combo.elements[position].value);
}

static void on_cbox_realize (GtkWidget * combobox, PreferencesWidget * widget)
{
    guint i=0,index=0;

    for (i = 0; i < widget->data.combo.n_elements; i ++)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) combobox,
         _(widget->data.combo.elements[i].label));

    if (widget->data.combo.enabled) {
        switch (widget->cfg_type) {
            case VALUE_INT:
                g_signal_connect(combobox, "changed",
                                 G_CALLBACK(on_cbox_changed_int), widget);
                for(i=0; i<widget->data.combo.n_elements; i++) {
                    if (GPOINTER_TO_INT(widget->data.combo.elements[i].value) == *((gint *) widget->cfg)) {
                        index = i;
                        break;
                    }
                }
                break;
            case VALUE_STRING:
                g_signal_connect(combobox, "changed",
                                 G_CALLBACK(on_cbox_changed_string), widget);
                for(i=0; i<widget->data.combo.n_elements; i++) {
                    if(!strcmp((gchar *)widget->data.combo.elements[i].value, *((gchar **)widget->cfg))) {
                        index = i;
                        break;
                    }
                }
                break;
            case VALUE_NULL:
                break;
            default:
                g_warning("Unhandled cbox value type");
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

    filepopup_settings_cover_name_include = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), filepopup_settings_cover_name_include, 1, 2, 0, 1,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_activates_default(GTK_ENTRY(filepopup_settings_cover_name_include), TRUE);

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

    filepopup_settings_cover_name_exclude = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), filepopup_settings_cover_name_exclude, 1, 2, 1, 2,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_activates_default(GTK_ENTRY(filepopup_settings_cover_name_exclude), TRUE);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);

    filepopup_settings_recurse_for_cover = gtk_check_button_new_with_mnemonic(_("Recursively search for cover"));
    gtk_container_add(GTK_CONTAINER(alignment), filepopup_settings_recurse_for_cover);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 45, 0);

    filepopup_settings_recurse_for_cover_depth_box = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), filepopup_settings_recurse_for_cover_depth_box);

    label_search_depth = gtk_label_new(_("Search depth: "));
    gtk_box_pack_start(GTK_BOX(filepopup_settings_recurse_for_cover_depth_box), label_search_depth, TRUE, TRUE, 0);
    gtk_misc_set_padding(GTK_MISC(label_search_depth), 4, 0);

    recurse_for_cover_depth_adj = (GtkAdjustment *) gtk_adjustment_new (0, 0,
     100, 1, 10, 0);
    filepopup_settings_recurse_for_cover_depth = gtk_spin_button_new(GTK_ADJUSTMENT(recurse_for_cover_depth_adj), 1, 0);
    gtk_box_pack_start(GTK_BOX(filepopup_settings_recurse_for_cover_depth_box), filepopup_settings_recurse_for_cover_depth, TRUE, TRUE, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(filepopup_settings_recurse_for_cover_depth), TRUE);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);

    filepopup_settings_use_file_cover = gtk_check_button_new_with_mnemonic(_("Use per-file cover"));
    gtk_container_add(GTK_CONTAINER(alignment), filepopup_settings_use_file_cover);

    label_misc = gtk_label_new(_("<b>Miscellaneous</b>"));
    gtk_box_pack_start(GTK_BOX(vbox), label_misc, FALSE, FALSE, 0);
    gtk_label_set_use_markup(GTK_LABEL(label_misc), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label_misc), 0, 0.5);

    alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);

    filepopup_settings_showprogressbar = gtk_check_button_new_with_mnemonic(_("Show Progress bar for the current track"));
    gtk_container_add(GTK_CONTAINER(alignment), filepopup_settings_showprogressbar);

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
    filepopup_settings_delay = gtk_spin_button_new(GTK_ADJUSTMENT(delay_adj), 1, 0);
    gtk_box_pack_start(GTK_BOX(hbox), filepopup_settings_delay, TRUE, TRUE, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(filepopup_settings_delay), TRUE);

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
                     G_CALLBACK(on_filepopup_settings_cancel_clicked),
                     NULL);
    g_signal_connect(G_OBJECT(btn_ok), "clicked",
                     G_CALLBACK(on_filepopup_settings_ok_clicked),
                     NULL);
    g_signal_connect(G_OBJECT(filepopup_settings_recurse_for_cover), "toggled",
                     G_CALLBACK(on_recurse_for_cover_toggled),
                     filepopup_settings_recurse_for_cover_depth_box);

    gtk_widget_grab_default(btn_ok);
    gtk_widget_show_all(vbox);
}

static void create_spin_button (PreferencesWidget * widget, GtkWidget * *
 label_pre, GtkWidget * * spin_btn, GtkWidget * * label_past, const gchar *
 domain)
{
     g_return_if_fail(widget->type == WIDGET_SPIN_BTN);

     * label_pre = gtk_label_new (dgettext (domain, widget->label));
     gtk_misc_set_alignment(GTK_MISC(*label_pre), 0, 0.5);
     gtk_misc_set_padding(GTK_MISC(*label_pre), 4, 0);

     *spin_btn = gtk_spin_button_new_with_range(widget->data.spin_btn.min,
                                                widget->data.spin_btn.max,
                                                widget->data.spin_btn.step);


     if (widget->tooltip)
         gtk_widget_set_tooltip_text (* spin_btn, dgettext (domain,
          widget->tooltip));

     if (widget->data.spin_btn.right_label) {
         * label_past = gtk_label_new (dgettext (domain,
          widget->data.spin_btn.right_label));
         gtk_misc_set_alignment(GTK_MISC(*label_past), 0, 0.5);
         gtk_misc_set_padding(GTK_MISC(*label_past), 4, 0);
     }

     switch (widget->cfg_type) {
         case VALUE_INT:
             g_signal_connect(G_OBJECT(*spin_btn), "value_changed",
                              G_CALLBACK(on_spin_btn_changed_gint),
                              widget->cfg);
             g_signal_connect(G_OBJECT(*spin_btn), "realize",
                              G_CALLBACK(on_spin_btn_realize_gint),
                              widget->cfg);
             break;
         case VALUE_FLOAT:
             g_signal_connect(G_OBJECT(*spin_btn), "value_changed",
                              G_CALLBACK(on_spin_btn_changed_gfloat),
                              widget->cfg);
             g_signal_connect(G_OBJECT(*spin_btn), "realize",
                              G_CALLBACK(on_spin_btn_realize_gfloat),
                              widget->cfg);
             break;
         case VALUE_NULL:
             break;
         default:
             g_warning("Unsupported value type for spin button");
     }
}

void create_font_btn (PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * font_btn, const gchar * domain)
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

    g_object_set_data ((GObject *) (* font_btn), "callback", (void *)
     widget->callback);

    g_signal_connect(G_OBJECT(*font_btn), "font_set",
                     G_CALLBACK(on_font_btn_font_set),
                     (gchar**)widget->cfg);
    g_signal_connect(G_OBJECT(*font_btn), "realize",
                     G_CALLBACK(on_font_btn_realize),
                     (gchar**)widget->cfg);
}

static void create_entry (PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * entry, const gchar * domain)
{
    *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(*entry), !widget->data.entry.password);

    if (widget->label)
        * label = gtk_label_new (dgettext (domain, widget->label));

    if (widget->tooltip)
        gtk_widget_set_tooltip_text (* entry, dgettext (domain, widget->tooltip));

    g_object_set_data ((GObject *) (* entry), "callback", (void *)
     widget->callback);

    switch (widget->cfg_type) {
        case VALUE_STRING:
            g_signal_connect(G_OBJECT(*entry), "realize",
                             G_CALLBACK(on_entry_realize),
                             widget->cfg);
            g_signal_connect(G_OBJECT(*entry), "changed",
                             G_CALLBACK(on_entry_changed),
                             widget->cfg);
            break;
        case VALUE_CFG_STRING:
            g_signal_connect(G_OBJECT(*entry), "realize",
                             G_CALLBACK(on_entry_cfg_realize),
                             widget->cfg);
            g_signal_connect(G_OBJECT(*entry), "changed",
                             G_CALLBACK(on_entry_cfg_changed),
                             widget->cfg);
            break;
        default:
            g_warning("Unhandled entry value type %d", widget->cfg_type);
    }
}

static void create_label (PreferencesWidget * widget, GtkWidget * * label,
 GtkWidget * * icon, const gchar * domain)
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
 GtkWidget * * combobox, const gchar * domain)
{
    * combobox = gtk_combo_box_text_new ();

    if (widget->label) {
        * label = gtk_label_new (dgettext (domain, widget->label));
        gtk_misc_set_alignment(GTK_MISC(*label), 1, 0.5);
    }

    g_signal_connect_after(G_OBJECT(*combobox), "realize",
                           G_CALLBACK(on_cbox_realize),
                           widget);
}

static void fill_table (GtkWidget * table, PreferencesWidget * elements, gint
 amt, const gchar * domain)
{
    gint x;
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
 gint amt, const gchar * domain) */
void create_widgets_with_domain (void * box, PreferencesWidget * widgets, gint
 amt, const gchar * domain)
{
    gint x;
    GtkWidget *alignment = NULL, *widget = NULL;
    GtkWidget *child_box = NULL;
    GSList *radio_btn_group = NULL;

    for (x = 0; x < amt; ++x) {
        if (widgets[x].child) { /* perhaps this logic can be better */
            if (!child_box) {
                child_box = gtk_vbox_new(FALSE, 0);
                g_object_set_data(G_OBJECT(widget), "child", child_box);
                alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
                gtk_box_pack_start(box, alignment, FALSE, FALSE, 0);
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 24, 0);
                gtk_container_add (GTK_CONTAINER (alignment), child_box);
            }
        } else
            child_box = NULL;

        alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
        gtk_box_pack_start(child_box ? GTK_BOX(child_box) : box, alignment, FALSE, FALSE, 0);

        if (radio_btn_group && widgets[x].type != WIDGET_RADIO_BTN)
            radio_btn_group = NULL;

        switch(widgets[x].type) {
            case WIDGET_CHK_BTN:
                gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
                widget = gtk_check_button_new_with_mnemonic (dgettext (domain,
                 widgets[x].label));
                g_object_set_data ((GObject *) widget, "callback",
                 (void *) widgets[x].callback);

                if (widgets[x].cfg_type == VALUE_CFG_BOOLEAN) {
                    g_signal_connect(G_OBJECT(widget), "toggled",
                                     G_CALLBACK(on_toggle_button_cfg_toggled),
                                     widgets[x].cfg);
                    g_signal_connect(G_OBJECT(widget), "realize",
                                     G_CALLBACK(on_toggle_button_cfg_realize),
                                     widgets[x].cfg);
                } else {
                    if (widgets[x].cfg) {
                        g_signal_connect(G_OBJECT(widget), "toggled",
                                         G_CALLBACK(on_toggle_button_toggled),
                                         widgets[x].cfg);
                    } else {
                        gtk_widget_set_sensitive(widget, FALSE);
                    }
                    g_signal_connect(G_OBJECT(widget), "realize",
                                     G_CALLBACK(on_toggle_button_realize),
                                     widgets[x].cfg);
                }
                break;
            case WIDGET_LABEL:
                gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 12, 6, 0, 0);

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
                gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
                widget = gtk_radio_button_new_with_mnemonic (radio_btn_group,
                 dgettext (domain, widgets[x].label));
                radio_btn_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget));
                g_signal_connect(G_OBJECT(widget), "toggled",
                                 G_CALLBACK(on_toggle_button_toggled),
                                 widgets[x].cfg);
                g_signal_connect(G_OBJECT(widget), "realize",
                                 G_CALLBACK(on_toggle_button_realize),
                                 widgets[x].cfg);
                break;
            case WIDGET_SPIN_BTN:
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 12, 0);

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
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);

                widget = gtk_hbox_new(FALSE, 6);

                GtkWidget *font_btn = NULL;
                create_font_btn (& widgets[x], & label, & font_btn, domain);

                if (label)
                    gtk_box_pack_start(GTK_BOX(widget), label, FALSE, FALSE, 0);
                if (font_btn)
                    gtk_box_pack_start(GTK_BOX(widget), font_btn, FALSE, FALSE, 0);
                break;
            case WIDGET_TABLE:
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);

                widget = gtk_table_new(widgets[x].data.table.rows, 3, FALSE);
                fill_table (widget, widgets[x].data.table.elem,
                 widgets[x].data.table.rows, domain);
                gtk_table_set_row_spacings(GTK_TABLE(widget), 6);
                break;
            case WIDGET_ENTRY:
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 6, 12);

                widget = gtk_hbox_new(FALSE, 6);

                GtkWidget *entry = NULL;
                create_entry (& widgets[x], & label, & entry, domain);

                if (label)
                    gtk_box_pack_start(GTK_BOX(widget), label, FALSE, FALSE, 0);
                if (entry)
                    gtk_box_pack_start(GTK_BOX(widget), entry, TRUE, TRUE, 0);
                break;
            case WIDGET_COMBO_BOX:
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);

                widget = gtk_hbox_new(FALSE, 10);

                GtkWidget *combo = NULL;
                create_cbox (& widgets[x], & label, & combo, domain);

                if (label)
                    gtk_box_pack_start(GTK_BOX(widget), label, FALSE, FALSE, 0);
                if (combo)
                    gtk_box_pack_start(GTK_BOX(widget), combo, FALSE, FALSE, 0);
                break;
            case WIDGET_BOX:
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 3, 0);

                if (widgets[x].data.box.horizontal) {
                    widget = gtk_hbox_new(FALSE, 0);
                } else {
                    widget = gtk_vbox_new(FALSE, 0);
                }

                create_widgets(GTK_BOX(widget), widgets[x].data.box.elem, widgets[x].data.box.n_elem);

                if (widgets[x].data.box.frame) {
                    GtkWidget *tmp;
                    tmp = widget;

                    widget = gtk_frame_new (dgettext (domain, widgets[x].label));
                    gtk_container_add(GTK_CONTAINER(widget), tmp);
                }
                break;
            case WIDGET_NOTEBOOK:
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 3, 0);

                widget = gtk_notebook_new();

                gint i;
                for (i = 0; i<widgets[x].data.notebook.n_tabs; i++) {
                    GtkWidget *vbox;
                    vbox = gtk_vbox_new(FALSE, 5);
                    create_widgets(GTK_BOX(vbox), widgets[x].data.notebook.tabs[i].settings, widgets[x].data.notebook.tabs[i].n_settings);

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
                /* shouldn't ever happen - expect things to break */
                g_error("This shouldn't ever happen - expect things to break.");
                continue;
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
    guint i;

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

static void
create_replay_gain_category(void)
{
    GtkWidget *rg_page_vbox;
    GtkWidget *widgets_vbox;

    rg_page_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (category_notebook), rg_page_vbox);

    widgets_vbox = gtk_vbox_new (FALSE, 0);
    create_widgets(GTK_BOX(widgets_vbox), replay_gain_page_widgets, G_N_ELEMENTS(replay_gain_page_widgets));
    gtk_box_pack_start (GTK_BOX (rg_page_vbox), widgets_vbox, TRUE, TRUE, 0);
}

static void show_numbers_cb (GtkToggleButton * numbers, void * unused)
{
    cfg.show_numbers_in_pl = gtk_toggle_button_get_active (numbers);

    hook_call ("title change", NULL);

    /* trigger playlist update */
    gchar * t = g_strdup (playlist_get_title (playlist_get_active ()));
    playlist_set_title (playlist_get_active (), t);
    g_free (t);
}

static void leading_zero_cb (GtkToggleButton * leading)
{
    cfg.leading_zero = gtk_toggle_button_get_active (leading);

    hook_call ("title change", NULL);

    /* trigger playlist update */
    gchar * t = g_strdup (playlist_get_title (playlist_get_active ()));
    playlist_set_title (playlist_get_active (), t);
    g_free (t);
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
    GtkWidget *titlestring_cbox;
    GtkWidget *label62;
    GtkWidget *label61;
    GtkWidget *alignment85;
    GtkWidget *label84;
    GtkWidget *alignment86;
    GtkWidget *hbox9;
    GtkWidget *vbox34;
    GtkWidget *checkbutton10;
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
     cfg.show_numbers_in_pl);
    g_signal_connect ((GObject *) numbers, "toggled", (GCallback)
     show_numbers_cb, 0);
    gtk_container_add ((GtkContainer *) numbers_alignment, numbers);

    numbers_alignment = gtk_alignment_new (0, 0, 0, 0);
    gtk_alignment_set_padding ((GtkAlignment *) numbers_alignment, 0, 0, 12, 0);
    gtk_box_pack_start ((GtkBox *) vbox5, numbers_alignment, 0, 0, 3);

    numbers = gtk_check_button_new_with_label (_("Show leading zeroes (02:00 "
     "instead of 2:00)"));
    gtk_toggle_button_set_active ((GtkToggleButton *) numbers, cfg.leading_zero);
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

    titlestring_cbox = gtk_combo_box_text_new ();

    gtk_table_attach (GTK_TABLE (table6), titlestring_cbox, 1, 3, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    gtk_combo_box_text_append_text ((GtkComboBoxText *) titlestring_cbox, _("TITLE"));
    gtk_combo_box_text_append_text ((GtkComboBoxText *) titlestring_cbox, _("ARTIST - TITLE"));
    gtk_combo_box_text_append_text ((GtkComboBoxText *) titlestring_cbox, _("ARTIST - ALBUM - TITLE"));
    gtk_combo_box_text_append_text ((GtkComboBoxText *) titlestring_cbox, _("ARTIST - ALBUM - TRACK. TITLE"));
    gtk_combo_box_text_append_text ((GtkComboBoxText *) titlestring_cbox, _("ARTIST [ ALBUM ] - TRACK. TITLE"));
    gtk_combo_box_text_append_text ((GtkComboBoxText *) titlestring_cbox, _("ALBUM - TITLE"));
    gtk_combo_box_text_append_text ((GtkComboBoxText *) titlestring_cbox, _("Custom"));

    titlestring_entry = gtk_entry_new ();
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

    checkbutton10 = gtk_check_button_new_with_mnemonic (_("Show popup information for playlist entries"));
    gtk_box_pack_start (GTK_BOX (vbox34), checkbutton10, TRUE, FALSE, 0);
    gtk_widget_set_tooltip_text (checkbutton10, _("Toggles popup information window for the pointed entry in the playlist. The window shows title of song, name of album, genre, year of publish, track number, track length, and artwork."));

    filepopup_for_tuple_settings_button = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox9), filepopup_for_tuple_settings_button, FALSE, FALSE, 0);

    gtk_widget_set_can_focus (filepopup_for_tuple_settings_button, FALSE);
    gtk_widget_set_tooltip_text (filepopup_for_tuple_settings_button, _("Edit settings for popup information"));
    gtk_button_set_relief (GTK_BUTTON (filepopup_for_tuple_settings_button), GTK_RELIEF_HALF);

    image8 = gtk_image_new_from_stock ("gtk-properties", GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (filepopup_for_tuple_settings_button), image8);



    g_signal_connect(G_OBJECT(checkbutton10), "toggled",
                     G_CALLBACK(on_show_filepopup_for_tuple_toggled),
                     NULL);
    g_signal_connect_after(G_OBJECT(checkbutton10), "realize",
                           G_CALLBACK(on_show_filepopup_for_tuple_realize),
                           NULL);
    g_signal_connect(G_OBJECT(filepopup_for_tuple_settings_button), "clicked",
                     G_CALLBACK(on_filepopup_for_tuple_settings_clicked),
                     NULL);

    g_signal_connect(titlestring_cbox, "realize",
                     G_CALLBACK(on_titlestring_cbox_realize),
                     titlestring_entry);
    g_signal_connect(titlestring_cbox, "changed",
                     G_CALLBACK(on_titlestring_cbox_changed),
                     titlestring_entry);

    g_signal_connect(titlestring_cbox, "changed",
                     G_CALLBACK(on_titlestring_cbox_changed),
                     titlestring_help_button);
    g_signal_connect(titlestring_help_button, "clicked",
                     G_CALLBACK(on_titlestring_help_button_clicked),
                     titlestring_tag_menu);

    g_signal_connect(G_OBJECT(titlestring_entry), "changed",
                     G_CALLBACK(on_titlestring_entry_changed),
                     NULL);
    g_signal_connect_after(G_OBJECT(titlestring_entry), "realize",
                           G_CALLBACK(on_titlestring_entry_realize),
                           NULL);



    /* Create window for filepopup settings */
    create_filepopup_settings();
}

static GtkWidget * output_config_button, * output_about_button;

static gboolean output_enum_cb (PluginHandle * plugin, GList * * list)
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

static void
create_audio_category(void)
{
    GtkWidget *audio_page_vbox;
    GtkWidget *alignment74;
    GtkWidget *label77;
    GtkWidget *alignment73;
    GtkWidget *vbox33;
    GtkWidget *table11;
    GtkWidget *label79;
    GtkAdjustment * output_plugin_bufsize_adj;
    GtkWidget *output_plugin_bufsize;
    GtkWidget *output_plugin_cbox;
    GtkWidget *label78;

    audio_page_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (category_notebook), audio_page_vbox);

    alignment74 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (audio_page_vbox), alignment74, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment74), 0, 12, 0, 0);

    label77 = gtk_label_new (_("<b>Audio System</b>"));
    gtk_container_add (GTK_CONTAINER (alignment74), label77);
    gtk_label_set_use_markup (GTK_LABEL (label77), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label77), 0, 0.5);

    alignment73 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (audio_page_vbox), alignment73, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment73), 0, 6, 12, 0);

    vbox33 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (alignment73), vbox33);

    table11 = gtk_table_new (3, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox33), table11, FALSE, FALSE, 0);
    gtk_table_set_row_spacings (GTK_TABLE (table11), 6);
    gtk_table_set_col_spacings (GTK_TABLE (table11), 6);

    label79 = gtk_label_new (_("Buffer size:"));
    gtk_table_attach (GTK_TABLE (table11), label79, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label79), 1, 0.5);

    output_plugin_bufsize_adj = (GtkAdjustment *) gtk_adjustment_new (0, 100,
     10000, 100, 1000, 0);
    output_plugin_bufsize = gtk_spin_button_new (GTK_ADJUSTMENT (output_plugin_bufsize_adj), 100, 0);
    gtk_table_attach (GTK_TABLE (table11), output_plugin_bufsize, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    output_plugin_cbox = gtk_combo_box_text_new ();

    gtk_table_attach (GTK_TABLE (table11), output_plugin_cbox, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    label78 = gtk_label_new (_("Current output plugin:"));
    gtk_table_attach (GTK_TABLE (table11), label78, 0, 1, 0, 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label78), 0, 0.5);

    GtkWidget * hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) audio_page_vbox, hbox, FALSE, FALSE, 0);

    output_config_button = gtk_button_new_from_stock (GTK_STOCK_PREFERENCES);
    output_about_button = gtk_button_new_from_stock (GTK_STOCK_ABOUT);

    gtk_box_pack_end ((GtkBox *) hbox, output_about_button, FALSE, FALSE, 0);
    gtk_box_pack_end ((GtkBox *) hbox, output_config_button, FALSE, FALSE, 0);

    create_widgets(GTK_BOX(audio_page_vbox), audio_page_widgets, G_N_ELEMENTS(audio_page_widgets));

    output_combo_fill ((GtkComboBox *) output_plugin_cbox);
    output_combo_update ((GtkComboBox *) output_plugin_cbox);
    g_signal_connect (output_plugin_cbox, "changed", (GCallback)
     output_combo_changed, NULL);
    g_signal_connect (output_config_button, "clicked", (GCallback)
     output_do_config, NULL);
    g_signal_connect (output_about_button, "clicked", (GCallback)
     output_do_about, NULL);

    g_signal_connect(G_OBJECT(output_plugin_bufsize), "value_changed",
                     G_CALLBACK(on_output_plugin_bufsize_value_changed),
                     NULL);
    g_signal_connect_after(G_OBJECT(output_plugin_bufsize), "realize",
                           G_CALLBACK(on_output_plugin_bufsize_realize),
                           NULL);
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

    gint types[] = {PLUGIN_TYPE_TRANSPORT, PLUGIN_TYPE_PLAYLIST,
     PLUGIN_TYPE_INPUT, PLUGIN_TYPE_EFFECT, PLUGIN_TYPE_VIS, PLUGIN_TYPE_GENERAL};
    const gchar * names[] = {N_("Transport"), N_("Playlist"), N_("Input"),
     N_("Effect"), N_("Visualization"), N_("General")};

    for (gint i = 0; i < G_N_ELEMENTS (types); i ++)
        gtk_notebook_append_page ((GtkNotebook *) notebook, plugin_view_new
         (types[i]), gtk_label_new (_(names[i])));
}

static gboolean
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
    gchar *aud_version_string;

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
    gtk_box_pack_start (GTK_BOX (hbox1), scrolledwindow6, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow6), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow6), GTK_SHADOW_IN);

    category_treeview = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolledwindow6), category_treeview);
    gtk_widget_set_size_request (category_treeview, 172, -1);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (category_treeview), FALSE);

    category_notebook = gtk_notebook_new ();
    gtk_box_pack_start (GTK_BOX (hbox1), category_notebook, TRUE, TRUE, 0);

    gtk_widget_set_can_focus (category_notebook, FALSE);
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (category_notebook), FALSE);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (category_notebook), FALSE);
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (category_notebook), TRUE);



    create_audio_category();
    create_replay_gain_category();
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
    on_category_treeview_realize ((GtkTreeView *) category_treeview,
     (GtkNotebook *) category_notebook);

    /* audacious version label */

    aud_version_string = g_strdup_printf
     ("<span size='small'>%s (%s)</span>", "Audacious " PACKAGE_VERSION,
     BUILDSTAMP);

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

void
show_prefs_window(void)
{
    gtk_window_present(GTK_WINDOW(prefswin)); /* show or raise prefs window */
}

void
hide_prefs_window(void)
{
    g_return_if_fail(prefswin);
    gtk_widget_hide(GTK_WIDGET(prefswin));
}

static void prefswin_page_queue_new (GtkWidget * container, const gchar * name,
 const gchar * imgurl)
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
/* gint prefswin_page_new (GtkWidget * container, const gchar * name,
 const gchar * imgurl) */
gint prefswin_page_new (void * container, const gchar * name, const gchar *
 imgurl)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GdkPixbuf *img = NULL;
    GtkTreeView *treeview = GTK_TREE_VIEW(category_treeview);
    gint id;

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
    gboolean ret;
    gint id;
    gint index = -1;

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
    gint vol[2];

    if (cfg.software_volume_control)
    {
        vol[0] = cfg.sw_volume_left;
        vol[1] = cfg.sw_volume_right;
    }
    else
        playback_get_volume (& vol[0], & vol[1]);

    hook_call ("volume set", vol);
}
