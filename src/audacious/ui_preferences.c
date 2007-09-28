/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2006  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "glade.h"

#include "plugin.h"
#include "pluginenum.h"
#include "input.h"
#include "effect.h"
#include "general.h"
#include "output.h"
#include "visualization.h"

#include "main.h"
#include "ui_skinned_textbox.h"
#include "strings.h"
#include "util.h"
#include "dnd.h"
#include "configdb.h"

#include "ui_main.h"
#include "ui_playlist.h"
#include "ui_skinselector.h"
#include "ui_preferences.h"
#include "ui_equalizer.h"
#include "ui_skinned_playlist.h"
#include "ui_skinned_window.h"

#include "build_stamp.h"

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
    gint id;
} Category;

typedef struct {
    const gchar *name;
    const gchar *tag;
}
TitleFieldTag;

static GtkWidget *prefswin = NULL;
static GtkWidget *filepopup_settings = NULL;
static GtkWidget *colorize_settings = NULL;
static GtkWidget *category_treeview = NULL;
static GtkWidget *category_notebook = NULL;
GtkWidget *filepopupbutton = NULL;

static Category categories[] = {
    {DATA_DIR "/images/appearance.png", N_("Appearance"), 1},
    {DATA_DIR "/images/audio.png", N_("Audio"), 6},
    {DATA_DIR "/images/connectivity.png",    N_("Connectivity"), 5},	
    {DATA_DIR "/images/eq.png",         N_("Equalizer"), 4},
    {DATA_DIR "/images/mouse.png",      N_("Mouse"), 2},
    {DATA_DIR "/images/playlist.png",   N_("Playlist"), 3},
    {DATA_DIR "/images/plugins.png",    N_("Plugins"), 0},
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

typedef struct {
    void *next;
    GtkWidget *container;
    char *pg_name;
    char *img_url;
} CategoryQueueEntry;

CategoryQueueEntry *category_queue = NULL;

static const guint n_title_field_tags = G_N_ELEMENTS(title_field_tags);

/* GLib 2.6 compatibility */
#if (! ((GLIB_MAJOR_VERSION > 2) || ((GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION >= 8))))
static const char *
g_get_host_name (void)
{
    static char hostname [HOST_NAME_MAX + 1];
    if (gethostname (hostname, HOST_NAME_MAX) == -1) {
        return _("localhost");
    }
    return hostname;
}
#endif

static void prefswin_page_queue_destroy(CategoryQueueEntry *ent);

static GladeXML *
prefswin_get_xml(void)
{
    return GLADE_XML(g_object_get_data(G_OBJECT(prefswin), "glade-xml"));
}

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
    GladeXML *xml;
    GtkWidget *notebook;
    
    g_return_if_fail(index >= 0 && index < n_categories);

    xml = prefswin_get_xml();
    notebook = glade_xml_get_widget(xml, "category_view");
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), index);
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
input_plugin_toggle(GtkCellRendererToggle * cell,
                    const gchar * path_str,
                    gpointer data)
{
    GtkTreeModel *model = GTK_TREE_MODEL(data);
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gint pluginnr;
    Plugin *plugin;
    /*GList *diplist, *tmplist; */

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter,
                       PLUGIN_VIEW_COL_ID, &pluginnr,
		       PLUGIN_VIEW_COL_PLUGIN_PTR, &plugin,
                       -1);

    /* do something with the value */
    plugin->enabled ^= 1;

    /* set new value */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       PLUGIN_VIEW_COL_ACTIVE, plugin->enabled, -1);

    /* clean up */
    gtk_tree_path_free(path);
}


static void
vis_plugin_toggle(GtkCellRendererToggle * cell,
                  const gchar * path_str,
                  gpointer data)
{
    GtkTreeModel *model = GTK_TREE_MODEL(data);
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;
    gint pluginnr;

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter,
                       PLUGIN_VIEW_COL_ACTIVE, &fixed,
                       PLUGIN_VIEW_COL_ID, &pluginnr, -1);

    /* do something with the value */
    fixed ^= 1;

    enable_vis_plugin(pluginnr, fixed);

    /* set new value */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       PLUGIN_VIEW_COL_ACTIVE, fixed, -1);

    /* clean up */
    gtk_tree_path_free(path);
}

static void
effect_plugin_toggle(GtkCellRendererToggle * cell,
                  const gchar * path_str,
                  gpointer data)
{
    GtkTreeModel *model = GTK_TREE_MODEL(data);
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;
    gint pluginnr;

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter,
                       PLUGIN_VIEW_COL_ACTIVE, &fixed,
                       PLUGIN_VIEW_COL_ID, &pluginnr, -1);

    /* do something with the value */
    fixed ^= 1;

    enable_effect_plugin(pluginnr, fixed);

    /* set new value */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       PLUGIN_VIEW_COL_ACTIVE, fixed, -1);

    /* clean up */
    gtk_tree_path_free(path);
}
static void
general_plugin_toggle(GtkCellRendererToggle * cell,
                      const gchar * path_str,
                      gpointer data)
{
    GtkTreeModel *model = GTK_TREE_MODEL(data);
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean fixed;
    gint pluginnr;

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter,
                       PLUGIN_VIEW_COL_ACTIVE, &fixed,
                       PLUGIN_VIEW_COL_ID, &pluginnr, -1);

    /* do something with the value */
    fixed ^= 1;

    enable_general_plugin(pluginnr, fixed);

    /* set new value */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       PLUGIN_VIEW_COL_ACTIVE, fixed, -1);

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
                       gpointer data)
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
    on_plugin_view_realize(treeview, G_CALLBACK(input_plugin_toggle), ip_data.input_list);
}

static void
on_effect_plugin_view_realize(GtkTreeView * treeview,
                              gpointer data)
{
    on_plugin_view_realize(treeview, G_CALLBACK(effect_plugin_toggle), ep_data.effect_list);
}

static void
on_general_plugin_view_realize(GtkTreeView * treeview,
                               gpointer data)
{
    on_plugin_view_realize(treeview, G_CALLBACK(general_plugin_toggle), gp_data.general_list);
}

static void
on_vis_plugin_view_realize(GtkTreeView * treeview,
                           gpointer data)
{
    on_plugin_view_realize(treeview, G_CALLBACK(vis_plugin_toggle), vp_data.vis_list);
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
    GladeXML *xml;
    GtkWidget *entry;
    gint item = GPOINTER_TO_INT(data);
    gint pos;
    
    xml = prefswin_get_xml();
    entry = glade_xml_get_widget(xml, "titlestring_entry");

    pos = gtk_editable_get_position(GTK_EDITABLE(entry));

    /* insert separator as needed */
    if (g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(entry)), -1) > 0)
        editable_insert_text(GTK_EDITABLE(entry), separator, &pos);

    editable_insert_text(GTK_EDITABLE(entry), _(title_field_tags[item].tag),
                         &pos);

    gtk_editable_set_position(GTK_EDITABLE(entry), pos);
}

