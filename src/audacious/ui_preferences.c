/*
 * ui_preferences.c
 * Copyright 2006-2012 William Pitcock, Tomasz Moń, Michael Färber, and
 *                     John Lindgren
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

#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudgui/libaudgui-gtk.h>

#include "debug.h"
#include "i18n.h"
#include "misc.h"
#include "output.h"
#include "playlist.h"
#include "plugin.h"
#include "plugins.h"
#include "preferences.h"
#include "ui_preferences.h"

#ifdef USE_CHARDET
#include <libguess.h>
#endif

enum CategoryViewCols {
    CATEGORY_VIEW_COL_ICON,
    CATEGORY_VIEW_COL_NAME,
    CATEGORY_VIEW_N_COLS
};

typedef struct {
    const char * icon_path;
    const char * name;
} Category;

typedef struct {
    int type;
    const char * name;
} PluginCategory;

typedef struct {
    const char * name;
    const char * tag;
} TitleFieldTag;

static const char aud_version_string[] =
 "<span size='small'>Audacious " VERSION " (" BUILDSTAMP ")</span>";

static GtkWidget * prefswin;
static GtkWidget * category_treeview, * category_notebook, * plugin_notebook;
static GtkWidget * titlestring_entry;

enum {
    CATEGORY_APPEARANCE = 0,
    CATEGORY_AUDIO,
    CATEGORY_NETWORK,
    CATEGORY_PLAYLIST,
    CATEGORY_SONG_INFO,
    CATEGORY_PLUGINS
};

static const Category categories[] = {
    { "appearance.png", N_("Appearance") },
    { "audio.png", N_("Audio") },
    { "connectivity.png", N_("Network") },
    { "playlist.png", N_("Playlist")} ,
    { "info.png", N_("Song Info") },
    { "plugins.png", N_("Plugins") }
};

static const PluginCategory plugin_categories[] = {
    { PLUGIN_TYPE_GENERAL, N_("General") },
    { PLUGIN_TYPE_EFFECT, N_("Effect") },
    { PLUGIN_TYPE_VIS, N_("Visualization") },
    { PLUGIN_TYPE_INPUT, N_("Input") },
    { PLUGIN_TYPE_PLAYLIST, N_("Playlist") },
    { PLUGIN_TYPE_TRANSPORT, N_("Transport") }
};

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
    { N_("Quality")    , "${quality}" }
};

#ifdef USE_CHARDET
static ComboBoxElements chardet_detector_presets[] = {
    { "", N_("None")},
    { GUESS_REGION_AR, N_("Arabic") },
    { GUESS_REGION_BL, N_("Baltic") },
    { GUESS_REGION_CN, N_("Chinese") },
    { GUESS_REGION_GR, N_("Greek") },
    { GUESS_REGION_HW, N_("Hebrew") },
    { GUESS_REGION_JP, N_("Japanese") },
    { GUESS_REGION_KR, N_("Korean") },
    { GUESS_REGION_PL, N_("Polish") },
    { GUESS_REGION_RU, N_("Russian") },
    { GUESS_REGION_TW, N_("Taiwanese") },
    { GUESS_REGION_TR, N_("Turkish") }
};
#endif

static ComboBoxElements bitdepth_elements[] = {
    { GINT_TO_POINTER (16), "16" },
    { GINT_TO_POINTER (24), "24" },
    { GINT_TO_POINTER (32), "32" },
    { GINT_TO_POINTER  (0), N_("Floating point") }
};

static GArray * iface_combo_elements;
static int iface_combo_selected;
static GtkWidget * iface_prefs_box;

static const ComboBoxElements * iface_combo_fill (int * n_elements);
static void iface_combo_changed (void);
static void * iface_create_prefs_box (void);

static PreferencesWidget appearance_page_widgets[] = {
 {WIDGET_LABEL, N_("<b>Interface Settings</b>")},
 {WIDGET_COMBO_BOX, N_("Interface plugin:"),
  .cfg_type = VALUE_INT, .cfg = & iface_combo_selected,
  .data.combo.fill = iface_combo_fill, .callback = iface_combo_changed},
 {WIDGET_CUSTOM, .data.populate = iface_create_prefs_box}};

static GArray * output_combo_elements;
static int output_combo_selected;
static GtkWidget * output_config_button;
static GtkWidget * output_about_button;

static const ComboBoxElements * output_combo_fill (int * n_elements);
static void output_combo_changed (void);
static void * output_create_config_button (void);
static void * output_create_about_button (void);
static void output_bit_depth_changed (void);

static PreferencesWidget output_combo_widgets[] = {
 {WIDGET_COMBO_BOX, N_("Output plugin:"),
  .cfg_type = VALUE_INT, .cfg = & output_combo_selected,
  .data.combo.fill = output_combo_fill, .callback = output_combo_changed},
 {WIDGET_CUSTOM, .data.populate = output_create_config_button},
 {WIDGET_CUSTOM, .data.populate = output_create_about_button}};

static PreferencesWidget audio_page_widgets[] = {
 {WIDGET_LABEL, N_("<b>Output Settings</b>")},
 {WIDGET_BOX, .data.box = {.elem = output_combo_widgets,
  .n_elem = ARRAY_LEN (output_combo_widgets), .horizontal = TRUE}},
 {WIDGET_COMBO_BOX, N_("Bit depth:"),
  .cfg_type = VALUE_INT, .cname = "output_bit_depth", .callback = output_bit_depth_changed,
  .data.combo = {bitdepth_elements, ARRAY_LEN (bitdepth_elements)}},
 {WIDGET_SPIN_BTN, N_("Buffer size:"),
  .cfg_type = VALUE_INT, .cname = "output_buffer_size",
  .data.spin_btn = {100, 10000, 1000, N_("ms")}},
 {WIDGET_CHK_BTN, N_("Soft clipping"),
  .cfg_type = VALUE_BOOLEAN, .cname = "soft_clipping"},
 {WIDGET_CHK_BTN, N_("Use software volume control (not recommended)"),
  .cfg_type = VALUE_BOOLEAN, .cname = "software_volume_control"},
 {WIDGET_LABEL, N_("<b>Replay Gain</b>")},
 {WIDGET_CHK_BTN, N_("Enable Replay Gain"),
  .cfg_type = VALUE_BOOLEAN, .cname = "enable_replay_gain"},
 {WIDGET_CHK_BTN, N_("Album mode"), .child = TRUE,
  .cfg_type = VALUE_BOOLEAN, .cname = "replay_gain_album"},
 {WIDGET_CHK_BTN, N_("Prevent clipping (recommended)"), .child = TRUE,
  .cfg_type = VALUE_BOOLEAN, .cname = "enable_clipping_prevention"},
 {WIDGET_LABEL, N_("<b>Adjust Levels</b>"), .child = TRUE},
 {WIDGET_SPIN_BTN, N_("Amplify all files:"), .child = TRUE,
  .cfg_type = VALUE_FLOAT, .cname = "replay_gain_preamp",
  .data.spin_btn = {-15, 15, 0.1, N_("dB")}},
 {WIDGET_SPIN_BTN, N_("Amplify untagged files:"), .child = TRUE,
  .cfg_type = VALUE_FLOAT, .cname = "default_gain",
  .data.spin_btn = {-15, 15, 0.1, N_("dB")}}};

static PreferencesWidget proxy_host_port_elements[] = {
 {WIDGET_ENTRY, N_("Proxy hostname:"), .cfg_type = VALUE_STRING, .cname = "proxy_host"},
 {WIDGET_ENTRY, N_("Proxy port:"), .cfg_type = VALUE_STRING, .cname = "proxy_port"}};

static PreferencesWidget proxy_auth_elements[] = {
 {WIDGET_ENTRY, N_("Proxy username:"), .cfg_type = VALUE_STRING, .cname = "proxy_user"},
 {WIDGET_ENTRY, N_("Proxy password:"), .cfg_type = VALUE_STRING, .cname = "proxy_pass",
  .data.entry.password = TRUE}};

static PreferencesWidget connectivity_page_widgets[] = {
 {WIDGET_LABEL, N_("<b>Proxy Configuration</b>"), NULL, NULL, NULL, FALSE},
 {WIDGET_CHK_BTN, N_("Enable proxy usage"), .cfg_type = VALUE_BOOLEAN, .cname = "use_proxy"},
 {WIDGET_TABLE, .child = TRUE, .data.table = {proxy_host_port_elements,
  ARRAY_LEN (proxy_host_port_elements)}},
 {WIDGET_CHK_BTN, N_("Use authentication with proxy"),
  .cfg_type = VALUE_BOOLEAN, .cname = "use_proxy_auth"},
 {WIDGET_TABLE, .child = TRUE, .data.table = {proxy_auth_elements,
  ARRAY_LEN (proxy_auth_elements)}}};

static PreferencesWidget chardet_elements[] = {
#ifdef USE_CHARDET
 {WIDGET_COMBO_BOX, N_("Auto character encoding detector for:"),
  .cfg_type = VALUE_STRING, .cname = "chardet_detector", .child = TRUE,
  .data.combo = {chardet_detector_presets, ARRAY_LEN (chardet_detector_presets)}},
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
 {WIDGET_CHK_BTN, N_("Do not load metadata for songs until played"),
  .cfg_type = VALUE_BOOLEAN, .cname = "metadata_on_play",
  .callback = playlist_trigger_scan},
 {WIDGET_LABEL, N_("<b>Compatibility</b>"), NULL, NULL, NULL, FALSE},
 {WIDGET_CHK_BTN, N_("Interpret \\ (backward slash) as a folder delimiter"),
  .cfg_type = VALUE_BOOLEAN, .cname = "convert_backslash"},
 {WIDGET_TABLE, .data.table = {chardet_elements, ARRAY_LEN (chardet_elements)}}};

static PreferencesWidget song_info_page_widgets[] = {
 {WIDGET_LABEL, N_("<b>Album Art</b>")},
 {WIDGET_LABEL, N_("Search for images matching these words (comma-separated):")},
 {WIDGET_ENTRY, .cfg_type = VALUE_STRING, .cname = "cover_name_include"},
 {WIDGET_LABEL, N_("Exclude images matching these words (comma-separated):")},
 {WIDGET_ENTRY, .cfg_type = VALUE_STRING, .cname = "cover_name_exclude"},
 {WIDGET_CHK_BTN, N_("Search for images matching song file name"),
  .cfg_type = VALUE_BOOLEAN, .cname = "use_file_cover"},
 {WIDGET_CHK_BTN, N_("Search recursively"),
  .cfg_type = VALUE_BOOLEAN, .cname = "recurse_for_cover"},
 {WIDGET_SPIN_BTN, N_("Search depth:"), .child = TRUE,
  .cfg_type = VALUE_INT, .cname = "recurse_for_cover_depth",
  .data.spin_btn = {0, 100, 1}},
 {WIDGET_LABEL, N_("<b>Popup Information</b>")},
 {WIDGET_CHK_BTN, N_("Show popup information"),
  .cfg_type = VALUE_BOOLEAN, .cname = "show_filepopup_for_tuple"},
 {WIDGET_SPIN_BTN, N_("Popup delay (tenths of a second):"), .child = TRUE,
  .cfg_type = VALUE_INT, .cname = "filepopup_delay",
  .data.spin_btn = {0, 100, 1}},
 {WIDGET_CHK_BTN, N_("Show time scale for current song"), .child = TRUE,
  .cfg_type = VALUE_BOOLEAN, .cname = "filepopup_showprogressbar"}};

#define TITLESTRING_NPRESETS 6

static const char * const titlestring_presets[TITLESTRING_NPRESETS] = {
    "${title}",
    "${?artist:${artist} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${?track-number:${track-number}. }${title}",
    "${?artist:${artist} }${?album:[ ${album} ] }${?artist:- }${?track-number:${track-number}. }${title}",
    "${?album:${album} - }${title}"
};

static const char * const titlestring_preset_names[TITLESTRING_NPRESETS] = {
    N_("TITLE"),
    N_("ARTIST - TITLE"),
    N_("ARTIST - ALBUM - TITLE"),
    N_("ARTIST - ALBUM - TRACK. TITLE"),
    N_("ARTIST [ ALBUM ] - TRACK. TITLE"),
    N_("ALBUM - TITLE")
};

static GArray * fill_plugin_combo (int type)
{
    GArray * array = g_array_new (FALSE, FALSE, sizeof (ComboBoxElements));
    g_array_set_size (array, plugin_count (type));

    for (int i = 0; i < array->len; i ++)
    {
        ComboBoxElements * elem = & g_array_index (array, ComboBoxElements, i);
        elem->label = plugin_get_name (plugin_by_index (type, i));
        elem->value = GINT_TO_POINTER (i);
    }

    return array;
}

static void change_category (int category)
{
    GtkTreeSelection * selection = gtk_tree_view_get_selection ((GtkTreeView *) category_treeview);
    GtkTreePath * path = gtk_tree_path_new_from_indices (category, -1);
    gtk_tree_selection_select_path (selection, path);
    gtk_tree_path_free (path);
}

static void category_changed (GtkTreeSelection * selection)
{
    GtkTreeModel * model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected (selection, & model, & iter))
    {
        GtkTreePath * path = gtk_tree_model_get_path (model, & iter);
        int category = gtk_tree_path_get_indices (path)[0];
        gtk_notebook_set_current_page ((GtkNotebook *) category_notebook, category);
        gtk_tree_path_free (path);
    }
}

static void titlestring_tag_menu_cb (GtkMenuItem * menuitem, void * data)
{
    const char * separator = " - ";
    int item = GPOINTER_TO_INT (data);
    int pos = gtk_editable_get_position ((GtkEditable *) titlestring_entry);

    /* insert separator as needed */
    if (gtk_entry_get_text ((GtkEntry *) titlestring_entry)[0])
        gtk_editable_insert_text ((GtkEditable *) titlestring_entry, separator, -1, & pos);

    gtk_editable_insert_text ((GtkEditable *) titlestring_entry, _(title_field_tags[item].tag), -1, & pos);
    gtk_editable_set_position ((GtkEditable *) titlestring_entry, pos);
}

