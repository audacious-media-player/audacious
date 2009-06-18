/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

/* #define AUD_DEBUG */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gdk/gdkkeysyms.h>

#include "plugin.h"
#include "pluginenum.h"
#include "input.h"
#include "effect.h"
#include "general.h"
#include "output.h"
#include "visualization.h"
#include "playlist.h"

#include "audstrings.h"
#include "util.h"
#include "dnd.h"
#include "configdb.h"
#include "preferences.h"

#include "ui_preferences.h"

#include "build_stamp.h"

#define TITLESTRING_UPDATE_TIMEOUT 3

enum CategoryViewCols {
    CATEGORY_VIEW_COL_ICON,
    CATEGORY_VIEW_COL_NAME,
    CATEGORY_VIEW_COL_ID,
    CATEGORY_VIEW_N_COLS
};

enum PluginViewCols {
    PLUGIN_VIEW_COL_ACTIVE,
    PLUGIN_VIEW_COL_DESC,
    PLUGIN_VIEW_COL_FILENAME,
    PLUGIN_VIEW_COL_ID,
    PLUGIN_VIEW_COL_PLUGIN_PTR,
    PLUGIN_VIEW_N_COLS
};

enum PluginViewType {
    PLUGIN_VIEW_TYPE_INPUT,
    PLUGIN_VIEW_TYPE_GENERAL,
    PLUGIN_VIEW_TYPE_VIS,
    PLUGIN_VIEW_TYPE_EFFECT
};

typedef struct {
    const gchar *icon_path;
    const gchar *name;
} Category;

typedef struct {
    const gchar *name;
    const gchar *tag;
} TitleFieldTag;

typedef struct {
    gint x;
    gint y;
} MenuPos;

static GtkWidget *prefswin = NULL;
static GtkWidget *filepopup_settings = NULL;
static GtkWidget *category_treeview = NULL;
static GtkWidget *category_notebook = NULL;
GtkWidget *filepopupbutton = NULL;

/* colorize settings scales */
GtkWidget *green_scale;
GtkWidget *red_scale;
GtkWidget *blue_scale;

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
    {DATA_DIR "/images/audio.png",        N_("Audio")},
    {DATA_DIR "/images/replay_gain.png",  N_("Replay Gain")},
    {DATA_DIR "/images/connectivity.png", N_("Connectivity")},
    {DATA_DIR "/images/mouse.png",        N_("Mouse")},
    {DATA_DIR "/images/playback.png",     N_("Playback")},
    {DATA_DIR "/images/playlist.png",     N_("Playlist")},
    {DATA_DIR "/images/plugins.png",      N_("Plugins")},
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


const gchar *chardet_detector_presets[] = {
    N_("None"),
    N_("Japanese"),
    N_("Taiwanese"),
    N_("Chinese"),
    N_("Korean"),
    N_("Russian"),
    N_("Greek"),
    N_("Hebrew"),
    N_("Turkish"),
    N_("Arabic"),
#ifdef HAVE_UDET
    N_("Universal")
#endif
};
const guint n_chardet_detector_presets = G_N_ELEMENTS(chardet_detector_presets);


typedef struct {
    void *next;
    GtkWidget *container;
    char *pg_name;
    char *img_url;
} CategoryQueueEntry;

CategoryQueueEntry *category_queue = NULL;

GtkWidget *ui_preferences_chardet_table_populate(void);
static GtkWidget *ui_preferences_bit_depth(void);

static PreferencesWidget audio_page_widgets[] = {
    {WIDGET_LABEL, N_("<b>Format Detection</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Detect file formats on demand, instead of immediately."), &cfg.playlist_detect, NULL,
        N_("When checked, Audacious will detect file formats on demand. This can result in a messier playlist, but delivers a major speed benefit."), FALSE},
    {WIDGET_CHK_BTN, N_("Detect file formats by extension."), &cfg.use_extension_probing, NULL,
        N_("When checked, Audacious will detect file formats based by extension. Only files with extensions of supported formats will be loaded."), FALSE},
    {WIDGET_LABEL, N_("<b>Bit Depth</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CUSTOM, NULL, NULL, NULL, NULL, FALSE, {.populate = ui_preferences_bit_depth}},
};

static PreferencesWidget audio_page_widgets2[] = {
    {WIDGET_LABEL, N_("<b>Volume Control</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Use software volume control"), &cfg.software_volume_control, NULL,
                     N_("Use software volume control. This may be useful for situations where your audio system does not support controlling the playback volume."), FALSE},
    {WIDGET_LABEL,   N_("<b>Advanced</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Bypass all of signal processing if possible"), &cfg.bypass_dsp, NULL,
                     N_("Try to pass input plugin's output directly to output plugin, if the latter supports "
                        "format produced by input plugin. If it's true, all signal processing will be disabled "
                        "(i.e. DSP plugins, equalizer, resampling, Replay Gain and software volume control)."), FALSE},
};

static PreferencesWidget rg_params_elements[] = {
    {WIDGET_SPIN_BTN, N_("Preamp:"), &cfg.replay_gain_preamp, NULL, NULL, FALSE, {.spin_btn = {-15, 15, 0.01, N_("dB")}}, VALUE_FLOAT},
    {WIDGET_SPIN_BTN, N_("Default gain:"), &cfg.default_gain, NULL, N_("This gain will be used if file doesn't contain Replay Gain metadata."), FALSE, {.spin_btn = {-15, 15, 0.01, N_("dB")}}, VALUE_FLOAT},
    {WIDGET_LABEL, NULL, NULL, NULL, NULL, FALSE, {.label = {"gtk-info", N_("<span size=\"small\">Please remember that the most efficient way to prevent signal clipping is not to use positive values above.</span>")}}},
};

static PreferencesWidget replay_gain_page_widgets[] = {
    {WIDGET_LABEL, N_("<b>Replay Gain configuration</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Enable Replay Gain"), &cfg.enable_replay_gain, NULL, NULL, FALSE},
    {WIDGET_LABEL, N_("<b>Replay Gain mode</b>"), NULL, NULL, NULL, TRUE},
    {WIDGET_RADIO_BTN, N_("Track gain/peak"), &cfg.replay_gain_track, NULL, NULL, TRUE},
    {WIDGET_RADIO_BTN, N_("Album gain/peak"), &cfg.replay_gain_album, NULL, NULL, TRUE},
    {WIDGET_LABEL, N_("<b>Miscellaneous</b>"), NULL, NULL, NULL, TRUE},
    {WIDGET_CHK_BTN, N_("Enable peak info clipping prevention"), &cfg.enable_clipping_prevention, NULL,
                     N_("Use peak value from Replay Gain info for clipping prevention"), TRUE},
    {WIDGET_CHK_BTN, N_("Dynamically adjust scale factor to prevent clipping"), &cfg.enable_adaptive_scaler, NULL,
                     N_("Decrease scale factor (gain) if clipping nevertheless occurred"), TRUE},
    {WIDGET_TABLE, NULL, NULL, NULL, NULL, TRUE, {.table = {rg_params_elements, G_N_ELEMENTS(rg_params_elements)}}},
};

static PreferencesWidget playback_page_widgets[] = {
    {WIDGET_LABEL, N_("<b>Playback</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Continue playback on startup"), &cfg.resume_playback_on_startup, NULL,
        N_("When Audacious starts, automatically begin playing from the point where we stopped before."), FALSE},
    {WIDGET_CHK_BTN, N_("Don't advance in the playlist"), &cfg.no_playlist_advance, NULL,
        N_("When finished playing a song, don't automatically advance to the next."), FALSE},
    {WIDGET_CHK_BTN, N_("Pause between songs"), &cfg.pause_between_songs, NULL, NULL, FALSE},
    {WIDGET_SPIN_BTN, N_("Pause for"), &cfg.pause_between_songs_time, NULL, NULL, TRUE, {.spin_btn = {1, 100, 1, N_("seconds")}}, VALUE_INT},
};

static PreferencesWidget playlist_page_widgets[] = {
    {WIDGET_LABEL, N_("<b>Filename</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Convert underscores to blanks"), &cfg.convert_underscore, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Convert %20 to blanks"), &cfg.convert_twenty, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Convert backslash '\\' to forward slash '/'"), &cfg.convert_slash, NULL, NULL, FALSE},
    {WIDGET_LABEL, N_("<b>Metadata</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Load metadata from playlists and files"), &cfg.use_pl_metadata, NULL, N_("Load metadata (tag information) from music files."), FALSE},
    {WIDGET_CUSTOM, NULL, NULL, NULL, NULL, TRUE, {.populate = ui_preferences_chardet_table_populate}},
    {WIDGET_LABEL, N_("<b>File Dialog</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_CHK_BTN, N_("Always refresh directory when opening file dialog"), &cfg.refresh_file_list, NULL, N_("Always refresh the file dialog (this will slow opening the dialog on large directories, and Gnome VFS should handle automatically)."), FALSE},
};

static PreferencesWidget mouse_params_elements[] = {
    {WIDGET_SPIN_BTN, N_("Changes volume by"), &cfg.mouse_change, NULL, NULL, FALSE, {.spin_btn = {1, 100, 1, N_("percent")}}, VALUE_INT},
    {WIDGET_SPIN_BTN, N_("Scrolls playlist by"), &cfg.scroll_pl_by, NULL, NULL, FALSE, {.spin_btn = {1, 100, 1, N_("lines")}}, VALUE_INT},
};

static PreferencesWidget mouse_page_widgets[] = {
    {WIDGET_LABEL, N_("<b>Mouse wheel</b>"), NULL, NULL, NULL, FALSE},
    {WIDGET_TABLE, NULL, NULL, NULL, NULL, TRUE, {.table = {mouse_params_elements, G_N_ELEMENTS(mouse_params_elements)}}},
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

void
prefswin_set_category(gint index)
{
    g_return_if_fail(index >= 0 && index < n_categories);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(category_treeview), index);
}

static void
output_plugin_open_prefs(GtkComboBox * cbox,
                         gpointer data)
{
    output_configure(gtk_combo_box_get_active(cbox));
}

static void
output_plugin_open_info(GtkComboBox * cbox,
                        gpointer data)
{
    output_about(gtk_combo_box_get_active(cbox));
}

static void
plugin_toggle(GtkCellRendererToggle * cell,
              const gchar * path_str,
              gpointer data)
{
    GtkTreeModel *model = GTK_TREE_MODEL(data);
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gint pluginnr;
    gint plugin_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(data), "plugin_type"));

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);

    if (plugin_type == PLUGIN_VIEW_TYPE_INPUT) {
        Plugin *plugin;
        /*GList *diplist, *tmplist; */

        gtk_tree_model_get(model, &iter,
                           PLUGIN_VIEW_COL_ID, &pluginnr,
                           PLUGIN_VIEW_COL_PLUGIN_PTR, &plugin, -1);

        /* do something with the value */
        plugin->enabled ^= 1;

        /* set new value */
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           PLUGIN_VIEW_COL_ACTIVE, plugin->enabled, -1);
    } else {
        gboolean fixed;
        gtk_tree_model_get(model, &iter,
                           PLUGIN_VIEW_COL_ACTIVE, &fixed,
                           PLUGIN_VIEW_COL_ID, &pluginnr, -1);

        /* do something with the value */
        fixed ^= 1;

        switch (plugin_type) {
            case PLUGIN_VIEW_TYPE_GENERAL:
                enable_general_plugin(pluginnr, fixed);
                break;
            case PLUGIN_VIEW_TYPE_VIS:
                enable_vis_plugin(pluginnr, fixed);
                break;
            case PLUGIN_VIEW_TYPE_EFFECT:
                enable_effect_plugin(pluginnr, fixed);
                break;
        }

        /* set new value */
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           PLUGIN_VIEW_COL_ACTIVE, fixed, -1);
    }

    /* clean up */
    gtk_tree_path_free(path);
}