static void
on_titlestring_help_button_clicked(GtkButton * button,
                                   gpointer data) 
{
    
    GtkMenu *menu;
    MenuPos *pos = g_new0(MenuPos, 1);
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

static void
on_titlestring_entry_changed(GtkWidget * entry,
                             gpointer data) 
{
    g_free(cfg.gentitle_format);
    cfg.gentitle_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
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
}

static void
on_mainwin_font_button_font_set(GtkFontButton * button,
                                gpointer data)
{
    g_free(cfg.mainwin_font);
    cfg.mainwin_font = g_strdup(gtk_font_button_get_font_name(button));

    ui_skinned_textbox_set_xfont(mainwin_info, cfg.mainwin_use_xfont, cfg.mainwin_font);
}

static void
on_use_bitmap_fonts_realize(GtkToggleButton * button,
                            gpointer data)
{
    gtk_toggle_button_set_active(button,
	cfg.mainwin_use_xfont != FALSE ? FALSE : TRUE);
}

static void
on_use_bitmap_fonts_toggled(GtkToggleButton * button,
                                    gpointer data)
{
    gboolean useit = gtk_toggle_button_get_active(button);
    cfg.mainwin_use_xfont = useit != FALSE ? FALSE : TRUE;
    ui_skinned_textbox_set_xfont(mainwin_info, cfg.mainwin_use_xfont, cfg.mainwin_font);
    playlistwin_set_sinfo_font(cfg.playlist_font);

    if (cfg.playlist_shaded) {
        playlistwin_update_list(playlist_get_active());
        ui_skinned_window_draw_all(playlistwin);
    }
}

static void
on_mainwin_font_button_realize(GtkFontButton * button,
                               gpointer data)
{
    gtk_font_button_set_font_name(button, cfg.mainwin_font);
}

static void
on_playlist_font_button_font_set(GtkFontButton * button,
                                 gpointer data)
{
    g_free(cfg.playlist_font);
    cfg.playlist_font = g_strdup(gtk_font_button_get_font_name(button));

    ui_skinned_playlist_set_font(cfg.playlist_font);
    playlistwin_set_sinfo_font(cfg.playlist_font);  /* propagate font setting to playlistwin_sinfo */
    playlistwin_update_list(playlist_get_active());
    gtk_widget_queue_draw(playlistwin_list);
}

static void
on_playlist_font_button_realize(GtkFontButton * button,
                                gpointer data)
{
    gtk_font_button_set_font_name(button, cfg.playlist_font);
}

static void
on_playlist_show_pl_numbers_realize(GtkToggleButton * button,
                                    gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.show_numbers_in_pl);
}

static void
on_playlist_show_pl_numbers_toggled(GtkToggleButton * button,
                                    gpointer data)
{
    cfg.show_numbers_in_pl = gtk_toggle_button_get_active(button);
    playlistwin_update_list(playlist_get_active());
    gtk_widget_queue_draw(playlistwin_list);
}

static void
on_playlist_show_pl_separator_realize(GtkToggleButton * button,
                                    gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.show_separator_in_pl);
}

static void
on_playlist_show_pl_separator_toggled(GtkToggleButton * button,
                                    gpointer data)
{
    cfg.show_separator_in_pl = gtk_toggle_button_get_active(button);
    playlistwin_update_list(playlist_get_active());
    gtk_widget_queue_draw(playlistwin_list);
}

/* format detection */
static void
on_audio_format_det_cb_toggled(GtkToggleButton * button,
                                    gpointer data)
{
    cfg.playlist_detect = gtk_toggle_button_get_active(button);
}

static void
on_audio_format_det_cb_realize(GtkToggleButton * button,
                                    gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.playlist_detect);
}

static void
on_detect_by_extension_cb_toggled(GtkToggleButton * button,
                                    gpointer data)
{
    cfg.use_extension_probing = gtk_toggle_button_get_active(button);
}

static void
on_detect_by_extension_cb_realize(GtkToggleButton * button,
                                    gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.use_extension_probing);
}

/* proxy */
static void
on_proxy_use_realize(GtkToggleButton * button,
                     gpointer data)
{
    ConfigDb *db;
    gboolean ret;

    db = bmp_cfg_db_open();

    if (bmp_cfg_db_get_bool(db, NULL, "use_proxy", &ret) != FALSE)
        gtk_toggle_button_set_active(button, ret);

    bmp_cfg_db_close(db);
}

static void
on_proxy_use_toggled(GtkToggleButton * button,
                     gpointer data)
{
    ConfigDb *db;
    gboolean ret = gtk_toggle_button_get_active(button);

    db = bmp_cfg_db_open();
    bmp_cfg_db_set_bool(db, NULL, "use_proxy", ret);
    bmp_cfg_db_close(db);
}

static void
on_proxy_auth_realize(GtkToggleButton * button,
                     gpointer data)
{
    ConfigDb *db;
    gboolean ret;

    db = bmp_cfg_db_open();

    if (bmp_cfg_db_get_bool(db, NULL, "proxy_use_auth", &ret) != FALSE)
        gtk_toggle_button_set_active(button, ret);

    bmp_cfg_db_close(db);
}

static void
on_proxy_auth_toggled(GtkToggleButton * button,
                     gpointer data)
{
    ConfigDb *db;
    gboolean ret = gtk_toggle_button_get_active(button);

    db = bmp_cfg_db_open();
    bmp_cfg_db_set_bool(db, NULL, "proxy_use_auth", ret);
    bmp_cfg_db_close(db);
}

static void
on_proxy_host_realize(GtkEntry * entry,
                     gpointer data)
{
    ConfigDb *db;
    gchar *ret;

    db = bmp_cfg_db_open();

    if (bmp_cfg_db_get_string(db, NULL, "proxy_host", &ret) != FALSE)
        gtk_entry_set_text(entry, ret);

    bmp_cfg_db_close(db);
}

static void
on_proxy_host_changed(GtkEntry * entry,
                     gpointer data)
{
    ConfigDb *db;
    gchar *ret = g_strdup(gtk_entry_get_text(entry));

    db = bmp_cfg_db_open();
    bmp_cfg_db_set_string(db, NULL, "proxy_host", ret);
    bmp_cfg_db_close(db);

    g_free(ret);
}

static void
on_proxy_port_realize(GtkEntry * entry,
                     gpointer data)
{
    ConfigDb *db;
    gchar *ret;

    db = bmp_cfg_db_open();

    if (bmp_cfg_db_get_string(db, NULL, "proxy_port", &ret) != FALSE)
        gtk_entry_set_text(entry, ret);

    bmp_cfg_db_close(db);
}

static void
on_proxy_port_changed(GtkEntry * entry,
                     gpointer data)
{
    ConfigDb *db;
    gchar *ret = g_strdup(gtk_entry_get_text(entry));

    db = bmp_cfg_db_open();
    bmp_cfg_db_set_string(db, NULL, "proxy_port", ret);
    bmp_cfg_db_close(db);

    g_free(ret);
}

