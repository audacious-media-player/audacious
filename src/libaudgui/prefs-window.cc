/*
 * prefs-window.c
 * Copyright 2006-2014 William Pitcock, Tomasz Moń, Michael Färber, and
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
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugin.h>
#include <libaudcore/plugins.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

#ifdef USE_CHARDET
#include <libguess.h>
#endif

enum CategoryViewCols {
    CATEGORY_VIEW_COL_ICON,
    CATEGORY_VIEW_COL_NAME,
    CATEGORY_VIEW_N_COLS
};

struct Category {
    const char * icon_path;
    const char * name;
};

struct PluginCategory {
    int type;
    const char * name;
};

struct TitleFieldTag {
    const char * name;
    const char * tag;
};

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

static const TitleFieldTag title_field_tags[] = {
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
static const ComboBoxElements chardet_detector_presets[] = {
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

static const ComboBoxElements bitdepth_elements[] = {
    { GINT_TO_POINTER (16), "16" },
    { GINT_TO_POINTER (24), "24" },
    { GINT_TO_POINTER (32), "32" },
    { GINT_TO_POINTER  (0), N_("Floating point") }
};

static Index<ComboBoxElements> iface_combo_elements;
static int iface_combo_selected;
static GtkWidget * iface_prefs_box;

static ArrayRef<const ComboBoxElements> iface_combo_fill ();
static void iface_combo_changed (void);
static void * iface_create_prefs_box (void);

static const PreferencesWidget appearance_page_widgets[] = {
    WidgetLabel (N_("<b>Interface Settings</b>")),
    WidgetCombo (N_("Interface plugin:"),
        WidgetInt (iface_combo_selected, iface_combo_changed),
        {0, iface_combo_fill}),
    WidgetCustom (iface_create_prefs_box)
};

static Index<ComboBoxElements> output_combo_elements;
static int output_combo_selected;
static GtkWidget * output_config_button;
static GtkWidget * output_about_button;

static ArrayRef<const ComboBoxElements> output_combo_fill ();
static void output_combo_changed (void);
static void * output_create_config_button (void);
static void * output_create_about_button (void);
static void output_bit_depth_changed (void);

static const PreferencesWidget output_combo_widgets[] = {
    WidgetCombo (N_("Output plugin:"),
        WidgetInt (output_combo_selected, output_combo_changed),
        {0, output_combo_fill}),
    WidgetCustom (output_create_config_button),
    WidgetCustom (output_create_about_button)
};

static const PreferencesWidget audio_page_widgets[] = {
    WidgetLabel (N_("<b>Output Settings</b>")),
    WidgetBox ({{output_combo_widgets}, true}),
    WidgetCombo (N_("Bit depth:"),
        WidgetInt (0, "output_bit_depth", output_bit_depth_changed),
        {{bitdepth_elements}}),
    WidgetSpin (N_("Buffer size:"),
        WidgetInt (0, "output_buffer_size"),
        {100, 10000, 1000, N_("ms")}),
    WidgetCheck (N_("Soft clipping"),
        WidgetBool (0, "soft_clipping")),
    WidgetCheck (N_("Use software volume control (not recommended)"),
        WidgetBool (0, "software_volume_control")),
    WidgetLabel (N_("<b>Replay Gain</b>")),
    WidgetCheck (N_("Enable Replay Gain"),
        WidgetBool (0, "enable_replay_gain")),
    WidgetCheck (N_("Album mode"),
        WidgetBool (0, "replay_gain_album"),
        WIDGET_CHILD),
    WidgetCheck (N_("Prevent clipping (recommended)"),
        WidgetBool (0, "enable_clipping_prevention"),
        WIDGET_CHILD),
    WidgetLabel (N_("<b>Adjust Levels</b>"),
        WIDGET_CHILD),
    WidgetSpin (N_("Amplify all files:"),
        WidgetFloat (0, "replay_gain_preamp"),
        {-15, 15, 0.1, N_("dB")},
        WIDGET_CHILD),
    WidgetSpin (N_("Amplify untagged files:"),
        WidgetFloat (0, "default_gain"),
        {-15, 15, 0.1, N_("dB")},
        WIDGET_CHILD)
};

static const PreferencesWidget proxy_host_port_elements[] = {
    WidgetEntry (N_("Proxy hostname:"),
        WidgetString (0, "proxy_host")),
    WidgetEntry (N_("Proxy port:"),
        WidgetString (0, "proxy_port"))
};

static const PreferencesWidget proxy_auth_elements[] = {
    WidgetEntry (N_("Proxy username:"),
        WidgetString (0, "proxy_user")),
    WidgetEntry (N_("Proxy password:"),
        WidgetString (0, "proxy_pass"),
        {true})
};

static const PreferencesWidget connectivity_page_widgets[] = {
    WidgetLabel (N_("<b>Proxy Configuration</b>")),
    WidgetCheck (N_("Enable proxy usage"),
        WidgetBool (0, "use_proxy")),
    WidgetTable ({{proxy_host_port_elements}},
        WIDGET_CHILD),
    WidgetCheck (N_("Use authentication with proxy"),
        WidgetBool (0, "use_proxy_auth")),
    WidgetTable ({{proxy_auth_elements}},
        WIDGET_CHILD)
};

static const PreferencesWidget chardet_elements[] = {
#ifdef USE_CHARDET
    WidgetCombo (N_("Auto character encoding detector for:"),
        WidgetString (0, "chardet_detector"),
        {{chardet_detector_presets}}),
#endif
    WidgetEntry (N_("Fallback character encodings:"),
        WidgetString (0, "chardet_fallback"))
};

static void send_title_change (void);
static void * create_titlestring_table (void);

static const PreferencesWidget playlist_page_widgets[] = {
    WidgetLabel (N_("<b>Behavior</b>")),
    WidgetCheck (N_("Continue playback on startup"),
        WidgetBool (0, "resume_playback_on_startup")),
    WidgetCheck (N_("Advance when the current song is deleted"),
        WidgetBool (0, "advance_on_delete")),
    WidgetCheck (N_("Clear the playlist when opening files"),
        WidgetBool (0, "clear_playlist")),
    WidgetCheck (N_("Open files in a temporary playlist"),
        WidgetBool (0, "open_to_temporary")),
    WidgetCheck (N_("Do not load metadata for songs until played"),
        WidgetBool (0, "metadata_on_play")),
    WidgetLabel (N_("<b>Compatibility</b>")),
    WidgetCheck (N_("Interpret \\ (backward slash) as a folder delimiter"),
        WidgetBool (0, "convert_backslash")),
    WidgetTable ({{chardet_elements}}),
    WidgetLabel (N_("<b>Song Display</b>")),
    WidgetCheck (N_("Show song numbers"),
        WidgetBool (0, "show_numbers_in_pl", send_title_change)),
    WidgetCheck (N_("Show leading zeroes (02:00 instead of 2:00)"),
        WidgetBool (0, "leading_zero", send_title_change)),
    WidgetCustom (create_titlestring_table)
};

static const PreferencesWidget song_info_page_widgets[] = {
    WidgetLabel (N_("<b>Album Art</b>")),
    WidgetLabel (N_("Search for images matching these words (comma-separated):")),
    WidgetEntry (0, WidgetString (0, "cover_name_include")),
    WidgetLabel (N_("Exclude images matching these words (comma-separated):")),
    WidgetEntry (0, WidgetString (0, "cover_name_exclude")),
    WidgetCheck (N_("Search for images matching song file name"),
        WidgetBool (0, "use_file_cover")),
    WidgetCheck (N_("Search recursively"),
        WidgetBool (0, "recurse_for_cover")),
    WidgetSpin (N_("Search depth:"),
        WidgetInt (0, "recurse_for_cover_depth"),
        {0, 100, 1},
        WIDGET_CHILD),
    WidgetLabel (N_("<b>Popup Information</b>")),
    WidgetCheck (N_("Show popup information"),
        WidgetBool (0, "show_filepopup_for_tuple")),
    WidgetSpin (N_("Popup delay (tenths of a second):"),
        WidgetInt (0, "filepopup_delay"),
        {0, 100, 1},
        WIDGET_CHILD),
    WidgetCheck (N_("Show time scale for current song"),
        WidgetBool (0, "filepopup_showprogressbar"),
        WIDGET_CHILD)
};

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

static Index<ComboBoxElements> fill_plugin_combo (int type)
{
    Index<ComboBoxElements> elems;
    elems.insert (0, aud_plugin_count (type));

    for (int i = 0; i < elems.len (); i ++)
    {
        elems[i].label = aud_plugin_get_name (aud_plugin_by_index (type, i));
        elems[i].value = GINT_TO_POINTER (i);
    }

    return elems;
}

static void change_category (int category)
{
    if (aud_get_headless_mode () && category > CATEGORY_APPEARANCE)
        category --;

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

static void send_title_change (void)
{
    hook_call ("title change", nullptr);
}

static void titlestring_tag_menu_cb (GtkMenuItem * menuitem, void * data)
{
    const char * separator = " - ";
    auto tag = (const TitleFieldTag *) data;
    int pos = gtk_editable_get_position ((GtkEditable *) titlestring_entry);

    /* insert separator as needed */
    if (gtk_entry_get_text ((GtkEntry *) titlestring_entry)[0])
        gtk_editable_insert_text ((GtkEditable *) titlestring_entry, separator, -1, & pos);

    gtk_editable_insert_text ((GtkEditable *) titlestring_entry, _(tag->tag), -1, & pos);
    gtk_editable_set_position ((GtkEditable *) titlestring_entry, pos);
}