static void
on_output_plugin_cbox_changed(GtkComboBox * combobox,
                              gpointer data)
{
    gint selected;
    selected = gtk_combo_box_get_active(combobox);

    set_current_output_plugin(selected);
}

static void
on_output_plugin_cbox_realize(GtkComboBox * cbox,
                              gpointer data)
{
    GList *olist = get_output_list();
    OutputPlugin *op, *cp = get_current_output_plugin();
    gint i = 0, selected = 0;

    if (!olist) {
        gtk_widget_set_sensitive(GTK_WIDGET(cbox), FALSE);
        return;
    }

    for (i = 0; olist; i++, olist = g_list_next(olist)) {
        op = OUTPUT_PLUGIN(olist->data);

        if (olist->data == cp)
            selected = i;

        gtk_combo_box_append_text(cbox, op->description);
    }

    gtk_combo_box_set_active(cbox, selected);
    g_signal_connect(cbox, "changed",
                     G_CALLBACK(on_output_plugin_cbox_changed), NULL);
}

static void
on_plugin_view_realize(GtkTreeView * treeview,
                       GCallback callback,
                       gpointer data,
                       gint plugin_type)
{
    GtkListStore *store;
    GtkTreeIter iter;

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    GList *ilist;
    gchar *description[2];
    gint id = 0;

    GList *list = (GList *) data;

    store = gtk_list_store_new(PLUGIN_VIEW_N_COLS,
                               G_TYPE_BOOLEAN, G_TYPE_STRING,
                               G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER);
    g_object_set_data(G_OBJECT(store), "plugin_type" , GINT_TO_POINTER(plugin_type));

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Enabled"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, FALSE);
    gtk_tree_view_column_set_fixed_width(column, 50);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled",
                     G_CALLBACK(callback), store);
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "active",
                                        PLUGIN_VIEW_COL_ACTIVE, NULL);

    gtk_tree_view_append_column(treeview, column);

    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(column, _("Description"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);


    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer,
                                        "text", PLUGIN_VIEW_COL_DESC, NULL);
    gtk_tree_view_append_column(treeview, column);

    column = gtk_tree_view_column_new();

    gtk_tree_view_column_set_title(column, _("Filename"));
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "text",
                                        PLUGIN_VIEW_COL_FILENAME, NULL);

    gtk_tree_view_append_column(treeview, column);

    MOWGLI_ITER_FOREACH(ilist, list)
    {
        Plugin *plugin = PLUGIN(ilist->data);

        description[0] = g_strdup(plugin->description);
        description[1] = g_strdup(plugin->filename);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           PLUGIN_VIEW_COL_ACTIVE, plugin->enabled,
                           PLUGIN_VIEW_COL_DESC, description[0],
                           PLUGIN_VIEW_COL_FILENAME, description[1],
                           PLUGIN_VIEW_COL_ID, id++,
                           PLUGIN_VIEW_COL_PLUGIN_PTR, plugin, -1);

        g_free(description[1]);
        g_free(description[0]);
    }

    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
}

static void
on_input_plugin_view_realize(GtkTreeView * treeview,
                             gpointer data)
{
    on_plugin_view_realize(treeview, G_CALLBACK(plugin_toggle), ip_data.input_list, PLUGIN_VIEW_TYPE_INPUT);
}

static void
on_effect_plugin_view_realize(GtkTreeView * treeview,
                              gpointer data)
{
    on_plugin_view_realize(treeview, G_CALLBACK(plugin_toggle), ep_data.effect_list, PLUGIN_VIEW_TYPE_EFFECT);
}

static void
on_general_plugin_view_realize(GtkTreeView * treeview,
                               gpointer data)
{
    on_plugin_view_realize(treeview, G_CALLBACK(plugin_toggle), gp_data.general_list, PLUGIN_VIEW_TYPE_GENERAL);
}

static void
on_vis_plugin_view_realize(GtkTreeView * treeview,
                           gpointer data)
{
    on_plugin_view_realize(treeview, G_CALLBACK(plugin_toggle), vp_data.vis_list, PLUGIN_VIEW_TYPE_VIS);
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
util_menu_position(GtkMenu * menu, gint * x, gint * y,
                   gboolean * push_in, gpointer data)
{
    GtkRequisition requisition;
    gint screen_width;
    gint screen_height;
    MenuPos *pos = data;

    gtk_widget_size_request(GTK_WIDGET(menu), &requisition);

    screen_width = gdk_screen_width();
    screen_height = gdk_screen_height();

    *x = CLAMP(pos->x - 2, 0, MAX(0, screen_width - requisition.width));
    *y = CLAMP(pos->y - 2, 0, MAX(0, screen_height - requisition.height));
}

static void
on_titlestring_help_button_clicked(GtkButton * button,
                                   gpointer data)
{
    GtkMenu *menu;
    MenuPos *pos = g_newa(MenuPos, 1);
    GdkWindow *parent;

    gint x_ro, y_ro;
    gint x_widget, y_widget;
    gint x_size, y_size;

    g_return_if_fail (button != NULL);
    g_return_if_fail (GTK_IS_MENU (data));

    parent = gtk_widget_get_parent_window(GTK_WIDGET(button));

    gdk_drawable_get_size(parent, &x_size, &y_size);
    gdk_window_get_root_origin(GTK_WIDGET(button)->window, &x_ro, &y_ro);
    gdk_window_get_position(GTK_WIDGET(button)->window, &x_widget, &y_widget);

    pos->x = x_size + x_ro;
    pos->y = y_size + y_ro - 100;

    menu = GTK_MENU(data);
    gtk_menu_popup (menu, NULL, NULL, util_menu_position, pos,
                    0, GDK_CURRENT_TIME);
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
        playlist_update_all_titles();
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

    playlist_update_all_titles(); /* update titles */
}

static void
on_font_btn_realize(GtkFontButton * button, gchar **cfg)
{
    gtk_font_button_set_font_name(button, *cfg);
}

static void
on_font_btn_font_set(GtkFontButton * button, gchar **cfg)
{
    g_free(*cfg);
    *cfg = g_strdup(gtk_font_button_get_font_name(button));
    AUDDBG("Returned font name: \"%s\"\n", *cfg);
    void (*callback) (void) = g_object_get_data(G_OBJECT(button), "callback");
    if (callback) callback();
}

/* proxy */
static void
on_proxy_button_realize(GtkToggleButton *button, gchar *cfg)
{
    g_return_if_fail(cfg != NULL);

    mcs_handle_t *db;
    gboolean ret;

    db = cfg_db_open();

    if (cfg_db_get_bool(db, NULL, cfg, &ret) != FALSE)
        gtk_toggle_button_set_active(button, ret);

    cfg_db_close(db);
}

static void
on_proxy_button_toggled(GtkToggleButton *button, gchar *cfg)
{
    g_return_if_fail(cfg != NULL);

    mcs_handle_t *db;
    gboolean ret = gtk_toggle_button_get_active(button);

    db = cfg_db_open();
    cfg_db_set_bool(db, NULL, cfg, ret);
    cfg_db_close(db);
}

static void
on_proxy_entry_changed(GtkEntry *entry, gchar *cfg)
{
    g_return_if_fail(cfg != NULL);

    mcs_handle_t *db;
    gchar *ret = g_strdup(gtk_entry_get_text(entry));

    db = cfg_db_open();
    cfg_db_set_string(db, NULL, cfg, ret);
    cfg_db_close(db);

    g_free(ret);
}

static void
on_proxy_entry_realize(GtkEntry *entry, gchar *cfg)
{
    g_return_if_fail(cfg != NULL);

    mcs_handle_t *db;
    gchar *ret;

    db = cfg_db_open();

    if (cfg_db_get_string(db, NULL, cfg, &ret) != FALSE)
        gtk_entry_set_text(entry, ret);

    cfg_db_close(db);
}

static void
plugin_treeview_open_prefs(GtkTreeView *treeview)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    Plugin *plugin = NULL;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;
    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_PLUGIN_PTR, &plugin, -1);

    g_return_if_fail(plugin != NULL);
    g_return_if_fail(plugin->configure != NULL);

    plugin_set_current(plugin);
    plugin->configure();
}