static void
on_proxy_user_realize(GtkEntry * entry,
                     gpointer data)
{
    ConfigDb *db;
    gchar *ret;

    db = bmp_cfg_db_open();

    if (bmp_cfg_db_get_string(db, NULL, "proxy_user", &ret) != FALSE)
        gtk_entry_set_text(entry, ret);

    bmp_cfg_db_close(db);
}

static void
on_proxy_user_changed(GtkEntry * entry,
                     gpointer data)
{
    ConfigDb *db;
    gchar *ret = g_strdup(gtk_entry_get_text(entry));

    db = bmp_cfg_db_open();
    bmp_cfg_db_set_string(db, NULL, "proxy_user", ret);
    bmp_cfg_db_close(db);

    g_free(ret);
}

static void
on_proxy_pass_realize(GtkEntry * entry,
                     gpointer data)
{
    ConfigDb *db;
    gchar *ret;

    db = bmp_cfg_db_open();

    if (bmp_cfg_db_get_string(db, NULL, "proxy_pass", &ret) != FALSE)
        gtk_entry_set_text(entry, ret);

    bmp_cfg_db_close(db);
}

static void
on_proxy_pass_changed(GtkEntry * entry,
                     gpointer data)
{
    ConfigDb *db;
    gchar *ret = g_strdup(gtk_entry_get_text(entry));

    db = bmp_cfg_db_open();
    bmp_cfg_db_set_string(db, NULL, "proxy_pass", ret);
    bmp_cfg_db_close(db);

    g_free(ret);
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
on_mouse_wheel_volume_realize(GtkSpinButton * button,
                              gpointer data)
{
    gtk_spin_button_set_value(button, cfg.mouse_change);
}

static void
on_mouse_wheel_volume_changed(GtkSpinButton * button,
                              gpointer data)
{
    cfg.mouse_change = gtk_spin_button_get_value_as_int(button);
}

static void
on_pause_between_songs_time_realize(GtkSpinButton * button,
                                    gpointer data)
{
    gtk_spin_button_set_value(button, cfg.pause_between_songs_time);
}

static void
on_pause_between_songs_time_changed(GtkSpinButton * button,
                                    gpointer data)
{
    cfg.pause_between_songs_time = gtk_spin_button_get_value_as_int(button);
}

static void
on_enable_src_realize(GtkToggleButton * button,
                                    gpointer data)
{
#ifdef USE_SRC
    ConfigDb *db;
    gboolean ret;

    db = bmp_cfg_db_open();

    if (bmp_cfg_db_get_bool(db, NULL, "enable_src", &ret) != FALSE)
        gtk_toggle_button_set_active(button, ret);

    bmp_cfg_db_close(db);
#else
    gtk_toggle_button_set_active(button, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
#endif
}

static void
on_enable_src_toggled(GtkToggleButton * button,
                                    gpointer data)
{
    ConfigDb *db;
    gboolean ret = gtk_toggle_button_get_active(button);

    db = bmp_cfg_db_open();
    bmp_cfg_db_set_bool(db, NULL, "enable_src", ret);
    bmp_cfg_db_close(db);
}

static void
on_src_rate_realize(GtkSpinButton * button,
                                    gpointer data)
{
#ifdef USE_SRC
    ConfigDb *db;
    gint value;

    db = bmp_cfg_db_open();

    if (bmp_cfg_db_get_int(db, NULL, "src_rate", &value) != FALSE)
        gtk_spin_button_set_value(button, (gdouble)value);

    bmp_cfg_db_close(db);
#else
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
#endif
}

static void
on_src_rate_value_changed(GtkSpinButton * button,
                                    gpointer data)
{
    ConfigDb *db;
    gint value = gtk_spin_button_get_value_as_int(button);

    db = bmp_cfg_db_open();
    bmp_cfg_db_set_int(db, NULL, "src_rate", value);
    bmp_cfg_db_close(db);
}

static void
on_src_converter_type_realize(GtkComboBox * box,
			      gpointer data)
{
#ifdef USE_SRC
    ConfigDb *db;
    gint value;

    db = bmp_cfg_db_open();

    if (bmp_cfg_db_get_int(db, NULL, "src_type", &value) != FALSE)
        gtk_combo_box_set_active(box, value);
    else
        gtk_combo_box_set_active(box, 0);

    bmp_cfg_db_close(db);
#else
    gtk_widget_set_sensitive(GTK_WIDGET(box), FALSE);
#endif
}

static void
on_src_converter_type_changed(GtkComboBox * box,
			      gpointer data)
{
    ConfigDb *db;
    gint value = gtk_combo_box_get_active(box);

    db = bmp_cfg_db_open();
    bmp_cfg_db_set_int(db, NULL, "src_type", value);
    bmp_cfg_db_close(db);
}

static void
on_mouse_wheel_scroll_pl_realize(GtkSpinButton * button,
                                 gpointer data)
{
    gtk_spin_button_set_value(button, cfg.scroll_pl_by);
}

static void
on_mouse_wheel_scroll_pl_changed(GtkSpinButton * button,
                                 gpointer data)
{
    cfg.scroll_pl_by = gtk_spin_button_get_value_as_int(button);
}

static void
on_playlist_convert_underscore_realize(GtkToggleButton * button,
                                       gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.convert_underscore);
}

static void
on_playlist_convert_underscore_toggled(GtkToggleButton * button,
                                       gpointer data)
{
    cfg.convert_underscore = gtk_toggle_button_get_active(button);
}

static void
on_playlist_no_advance_realize(GtkToggleButton * button, gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.no_playlist_advance);
}

static void
on_playlist_no_advance_toggled(GtkToggleButton * button, gpointer data)
{
    cfg.no_playlist_advance = gtk_toggle_button_get_active(button);
}

static void
on_continue_playback_on_startup_realize(GtkToggleButton * button, gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.resume_playback_on_startup);
}

static void
on_software_volume_control_toggled(GtkToggleButton * button, gpointer data)
{
    cfg.software_volume_control = gtk_toggle_button_get_active(button);
}

static void
on_software_volume_control_realize(GtkToggleButton * button, gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.software_volume_control);
}

static void
on_continue_playback_on_startup_toggled(GtkToggleButton * button, gpointer data)
{
    cfg.resume_playback_on_startup = gtk_toggle_button_get_active(button);
}

static void
on_refresh_file_list_realize(GtkToggleButton * button, gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.refresh_file_list);
}

static void
on_refresh_file_list_toggled(GtkToggleButton * button, gpointer data)
{
    cfg.refresh_file_list = gtk_toggle_button_get_active(button);
}

static void
on_playlist_convert_twenty_realize(GtkToggleButton * button, gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.convert_twenty);
}

static void
on_playlist_convert_twenty_toggled(GtkToggleButton * button, gpointer data)
{
    cfg.convert_twenty = gtk_toggle_button_get_active(button);
}