static void on_titlestring_help_button_clicked (GtkButton * button, void * menu)
{
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
    set_str (NULL, "generic_title_format", format);
    update_titlestring_cbox (cbox, format);
    playlist_reformat_titles ();
}

static void on_titlestring_cbox_changed (GtkComboBox * cbox, GtkEntry * entry)
{
    int preset = gtk_combo_box_get_active (cbox);
    if (preset < TITLESTRING_NPRESETS)
        gtk_entry_set_text (entry, titlestring_presets[preset]);
}

static void fill_category_list (GtkTreeView * treeview, GtkNotebook * notebook)
{
    GtkTreeViewColumn * column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, _("Category"));
    gtk_tree_view_append_column (treeview, column);
    gtk_tree_view_column_set_spacing (column, 2);

    GtkCellRenderer * renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", 0, NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", 1, NULL);

    g_object_set ((GObject *) renderer, "wrap-width", 96, "wrap-mode",
     PANGO_WRAP_WORD_CHAR, NULL);

    GtkListStore * store = gtk_list_store_new (CATEGORY_VIEW_N_COLS,
     GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model (treeview, (GtkTreeModel *) store);

    const char * data_dir = get_path (AUD_PATH_DATA_DIR);

    for (int i = 0; i < ARRAY_LEN (categories); i ++)
    {
        SCONCAT3 (path, data_dir, "/images/", categories[i].icon_path);

        GtkTreeIter iter;

        gtk_list_store_append (store, & iter);
        gtk_list_store_set (store, & iter, CATEGORY_VIEW_COL_NAME,
         gettext (categories[i].name), -1);

        GdkPixbuf * img = gdk_pixbuf_new_from_file (path, NULL);

        if (img)
        {
            gtk_list_store_set (store, & iter, CATEGORY_VIEW_COL_ICON, img, -1);
            g_object_unref (img);
        }
    }

    g_object_unref (store);

    GtkTreeSelection * selection = gtk_tree_view_get_selection (treeview);
    g_signal_connect (selection, "changed", (GCallback) category_changed, NULL);
}