static void
plugin_treeview_open_info(GtkTreeView *treeview)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    Plugin *plugin = NULL;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;
    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_PLUGIN_PTR, &plugin, -1);

    g_return_if_fail(plugin != NULL);

    plugin_set_current(plugin);
    plugin->about();
}

static void
plugin_treeview_enable_prefs(GtkTreeView * treeview, GtkButton * button)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    Plugin *plugin = NULL;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_PLUGIN_PTR, &plugin, -1);

    g_return_if_fail(plugin != NULL);

    gtk_widget_set_sensitive(GTK_WIDGET(button), plugin->configure != NULL);
}

static void
plugin_treeview_enable_info(GtkTreeView * treeview, GtkButton * button)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    Plugin *plugin = NULL;

    selection = gtk_tree_view_get_selection(treeview);
    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, PLUGIN_VIEW_COL_PLUGIN_PTR, &plugin, -1);

    g_return_if_fail(plugin != NULL);

    gtk_widget_set_sensitive(GTK_WIDGET(button), plugin->about != NULL);
}


static void
output_plugin_enable_info(GtkComboBox * cbox, GtkButton * button)
{
    GList *plist;

    gint id = gtk_combo_box_get_active(cbox);

    plist = get_output_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             OUTPUT_PLUGIN(plist->data)->about != NULL);
}

static void
output_plugin_enable_prefs(GtkComboBox * cbox, GtkButton * button)
{
    GList *plist;
    gint id = gtk_combo_box_get_active(cbox);

    plist = get_output_list();
    plist = g_list_nth(plist, id);

    gtk_widget_set_sensitive(GTK_WIDGET(button),
                             OUTPUT_PLUGIN(plist->data)->configure != NULL);
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
on_enable_src_realize(GtkToggleButton * button,
                      gpointer data)
{
#ifdef USE_SAMPLERATE
    gtk_toggle_button_set_active(button, cfg.enable_src);
#else
    gtk_toggle_button_set_active(button, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
#endif
}

static void
on_enable_src_toggled(GtkToggleButton * button, gpointer data)
{
#ifdef USE_SAMPLERATE
    cfg.enable_src = gtk_toggle_button_get_active(button);
#endif
}

static void
on_src_rate_realize(GtkSpinButton * button,
                    gpointer data)
{
#ifdef USE_SAMPLERATE
    gtk_spin_button_set_value(button, (gdouble)cfg.src_rate);
#else
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
#endif
}

static void
on_src_rate_value_changed(GtkSpinButton * button,
                          gpointer data)
{
#ifdef USE_SAMPLERATE
    cfg.src_rate = gtk_spin_button_get_value_as_int(button);
#endif
}

static void
on_src_converter_type_realize(GtkComboBox * box,
                              gpointer data)
{
#ifdef USE_SAMPLERATE
    gtk_combo_box_set_active(box, cfg.src_type);
#else
    gtk_widget_set_sensitive(GTK_WIDGET(box), FALSE);
#endif
}

static void
on_src_converter_type_changed(GtkComboBox * box,
                              gpointer data)
{
#ifdef USE_SAMPLERATE
    cfg.src_type = gtk_combo_box_get_active(box);
#endif
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
    g_object_set(G_OBJECT(renderer), "wrap-width", width - 64 - 20, "wrap-mode", PANGO_WRAP_WORD, NULL);

    store = gtk_list_store_new(CATEGORY_VIEW_N_COLS,
                               GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));

    for (i = 0; i < n_categories; i++) {
        img = gdk_pixbuf_new_from_file(categories[i].icon_path, NULL);
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
on_chardet_detector_cbox_changed(GtkComboBox * combobox, gpointer data)
{
    mcs_handle_t *db;
    gint position = 0;

    position = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    cfg.chardet_detector = (char *)chardet_detector_presets[position];

    db = cfg_db_open();
    cfg_db_set_string(db, NULL, "chardet_detector", cfg.chardet_detector);
    cfg_db_close(db);
    if (data != NULL)
        gtk_widget_set_sensitive(GTK_WIDGET(data), 1);
}

static void
on_chardet_detector_cbox_realize(GtkComboBox *combobox, gpointer data)
{
    mcs_handle_t *db;
    gchar *ret=NULL;
    guint i=0,index=0;

    for(i=0; i<n_chardet_detector_presets; i++) {
        gtk_combo_box_append_text(combobox, _(chardet_detector_presets[i]));
    }

    db = cfg_db_open();
    if(cfg_db_get_string(db, NULL, "chardet_detector", &ret) != FALSE) {
        for(i=0; i<n_chardet_detector_presets; i++) {
            if(!strcmp(chardet_detector_presets[i], ret)) {
                cfg.chardet_detector = (char *)chardet_detector_presets[i];
                index = i;
            }
        }
    }
    cfg_db_close(db);

#ifdef USE_CHARDET
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), index);

    if (data != NULL)
        gtk_widget_set_sensitive(GTK_WIDGET(data), TRUE);

    g_signal_connect(combobox, "changed",
                     G_CALLBACK(on_chardet_detector_cbox_changed), NULL);
#else
    gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), -1);
    gtk_widget_set_sensitive(GTK_WIDGET(combobox), 0);
#endif
    if(ret)
        g_free(ret);
}

static void
on_chardet_fallback_realize(GtkEntry *entry, gpointer data)
{
    mcs_handle_t *db;
    gchar *ret = NULL;

    db = cfg_db_open();

    if (cfg_db_get_string(db, NULL, "chardet_fallback", &ret) != FALSE) {
        if(cfg.chardet_fallback)
            g_free(cfg.chardet_fallback);

        if(ret && strncasecmp(ret, "None", sizeof("None"))) {
            cfg.chardet_fallback = ret;
        } else {
            cfg.chardet_fallback = g_strdup("");
        }
        gtk_entry_set_text(entry, cfg.chardet_fallback);
    }

    cfg_db_close(db);
}

static void
on_chardet_fallback_changed(GtkEntry *entry, gpointer data)
{
    mcs_handle_t *db;
    gchar *ret = NULL;

    if(cfg.chardet_fallback)
        g_free(cfg.chardet_fallback);

    ret = g_strdup(gtk_entry_get_text(entry));

    if(ret == NULL)
        cfg.chardet_fallback = g_strdup("");
    else
        cfg.chardet_fallback = ret;

    db = cfg_db_open();

    if(cfg.chardet_fallback == NULL || !strcmp(cfg.chardet_fallback, ""))
        cfg_db_set_string(db, NULL, "chardet_fallback", "None");
    else
        cfg_db_set_string(db, NULL, "chardet_fallback", cfg.chardet_fallback);

    cfg_db_close(db);
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
    if (callback) callback();
    GtkWidget *child = g_object_get_data(G_OBJECT(button), "child");
    if (child) gtk_widget_set_sensitive(GTK_WIDGET(child), *cfg);
}