static void
on_playlist_convert_slash_realize(GtkToggleButton * button, gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.convert_slash);
}

static void
on_playlist_convert_slash_toggled(GtkToggleButton * button, gpointer data)
{
    cfg.convert_slash = gtk_toggle_button_get_active(button);
}

static void
on_use_pl_metadata_realize(GtkToggleButton * button,
                           gpointer data)
{
    gboolean state = cfg.use_pl_metadata;
    gtk_toggle_button_set_active(button, state);
    gtk_widget_set_sensitive(GTK_WIDGET(data), state);
}

static void
on_use_pl_metadata_toggled(GtkToggleButton * button,
                           gpointer data)
{
    gboolean state = gtk_toggle_button_get_active(button);
    cfg.use_pl_metadata = state;
    gtk_widget_set_sensitive(GTK_WIDGET(data), state);
}

static void
on_pause_between_songs_realize(GtkToggleButton * button,
                               gpointer data)
{
    gboolean state = cfg.pause_between_songs;
    gtk_toggle_button_set_active(button, state);
    gtk_widget_set_sensitive(GTK_WIDGET(data), state);
}

static void
on_pause_between_songs_toggled(GtkToggleButton * button,
                               gpointer data)
{
    gboolean state = gtk_toggle_button_get_active(button);
    cfg.pause_between_songs = state;
    gtk_widget_set_sensitive(GTK_WIDGET(data), state);
}

static void
on_pl_metadata_on_load_realize(GtkRadioButton * button,
                               gpointer data)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 cfg.get_info_on_load);
}

static void
on_pl_metadata_on_display_realize(GtkRadioButton * button,
                                  gpointer data)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 cfg.get_info_on_demand);
}

static void
on_pl_metadata_on_load_toggled(GtkRadioButton * button,
                               gpointer data)
{
    cfg.get_info_on_load = 
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}

static void
on_pl_metadata_on_display_toggled(GtkRadioButton * button,
                                  gpointer data)
{
    cfg.get_info_on_demand =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
}

static void
on_custom_cursors_realize(GtkToggleButton * button,
                          gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.custom_cursors);
}

static void
on_custom_cursors_toggled(GtkToggleButton *togglebutton,
                          gpointer data)
{
    cfg.custom_cursors = gtk_toggle_button_get_active(togglebutton);
    skin_reload_forced();
}

static void
on_eq_dir_preset_entry_realize(GtkEntry * entry,
                               gpointer data)
{
    gtk_entry_set_text(entry, cfg.eqpreset_default_file);
}

static void
on_eq_dir_preset_entry_changed(GtkEntry * entry,
                               gpointer data)
{
    g_free(cfg.eqpreset_default_file);
    cfg.eqpreset_default_file = g_strdup(gtk_entry_get_text(entry));
}

static void
on_eq_file_preset_entry_realize(GtkEntry * entry,
                                gpointer data)
{
    gtk_entry_set_text(entry, cfg.eqpreset_extension);
}

static void
on_eq_file_preset_entry_changed(GtkEntry * entry, gpointer data)
{
    const gchar *text = gtk_entry_get_text(entry);

    while (*text == '.')
        text++;

    g_free(cfg.eqpreset_extension);
    cfg.eqpreset_extension = g_strdup(text);
}


/* FIXME: implement these */

static void
on_eq_preset_view_realize(GtkTreeView * treeview,
                          gpointer data)
{}

static void
on_eq_preset_add_clicked(GtkButton * button,
                         gpointer data)
{}

static void
on_eq_preset_remove_clicked(GtkButton * button,
                            gpointer data)
{}

static void
on_skin_refresh_button_clicked(GtkButton * button,
                               gpointer data)
{
    GladeXML *xml;
    GtkWidget *widget, *widget2;

    const mode_t mode755 = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    del_directory(bmp_paths[BMP_PATH_SKIN_THUMB_DIR]);
    make_directory(bmp_paths[BMP_PATH_SKIN_THUMB_DIR], mode755);

    xml = prefswin_get_xml();

    widget = glade_xml_get_widget(xml, "skin_view");
    widget2 = glade_xml_get_widget(xml, "skin_refresh_button");
    skin_view_update(GTK_TREE_VIEW(widget), GTK_WIDGET(widget2));
}

static gboolean
on_skin_view_realize(GtkTreeView * treeview,
                     gpointer data)
{
    GladeXML *xml;
    GtkWidget *widget;

    xml = prefswin_get_xml();
    widget = glade_xml_get_widget(xml, "skin_refresh_button");
    skin_view_realize(treeview);

    return TRUE;
}

static void
on_category_view_realize(GtkTreeView * treeview,
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
                           categories[i].id, -1);
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
on_skin_view_drag_data_received(GtkWidget * widget,
                                GdkDragContext * context,
                                gint x, gint y,
                                GtkSelectionData * selection_data,
                                guint info, guint time,
                                gpointer user_data) 
{
    ConfigDb *db;
    gchar *path;

    GladeXML *xml;
    GtkWidget *widget2;

    if (!selection_data->data) {
        g_warning("DND data string is NULL");
        return;
    }

    path = (gchar *) selection_data->data;

    /* FIXME: use a real URL validator/parser */

    if (str_has_prefix_nocase(path, "file:///")) {
        path[strlen(path) - 2] = 0; /* Why the hell a CR&LF? */
        path += 7;
    }
    else if (str_has_prefix_nocase(path, "file:")) {
        path += 5;
    }

    if (file_is_archive(path)) {
        bmp_active_skin_load(path);
        skin_install_skin(path);
        xml = prefswin_get_xml();
        widget2 = glade_xml_get_widget(xml, "skin_refresh_button");
	skin_view_update(GTK_TREE_VIEW(widget), GTK_WIDGET(widget2));
        /* Change skin name in the config file */
        db = bmp_cfg_db_open();
        bmp_cfg_db_set_string(db, NULL, "skin", path);
        bmp_cfg_db_close(db);
    }
			   			   
}

static void
on_chardet_detector_cbox_changed(GtkComboBox * combobox, gpointer data)
{
    ConfigDb *db;
    gint position = 0;

    position = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
    cfg.chardet_detector = (char *)chardet_detector_presets[position];

    db = bmp_cfg_db_open();
    bmp_cfg_db_set_string(db, NULL, "chardet_detector", cfg.chardet_detector);
    bmp_cfg_db_close(db);
    if (data != NULL)
        gtk_widget_set_sensitive(GTK_WIDGET(data), 1);
}