static GtkWidget * create_titlestring_tag_menu (void)
{
    GtkWidget * titlestring_tag_menu = gtk_menu_new ();

    for (int i = 0; i < ARRAY_LEN (title_field_tags); i ++)
    {
        GtkWidget * menu_item = gtk_menu_item_new_with_label (_(title_field_tags[i].name));
        gtk_menu_shell_append ((GtkMenuShell *) titlestring_tag_menu, menu_item);
        g_signal_connect(menu_item, "activate",
         (GCallback) titlestring_tag_menu_cb, GINT_TO_POINTER (i));
    };

    gtk_widget_show_all (titlestring_tag_menu);

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

    char * format = get_str (NULL, "generic_title_format");
    update_titlestring_cbox ((GtkComboBox *) * cbox, format);
    gtk_entry_set_text ((GtkEntry *) * entry, format);
    str_unref (format);

    g_signal_connect (* cbox, "changed", (GCallback) on_titlestring_cbox_changed, * entry);
    g_signal_connect (* entry, "changed", (GCallback) on_titlestring_entry_changed, * cbox);
}

static void create_playlist_category (void)
{
    GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add ((GtkContainer *) category_notebook, vbox);

    create_widgets ((GtkBox *) vbox, playlist_page_widgets, ARRAY_LEN (playlist_page_widgets));

    GtkWidget * alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start ((GtkBox *) vbox, alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding ((GtkAlignment *) alignment, 12, 3, 0, 0);

    GtkWidget * label = gtk_label_new (_("<b>Song Display</b>"));
    gtk_container_add ((GtkContainer *) alignment, label);
    gtk_label_set_use_markup ((GtkLabel *) label, TRUE);
    gtk_misc_set_alignment ((GtkMisc *) label, 0, 0.5);

    GtkWidget * numbers_alignment = gtk_alignment_new (0, 0, 0, 0);
    gtk_alignment_set_padding ((GtkAlignment *) numbers_alignment, 0, 0, 12, 0);
    gtk_box_pack_start ((GtkBox *) vbox, numbers_alignment, 0, 0, 3);

    GtkWidget * numbers = gtk_check_button_new_with_label (_("Show song numbers"));
    gtk_toggle_button_set_active ((GtkToggleButton *) numbers,
     get_bool (NULL, "show_numbers_in_pl"));
    g_signal_connect (numbers, "toggled", (GCallback) show_numbers_cb, 0);
    gtk_container_add ((GtkContainer *) numbers_alignment, numbers);

    numbers_alignment = gtk_alignment_new (0, 0, 0, 0);
    gtk_alignment_set_padding ((GtkAlignment *) numbers_alignment, 0, 0, 12, 0);
    gtk_box_pack_start ((GtkBox *) vbox, numbers_alignment, 0, 0, 3);

    numbers = gtk_check_button_new_with_label (
     _("Show leading zeroes (02:00 instead of 2:00)"));
    gtk_toggle_button_set_active ((GtkToggleButton *) numbers, get_bool (NULL, "leading_zero"));
    g_signal_connect (numbers, "toggled", (GCallback) leading_zero_cb, 0);
    gtk_container_add ((GtkContainer *) numbers_alignment, numbers);

    alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_box_pack_start ((GtkBox *) vbox, alignment, FALSE, FALSE, 0);
    gtk_alignment_set_padding ((GtkAlignment *) alignment, 0, 0, 12, 0);

    GtkWidget * grid = gtk_grid_new ();
    gtk_container_add ((GtkContainer *) alignment, grid);
    gtk_grid_set_row_spacing ((GtkGrid *) grid, 4);
    gtk_grid_set_column_spacing ((GtkGrid *) grid, 12);

    label = gtk_label_new (_("Title format:"));
    gtk_grid_attach ((GtkGrid *) grid, label, 0, 0, 1, 1);
    gtk_label_set_justify ((GtkLabel *) label, GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment ((GtkMisc *) label, 1, 0.5);

    label = gtk_label_new (_("Custom string:"));
    gtk_grid_attach ((GtkGrid *) grid, label, 0, 1, 1, 1);
    gtk_label_set_justify ((GtkLabel *) label, GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment ((GtkMisc *) label, 1, 0.5);

    GtkWidget * titlestring_cbox;
    create_titlestring_widgets (& titlestring_cbox, & titlestring_entry);
    gtk_widget_set_hexpand (titlestring_cbox, TRUE);
    gtk_widget_set_hexpand (titlestring_entry, TRUE);
    gtk_grid_attach ((GtkGrid *) grid, titlestring_cbox, 1, 0, 1, 1);
    gtk_grid_attach ((GtkGrid *) grid, titlestring_entry, 1, 1, 1, 1);

    GtkWidget * titlestring_help_button = gtk_button_new ();
    gtk_widget_set_can_focus (titlestring_help_button, FALSE);
    gtk_button_set_focus_on_click ((GtkButton *) titlestring_help_button, FALSE);
    gtk_button_set_relief ((GtkButton *) titlestring_help_button, GTK_RELIEF_HALF);
    gtk_grid_attach ((GtkGrid *) grid, titlestring_help_button, 2, 1, 1, 1);

    GtkWidget * titlestring_tag_menu = create_titlestring_tag_menu ();

    g_signal_connect (titlestring_help_button, "clicked",
     (GCallback) on_titlestring_help_button_clicked, titlestring_tag_menu);

    GtkWidget * image = gtk_image_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON);
    gtk_container_add ((GtkContainer *) titlestring_help_button, image);
}

static void create_song_info_category (void)
{
    GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add ((GtkContainer *) category_notebook, vbox);
    create_widgets ((GtkBox *) vbox, song_info_page_widgets,
     ARRAY_LEN (song_info_page_widgets));
}

static void iface_fill_prefs_box (void)
{
    Plugin * header = plugin_get_header (plugin_get_current (PLUGIN_TYPE_IFACE));
    if (header && header->prefs)
        create_widgets_with_domain (iface_prefs_box, header->prefs->widgets,
         header->prefs->n_widgets, header->domain);
}

static void iface_combo_changed (void)
{
    gtk_container_foreach ((GtkContainer *) iface_prefs_box, (GtkCallback) gtk_widget_destroy, NULL);

    plugin_enable (plugin_by_index (PLUGIN_TYPE_IFACE, iface_combo_selected), TRUE);

    iface_fill_prefs_box ();
    gtk_widget_show_all (iface_prefs_box);
}

static const ComboBoxElements * iface_combo_fill (int * n_elements)
{
    if (! iface_combo_elements)
    {
        iface_combo_elements = fill_plugin_combo (PLUGIN_TYPE_IFACE);
        iface_combo_selected = plugin_get_index (plugin_get_current (PLUGIN_TYPE_IFACE));
    }

    * n_elements = iface_combo_elements->len;
    return (const ComboBoxElements *) iface_combo_elements->data;
}

static void * iface_create_prefs_box (void)
{
    iface_prefs_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    iface_fill_prefs_box ();
    return iface_prefs_box;
}

static void create_appearance_category (void)
{
    GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add ((GtkContainer *) category_notebook, vbox);
    create_widgets ((GtkBox *) vbox, appearance_page_widgets, ARRAY_LEN (appearance_page_widgets));
}

static void output_combo_changed (void)
{
    PluginHandle * plugin = plugin_by_index (PLUGIN_TYPE_OUTPUT, output_combo_selected);

    if (plugin_enable (plugin, TRUE))
    {
        gtk_widget_set_sensitive (output_config_button, plugin_has_configure (plugin));
        gtk_widget_set_sensitive (output_about_button, plugin_has_about (plugin));
    }
}

static const ComboBoxElements * output_combo_fill (int * n_elements)
{
    if (! output_combo_elements)
    {
        output_combo_elements = fill_plugin_combo (PLUGIN_TYPE_OUTPUT);
        output_combo_selected = plugin_get_index (output_plugin_get_current ());
    }

    * n_elements = output_combo_elements->len;
    return (const ComboBoxElements *) output_combo_elements->data;
}

static void output_bit_depth_changed (void)
{
    output_reset (OUTPUT_RESET_SOFT);
}

static void output_do_config (void * unused)
{
    plugin_do_configure (output_plugin_get_current ());
}

static void output_do_about (void * unused)
{
    plugin_do_about (output_plugin_get_current ());
}

static void * output_create_config_button (void)
{
    bool_t enabled = plugin_has_configure (output_plugin_get_current ());

    output_config_button = audgui_button_new (_("_Settings"),
     "preferences-system", output_do_config, NULL);
    gtk_widget_set_sensitive (output_config_button, enabled);

    return output_config_button;
}

static void * output_create_about_button (void)
{
    bool_t enabled = plugin_has_about (output_plugin_get_current ());

    output_about_button = audgui_button_new (_("_About"), "help-about", output_do_about, NULL);
    gtk_widget_set_sensitive (output_about_button, enabled);

    return output_about_button;
}

static void create_audio_category (void)
{
    GtkWidget * audio_page_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    create_widgets ((GtkBox *) audio_page_vbox, audio_page_widgets, ARRAY_LEN (audio_page_widgets));
    gtk_container_add ((GtkContainer *) category_notebook, audio_page_vbox);
}

static void create_connectivity_category (void)
{
    GtkWidget * connectivity_page_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add ((GtkContainer *) category_notebook, connectivity_page_vbox);

    GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start ((GtkBox *) connectivity_page_vbox, vbox, TRUE, TRUE, 0);

    create_widgets ((GtkBox *) vbox, connectivity_page_widgets, ARRAY_LEN (connectivity_page_widgets));
}

static void create_plugin_category (void)
{
    plugin_notebook = gtk_notebook_new ();
    gtk_container_add ((GtkContainer *) category_notebook, plugin_notebook);

    for (int i = 0; i < ARRAY_LEN (plugin_categories); i ++)
    {
        const PluginCategory * cat = & plugin_categories[i];
        gtk_notebook_append_page ((GtkNotebook *) plugin_notebook,
         plugin_view_new (cat->type), gtk_label_new (_(cat->name)));
    }
}

static void destroy_cb (void)
{
    prefswin = NULL;
    category_treeview = NULL;
    category_notebook = NULL;
    titlestring_entry = NULL;

    if (iface_combo_elements)
    {
        g_array_free (iface_combo_elements, TRUE);
        iface_combo_elements = NULL;
    }

    if (output_combo_elements)
    {
        g_array_free (output_combo_elements, TRUE);
        output_combo_elements = NULL;
    }
}

static void create_prefs_window (void)
{
    prefswin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint ((GtkWindow *) prefswin, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_container_set_border_width ((GtkContainer *) prefswin, 12);
    gtk_window_set_title ((GtkWindow *) prefswin, _("Audacious Settings"));
    gtk_window_set_default_size ((GtkWindow *) prefswin, 680, 400);

    GtkWidget * vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add ((GtkContainer *) prefswin, vbox);

    GtkWidget * hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  8);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, TRUE, TRUE, 0);

    GtkWidget * scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start ((GtkBox *) hbox, scrolledwindow, FALSE, FALSE, 0);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolledwindow,
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scrolledwindow, GTK_SHADOW_IN);

    category_treeview = gtk_tree_view_new ();
    gtk_container_add ((GtkContainer *) scrolledwindow, category_treeview);
    gtk_widget_set_size_request (scrolledwindow, 168, -1);
    gtk_tree_view_set_headers_visible ((GtkTreeView *) category_treeview, FALSE);

    category_notebook = gtk_notebook_new ();
    gtk_box_pack_start ((GtkBox *) hbox, category_notebook, TRUE, TRUE, 0);

    gtk_widget_set_can_focus (category_notebook, FALSE);
    gtk_notebook_set_show_tabs ((GtkNotebook *) category_notebook, FALSE);
    gtk_notebook_set_show_border ((GtkNotebook *) category_notebook, FALSE);

    create_appearance_category ();
    create_audio_category ();
    create_connectivity_category ();
    create_playlist_category ();
    create_song_info_category ();
    create_plugin_category ();

    GtkWidget * hseparator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start ((GtkBox *) vbox, hseparator, FALSE, FALSE, 6);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  0);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, FALSE, FALSE, 0);

    GtkWidget * audversionlabel = gtk_label_new (aud_version_string);
    gtk_box_pack_start ((GtkBox *) hbox, audversionlabel, FALSE, FALSE, 0);
    gtk_label_set_use_markup ((GtkLabel *) audversionlabel, TRUE);

    GtkWidget * prefswin_button_box = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start ((GtkBox *) hbox, prefswin_button_box, TRUE, TRUE, 0);
    gtk_button_box_set_layout ((GtkButtonBox *) prefswin_button_box, GTK_BUTTONBOX_END);
    gtk_box_set_spacing ((GtkBox *) prefswin_button_box, 6);

    GtkWidget * close = audgui_button_new (_("_Close"), "window-close",
     (AudguiCallback) gtk_widget_destroy, prefswin);
    gtk_container_add ((GtkContainer *) prefswin_button_box, close);
    gtk_widget_set_can_default (close, TRUE);

    fill_category_list ((GtkTreeView *) category_treeview, (GtkNotebook *) category_notebook);

    gtk_widget_show_all (vbox);

    g_signal_connect (prefswin, "destroy", (GCallback) destroy_cb, NULL);

    audgui_destroy_on_escape (prefswin);
}

void show_prefs_window (void)
{
    if (! prefswin)
        create_prefs_window ();

    change_category (CATEGORY_APPEARANCE);

    gtk_window_present ((GtkWindow *) prefswin);
}

void show_prefs_for_plugin_type (int type)
{
    if (! prefswin)
        create_prefs_window ();

    if (type == PLUGIN_TYPE_IFACE)
        change_category (CATEGORY_APPEARANCE);
    else if (type == PLUGIN_TYPE_OUTPUT)
        change_category (CATEGORY_AUDIO);
    else
    {
        change_category (CATEGORY_PLUGINS);

        for (int i = 0; i < ARRAY_LEN (plugin_categories); i ++)
        {
            if (plugin_categories[i].type == type)
                gtk_notebook_set_current_page ((GtkNotebook *) plugin_notebook, i);
        }
    }

    gtk_window_present ((GtkWindow *) prefswin);
}

void hide_prefs_window (void)
{
    if (prefswin)
        gtk_widget_destroy (prefswin);
}