static void
on_toggle_button_realize(GtkToggleButton * button, gboolean *cfg)
{
    gtk_toggle_button_set_active(button, *cfg);
    GtkWidget *child = g_object_get_data(G_OBJECT(button), "child");
    if (child) gtk_widget_set_sensitive(GTK_WIDGET(child), *cfg);
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

    GtkObject *recurse_for_cover_depth_adj;
    GtkObject *delay_adj;
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

    recurse_for_cover_depth_adj = gtk_adjustment_new(0, 0, 100, 1, 10, 10);
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

    delay_adj = gtk_adjustment_new(0, 0, 100, 1, 10, 10);
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
    GTK_WIDGET_SET_FLAGS(btn_ok, GTK_CAN_DEFAULT);

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

GtkWidget *
ui_preferences_chardet_table_populate(void)
{
    GtkWidget *widget = gtk_table_new(2, 2, FALSE);
    GtkWidget *label;

    label = gtk_label_new(_("Auto character encoding detector for:"));
    gtk_table_attach(GTK_TABLE(widget), label, 0, 1, 0, 1,
                     (GtkAttachOptions) (0),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

    GtkWidget *combobox = gtk_combo_box_new_text();
    gtk_table_attach(GTK_TABLE(widget), combobox, 1, 2, 0, 1,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    g_signal_connect_after(G_OBJECT(combobox), "realize",
                           G_CALLBACK(on_chardet_detector_cbox_realize),
                           NULL);

    GtkWidget *entry = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(widget), entry, 1, 2, 1, 2,
                     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_widget_set_tooltip_text (entry, _("List of character encodings used for fall back conversion of metadata. If automatic character encoding detector failed or has been disabled, encodings in this list would be treated as candidates of the encoding of metadata, and fall back conversion from these encodings to UTF-8 would be attempted."));

    label = gtk_label_new(_("Fallback character encodings:"));
    gtk_table_attach(GTK_TABLE(widget), label, 0, 1, 1, 2,
                     (GtkAttachOptions) (0),
                     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

    g_signal_connect(G_OBJECT(entry), "changed",
                     G_CALLBACK(on_chardet_fallback_changed),
                     NULL);
    g_signal_connect_after(G_OBJECT(entry), "realize",
                           G_CALLBACK(on_chardet_fallback_realize),
                           NULL);

    return widget;
}

static void
on_bit_depth_cbox_changed(GtkWidget *cbox, gpointer data)
{
    gint active = gtk_combo_box_get_active(GTK_COMBO_BOX(cbox));
    cfg.output_bit_depth = (active == 1) ? 24 : 16;
}

GtkWidget *
ui_preferences_bit_depth(void)
{
    GtkWidget *box = gtk_hbox_new(FALSE, 10);
    GtkWidget *label = gtk_label_new(_("Output bit depth:"));
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

    GtkWidget *combo = gtk_combo_box_new_text ();
    gtk_combo_box_append_text(GTK_COMBO_BOX (combo), "16");
    gtk_combo_box_append_text(GTK_COMBO_BOX (combo), "24");
    gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 0);

    gint active = (cfg.output_bit_depth == 24) ? 1 : 0;
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), active);
    g_signal_connect(combo, "changed", G_CALLBACK(on_bit_depth_cbox_changed), NULL);

    gtk_widget_set_tooltip_text(box,
                                _("All streams will be converted to this bit depth.\n"
                                "This should be the max supported bit depth of\nthe sound card or output plugin."));

    return box;
}

void
create_spin_button(PreferencesWidget *widget, GtkWidget **label_pre, GtkWidget **spin_btn, GtkWidget **label_past)
{
     g_return_if_fail(widget->type == WIDGET_SPIN_BTN);

     *label_pre = gtk_label_new(_(widget->label));
     gtk_misc_set_alignment(GTK_MISC(*label_pre), 0, 0.5);
     gtk_misc_set_padding(GTK_MISC(*label_pre), 4, 0);

     *spin_btn = gtk_spin_button_new_with_range(widget->data.spin_btn.min,
                                                widget->data.spin_btn.max,
                                                widget->data.spin_btn.step);


     if (widget->tooltip)
         gtk_widget_set_tooltip_text(*spin_btn, _(widget->tooltip));

     if (widget->data.spin_btn.right_label) {
         *label_past = gtk_label_new(_(widget->data.spin_btn.right_label));
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
         default:
             g_warning("Unsupported value type for spin button");
     }
}

void
fill_table(GtkWidget *table, PreferencesWidget *elements, gint amt)
{
    int x;
    GtkWidget *widget_left, *widget_middle, *widget_right;

    for (x = 0; x < amt; ++x) {
        widget_left = widget_middle = widget_right = NULL;
        switch (elements[x].type) {
            case WIDGET_SPIN_BTN:
                create_spin_button(&elements[x], &widget_left, &widget_middle, &widget_right);

                break;
            case WIDGET_LABEL:
                if (elements[x].data.label.stock_id)
                    widget_left = gtk_image_new_from_stock(elements[x].data.label.stock_id, GTK_ICON_SIZE_BUTTON);
                if (elements[x].data.label.markup) {
                    widget_middle = gtk_label_new(N_(elements[x].data.label.markup));
                    gtk_label_set_use_markup(GTK_LABEL(widget_middle), TRUE);
                } else
                    widget_middle = gtk_label_new(N_(elements[x].label));

                gtk_label_set_line_wrap (GTK_LABEL(widget_middle), TRUE);
                gtk_misc_set_alignment (GTK_MISC(widget_middle), 0, 0.5);
                break;
            default:
                g_message("Unsupported widget in table");
        }

        if (widget_left)
            gtk_table_attach(GTK_TABLE (table), widget_left, 0, 1, x, x+1,
                             (GtkAttachOptions) (0),
                             (GtkAttachOptions) (0), 0, 0);

        if (widget_middle)
            gtk_table_attach(GTK_TABLE(table), widget_middle, 1, 2, x, x+1,
                             (GtkAttachOptions) (GTK_FILL),
                             (GtkAttachOptions) (0), 4, 0);

        if (widget_right)
            gtk_table_attach(GTK_TABLE(table), widget_right, 2, 3, x, x+1,
                             (GtkAttachOptions) (0),
                             (GtkAttachOptions) (0), 0, 0);
    }
}

/* it's at early stage */
void
create_widgets(GtkBox *box, PreferencesWidget *widgets, gint amt)
{
    int x;
    GtkWidget *alignment = NULL, *widget = NULL;
    GtkWidget *child_box = NULL;
    GSList *radio_btn_group = NULL;
    int table_line=0;  /* used for WIDGET_SPIN_BTN */

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
                widget = gtk_check_button_new_with_mnemonic(_(widgets[x].label));
                g_object_set_data(G_OBJECT(widget), "callback", widgets[x].callback);
                g_signal_connect(G_OBJECT(widget), "toggled",
                                 G_CALLBACK(on_toggle_button_toggled),
                                 widgets[x].cfg);
                g_signal_connect(G_OBJECT(widget), "realize",
                                 G_CALLBACK(on_toggle_button_realize),
                                 widgets[x].cfg);
                break;
            case WIDGET_LABEL:
                gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 12, 6, 0, 0);
                widget = gtk_label_new_with_mnemonic(_(widgets[x].label));
                gtk_label_set_use_markup(GTK_LABEL(widget), TRUE);
                gtk_misc_set_alignment(GTK_MISC(widget), 0, 0.5);
                break;
            case WIDGET_RADIO_BTN:
                gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
                widget = gtk_radio_button_new_with_mnemonic(radio_btn_group, _(widgets[x].label));
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

                GtkWidget *label_pre = NULL, *spin_btn = NULL, *label_past;
                create_spin_button(&widgets[x], &label_pre, &spin_btn, &label_past);

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

                if (x > 1 && widgets[x-1].type == WIDGET_FONT_BTN) {
                    table_line++;
                } else {
                    /* check how many WIDGET_FONT_BTNs are there */
                    gint lines = 0, i;
                    for (i=x; i<amt && widgets[i].type == WIDGET_FONT_BTN; i++)
                        lines++;

                    widget = gtk_table_new(lines, 2, FALSE);
                    gtk_table_set_row_spacings(GTK_TABLE(widget), 8);
                    gtk_table_set_col_spacings(GTK_TABLE(widget), 2);
                    table_line=0;
                }

                GtkWidget *label = gtk_label_new_with_mnemonic(_(widgets[x].label));
                gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
                gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
                gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
                gtk_table_attach(GTK_TABLE (widget), label, 0, 1, table_line, table_line+1,
                                 (GtkAttachOptions) (0),
                                 (GtkAttachOptions) (0), 0, 0);

                GtkWidget *font_btn = gtk_font_button_new();
                gtk_table_attach(GTK_TABLE(widget), font_btn, 1, 2, table_line, table_line+1,
                                 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                                 (GtkAttachOptions) (0), 0, 0);

                gtk_font_button_set_use_font(GTK_FONT_BUTTON(font_btn), TRUE);
                gtk_font_button_set_use_size(GTK_FONT_BUTTON(font_btn), TRUE);
                gtk_label_set_mnemonic_widget(GTK_LABEL(label), font_btn);
                if (widgets[x].tooltip)
                    gtk_font_button_set_title (GTK_FONT_BUTTON (font_btn), _(widgets[x].tooltip));
                g_object_set_data(G_OBJECT(font_btn), "callback", widgets[x].callback);

                g_signal_connect(G_OBJECT(font_btn), "font_set",
                                 G_CALLBACK(on_font_btn_font_set),
                                 (char**)widgets[x].cfg);
                g_signal_connect(G_OBJECT(font_btn), "realize",
                                 G_CALLBACK(on_font_btn_realize),
                                 (char**)widgets[x].cfg);
                break;
            case WIDGET_TABLE:
                gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);

                widget = gtk_table_new(widgets[x].data.table.rows, 3, FALSE);
                fill_table(widget, widgets[x].data.table.elem, widgets[x].data.table.rows);
                gtk_table_set_row_spacings(GTK_TABLE(widget), 6);
                break;
            default:
                /* shouldn't ever happen - expect things to break */
                continue;
        }

        if (widget && !gtk_widget_get_parent(widget))
            gtk_container_add(GTK_CONTAINER(alignment), widget);
        if (widget && widgets[x].tooltip && widgets[x].type != WIDGET_SPIN_BTN)
            gtk_widget_set_tooltip_text(widget, _(widgets[x].tooltip));
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
create_mouse_category(void)
{
    GtkWidget *mouse_page_vbox;
    GtkWidget *vbox20;

    mouse_page_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (category_notebook), mouse_page_vbox);

    vbox20 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (mouse_page_vbox), vbox20, TRUE, TRUE, 0);

    create_widgets(GTK_BOX(vbox20), mouse_page_widgets, G_N_ELEMENTS(mouse_page_widgets));
}