static void
on_chardet_detector_cbox_realize(GtkComboBox *combobox, gpointer data)
{
    ConfigDb *db;
    gchar *ret=NULL;
    guint i=0,index=0;

    for(i=0; i<n_chardet_detector_presets; i++) {
        gtk_combo_box_append_text(combobox, chardet_detector_presets[i]);
    }

    db = bmp_cfg_db_open();
    if(bmp_cfg_db_get_string(db, NULL, "chardet_detector", &ret) != FALSE) {
        for(i=0; i<n_chardet_detector_presets; i++) {
            if(!strcmp(chardet_detector_presets[i], ret)) {
                cfg.chardet_detector = (char *)chardet_detector_presets[i];
                index = i;
            }
        }
    }
    bmp_cfg_db_close(db);

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
    ConfigDb *db;
    gchar *ret = NULL;

    db = bmp_cfg_db_open();

    if (bmp_cfg_db_get_string(db, NULL, "chardet_fallback", &ret) != FALSE) {
        if(cfg.chardet_fallback)
            g_free(cfg.chardet_fallback);

        if(ret && strncasecmp(ret, "None", sizeof("None"))) {
            cfg.chardet_fallback = ret;
        } else {
            cfg.chardet_fallback = g_strdup("");
        }
        gtk_entry_set_text(entry, cfg.chardet_fallback);
    }

    bmp_cfg_db_close(db);
}

static void
on_chardet_fallback_changed(GtkEntry *entry, gpointer data)
{
    ConfigDb *db;
    gchar *ret = NULL;

    if(cfg.chardet_fallback)
        g_free(cfg.chardet_fallback);

    ret = g_strdup(gtk_entry_get_text(entry));

    if(ret == NULL)
        cfg.chardet_fallback = g_strdup("");
    else
        cfg.chardet_fallback = ret;

    db = bmp_cfg_db_open();

    if(cfg.chardet_fallback == NULL || !strcmp(cfg.chardet_fallback, ""))
        bmp_cfg_db_set_string(db, NULL, "chardet_fallback", "None");
    else
        bmp_cfg_db_set_string(db, NULL, "chardet_fallback", cfg.chardet_fallback);

    bmp_cfg_db_close(db);
}

static void
on_show_filepopup_for_tuple_realize(GtkToggleButton * button, gpointer data)
{
    GladeXML *xml = prefswin_get_xml();
    GtkWidget *settings_button = glade_xml_get_widget(xml, "filepopup_for_tuple_settings_button");

    gtk_toggle_button_set_active(button, cfg.show_filepopup_for_tuple);
    filepopupbutton = (GtkWidget *)button;

    gtk_widget_set_sensitive(settings_button, cfg.show_filepopup_for_tuple);
}

static void
on_show_filepopup_for_tuple_toggled(GtkToggleButton * button, gpointer data)
{
    GladeXML *xml = prefswin_get_xml();
    GtkWidget *settings_button = glade_xml_get_widget(xml, "filepopup_for_tuple_settings_button");

    cfg.show_filepopup_for_tuple = gtk_toggle_button_get_active(button);

    gtk_widget_set_sensitive(settings_button, cfg.show_filepopup_for_tuple);
}

static void
on_recurse_for_cover_toggled(GtkToggleButton *button, gpointer data)
{
	gtk_widget_set_sensitive(GTK_WIDGET(data),
		gtk_toggle_button_get_active(button));
}

static void
on_colorize_button_clicked(GtkButton *button, gpointer data)
{
	GladeXML *xml = prefswin_get_xml();
	GtkWidget *widget;

	widget = glade_xml_get_widget(xml, "red_scale");
	gtk_range_set_value(GTK_RANGE(widget), cfg.colorize_r);

	widget = glade_xml_get_widget(xml, "green_scale");
	gtk_range_set_value(GTK_RANGE(widget), cfg.colorize_g);

	widget = glade_xml_get_widget(xml, "blue_scale");
	gtk_range_set_value(GTK_RANGE(widget), cfg.colorize_b);

	gtk_widget_show(colorize_settings);
}

static void
on_red_scale_value_changed(GtkHScale *scale, gpointer data)
{
	//GladeXML *xml = prefswin_get_xml();
	//GtkWidget *widget;
	gint value;

	value = gtk_range_get_value(GTK_RANGE(scale));

	if (value != cfg.colorize_r)
	{
		cfg.colorize_r = value;

		/* reload the skin to apply the change */
		skin_reload_forced();
		ui_skinned_window_draw_all(mainwin);
		ui_skinned_window_draw_all(equalizerwin);
		ui_skinned_window_draw_all(playlistwin);
	}
}

static void
on_green_scale_value_changed(GtkHScale *scale, gpointer data)
{
	//GladeXML *xml = prefswin_get_xml();
	//GtkWidget *widget;
	gint value;

	value = gtk_range_get_value(GTK_RANGE(scale));

	if (value != cfg.colorize_r)
	{
		cfg.colorize_g = value;

		/* reload the skin to apply the change */
		skin_reload_forced();
		ui_skinned_window_draw_all(mainwin);
		ui_skinned_window_draw_all(equalizerwin);
		ui_skinned_window_draw_all(playlistwin);
	}
}

static void
on_blue_scale_value_changed(GtkHScale *scale, gpointer data)
{
	//GladeXML *xml = prefswin_get_xml();
	//GtkWidget *widget;
	gint value;

	value = gtk_range_get_value(GTK_RANGE(scale));

	if (value != cfg.colorize_r)
	{
		cfg.colorize_b = value;

		/* reload the skin to apply the change */
		skin_reload_forced();
		ui_skinned_window_draw_all(mainwin);
		ui_skinned_window_draw_all(equalizerwin);
		ui_skinned_window_draw_all(playlistwin);
	}
}

static void
on_colorize_close_clicked(GtkButton *button, gpointer data)
{
	gtk_widget_hide(colorize_settings);
}

static void
on_filepopup_for_tuple_settings_clicked(GtkButton *button, gpointer data)
{
	GladeXML *xml = prefswin_get_xml();
	GtkWidget *widget, *widget2;

	widget = glade_xml_get_widget(xml, "filepopup_settings_cover_name_include");
	gtk_entry_set_text(GTK_ENTRY(widget), cfg.cover_name_include);

	widget = glade_xml_get_widget(xml, "filepopup_settings_cover_name_exclude");
	gtk_entry_set_text(GTK_ENTRY(widget), cfg.cover_name_exclude);

	widget2 = glade_xml_get_widget(xml, "filepopup_settings_recurse_for_cover");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget2), cfg.recurse_for_cover);

	widget = glade_xml_get_widget(xml, "filepopup_settings_recurse_for_cover_depth");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), cfg.recurse_for_cover_depth);

	widget = glade_xml_get_widget(xml, "filepopup_settings_recurse_for_cover_depth_box");
	on_recurse_for_cover_toggled(GTK_TOGGLE_BUTTON(widget2), widget);

	widget = glade_xml_get_widget(xml, "filepopup_settings_use_file_cover");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), cfg.use_file_cover);

	widget = glade_xml_get_widget(xml, "filepopup_settings_showprogressbar");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), cfg.filepopup_showprogressbar);

	widget = glade_xml_get_widget(xml, "filepopup_settings_delay");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), cfg.filepopup_delay);

	gtk_widget_show(filepopup_settings);
}