static void on_titlestring_help_button_clicked (GtkButton * button, void * menu)
{
    gtk_menu_popup ((GtkMenu *) menu, nullptr, nullptr, nullptr, nullptr, 0, GDK_CURRENT_TIME);
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
    aud_set_str (nullptr, "generic_title_format", format);
    update_titlestring_cbox (cbox, format);
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
    gtk_tree_view_column_pack_start (column, renderer, false);
    gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", 0, nullptr);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, false);
    gtk_tree_view_column_set_attributes (column, renderer, "text", 1, nullptr);

    g_object_set ((GObject *) renderer, "wrap-width", 96, "wrap-mode",
     PANGO_WRAP_WORD_CHAR, nullptr);

    GtkListStore * store = gtk_list_store_new (CATEGORY_VIEW_N_COLS,
     GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
    gtk_tree_view_set_model (treeview, (GtkTreeModel *) store);

    const char * data_dir = aud_get_path (AudPath::DataDir);

    for (const Category & category : categories)
    {
        if (& category == & categories[CATEGORY_APPEARANCE] && aud_get_headless_mode ())
            continue;

        GtkTreeIter iter;
        gtk_list_store_append (store, & iter);
        gtk_list_store_set (store, & iter, CATEGORY_VIEW_COL_NAME,
         gettext (category.name), -1);

        StringBuf path = filename_build ({data_dir, "images", category.icon_path});
        GdkPixbuf * img = gdk_pixbuf_new_from_file (path, nullptr);

        if (img)
        {
            gtk_list_store_set (store, & iter, CATEGORY_VIEW_COL_ICON, img, -1);
            g_object_unref (img);
        }
    }

    g_object_unref (store);

    GtkTreeSelection * selection = gtk_tree_view_get_selection (treeview);
    g_signal_connect (selection, "changed", (GCallback) category_changed, nullptr);
}

static GtkWidget * create_titlestring_tag_menu (void)
{
    GtkWidget * titlestring_tag_menu = gtk_menu_new ();

    for (const TitleFieldTag & tag : title_field_tags)
    {
        GtkWidget * menu_item = gtk_menu_item_new_with_label (_(tag.name));
        gtk_menu_shell_append ((GtkMenuShell *) titlestring_tag_menu, menu_item);
        g_signal_connect (menu_item, "activate",
         (GCallback) titlestring_tag_menu_cb, (void *) & tag);
    }

    gtk_widget_show_all (titlestring_tag_menu);

    return titlestring_tag_menu;
}

static void create_titlestring_widgets (GtkWidget * * cbox, GtkWidget * * entry)
{
    * cbox = gtk_combo_box_text_new ();
    for (int i = 0; i < TITLESTRING_NPRESETS; i ++)
        gtk_combo_box_text_append_text ((GtkComboBoxText *) * cbox, _(titlestring_preset_names[i]));
    gtk_combo_box_text_append_text ((GtkComboBoxText *) * cbox, _("Custom"));

    * entry = gtk_entry_new ();

    String format = aud_get_str (nullptr, "generic_title_format");
    update_titlestring_cbox ((GtkComboBox *) * cbox, format);
    gtk_entry_set_text ((GtkEntry *) * entry, format);

    g_signal_connect (* cbox, "changed", (GCallback) on_titlestring_cbox_changed, * entry);
    g_signal_connect (* entry, "changed", (GCallback) on_titlestring_entry_changed, * cbox);
}

static void * create_titlestring_table (void)
{
    GtkWidget * grid = gtk_table_new (0, 0, false);
    gtk_table_set_row_spacings ((GtkTable *) grid, 6);
    gtk_table_set_col_spacings ((GtkTable *) grid, 6);

    GtkWidget * label = gtk_label_new (_("Title format:"));
    gtk_misc_set_alignment ((GtkMisc *) label, 1, 0.5);
    gtk_table_attach ((GtkTable *) grid, label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

    label = gtk_label_new (_("Custom string:"));
    gtk_misc_set_alignment ((GtkMisc *) label, 1, 0.5);
    gtk_table_attach ((GtkTable *) grid, label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

    GtkWidget * titlestring_cbox;
    create_titlestring_widgets (& titlestring_cbox, & titlestring_entry);
    gtk_table_attach_defaults ((GtkTable *) grid, titlestring_cbox, 1, 2, 0, 1);
    gtk_table_attach_defaults ((GtkTable *) grid, titlestring_entry, 1, 2, 1, 2);

    GtkWidget * titlestring_help_button = gtk_button_new ();
    gtk_widget_set_can_focus (titlestring_help_button, false);
    gtk_button_set_focus_on_click ((GtkButton *) titlestring_help_button, false);
    gtk_button_set_relief ((GtkButton *) titlestring_help_button, GTK_RELIEF_HALF);
    gtk_table_attach ((GtkTable *) grid, titlestring_help_button, 2, 3, 1, 2,
     GTK_FILL, GTK_FILL, 0, 0);

    GtkWidget * titlestring_tag_menu = create_titlestring_tag_menu ();

    g_signal_connect (titlestring_help_button, "clicked",
     (GCallback) on_titlestring_help_button_clicked, titlestring_tag_menu);

    GtkWidget * image = gtk_image_new_from_icon_name ("list-add", GTK_ICON_SIZE_BUTTON);
    gtk_container_add ((GtkContainer *) titlestring_help_button, image);

    return grid;
}

static void create_playlist_category (void)
{
    GtkWidget * vbox = gtk_vbox_new (false, 0);
    gtk_container_add ((GtkContainer *) category_notebook, vbox);
    audgui_create_widgets (vbox, playlist_page_widgets);
}

static void create_song_info_category (void)
{
    GtkWidget * vbox = gtk_vbox_new (false, 0);
    gtk_container_add ((GtkContainer *) category_notebook, vbox);
    audgui_create_widgets (vbox, song_info_page_widgets);
}

static void iface_fill_prefs_box (void)
{
    Plugin * header = (Plugin *) aud_plugin_get_header (aud_plugin_get_current (PLUGIN_TYPE_IFACE));
    if (header && header->prefs)
        audgui_create_widgets_with_domain (iface_prefs_box, header->prefs->widgets, header->domain);
}

static int iface_combo_changed_finish (void *)
{
    iface_fill_prefs_box ();
    gtk_widget_show_all (iface_prefs_box);

    audgui_cleanup ();

    return G_SOURCE_REMOVE;
}

static void iface_combo_changed (void)
{
    /* prevent audgui from being shut down during the switch */
    audgui_init ();

    gtk_container_foreach ((GtkContainer *) iface_prefs_box,
     (GtkCallback) gtk_widget_destroy, nullptr);

    aud_plugin_enable (aud_plugin_by_index (PLUGIN_TYPE_IFACE, iface_combo_selected), true);

    /* now wait till we have restarted into the new main loop */
    g_idle_add_full (G_PRIORITY_HIGH, iface_combo_changed_finish, nullptr, nullptr);
}

static ArrayRef<const ComboBoxElements> iface_combo_fill ()
{
    if (! iface_combo_elements.len ())
    {
        iface_combo_elements = fill_plugin_combo (PLUGIN_TYPE_IFACE);
        iface_combo_selected = aud_plugin_get_index (aud_plugin_get_current (PLUGIN_TYPE_IFACE));
    }

    return {iface_combo_elements.begin (), iface_combo_elements.len ()};
}

static void * iface_create_prefs_box (void)
{
    iface_prefs_box = gtk_vbox_new (false, 0);
    iface_fill_prefs_box ();
    return iface_prefs_box;
}

static void create_appearance_category (void)
{
    GtkWidget * vbox = gtk_vbox_new (false, 0);
    gtk_container_add ((GtkContainer *) category_notebook, vbox);
    audgui_create_widgets (vbox, appearance_page_widgets);
}

static void output_combo_changed (void)
{
    PluginHandle * plugin = aud_plugin_by_index (PLUGIN_TYPE_OUTPUT, output_combo_selected);

    if (aud_plugin_enable (plugin, true))
    {
        gtk_widget_set_sensitive (output_config_button, aud_plugin_has_configure (plugin));
        gtk_widget_set_sensitive (output_about_button, aud_plugin_has_about (plugin));
    }
}

static ArrayRef<const ComboBoxElements> output_combo_fill ()
{
    if (! output_combo_elements.len ())
    {
        output_combo_elements = fill_plugin_combo (PLUGIN_TYPE_OUTPUT);
        output_combo_selected = aud_plugin_get_index (aud_plugin_get_current (PLUGIN_TYPE_OUTPUT));
    }

    return {output_combo_elements.begin (), output_combo_elements.len ()};
}

static void output_bit_depth_changed (void)
{
    aud_output_reset (OutputReset::ReopenStream);
}

static void output_do_config (void * unused)
{
    audgui_show_plugin_prefs (aud_plugin_get_current (PLUGIN_TYPE_OUTPUT));
}

static void output_do_about (void * unused)
{
    audgui_show_plugin_about (aud_plugin_get_current (PLUGIN_TYPE_OUTPUT));
}

static void * output_create_config_button (void)
{
    gboolean enabled = aud_plugin_has_configure (aud_plugin_get_current (PLUGIN_TYPE_OUTPUT));

    output_config_button = audgui_button_new (_("_Settings"),
     "preferences-system", output_do_config, nullptr);
    gtk_widget_set_sensitive (output_config_button, enabled);

    return output_config_button;
}

static void * output_create_about_button (void)
{
    gboolean enabled = aud_plugin_has_about (aud_plugin_get_current (PLUGIN_TYPE_OUTPUT));

    output_about_button = audgui_button_new (_("_About"), "help-about", output_do_about, nullptr);
    gtk_widget_set_sensitive (output_about_button, enabled);

    return output_about_button;
}

static void create_audio_category (void)
{
    GtkWidget * audio_page_vbox = gtk_vbox_new (false, 0);
    audgui_create_widgets (audio_page_vbox, audio_page_widgets);
    gtk_container_add ((GtkContainer *) category_notebook, audio_page_vbox);
}

static void create_connectivity_category (void)
{
    GtkWidget * connectivity_page_vbox = gtk_vbox_new (false, 0);
    gtk_container_add ((GtkContainer *) category_notebook, connectivity_page_vbox);

    GtkWidget * vbox = gtk_vbox_new (false, 0);
    gtk_box_pack_start ((GtkBox *) connectivity_page_vbox, vbox, true, true, 0);

    audgui_create_widgets (vbox, connectivity_page_widgets);
}

static void create_plugin_category (void)
{
    plugin_notebook = gtk_notebook_new ();
    gtk_container_add ((GtkContainer *) category_notebook, plugin_notebook);

    for (const PluginCategory & category : plugin_categories)
        gtk_notebook_append_page ((GtkNotebook *) plugin_notebook,
         plugin_view_new (category.type), gtk_label_new (_(category.name)));
}

static void destroy_cb (void)
{
    prefswin = nullptr;
    category_treeview = nullptr;
    category_notebook = nullptr;
    titlestring_entry = nullptr;

    iface_combo_elements.clear ();
    output_combo_elements.clear ();
}

static void create_prefs_window (void)
{
    prefswin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint ((GtkWindow *) prefswin, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_container_set_border_width ((GtkContainer *) prefswin, 12);
    gtk_window_set_title ((GtkWindow *) prefswin, _("Audacious Settings"));
    gtk_window_set_default_size ((GtkWindow *) prefswin, 680, 400);

    GtkWidget * vbox = gtk_vbox_new (false, 0);
    gtk_container_add ((GtkContainer *) prefswin, vbox);

    GtkWidget * hbox = gtk_hbox_new (false, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, true, true, 0);

    GtkWidget * scrolledwindow = gtk_scrolled_window_new (nullptr, nullptr);
    gtk_box_pack_start ((GtkBox *) hbox, scrolledwindow, false, false, 0);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolledwindow,
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scrolledwindow, GTK_SHADOW_IN);

    category_treeview = gtk_tree_view_new ();
    gtk_container_add ((GtkContainer *) scrolledwindow, category_treeview);
    gtk_widget_set_size_request (scrolledwindow, 168, -1);
    gtk_tree_view_set_headers_visible ((GtkTreeView *) category_treeview, false);

    category_notebook = gtk_notebook_new ();
    gtk_box_pack_start ((GtkBox *) hbox, category_notebook, true, true, 0);

    gtk_widget_set_can_focus (category_notebook, false);
    gtk_notebook_set_show_tabs ((GtkNotebook *) category_notebook, false);
    gtk_notebook_set_show_border ((GtkNotebook *) category_notebook, false);

    if (! aud_get_headless_mode ())
        create_appearance_category ();

    create_audio_category ();
    create_connectivity_category ();
    create_playlist_category ();
    create_song_info_category ();
    create_plugin_category ();

    GtkWidget * hseparator = gtk_hseparator_new ();
    gtk_box_pack_start ((GtkBox *) vbox, hseparator, false, false, 6);

    hbox = gtk_hbox_new (false, 0);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, false, false, 0);

    GtkWidget * audversionlabel = gtk_label_new (aud_version_string);
    gtk_box_pack_start ((GtkBox *) hbox, audversionlabel, false, false, 0);
    gtk_label_set_use_markup ((GtkLabel *) audversionlabel, true);

    GtkWidget * prefswin_button_box = gtk_hbutton_box_new ();
    gtk_box_pack_start ((GtkBox *) hbox, prefswin_button_box, true, true, 0);
    gtk_button_box_set_layout ((GtkButtonBox *) prefswin_button_box, GTK_BUTTONBOX_END);
    gtk_box_set_spacing ((GtkBox *) prefswin_button_box, 6);

    GtkWidget * close = audgui_button_new (_("_Close"), "window-close",
     (AudguiCallback) gtk_widget_destroy, prefswin);
    gtk_container_add ((GtkContainer *) prefswin_button_box, close);
    gtk_widget_set_can_default (close, true);

    fill_category_list ((GtkTreeView *) category_treeview, (GtkNotebook *) category_notebook);

    gtk_widget_show_all (vbox);

    g_signal_connect (prefswin, "destroy", (GCallback) destroy_cb, nullptr);

    audgui_destroy_on_escape (prefswin);
}

EXPORT void audgui_show_prefs_window (void)
{
    if (! prefswin)
        create_prefs_window ();

    change_category (CATEGORY_APPEARANCE);

    gtk_window_present ((GtkWindow *) prefswin);
}

EXPORT void audgui_show_prefs_for_plugin_type (int type)
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

        for (const PluginCategory & category : plugin_categories)
        {
            if (category.type == type)
                gtk_notebook_set_current_page ((GtkNotebook *) plugin_notebook,
                 & category - plugin_categories);
        }
    }

    gtk_window_present ((GtkWindow *) prefswin);
}

EXPORT void audgui_hide_prefs_window (void)
{
    if (prefswin)
        gtk_widget_destroy (prefswin);
}