static void
create_playback_category(void)
{
    GtkWidget *playback_page_vbox;
    GtkWidget *widgets_vbox;

    playback_page_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (category_notebook), playback_page_vbox);

    widgets_vbox = gtk_vbox_new (FALSE, 0);
    create_widgets(GTK_BOX(widgets_vbox), playback_page_widgets, G_N_ELEMENTS(playback_page_widgets));
    gtk_box_pack_start (GTK_BOX (playback_page_vbox), widgets_vbox, TRUE, TRUE, 0);
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
    Playlist * playlist;
    char * title;

    cfg.show_numbers_in_pl = gtk_toggle_button_get_active (numbers);

    playlist = playlist_get_active ();
    hook_call ("playlist update", playlist);
    title = playlist_get_info_text (playlist);
    hook_call ("title change", title);
    g_free (title);
}

static void
create_playlist_category(void)
{
    GtkWidget *playlist_page_vbox;
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

    playlist_page_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (category_notebook), playlist_page_vbox);

    vbox5 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (playlist_page_vbox), vbox5, TRUE, TRUE, 0);

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

    numbers = gtk_check_button_new_with_label (_ ("Show song numbers"));
    gtk_toggle_button_set_active ((GtkToggleButton *) numbers,
     cfg.show_numbers_in_pl);
    g_signal_connect ((GObject *) numbers, "toggled", (GCallback)
     show_numbers_cb, 0);
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
    GTK_WIDGET_UNSET_FLAGS (titlestring_help_button, GTK_CAN_FOCUS);
    gtk_widget_set_tooltip_text (titlestring_help_button, _("Show information about titlestring format"));
    gtk_button_set_relief (GTK_BUTTON (titlestring_help_button), GTK_RELIEF_HALF);
    gtk_button_set_focus_on_click (GTK_BUTTON (titlestring_help_button), FALSE);

    image1 = gtk_image_new_from_stock ("gtk-index", GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (titlestring_help_button), image1);

    titlestring_cbox = gtk_combo_box_new_text ();
    gtk_table_attach (GTK_TABLE (table6), titlestring_cbox, 1, 3, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_combo_box_append_text (GTK_COMBO_BOX (titlestring_cbox), _("TITLE"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (titlestring_cbox), _("ARTIST - TITLE"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (titlestring_cbox), _("ARTIST - ALBUM - TITLE"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (titlestring_cbox), _("ARTIST - ALBUM - TRACK. TITLE"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (titlestring_cbox), _("ARTIST [ ALBUM ] - TRACK. TITLE"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (titlestring_cbox), _("ALBUM - TITLE"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (titlestring_cbox), _("Custom"));

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
    GTK_WIDGET_UNSET_FLAGS (filepopup_for_tuple_settings_button, GTK_CAN_FOCUS);
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


static void
create_audio_category(void)
{
    GtkWidget *audio_page_vbox;
    GtkWidget *alignment74;
    GtkWidget *label77;
    GtkWidget *alignment73;
    GtkWidget *vbox33;
    GtkWidget *table11;
    GtkWidget *image7;
    GtkWidget *label79;
    GtkWidget *label82;
    GtkObject *output_plugin_bufsize_adj;
    GtkWidget *output_plugin_bufsize;
    GtkWidget *output_plugin_cbox;
    GtkWidget *label78;
    GtkWidget *alignment82;
    GtkWidget *output_plugin_button_box;
    GtkWidget *output_plugin_prefs;
    GtkWidget *alignment76;
    GtkWidget *hbox7;
    GtkWidget *image5;
    GtkWidget *label80;
    GtkWidget *output_plugin_info;
    GtkWidget *alignment77;
    GtkWidget *hbox8;
    GtkWidget *image6;
    GtkWidget *label81;
    GtkWidget *alignment90;
    GtkWidget *label93;
    GtkWidget *alignment92;
    GtkWidget *enable_src;
    GtkWidget *alignment91;
    GtkWidget *vbox36;
    GtkWidget *table13;
    GtkWidget *src_converter_type;
    GtkWidget *label94;
    GtkWidget *label92;
    GtkWidget *image9;
    GtkObject *src_rate_adj;
    GtkWidget *src_rate;
    GtkWidget *label91;
    GtkWidget *alignment4;

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

    image7 = gtk_image_new_from_stock ("gtk-info", GTK_ICON_SIZE_BUTTON);
    gtk_table_attach (GTK_TABLE (table11), image7, 0, 1, 2, 3,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (image7), 1, 0);

    label79 = gtk_label_new (_("Buffer size:"));
    gtk_table_attach (GTK_TABLE (table11), label79, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label79), 1, 0.5);

    label82 = gtk_label_new (_("<span size=\"small\">This is the amount of time to prebuffer audio streams by, in milliseconds.\nIncrease this value if you are experiencing audio skipping.\nPlease note however, that high values will result in Audacious performing poorly.</span>"));
    gtk_table_attach (GTK_TABLE (table11), label82, 1, 2, 2, 3,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_use_markup (GTK_LABEL (label82), TRUE);
    gtk_label_set_line_wrap (GTK_LABEL (label82), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label82), 0, 0.5);

    output_plugin_bufsize_adj = gtk_adjustment_new (0, 0, 600000, 100, 1000, 1000);
    output_plugin_bufsize = gtk_spin_button_new (GTK_ADJUSTMENT (output_plugin_bufsize_adj), 1, 0);
    gtk_table_attach (GTK_TABLE (table11), output_plugin_bufsize, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    output_plugin_cbox = gtk_combo_box_new_text ();
    gtk_table_attach (GTK_TABLE (table11), output_plugin_cbox, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    label78 = gtk_label_new (_("Current output plugin:"));
    gtk_table_attach (GTK_TABLE (table11), label78, 0, 1, 0, 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label78), 0, 0.5);

    alignment82 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (audio_page_vbox), alignment82, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment82), 0, 0, 12, 0);

    output_plugin_button_box = gtk_hbutton_box_new ();
    gtk_container_add (GTK_CONTAINER (alignment82), output_plugin_button_box);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (output_plugin_button_box), GTK_BUTTONBOX_START);
    gtk_box_set_spacing (GTK_BOX (output_plugin_button_box), 8);

    output_plugin_prefs = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER (output_plugin_button_box), output_plugin_prefs);
    gtk_widget_set_sensitive (output_plugin_prefs, FALSE);
    GTK_WIDGET_SET_FLAGS (output_plugin_prefs, GTK_CAN_DEFAULT);

    alignment76 = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (output_plugin_prefs), alignment76);

    hbox7 = gtk_hbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (alignment76), hbox7);

    image5 = gtk_image_new_from_stock ("gtk-preferences", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start (GTK_BOX (hbox7), image5, FALSE, FALSE, 0);

    label80 = gtk_label_new_with_mnemonic (_("Output Plugin Preferences"));
    gtk_box_pack_start (GTK_BOX (hbox7), label80, FALSE, FALSE, 0);

    output_plugin_info = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER (output_plugin_button_box), output_plugin_info);
    gtk_widget_set_sensitive (output_plugin_info, FALSE);
    GTK_WIDGET_SET_FLAGS (output_plugin_info, GTK_CAN_DEFAULT);

    alignment77 = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (output_plugin_info), alignment77);

    hbox8 = gtk_hbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (alignment77), hbox8);

    image6 = gtk_image_new_from_stock ("gtk-about", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start (GTK_BOX (hbox8), image6, FALSE, FALSE, 0);

    label81 = gtk_label_new_with_mnemonic (_("Output Plugin Information"));
    gtk_box_pack_start (GTK_BOX (hbox8), label81, FALSE, FALSE, 0);

    create_widgets(GTK_BOX(audio_page_vbox), audio_page_widgets, G_N_ELEMENTS(audio_page_widgets));

    alignment90 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (audio_page_vbox), alignment90, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment90), 12, 12, 0, 0);

    label93 = gtk_label_new (_("<b>Sampling Rate Converter</b>"));
    gtk_container_add (GTK_CONTAINER (alignment90), label93);
    gtk_label_set_use_markup (GTK_LABEL (label93), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label93), 0, 0.5);

    alignment92 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (audio_page_vbox), alignment92, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment92), 0, 0, 12, 0);

    enable_src = gtk_check_button_new_with_mnemonic (_("Enable Sampling Rate Converter"));
    gtk_container_add (GTK_CONTAINER (alignment92), enable_src);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (enable_src), TRUE);

    alignment91 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (audio_page_vbox), alignment91, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment91), 0, 6, 12, 0);

    vbox36 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (alignment91), vbox36);

    table13 = gtk_table_new (3, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox36), table13, FALSE, FALSE, 0);
    gtk_table_set_row_spacings (GTK_TABLE (table13), 6);
    gtk_table_set_col_spacings (GTK_TABLE (table13), 6);

    src_converter_type = gtk_combo_box_new_text ();
    gtk_table_attach (GTK_TABLE (table13), src_converter_type, 1, 2, 1, 2,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_combo_box_append_text (GTK_COMBO_BOX (src_converter_type), _("Best Sinc Interpolation"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (src_converter_type), _("Medium Sinc Interpolation"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (src_converter_type), _("Fastest Sinc Interpolation"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (src_converter_type), _("ZOH Interpolation"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (src_converter_type), _("Linear Interpolation"));

    label94 = gtk_label_new (_("Interpolation Engine:"));
    gtk_table_attach (GTK_TABLE (table13), label94, 0, 1, 1, 2,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label94), 0, 0.5);

    label92 = gtk_label_new (_("<span size=\"small\">All streams will be converted to this sampling rate.\nThis should be the max supported sampling rate of\nthe sound card or output plugin.</span>"));
    gtk_table_attach (GTK_TABLE (table13), label92, 1, 2, 2, 3,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_use_markup (GTK_LABEL (label92), TRUE);
    gtk_label_set_line_wrap (GTK_LABEL (label92), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label92), 0, 0.5);

    image9 = gtk_image_new_from_stock ("gtk-info", GTK_ICON_SIZE_BUTTON);
    gtk_table_attach (GTK_TABLE (table13), image9, 0, 1, 2, 3,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (image9), 1, 0);

    src_rate_adj = gtk_adjustment_new (96000, 1000, 768000, 1000, 1000, 1000);
    src_rate = gtk_spin_button_new (GTK_ADJUSTMENT (src_rate_adj), 1, 0);
    gtk_table_attach (GTK_TABLE (table13), src_rate, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    label91 = gtk_label_new (_("Sampling Rate [Hz]:"));
    gtk_table_attach (GTK_TABLE (table13), label91, 0, 1, 0, 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label91), 0, 0.5);

    alignment4 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (audio_page_vbox), alignment4, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment4), 12, 12, 0, 0);

    create_widgets(GTK_BOX(audio_page_vbox), audio_page_widgets2, G_N_ELEMENTS(audio_page_widgets2));

    g_signal_connect(G_OBJECT(output_plugin_bufsize), "value_changed",
                     G_CALLBACK(on_output_plugin_bufsize_value_changed),
                     NULL);
    g_signal_connect_after(G_OBJECT(output_plugin_bufsize), "realize",
                           G_CALLBACK(on_output_plugin_bufsize_realize),
                           NULL);
    g_signal_connect_after(G_OBJECT(output_plugin_cbox), "realize",
                           G_CALLBACK(on_output_plugin_cbox_realize),
                           NULL);
    g_signal_connect(G_OBJECT(enable_src), "toggled",
                     G_CALLBACK(on_enable_src_toggled),
                     NULL);
    g_signal_connect(G_OBJECT(enable_src), "realize",
                     G_CALLBACK(on_enable_src_realize),
                     NULL);
    g_signal_connect(G_OBJECT(src_converter_type), "changed",
                     G_CALLBACK(on_src_converter_type_changed),
                     NULL);
    g_signal_connect_after(G_OBJECT(src_converter_type), "realize",
                           G_CALLBACK(on_src_converter_type_realize),
                           NULL);
    g_signal_connect(G_OBJECT(src_rate), "value_changed",
                     G_CALLBACK(on_src_rate_value_changed),
                     NULL);
    g_signal_connect(G_OBJECT(src_rate), "realize",
                     G_CALLBACK(on_src_rate_realize),
                     NULL);

    /* plugin->output page */

    g_signal_connect(G_OBJECT(output_plugin_cbox), "changed",
                     G_CALLBACK(output_plugin_enable_prefs),
                     output_plugin_prefs);
    g_signal_connect_swapped(G_OBJECT(output_plugin_prefs), "clicked",
                             G_CALLBACK(output_plugin_open_prefs),
                             output_plugin_cbox);

    g_signal_connect(G_OBJECT(output_plugin_cbox), "changed",
                     G_CALLBACK(output_plugin_enable_info),
                     output_plugin_info);
    g_signal_connect_swapped(G_OBJECT(output_plugin_info), "clicked",
                             G_CALLBACK(output_plugin_open_info),
                             output_plugin_cbox);

}

static void
create_connectivity_category(void)
{
    GtkWidget *connectivity_page_vbox;
    GtkWidget *vbox29;
    GtkWidget *alignment63;
    GtkWidget *connectivity_page_label;
    GtkWidget *alignment68;
    GtkWidget *vbox30;
    GtkWidget *alignment65;
    GtkWidget *proxy_use;
    GtkWidget *table8;
    GtkWidget *proxy_port;
    GtkWidget *proxy_host;
    GtkWidget *label69;
    GtkWidget *label68;
    GtkWidget *alignment67;
    GtkWidget *proxy_auth;
    GtkWidget *table9;
    GtkWidget *proxy_pass;
    GtkWidget *proxy_user;
    GtkWidget *label71;
    GtkWidget *label70;
    GtkWidget *alignment72;
    GtkWidget *hbox6;
    GtkWidget *image4;
    GtkWidget *label75;

    connectivity_page_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (category_notebook), connectivity_page_vbox);

    vbox29 = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (connectivity_page_vbox), vbox29, TRUE, TRUE, 0);

    alignment63 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox29), alignment63, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment63), 0, 12, 0, 0);

    connectivity_page_label = gtk_label_new (_("<b>Proxy Configuration</b>"));
    gtk_container_add (GTK_CONTAINER (alignment63), connectivity_page_label);
    gtk_label_set_use_markup (GTK_LABEL (connectivity_page_label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (connectivity_page_label), 0, 0.5);

    alignment68 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox29), alignment68, TRUE, TRUE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment68), 0, 0, 12, 0);

    vbox30 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (alignment68), vbox30);

    alignment65 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox30), alignment65, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment65), 0, 12, 0, 0);

    proxy_use = gtk_check_button_new_with_mnemonic (_("Enable proxy usage"));
    gtk_container_add (GTK_CONTAINER (alignment65), proxy_use);

    table8 = gtk_table_new (2, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox30), table8, FALSE, FALSE, 0);
    gtk_table_set_row_spacings (GTK_TABLE (table8), 6);
    gtk_table_set_col_spacings (GTK_TABLE (table8), 6);

    proxy_port = gtk_entry_new ();
    gtk_table_attach (GTK_TABLE (table8), proxy_port, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    proxy_host = gtk_entry_new ();
    gtk_table_attach (GTK_TABLE (table8), proxy_host, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    label69 = gtk_label_new (_("Proxy port:"));
    gtk_table_attach (GTK_TABLE (table8), label69, 0, 1, 1, 2,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label69), 0, 0.5);

    label68 = gtk_label_new (_("Proxy hostname:"));
    gtk_table_attach (GTK_TABLE (table8), label68, 0, 1, 0, 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label68), 0, 0);

    alignment67 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox30), alignment67, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment67), 12, 12, 0, 0);

    proxy_auth = gtk_check_button_new_with_mnemonic (_("Use authentication with proxy"));
    gtk_container_add (GTK_CONTAINER (alignment67), proxy_auth);

    table9 = gtk_table_new (2, 2, FALSE);
    gtk_box_pack_start (GTK_BOX (vbox30), table9, FALSE, FALSE, 0);
    gtk_table_set_row_spacings (GTK_TABLE (table9), 6);
    gtk_table_set_col_spacings (GTK_TABLE (table9), 6);

    proxy_pass = gtk_entry_new ();
    gtk_table_attach (GTK_TABLE (table9), proxy_pass, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_visibility (GTK_ENTRY (proxy_pass), FALSE);

    proxy_user = gtk_entry_new ();
    gtk_table_attach (GTK_TABLE (table9), proxy_user, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    label71 = gtk_label_new (_("Proxy password:"));
    gtk_table_attach (GTK_TABLE (table9), label71, 0, 1, 1, 2,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label71), 0, 0.5);

    label70 = gtk_label_new (_("Proxy username:"));
    gtk_table_attach (GTK_TABLE (table9), label70, 0, 1, 0, 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label70), 0, 0);

    alignment72 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox30), alignment72, FALSE, FALSE, 0);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment72), 6, 0, 0, 0);

    hbox6 = gtk_hbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (alignment72), hbox6);

    image4 = gtk_image_new_from_stock ("gtk-dialog-warning", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start (GTK_BOX (hbox6), image4, FALSE, FALSE, 0);
    gtk_misc_set_padding (GTK_MISC (image4), 3, 0);

    label75 = gtk_label_new (_("<span size=\"small\">Changing these settings will require a restart of Audacious.</span>"));
    gtk_box_pack_start (GTK_BOX (hbox6), label75, FALSE, FALSE, 0);
    gtk_label_set_use_markup (GTK_LABEL (label75), TRUE);


    g_signal_connect(G_OBJECT(proxy_use), "toggled",
                     G_CALLBACK(on_proxy_button_toggled),
                     "use_proxy");
    g_signal_connect(G_OBJECT(proxy_use), "realize",
                     G_CALLBACK(on_proxy_button_realize),
                     "use_proxy");
    g_signal_connect(G_OBJECT(proxy_port), "changed",
                     G_CALLBACK(on_proxy_entry_changed),
                     "proxy_port");
    g_signal_connect(G_OBJECT(proxy_port), "realize",
                     G_CALLBACK(on_proxy_entry_realize),
                     "proxy_port");
    g_signal_connect(G_OBJECT(proxy_host), "changed",
                     G_CALLBACK(on_proxy_entry_changed),
                     "proxy_host");
    g_signal_connect(G_OBJECT(proxy_host), "realize",
                     G_CALLBACK(on_proxy_entry_realize),
                     "proxy_host");
    g_signal_connect(G_OBJECT(proxy_auth), "toggled",
                     G_CALLBACK(on_proxy_button_toggled),
                     "proxy_use_auth");
    g_signal_connect(G_OBJECT(proxy_auth), "realize",
                     G_CALLBACK(on_proxy_button_realize),
                     "proxy_use_auth");
    g_signal_connect(G_OBJECT(proxy_pass), "changed",
                     G_CALLBACK(on_proxy_entry_changed),
                     "proxy_pass");
    g_signal_connect(G_OBJECT(proxy_pass), "realize",
                     G_CALLBACK(on_proxy_entry_realize),
                     "proxy_pass");
    g_signal_connect(G_OBJECT(proxy_user), "changed",
                     G_CALLBACK(on_proxy_entry_changed),
                     "proxy_user");
    g_signal_connect(G_OBJECT(proxy_user), "realize",
                     G_CALLBACK(on_proxy_entry_realize),
                     "proxy_user");
}