static void
on_filepopup_settings_ok_clicked(GtkButton *button, gpointer data)
{
	GladeXML *xml = prefswin_get_xml();
	GtkWidget *widget;

	widget = glade_xml_get_widget(xml, "filepopup_settings_cover_name_include");
	g_free(cfg.cover_name_include);
	cfg.cover_name_include = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

	widget = glade_xml_get_widget(xml, "filepopup_settings_cover_name_exclude");
	g_free(cfg.cover_name_exclude);
	cfg.cover_name_exclude = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));

	widget = glade_xml_get_widget(xml, "filepopup_settings_recurse_for_cover");
	cfg.recurse_for_cover = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	widget = glade_xml_get_widget(xml, "filepopup_settings_recurse_for_cover_depth");
	cfg.recurse_for_cover_depth = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));

	widget = glade_xml_get_widget(xml, "filepopup_settings_use_file_cover");
	cfg.use_file_cover = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	widget = glade_xml_get_widget(xml, "filepopup_settings_showprogressbar");
	cfg.filepopup_showprogressbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	widget = glade_xml_get_widget(xml, "filepopup_settings_delay");
	cfg.filepopup_delay = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));

	gtk_widget_hide(filepopup_settings);
}

static void
on_filepopup_settings_cancel_clicked(GtkButton *button, gpointer data)
{
	gtk_widget_hide(filepopup_settings);
}

static void
on_xmms_style_fileselector_realize(GtkToggleButton * button,
                                   gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.use_xmms_style_fileselector);
}

static void
on_xmms_style_fileselector_toggled(GtkToggleButton * button,
                                   gpointer data)
{
    cfg.use_xmms_style_fileselector = gtk_toggle_button_get_active(button);
}

static void
on_show_wm_decorations_realize(GtkToggleButton * button,
                                   gpointer data)
{
    gtk_toggle_button_set_active(button, cfg.show_wm_decorations);
}

static void
on_show_wm_decorations_toggled(GtkToggleButton * button,
                                   gpointer data)
{
    extern GtkWidget *equalizerwin;
    cfg.show_wm_decorations = gtk_toggle_button_get_active(button);
    gtk_window_set_decorated(GTK_WINDOW(mainwin), cfg.show_wm_decorations);
    gtk_window_set_decorated(GTK_WINDOW(playlistwin), cfg.show_wm_decorations);
    gtk_window_set_decorated(GTK_WINDOW(equalizerwin), cfg.show_wm_decorations);
}

static void
on_reload_plugins_clicked(GtkButton * button, gpointer data)
{
    /* TBD: should every playlist entry have to be reprobed?
     * Pointers could come back stale if new plugins are added or
     * symbol sizes change.                       - nenolod
     */

    bmp_config_save();
    plugin_system_cleanup();
    bmp_config_free();
    bmp_config_load();
    plugin_system_init();
}

/* FIXME: complete the map */
FUNC_MAP_BEGIN(prefswin_func_map)
    FUNC_MAP_ENTRY(on_input_plugin_view_realize)
    FUNC_MAP_ENTRY(on_output_plugin_cbox_realize)
    FUNC_MAP_ENTRY(on_general_plugin_view_realize)
    FUNC_MAP_ENTRY(on_vis_plugin_view_realize)
    FUNC_MAP_ENTRY(on_effect_plugin_view_realize)
    FUNC_MAP_ENTRY(on_custom_cursors_realize)
    FUNC_MAP_ENTRY(on_custom_cursors_toggled)
    FUNC_MAP_ENTRY(on_mainwin_font_button_realize)
    FUNC_MAP_ENTRY(on_mainwin_font_button_font_set)
    FUNC_MAP_ENTRY(on_use_bitmap_fonts_realize)
    FUNC_MAP_ENTRY(on_use_bitmap_fonts_toggled)
    FUNC_MAP_ENTRY(on_mouse_wheel_volume_realize)
    FUNC_MAP_ENTRY(on_mouse_wheel_volume_changed)
    FUNC_MAP_ENTRY(on_mouse_wheel_scroll_pl_realize)
    FUNC_MAP_ENTRY(on_mouse_wheel_scroll_pl_changed)
    FUNC_MAP_ENTRY(on_pause_between_songs_time_realize)
    FUNC_MAP_ENTRY(on_pause_between_songs_time_changed)
    FUNC_MAP_ENTRY(on_enable_src_realize)
    FUNC_MAP_ENTRY(on_enable_src_toggled)
    FUNC_MAP_ENTRY(on_src_rate_realize)
    FUNC_MAP_ENTRY(on_src_rate_value_changed)
    FUNC_MAP_ENTRY(on_src_converter_type_realize)
    FUNC_MAP_ENTRY(on_src_converter_type_changed)
    FUNC_MAP_ENTRY(on_pl_metadata_on_load_realize)
    FUNC_MAP_ENTRY(on_pl_metadata_on_load_toggled)
    FUNC_MAP_ENTRY(on_pl_metadata_on_display_realize)
    FUNC_MAP_ENTRY(on_pl_metadata_on_display_toggled)
    FUNC_MAP_ENTRY(on_playlist_show_pl_numbers_realize)
    FUNC_MAP_ENTRY(on_playlist_show_pl_numbers_toggled)
    FUNC_MAP_ENTRY(on_playlist_show_pl_separator_realize)
    FUNC_MAP_ENTRY(on_playlist_show_pl_separator_toggled)
    FUNC_MAP_ENTRY(on_playlist_convert_twenty_realize)
    FUNC_MAP_ENTRY(on_playlist_convert_twenty_toggled)
    FUNC_MAP_ENTRY(on_playlist_convert_underscore_realize)
    FUNC_MAP_ENTRY(on_playlist_convert_underscore_toggled)
    FUNC_MAP_ENTRY(on_playlist_convert_slash_realize)
    FUNC_MAP_ENTRY(on_playlist_convert_slash_toggled)
    FUNC_MAP_ENTRY(on_playlist_font_button_realize)
    FUNC_MAP_ENTRY(on_playlist_font_button_font_set)
    FUNC_MAP_ENTRY(on_playlist_no_advance_realize)
    FUNC_MAP_ENTRY(on_playlist_no_advance_toggled)
    FUNC_MAP_ENTRY(on_refresh_file_list_realize)
    FUNC_MAP_ENTRY(on_refresh_file_list_toggled)
    FUNC_MAP_ENTRY(on_skin_view_realize)
    FUNC_MAP_ENTRY(on_titlestring_entry_realize)
    FUNC_MAP_ENTRY(on_titlestring_entry_changed)
    FUNC_MAP_ENTRY(on_eq_dir_preset_entry_realize)
    FUNC_MAP_ENTRY(on_eq_dir_preset_entry_changed)
    FUNC_MAP_ENTRY(on_eq_file_preset_entry_realize)
    FUNC_MAP_ENTRY(on_eq_file_preset_entry_changed)
    FUNC_MAP_ENTRY(on_eq_preset_view_realize)
    FUNC_MAP_ENTRY(on_eq_preset_add_clicked)
    FUNC_MAP_ENTRY(on_eq_preset_remove_clicked)
    FUNC_MAP_ENTRY(on_skin_refresh_button_clicked)
    FUNC_MAP_ENTRY(on_proxy_use_toggled)
    FUNC_MAP_ENTRY(on_proxy_use_realize)
    FUNC_MAP_ENTRY(on_proxy_auth_toggled)
    FUNC_MAP_ENTRY(on_proxy_auth_realize)
    FUNC_MAP_ENTRY(on_proxy_host_realize)
    FUNC_MAP_ENTRY(on_proxy_host_changed)
    FUNC_MAP_ENTRY(on_proxy_port_realize)
    FUNC_MAP_ENTRY(on_proxy_port_changed)
    FUNC_MAP_ENTRY(on_proxy_user_realize)
    FUNC_MAP_ENTRY(on_proxy_user_changed)
    FUNC_MAP_ENTRY(on_proxy_pass_realize)
    FUNC_MAP_ENTRY(on_proxy_pass_changed)
    FUNC_MAP_ENTRY(on_chardet_detector_cbox_realize)
    FUNC_MAP_ENTRY(on_chardet_detector_cbox_changed)
    FUNC_MAP_ENTRY(on_chardet_fallback_realize)
    FUNC_MAP_ENTRY(on_chardet_fallback_changed)
    FUNC_MAP_ENTRY(on_output_plugin_bufsize_realize)
    FUNC_MAP_ENTRY(on_output_plugin_bufsize_value_changed)
    FUNC_MAP_ENTRY(on_audio_format_det_cb_toggled)
    FUNC_MAP_ENTRY(on_audio_format_det_cb_realize)
    FUNC_MAP_ENTRY(on_detect_by_extension_cb_toggled)
    FUNC_MAP_ENTRY(on_detect_by_extension_cb_realize)
    FUNC_MAP_ENTRY(on_show_filepopup_for_tuple_realize)
    FUNC_MAP_ENTRY(on_show_filepopup_for_tuple_toggled)
    FUNC_MAP_ENTRY(on_filepopup_for_tuple_settings_clicked)
    FUNC_MAP_ENTRY(on_continue_playback_on_startup_realize)
    FUNC_MAP_ENTRY(on_continue_playback_on_startup_toggled)
    FUNC_MAP_ENTRY(on_software_volume_control_realize)
    FUNC_MAP_ENTRY(on_software_volume_control_toggled)

    /* Filepopup settings */
    FUNC_MAP_ENTRY(on_filepopup_settings_ok_clicked)
    FUNC_MAP_ENTRY(on_filepopup_settings_cancel_clicked)

    /* XMMS fileselector option -nenolod */
    FUNC_MAP_ENTRY(on_xmms_style_fileselector_toggled)
    FUNC_MAP_ENTRY(on_xmms_style_fileselector_realize)

    /* show window manager decorations */
    FUNC_MAP_ENTRY(on_show_wm_decorations_toggled)
    FUNC_MAP_ENTRY(on_show_wm_decorations_realize)

    /* colorize */
    FUNC_MAP_ENTRY(on_colorize_button_clicked)
    FUNC_MAP_ENTRY(on_red_scale_value_changed)
    FUNC_MAP_ENTRY(on_green_scale_value_changed)
    FUNC_MAP_ENTRY(on_blue_scale_value_changed)
    FUNC_MAP_ENTRY(on_colorize_close_clicked)

    FUNC_MAP_ENTRY(on_reload_plugins_clicked)