static void
create_plugin_category(void)
{
    GtkWidget *plugin_page_vbox;
    GtkWidget *plugin_notebook;
    GtkWidget *plugin_input_vbox;
    GtkWidget *alignment43;
    GtkWidget *input_plugin_list_label;
    GtkWidget *scrolledwindow3;
    GtkWidget *input_plugin_view;
    GtkWidget *input_plugin_button_box;
    GtkWidget *input_plugin_prefs;
    GtkWidget *input_plugin_info;
    GtkWidget *plugin_input_label;
    GtkWidget *plugin_general_vbox;
    GtkWidget *alignment45;
    GtkWidget *label11;
    GtkWidget *scrolledwindow5;
    GtkWidget *general_plugin_view;
    GtkWidget *general_plugin_button_box;
    GtkWidget *general_plugin_prefs;
    GtkWidget *general_plugin_info;
    GtkWidget *plugin_general_label;
    GtkWidget *vbox21;
    GtkWidget *alignment46;
    GtkWidget *label53;
    GtkWidget *scrolledwindow7;
    GtkWidget *vis_plugin_view;
    GtkWidget *hbuttonbox6;
    GtkWidget *vis_plugin_prefs;
    GtkWidget *vis_plugin_info;
    GtkWidget *vis_label;
    GtkWidget *vbox25;
    GtkWidget *alignment58;
    GtkWidget *label64;
    GtkWidget *scrolledwindow9;
    GtkWidget *effect_plugin_view;
    GtkWidget *hbuttonbox9;
    GtkWidget *effect_plugin_prefs;
    GtkWidget *effect_plugin_info;
    GtkWidget *effects_label;

    plugin_page_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (category_notebook), plugin_page_vbox);

    plugin_notebook = gtk_notebook_new ();
    gtk_box_pack_start (GTK_BOX (plugin_page_vbox), plugin_notebook, TRUE, TRUE, 0);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (plugin_notebook), FALSE);

    plugin_input_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (plugin_notebook), plugin_input_vbox);
    gtk_container_set_border_width (GTK_CONTAINER (plugin_input_vbox), 12);

    alignment43 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (plugin_input_vbox), alignment43, FALSE, FALSE, 4);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment43), 0, 6, 0, 0);

    input_plugin_list_label = gtk_label_new_with_mnemonic (_("_Decoder list:"));
    gtk_container_add (GTK_CONTAINER (alignment43), input_plugin_list_label);
    gtk_label_set_use_markup (GTK_LABEL (input_plugin_list_label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (input_plugin_list_label), 0, 0.5);

    scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX (plugin_input_vbox), scrolledwindow3, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_SHADOW_IN);

    input_plugin_view = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolledwindow3), input_plugin_view);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (input_plugin_view), TRUE);
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (input_plugin_view), TRUE);

    input_plugin_button_box = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (plugin_input_vbox), input_plugin_button_box, FALSE, FALSE, 8);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (input_plugin_button_box), GTK_BUTTONBOX_START);
    gtk_box_set_spacing (GTK_BOX (input_plugin_button_box), 8);

    input_plugin_prefs = gtk_button_new_from_stock ("gtk-preferences");
    gtk_container_add (GTK_CONTAINER (input_plugin_button_box), input_plugin_prefs);
    gtk_widget_set_sensitive (input_plugin_prefs, FALSE);
    GTK_WIDGET_SET_FLAGS (input_plugin_prefs, GTK_CAN_DEFAULT);

    input_plugin_info = gtk_button_new_from_stock ("gtk-dialog-info");
    gtk_container_add (GTK_CONTAINER (input_plugin_button_box), input_plugin_info);
    gtk_widget_set_sensitive (input_plugin_info, FALSE);
    GTK_WIDGET_SET_FLAGS (input_plugin_info, GTK_CAN_DEFAULT);

    plugin_input_label = gtk_label_new (_("<span size=\"medium\"><b>Decoders</b></span>"));
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (plugin_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (plugin_notebook), 0), plugin_input_label);
    gtk_label_set_use_markup (GTK_LABEL (plugin_input_label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (plugin_input_label), 0, 0);

    plugin_general_vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (plugin_notebook), plugin_general_vbox);
    gtk_container_set_border_width (GTK_CONTAINER (plugin_general_vbox), 12);

    alignment45 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (plugin_general_vbox), alignment45, FALSE, FALSE, 4);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment45), 0, 6, 0, 0);

    label11 = gtk_label_new_with_mnemonic (_("_General plugin list:"));
    gtk_container_add (GTK_CONTAINER (alignment45), label11);
    gtk_label_set_use_markup (GTK_LABEL (label11), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label11), 0, 0.5);

    scrolledwindow5 = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX (plugin_general_vbox), scrolledwindow5, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow5), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow5), GTK_SHADOW_IN);

    general_plugin_view = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolledwindow5), general_plugin_view);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (general_plugin_view), TRUE);
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (general_plugin_view), TRUE);

    general_plugin_button_box = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (plugin_general_vbox), general_plugin_button_box, FALSE, FALSE, 8);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (general_plugin_button_box), GTK_BUTTONBOX_START);
    gtk_box_set_spacing (GTK_BOX (general_plugin_button_box), 8);

    general_plugin_prefs = gtk_button_new_from_stock ("gtk-preferences");
    gtk_container_add (GTK_CONTAINER (general_plugin_button_box), general_plugin_prefs);
    gtk_widget_set_sensitive (general_plugin_prefs, FALSE);
    GTK_WIDGET_SET_FLAGS (general_plugin_prefs, GTK_CAN_DEFAULT);

    general_plugin_info = gtk_button_new_from_stock ("gtk-dialog-info");
    gtk_container_add (GTK_CONTAINER (general_plugin_button_box), general_plugin_info);
    gtk_widget_set_sensitive (general_plugin_info, FALSE);
    GTK_WIDGET_SET_FLAGS (general_plugin_info, GTK_CAN_DEFAULT);

    plugin_general_label = gtk_label_new (_("<span size=\"medium\"><b>General</b></span>"));
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (plugin_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (plugin_notebook), 1), plugin_general_label);
    gtk_label_set_use_markup (GTK_LABEL (plugin_general_label), TRUE);

    vbox21 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (plugin_notebook), vbox21);
    gtk_container_set_border_width (GTK_CONTAINER (vbox21), 12);

    alignment46 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox21), alignment46, FALSE, FALSE, 4);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment46), 0, 6, 0, 0);

    label53 = gtk_label_new_with_mnemonic (_("_Visualization plugin list:"));
    gtk_container_add (GTK_CONTAINER (alignment46), label53);
    gtk_label_set_use_markup (GTK_LABEL (label53), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label53), 0, 0.5);

    scrolledwindow7 = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX (vbox21), scrolledwindow7, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow7), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow7), GTK_SHADOW_IN);

    vis_plugin_view = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolledwindow7), vis_plugin_view);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (vis_plugin_view), TRUE);
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (vis_plugin_view), TRUE);

    hbuttonbox6 = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (vbox21), hbuttonbox6, FALSE, FALSE, 8);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox6), GTK_BUTTONBOX_START);
    gtk_box_set_spacing (GTK_BOX (hbuttonbox6), 8);

    vis_plugin_prefs = gtk_button_new_from_stock ("gtk-preferences");
    gtk_container_add (GTK_CONTAINER (hbuttonbox6), vis_plugin_prefs);
    gtk_widget_set_sensitive (vis_plugin_prefs, FALSE);
    GTK_WIDGET_SET_FLAGS (vis_plugin_prefs, GTK_CAN_DEFAULT);

    vis_plugin_info = gtk_button_new_from_stock ("gtk-dialog-info");
    gtk_container_add (GTK_CONTAINER (hbuttonbox6), vis_plugin_info);
    gtk_widget_set_sensitive (vis_plugin_info, FALSE);
    GTK_WIDGET_SET_FLAGS (vis_plugin_info, GTK_CAN_DEFAULT);

    vis_label = gtk_label_new (_("<b>Visualization</b>"));
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (plugin_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (plugin_notebook), 2), vis_label);
    gtk_label_set_use_markup (GTK_LABEL (vis_label), TRUE);

    vbox25 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (plugin_notebook), vbox25);
    gtk_container_set_border_width (GTK_CONTAINER (vbox25), 12);

    alignment58 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start (GTK_BOX (vbox25), alignment58, FALSE, FALSE, 4);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment58), 0, 6, 0, 0);

    label64 = gtk_label_new_with_mnemonic (_("_Effect plugin list:"));
    gtk_container_add (GTK_CONTAINER (alignment58), label64);
    gtk_label_set_use_markup (GTK_LABEL (label64), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label64), 0, 0.5);

    scrolledwindow9 = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX (vbox25), scrolledwindow9, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow9), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow9), GTK_SHADOW_IN);

    effect_plugin_view = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolledwindow9), effect_plugin_view);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (effect_plugin_view), TRUE);
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (effect_plugin_view), TRUE);

    hbuttonbox9 = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (vbox25), hbuttonbox9, FALSE, FALSE, 8);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox9), GTK_BUTTONBOX_START);
    gtk_box_set_spacing (GTK_BOX (hbuttonbox9), 8);

    effect_plugin_prefs = gtk_button_new_from_stock ("gtk-preferences");
    gtk_container_add (GTK_CONTAINER (hbuttonbox9), effect_plugin_prefs);
    gtk_widget_set_sensitive (effect_plugin_prefs, FALSE);
    GTK_WIDGET_SET_FLAGS (effect_plugin_prefs, GTK_CAN_DEFAULT);

    effect_plugin_info = gtk_button_new_from_stock ("gtk-dialog-info");
    gtk_container_add (GTK_CONTAINER (hbuttonbox9), effect_plugin_info);
    gtk_widget_set_sensitive (effect_plugin_info, FALSE);
    GTK_WIDGET_SET_FLAGS (effect_plugin_info, GTK_CAN_DEFAULT);

    effects_label = gtk_label_new (_("<b>Effects</b>"));
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (plugin_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (plugin_notebook), 3), effects_label);
    gtk_label_set_use_markup (GTK_LABEL (effects_label), TRUE);



    gtk_label_set_mnemonic_widget (GTK_LABEL (input_plugin_list_label), category_notebook);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label11), category_notebook);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label53), category_notebook);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label64), category_notebook);



    g_signal_connect_after(G_OBJECT(input_plugin_view), "realize",
                           G_CALLBACK(on_input_plugin_view_realize),
                           NULL);
    g_signal_connect_after(G_OBJECT(general_plugin_view), "realize",
                           G_CALLBACK(on_general_plugin_view_realize),
                           NULL);
    g_signal_connect_after(G_OBJECT(vis_plugin_view), "realize",
                           G_CALLBACK(on_vis_plugin_view_realize),
                           NULL);
    g_signal_connect_after(G_OBJECT(effect_plugin_view), "realize",
                           G_CALLBACK(on_effect_plugin_view_realize),
                           NULL);



    /* plugin->input page */
    g_object_set_data(G_OBJECT(input_plugin_view), "plugin_type" , GINT_TO_POINTER(PLUGIN_VIEW_TYPE_INPUT));
    g_signal_connect(G_OBJECT(input_plugin_view), "row-activated",
                     G_CALLBACK(plugin_treeview_open_prefs),
                     NULL);
    g_signal_connect(G_OBJECT(input_plugin_view), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_prefs),
                     input_plugin_prefs);

    g_signal_connect_swapped(G_OBJECT(input_plugin_prefs), "clicked",
                             G_CALLBACK(plugin_treeview_open_prefs),
                             input_plugin_view);

    g_signal_connect(G_OBJECT(input_plugin_view), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_info),
                     input_plugin_info);
    g_signal_connect_swapped(G_OBJECT(input_plugin_info), "clicked",
                             G_CALLBACK(plugin_treeview_open_info),
                             input_plugin_view);


    /* plugin->general page */

    g_object_set_data(G_OBJECT(general_plugin_view), "plugin_type" , GINT_TO_POINTER(PLUGIN_VIEW_TYPE_GENERAL));
    g_signal_connect(G_OBJECT(general_plugin_view), "row-activated",
                     G_CALLBACK(plugin_treeview_open_prefs),
                     NULL);

    g_signal_connect(G_OBJECT(general_plugin_view), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_prefs),
                     general_plugin_prefs);

    g_signal_connect_swapped(G_OBJECT(general_plugin_prefs), "clicked",
                             G_CALLBACK(plugin_treeview_open_prefs),
                             general_plugin_view);

    g_signal_connect(G_OBJECT(general_plugin_view), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_info),
                     general_plugin_info);
    g_signal_connect_swapped(G_OBJECT(general_plugin_info), "clicked",
                             G_CALLBACK(plugin_treeview_open_info),
                             general_plugin_view);


    /* plugin->vis page */

    g_object_set_data(G_OBJECT(vis_plugin_view), "plugin_type" , GINT_TO_POINTER(PLUGIN_VIEW_TYPE_VIS));
    g_signal_connect(G_OBJECT(vis_plugin_view), "row-activated",
                     G_CALLBACK(plugin_treeview_open_prefs),
                     NULL);
    g_signal_connect_swapped(G_OBJECT(vis_plugin_prefs), "clicked",
                             G_CALLBACK(plugin_treeview_open_prefs),
                             vis_plugin_view);
    g_signal_connect(G_OBJECT(vis_plugin_view), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_prefs), vis_plugin_prefs);

    g_signal_connect(G_OBJECT(vis_plugin_view), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_info), vis_plugin_info);
    g_signal_connect_swapped(G_OBJECT(vis_plugin_info), "clicked",
                             G_CALLBACK(plugin_treeview_open_info),
                             vis_plugin_view);


    /* plugin->effects page */

    g_object_set_data(G_OBJECT(effect_plugin_view), "plugin_type" , GINT_TO_POINTER(PLUGIN_VIEW_TYPE_EFFECT));
    g_signal_connect(G_OBJECT(effect_plugin_view), "row-activated",
                     G_CALLBACK(plugin_treeview_open_prefs),
                     NULL);
    g_signal_connect_swapped(G_OBJECT(effect_plugin_prefs), "clicked",
                             G_CALLBACK(plugin_treeview_open_prefs),
                             effect_plugin_view);
    g_signal_connect(G_OBJECT(effect_plugin_view), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_prefs), effect_plugin_prefs);

    g_signal_connect(G_OBJECT(effect_plugin_view), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_info), effect_plugin_info);
    g_signal_connect_swapped(G_OBJECT(effect_plugin_info), "clicked",
                             G_CALLBACK(plugin_treeview_open_info),
                             effect_plugin_view);

}

void
create_prefs_window(void)
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
    GTK_WIDGET_UNSET_FLAGS (category_notebook, GTK_CAN_FOCUS);
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (category_notebook), FALSE);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (category_notebook), FALSE);
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (category_notebook), TRUE);



    create_audio_category();
    create_replay_gain_category();
    create_connectivity_category();
    create_mouse_category();
    create_playback_category();
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
    GTK_WIDGET_SET_FLAGS (close, GTK_CAN_DEFAULT);
    gtk_widget_add_accelerator (close, "clicked", accel_group,
                                GDK_Escape, (GdkModifierType) 0,
                                GTK_ACCEL_VISIBLE);


    gtk_window_add_accel_group (GTK_WINDOW (prefswin), accel_group);

    /* connect signals */
    g_signal_connect(G_OBJECT(prefswin), "delete_event",
                     G_CALLBACK(gtk_widget_hide_on_delete),
                     NULL);
    g_signal_connect_swapped(G_OBJECT(close), "clicked",
                             G_CALLBACK(gtk_widget_hide),
                             GTK_OBJECT (prefswin));

    /* create category view */
    g_signal_connect_after(G_OBJECT(category_treeview), "realize",
                           G_CALLBACK(on_category_treeview_realize),
                           category_notebook);


    /* audacious version label */

    aud_version_string = g_strdup_printf("<span size='small'>%s (%s) (%s@%s)</span>",
                                         "Audacious " PACKAGE_VERSION ,
                                         svn_stamp ,
                                         g_get_user_name() , g_get_host_name() );

    gtk_label_set_markup( GTK_LABEL(audversionlabel) , aud_version_string );
    g_free(aud_version_string);
    gtk_widget_show_all(vbox);
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

static void
prefswin_page_queue_new(GtkWidget *container, gchar *name, gchar *imgurl)
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
gint
prefswin_page_new(GtkWidget *container, gchar *name, gchar *imgurl)
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