FUNC_MAP_END

void
create_prefs_window(void)
{
    const gchar *glade_file = DATA_DIR "/glade/prefswin.glade";

    GladeXML *xml;
    GtkWidget *widget, *widget2;
    gchar *aud_version_string;

    GtkWidget *titlestring_tag_menu, *menu_item;
    guint i;
        
    /* load the interface */
    xml = glade_xml_new_or_die(_("Preferences Window"), glade_file, NULL,
                               NULL);


    /* connect the signals in the interface */
    glade_xml_signal_autoconnect_map(xml, prefswin_func_map);

    prefswin = glade_xml_get_widget(xml, "prefswin");
    g_object_set_data(G_OBJECT(prefswin), "glade-xml", xml);

    /* create category view */
    widget = glade_xml_get_widget(xml, "category_view");
    widget2 = glade_xml_get_widget(xml, "category_notebook");
    g_signal_connect_after(G_OBJECT(widget), "realize",
                           G_CALLBACK(on_category_view_realize),
                           widget2);

    category_treeview = GTK_WIDGET(widget);
    category_notebook = GTK_WIDGET(widget2);

    /* plugin->input page */

    widget = glade_xml_get_widget(xml, "input_plugin_view");
    widget2 = glade_xml_get_widget(xml, "input_plugin_prefs");
    g_object_set_data(G_OBJECT(widget), "plugin_type" , GINT_TO_POINTER(PLUGIN_VIEW_TYPE_INPUT));
    g_signal_connect(G_OBJECT(widget), "row-activated",
                     G_CALLBACK(plugin_treeview_open_prefs),
                     NULL);
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_prefs),
                     widget2);

    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(plugin_treeview_open_prefs),
                             widget);
    widget2 = glade_xml_get_widget(xml, "input_plugin_info");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_info),
                     widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(plugin_treeview_open_info),
                             widget);

    /* plugin->output page */

    widget = glade_xml_get_widget(xml, "output_plugin_cbox");

    widget2 = glade_xml_get_widget(xml, "output_plugin_prefs");
    g_signal_connect(G_OBJECT(widget), "changed",
                     G_CALLBACK(output_plugin_enable_prefs),
                     widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(output_plugin_open_prefs),
                             widget);

    widget2 = glade_xml_get_widget(xml, "output_plugin_info");
    g_signal_connect(G_OBJECT(widget), "changed",
                     G_CALLBACK(output_plugin_enable_info),
                     widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(output_plugin_open_info),
                             widget);

    /* plugin->general page */

    widget = glade_xml_get_widget(xml, "general_plugin_view");
    g_object_set_data(G_OBJECT(widget), "plugin_type" , GINT_TO_POINTER(PLUGIN_VIEW_TYPE_GENERAL));
    g_signal_connect(G_OBJECT(widget), "row-activated",
                     G_CALLBACK(plugin_treeview_open_prefs),
                     NULL);

    widget2 = glade_xml_get_widget(xml, "general_plugin_prefs");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_prefs),
                     widget2);

    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(plugin_treeview_open_prefs),
                             widget);

    widget2 = glade_xml_get_widget(xml, "general_plugin_info");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_info),
                     widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(plugin_treeview_open_info),
                             widget);


    /* plugin->vis page */

    widget = glade_xml_get_widget(xml, "vis_plugin_view");
    widget2 = glade_xml_get_widget(xml, "vis_plugin_prefs");

    g_object_set_data(G_OBJECT(widget), "plugin_type" , GINT_TO_POINTER(PLUGIN_VIEW_TYPE_VIS));
    g_signal_connect(G_OBJECT(widget), "row-activated",
                     G_CALLBACK(plugin_treeview_open_prefs),
                     NULL);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(plugin_treeview_open_prefs),
                             widget);
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_prefs), widget2);


    widget2 = glade_xml_get_widget(xml, "vis_plugin_info");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_info), widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(plugin_treeview_open_info),
                             widget);


    /* plugin->effects page */

    widget = glade_xml_get_widget(xml, "effect_plugin_view");
    widget2 = glade_xml_get_widget(xml, "effect_plugin_prefs");

    g_object_set_data(G_OBJECT(widget), "plugin_type" , GINT_TO_POINTER(PLUGIN_VIEW_TYPE_EFFECT));
    g_signal_connect(G_OBJECT(widget), "row-activated",
                     G_CALLBACK(plugin_treeview_open_prefs),
                     NULL);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(plugin_treeview_open_prefs),
                             widget);
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_prefs), widget2);


    widget2 = glade_xml_get_widget(xml, "effect_plugin_info");
    g_signal_connect(G_OBJECT(widget), "cursor-changed",
                     G_CALLBACK(plugin_treeview_enable_info), widget2);
    g_signal_connect_swapped(G_OBJECT(widget2), "clicked",
                             G_CALLBACK(plugin_treeview_open_info),
                             widget);

    /* playlist page */

    widget = glade_xml_get_widget(xml, "pause_between_songs_box");
    widget2 = glade_xml_get_widget(xml, "pause_between_songs");
    g_signal_connect_after(G_OBJECT(widget2), "realize",
                           G_CALLBACK(on_pause_between_songs_realize),
                           widget);
    g_signal_connect(G_OBJECT(widget2), "toggled",
                     G_CALLBACK(on_pause_between_songs_toggled),
                     widget);

    widget = glade_xml_get_widget(xml, "playlist_use_metadata_box");
    widget2 = glade_xml_get_widget(xml, "playlist_use_metadata");
    g_signal_connect_after(G_OBJECT(widget2), "realize",
                           G_CALLBACK(on_use_pl_metadata_realize),
                           widget);
    g_signal_connect(G_OBJECT(widget2), "toggled",
                     G_CALLBACK(on_use_pl_metadata_toggled),
                     widget);

    widget = glade_xml_get_widget(xml, "skin_view");
    g_signal_connect(widget, "drag-data-received",
                     G_CALLBACK(on_skin_view_drag_data_received),
                     NULL);
    bmp_drag_dest_set(widget);

    g_signal_connect(mainwin, "drag-data-received",
                     G_CALLBACK(mainwin_drag_data_received),
                     widget);

    widget = glade_xml_get_widget(xml, "skin_refresh_button");
    g_signal_connect(widget, "clicked",
                     G_CALLBACK(on_skin_refresh_button_clicked),
                     NULL);

    widget = glade_xml_get_widget(xml, "playlist_font_button");
    g_signal_connect(mainwin, "drag-data-received",
                     G_CALLBACK(mainwin_drag_data_received),
                     widget);

    widget = glade_xml_get_widget(xml, "titlestring_cbox");
    widget2 = glade_xml_get_widget(xml, "titlestring_entry");
    g_signal_connect(widget, "realize",
                     G_CALLBACK(on_titlestring_cbox_realize),
                     widget2);
    g_signal_connect(widget, "changed",
                     G_CALLBACK(on_titlestring_cbox_changed),
                     widget2);

    /* FIXME: move this into a function */
    /* create tag menu */
    titlestring_tag_menu = gtk_menu_new();
    for(i = 0; i < n_title_field_tags; i++) {
    	menu_item = gtk_menu_item_new_with_label(_(title_field_tags[i].name));
	gtk_menu_shell_append(GTK_MENU_SHELL(titlestring_tag_menu), menu_item);
        g_signal_connect(menu_item, "activate",
                         G_CALLBACK(titlestring_tag_menu_callback), 
                         GINT_TO_POINTER(i));
    };
    gtk_widget_show_all(titlestring_tag_menu);
    
    widget = glade_xml_get_widget(xml, "titlestring_help_button");
    widget2 = glade_xml_get_widget(xml, "titlestring_cbox");

    g_signal_connect(widget2, "changed",
                     G_CALLBACK(on_titlestring_cbox_changed),
                     widget);
    g_signal_connect(widget, "clicked",
                     G_CALLBACK(on_titlestring_help_button_clicked),
                     titlestring_tag_menu);

   /* audacious version label */
   widget = glade_xml_get_widget(xml, "audversionlabel");

   aud_version_string = g_strdup_printf("<span size='small'>%s (%s) (%s@%s)</span>",
		    "Audacious " PACKAGE_VERSION ,
                    svn_stamp , g_get_user_name() , g_get_host_name() );

   gtk_label_set_markup( GTK_LABEL(widget) , aud_version_string );
   g_free(aud_version_string);

	/* Create window for filepopup settings */
	filepopup_settings = glade_xml_get_widget(xml, "filepopup_for_tuple_settings");
	gtk_window_set_transient_for(GTK_WINDOW(filepopup_settings), GTK_WINDOW(prefswin));

	widget = glade_xml_get_widget(xml, "filepopup_settings_recurse_for_cover_depth_box");
	widget2 = glade_xml_get_widget(xml, "filepopup_settings_recurse_for_cover");
	g_signal_connect(G_OBJECT(widget2), "toggled",
		G_CALLBACK(on_recurse_for_cover_toggled),
		widget);

	/* Create window for color adjustment settings */
	colorize_settings = glade_xml_get_widget(xml, "colorize_popup");
	gtk_window_set_transient_for(GTK_WINDOW(colorize_settings), GTK_WINDOW(prefswin));
	gtk_widget_hide(colorize_settings);
}

void
show_prefs_window(void)
{
    static gboolean skinlist_filled = FALSE;

    gtk_window_present(GTK_WINDOW(prefswin)); /* show or raise prefs window */

    if ( !skinlist_filled )
    {
      GladeXML *xml;
      GtkWidget *widget, *widget2;

      xml = prefswin_get_xml();

      widget = glade_xml_get_widget(xml, "skin_view");
      widget2 = glade_xml_get_widget(xml, "skin_refresh_button");
      skin_view_update(GTK_TREE_VIEW(widget), GTK_WIDGET(widget2));
      skinlist_filled = TRUE;
    }
}

static void
prefswin_page_queue_new(GtkWidget *container, gchar *name, gchar *imgurl)
{
    CategoryQueueEntry *ent = g_malloc0(sizeof(CategoryQueueEntry));

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
	}

	if (index > id)
	{
	    index--;
	    gtk_list_store_set(GTK_LIST_STORE(model), &iter, CATEGORY_VIEW_COL_ID, index, -1);
	}

        ret = gtk_tree_model_iter_next(model, &iter);
    }
}
