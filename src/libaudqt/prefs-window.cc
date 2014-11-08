/*
 * prefs-window.cc
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

#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSignalMapper>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugin.h>
#include <libaudcore/plugins.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include "libaudqt.h"
#include "prefs-pluginlist-model.h"

#ifdef USE_CHARDET
#include <libguess.h>
#endif

namespace audqt {

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

enum {
    CATEGORY_APPEARANCE = 0,
    CATEGORY_AUDIO,
    CATEGORY_NETWORK,
    CATEGORY_PLAYLIST,
    CATEGORY_SONG_INFO,
    CATEGORY_PLUGINS,
    CATEGORY_COUNT
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
static const ComboItem chardet_detector_presets[] = {
    ComboItem (N_("None"), ""),
    ComboItem (N_("Arabic"), GUESS_REGION_AR),
    ComboItem (N_("Baltic"), GUESS_REGION_BL),
    ComboItem (N_("Chinese"), GUESS_REGION_CN),
    ComboItem (N_("Greek"), GUESS_REGION_GR),
    ComboItem (N_("Hebrew"), GUESS_REGION_HW),
    ComboItem (N_("Japanese"), GUESS_REGION_JP),
    ComboItem (N_("Korean"), GUESS_REGION_KR),
    ComboItem (N_("Polish"), GUESS_REGION_PL),
    ComboItem (N_("Russian"), GUESS_REGION_RU),
    ComboItem (N_("Taiwanese"), GUESS_REGION_TW),
    ComboItem (N_("Turkish"), GUESS_REGION_TR)
};
#endif

static const ComboItem bitdepth_elements[] = {
    ComboItem ("16", 16),
    ComboItem ("24", 24),
    ComboItem ("32", 32),
    ComboItem (N_("Floating point"), 0)
};

static Index<ComboItem> iface_combo_elements;
static int iface_combo_selected;
static QWidget * iface_prefs_box;

static ArrayRef<const ComboItem> iface_combo_fill ();
static void iface_combo_changed (void);
static void * iface_create_prefs_box (void);

static const PreferencesWidget appearance_page_widgets[] = {
    WidgetLabel (N_("<b>Interface Settings</b>")),
    WidgetCombo (N_("Interface plugin:"),
        WidgetInt (iface_combo_selected, iface_combo_changed),
        {0, iface_combo_fill}),
    WidgetCustomQt (iface_create_prefs_box)
};

static Index<ComboItem> output_combo_elements;
static int output_combo_selected;
static QPushButton * output_config_button;
static QPushButton * output_about_button;

static ArrayRef<const ComboItem> output_combo_fill ();
static void output_combo_changed (void);
static void * output_create_config_button (void);
static void * output_create_about_button (void);
static void output_bit_depth_changed (void);

static const PreferencesWidget output_combo_widgets[] = {
    WidgetCombo (N_("Output plugin:"),
        WidgetInt (output_combo_selected, output_combo_changed),
        {0, output_combo_fill}),
    WidgetCustomQt (output_create_config_button),
    WidgetCustomQt (output_create_about_button)
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
    WidgetLabel (N_("<b>Network Settings</b>")),
    WidgetSpin (N_("Buffer size:"),
        WidgetInt (0, "net_buffer_kb"),
        {16, 1024, 16, N_("KiB")}),
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
    WidgetCheck (N_("Resume playback on startup"),
        WidgetBool (0, "resume_playback_on_startup")),
    WidgetCheck (N_("Pause instead of resuming immediately"),
        WidgetBool (0, "always_resume_paused"),
        WIDGET_CHILD),
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
    WidgetCustomQt (create_titlestring_table),
    WidgetLabel (N_("<b>Advanced</b>")),
    WidgetCheck (N_("Do not load metadata for songs until played"),
        WidgetBool (0, "metadata_on_play")),
    WidgetCheck (N_("Probe content of files with no recognized filename extension"),
        WidgetBool (0, "slow_probe"))
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

static void * create_titlestring_table (void)
{
    QWidget * w = new QWidget;
    QGridLayout * l = new QGridLayout (w);

    QLabel * lbl = new QLabel (_("Title format:"), w);
    l->addWidget (lbl, 0, 0);

    QComboBox * cbox = new QComboBox (w);
    l->addWidget (cbox, 0, 1);

    for (int i = 0; i < TITLESTRING_NPRESETS; i++)
        cbox->addItem (translate_str (titlestring_preset_names [i]), i);
    cbox->addItem (_("Custom"), TITLESTRING_NPRESETS);
    cbox->setCurrentIndex (TITLESTRING_NPRESETS);

    lbl = new QLabel (_("Custom string:"), w);
    l->addWidget (lbl, 1, 0);

    QLineEdit * le = new QLineEdit (w);
    l->addWidget (le, 1, 1);

    w->setLayout (l);

    String format = aud_get_str (nullptr, "generic_title_format");
    le->setText ((const char *) format);
    for (int i = 0; i < TITLESTRING_NPRESETS; i++)
    {
        if (! strcmp (titlestring_presets [i], format))
        {
            cbox->setCurrentIndex (i);
        }
    }

    QObject::connect (le, &QLineEdit::textChanged, [=] (const QString & text) {
        aud_set_str (nullptr, "generic_title_format", text.toLocal8Bit ().data ());
        send_title_change ();
    });

    QObject::connect (cbox,
                      static_cast <void (QComboBox::*) (int)> (&QComboBox::currentIndexChanged),
                      [=] (int idx) {
        if (idx < TITLESTRING_NPRESETS)
            le->setText (titlestring_presets [idx]);
    });

    /* build menu */
    QPushButton * btn_mnu = new QPushButton (w);
    btn_mnu->setIcon (QIcon::fromTheme ("list-add"));
    l->addWidget (btn_mnu, 1, 2);

    QMenu * mnu_fields = new QMenu (w);

    for (auto & t : title_field_tags)
    {
        QAction * a = mnu_fields->addAction (_(t.name));
        QObject::connect (a, &QAction::triggered, [=] () {
            le->insert (t.tag);
        });
    }

    QObject::connect (btn_mnu, &QAbstractButton::clicked, [=] () {
        mnu_fields->popup (btn_mnu->mapToGlobal (QPoint (0, 0)));
    });

    return w;
}

static Index<ComboItem> fill_plugin_combo (int type)
{
    Index<ComboItem> elems;
    int i = 0;

    for (PluginHandle * plugin : aud_plugin_list (type))
        elems.append (aud_plugin_get_name (plugin), i ++);

    return elems;
}

static void send_title_change (void)
{
    hook_call ("title change", nullptr);
}

static void iface_fill_prefs_box (void)
{
    Plugin * header = (Plugin *) aud_plugin_get_header (aud_plugin_get_current (PLUGIN_TYPE_IFACE));
    if (header && header->info.prefs)
    {
        QVBoxLayout * vbox = new QVBoxLayout;

        prefs_populate (vbox, header->info.prefs->widgets, header->info.domain);
        iface_prefs_box->setLayout (vbox);
    }
}

#ifdef XXX_NOTYET
static int iface_combo_changed_finish (void *)
{
    iface_fill_prefs_box ();
    gtk_widget_show_all (iface_prefs_box);

    audgui_cleanup ();

    return G_SOURCE_REMOVE;
}
#endif

static void iface_combo_changed (void)
{
#ifdef XXX_NOTYET
    /* prevent audgui from being shut down during the switch */
    audgui_init ();

    gtk_container_foreach ((GtkContainer *) iface_prefs_box,
     (GtkCallback) gtk_widget_destroy, nullptr);

    aud_plugin_enable (aud_plugin_by_index (PLUGIN_TYPE_IFACE, iface_combo_selected), true);

    /* now wait till we have restarted into the new main loop */
    g_idle_add_full (G_PRIORITY_HIGH, iface_combo_changed_finish, nullptr, nullptr);
#endif
}

static ArrayRef<const ComboItem> iface_combo_fill ()
{
    if (! iface_combo_elements.len ())
    {
        iface_combo_elements = fill_plugin_combo (PLUGIN_TYPE_IFACE);
        iface_combo_selected = aud_plugin_list (PLUGIN_TYPE_IFACE)
         .find (aud_plugin_get_current (PLUGIN_TYPE_IFACE));
    }

    return {iface_combo_elements.begin (), iface_combo_elements.len ()};
}

static void * iface_create_prefs_box (void)
{
    iface_prefs_box = new QWidget;
    iface_fill_prefs_box ();
    return iface_prefs_box;
}

static void output_combo_changed (void)
{
    PluginHandle * plugin = aud_plugin_list (PLUGIN_TYPE_OUTPUT)[output_combo_selected];

    if (aud_plugin_enable (plugin, true))
    {
        output_config_button->setEnabled (aud_plugin_has_configure (plugin));
        output_about_button->setEnabled (aud_plugin_has_about (plugin));
    }
}

static void * output_create_config_button (void)
{
    bool enabled = aud_plugin_has_configure (aud_plugin_get_current (PLUGIN_TYPE_OUTPUT));

    output_config_button = new QPushButton (translate_str (N_("_Settings")));
    output_config_button->setEnabled (enabled);

    QObject::connect (output_config_button, &QAbstractButton::clicked, [=] (bool) {
        plugin_prefs (aud_plugin_get_current (PLUGIN_TYPE_OUTPUT));
    });

    return output_config_button;
}

static void * output_create_about_button (void)
{
    bool enabled = aud_plugin_has_about (aud_plugin_get_current (PLUGIN_TYPE_OUTPUT));

    output_about_button = new QPushButton (translate_str (N_("_About")));
    output_about_button->setEnabled (enabled);

    QObject::connect (output_about_button, &QAbstractButton::clicked, [=] (bool) {
        plugin_about (aud_plugin_get_current (PLUGIN_TYPE_OUTPUT));
    });

    return output_about_button;
}

static ArrayRef<const ComboItem> output_combo_fill ()
{
    if (! output_combo_elements.len ())
    {
        output_combo_elements = fill_plugin_combo (PLUGIN_TYPE_OUTPUT);
        output_combo_selected = aud_plugin_list (PLUGIN_TYPE_OUTPUT)
         .find (aud_plugin_get_current (PLUGIN_TYPE_OUTPUT));
    }

    return {output_combo_elements.begin (), output_combo_elements.len ()};
}

static void output_bit_depth_changed (void)
{
    aud_output_reset (OutputReset::ReopenStream);
}

static void create_appearance_category (QStackedWidget * category_notebook)
{
    QWidget * w = new QWidget;
    QVBoxLayout * vbox = new QVBoxLayout;

    vbox->setContentsMargins (0, 0, 0, 0);
    vbox->setSpacing (0);
    vbox->setMargin (0);
    prefs_populate (vbox, appearance_page_widgets, nullptr);

    w->setLayout (vbox);
    category_notebook->addWidget (w);
}

static void create_audio_category (QStackedWidget * category_notebook)
{
    QWidget * audio_page = new QWidget;
    QVBoxLayout * audio_page_vbox = new QVBoxLayout;

    audio_page_vbox->setContentsMargins (0, 0, 0, 0);
    audio_page_vbox->setSpacing (0);
    audio_page_vbox->setMargin (0);
    prefs_populate (audio_page_vbox, audio_page_widgets, nullptr);

    audio_page->setLayout (audio_page_vbox);
    category_notebook->addWidget (audio_page);
}

static void create_connectivity_category (QStackedWidget * category_notebook)
{
    QWidget * connectivity_page = new QWidget;
    QVBoxLayout * connectivity_page_vbox = new QVBoxLayout;

    connectivity_page_vbox->setContentsMargins (0, 0, 0, 0);
    connectivity_page_vbox->setSpacing (0);
    connectivity_page_vbox->setMargin (0);
    prefs_populate (connectivity_page_vbox, connectivity_page_widgets, nullptr);

    connectivity_page->setLayout (connectivity_page_vbox);
    category_notebook->addWidget (connectivity_page);
}

static void create_playlist_category (QStackedWidget * category_notebook)
{
    QWidget * playlist_page = new QWidget;
    QVBoxLayout * playlist_page_vbox = new QVBoxLayout;

    playlist_page_vbox->setContentsMargins (0, 0, 0, 0);
    playlist_page_vbox->setSpacing (0);
    playlist_page_vbox->setMargin (0);
    prefs_populate (playlist_page_vbox, playlist_page_widgets, nullptr);

    playlist_page->setLayout (playlist_page_vbox);
    category_notebook->addWidget (playlist_page);
}

static void create_song_info_category (QStackedWidget * category_notebook)
{
    QWidget * song_info_page = new QWidget;
    QVBoxLayout * song_info_page_vbox = new QVBoxLayout;

    song_info_page_vbox->setContentsMargins (0, 0, 0, 0);
    song_info_page_vbox->setSpacing (0);
    song_info_page_vbox->setMargin (0);
    prefs_populate (song_info_page_vbox, song_info_page_widgets, nullptr);

    song_info_page->setLayout (song_info_page_vbox);
    category_notebook->addWidget (song_info_page);
}

static void about_btn_watch (QPushButton * btn, PluginHandle * ph)
{
    bool enabled = (aud_plugin_has_about (ph) && aud_plugin_get_enabled (ph));
    btn->setEnabled (enabled);
}

static void settings_btn_watch (QPushButton * btn, PluginHandle * ph)
{
    bool enabled = (aud_plugin_has_configure (ph) && aud_plugin_get_enabled (ph));
    btn->setEnabled (enabled);
}

static void create_plugin_category_page (int category_id, const char * category_name, QTabWidget * parent)
{
    QWidget * w = new QWidget;
    QVBoxLayout * vbox = new QVBoxLayout;

    w->setLayout (vbox);
    parent->addTab (w, category_name);

    QTreeView * view = new QTreeView;
    PluginListModel * plm = new PluginListModel (0, category_id);

    view->setIndentation (0);
    view->setModel (plm);
    view->header ()->hide ();

    vbox->addWidget (view);

    QDialogButtonBox * bbox = new QDialogButtonBox;
    vbox->addWidget (bbox);

    QPushButton * about_btn = new QPushButton (translate_str (N_("_About")));
    about_btn->setEnabled (false);

    QPushButton * settings_btn = new QPushButton (translate_str (N_("_Settings")));
    settings_btn->setEnabled (false);

    bbox->addButton (about_btn, QDialogButtonBox::ActionRole);
    bbox->addButton (settings_btn, QDialogButtonBox::ActionRole);

    QItemSelectionModel * model = view->selectionModel ();
    QObject::connect (model, &QItemSelectionModel::selectionChanged, [=] (const QItemSelection & selected, const QItemSelection & deselected) {
        if (selected.length () < 1)
            return;

        auto & list = aud_plugin_list (category_id);
        int idx = selected.indexes () [0].row ();
        if (idx < 0 || idx >= list.len ())
            return;

        AUDDBG ("plugin %s selected\n", aud_plugin_get_name (list[idx]));

        about_btn_watch (about_btn, list[idx]);
        settings_btn_watch (settings_btn, list[idx]);
    });

    QObject::connect (about_btn, &QAbstractButton::clicked, [=] (bool) {
        const QItemSelection & selected = model->selection ();
        if (selected.length () < 1)
            return;

        auto & list = aud_plugin_list (category_id);
        int idx = selected.indexes () [0].row ();
        if (idx < 0 || idx >= list.len ())
            return;

        AUDDBG ("plugin %s: about\n", aud_plugin_get_name (list[idx]));

        plugin_about (list[idx]);
    });

    QObject::connect (settings_btn, &QAbstractButton::clicked, [=] (bool) {
        const QItemSelection & selected = model->selection ();
        if (selected.length () < 1)
            return;

        auto & list = aud_plugin_list (category_id);
        int idx = selected.indexes () [0].row ();
        if (idx < 0 || idx >= list.len ())
            return;

        AUDDBG ("plugin %s: settings\n", aud_plugin_get_name (list[idx]));

        plugin_prefs (list[idx]);
    });
}

static QTabWidget * plugin_tabs = nullptr;

static void create_plugin_category (QStackedWidget * parent)
{
    plugin_tabs = new QTabWidget;

    for (const PluginCategory & w : plugin_categories)
    {
        create_plugin_category_page (w.type, _(w.name), plugin_tabs);
    }

    parent->addWidget (plugin_tabs);
}

static QDialog * s_prefswin = nullptr;
static QStackedWidget * s_category_notebook = nullptr;

static void create_prefs_window ()
{
    QVBoxLayout * vbox_parent = new QVBoxLayout;
    QToolBar * toolbar = new QToolBar;

    s_prefswin = new QDialog;
    s_prefswin->setWindowTitle (_("Audacious Settings"));
    s_prefswin->setLayout (vbox_parent);
    s_prefswin->setAttribute (Qt::WA_DeleteOnClose);

    QObject::connect (s_prefswin, & QObject::destroyed, [] () {
        s_prefswin = nullptr;
    });

    vbox_parent->setSpacing (0);
    vbox_parent->setMargin (0);
    vbox_parent->setContentsMargins (0, 0, 0, 0);
    vbox_parent->addWidget (toolbar);

    QWidget * child = new QWidget;
    QVBoxLayout * child_vbox = new QVBoxLayout;

    vbox_parent->addWidget (child);

    child->setLayout (child_vbox);

    s_category_notebook = new QStackedWidget;
    child_vbox->addWidget (s_category_notebook);

    create_appearance_category (s_category_notebook);
    create_audio_category (s_category_notebook);
    create_connectivity_category (s_category_notebook);
    create_playlist_category (s_category_notebook);
    create_song_info_category (s_category_notebook);
    create_plugin_category (s_category_notebook);

    QDialogButtonBox * bbox = new QDialogButtonBox (QDialogButtonBox::Close);
    child_vbox->addWidget (bbox);

    QObject::connect (bbox, &QDialogButtonBox::rejected, s_prefswin, &QObject::deleteLater);

    toolbar->setToolButtonStyle (Qt::ToolButtonTextUnderIcon);

    QSignalMapper * mapper = new QSignalMapper;
    const char * data_dir = aud_get_path (AudPath::DataDir);

    QObject::connect (mapper, static_cast <void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
                      s_category_notebook, static_cast <void (QStackedWidget::*)(int)>(&QStackedWidget::setCurrentIndex));

    for (int i = 0; i < CATEGORY_COUNT; i++)
    {
        QIcon ico (QString (filename_build ({data_dir, "images", categories[i].icon_path})));
        QAction * a = new QAction (ico, translate_str (categories[i].name), nullptr);

        toolbar->addAction (a);

        mapper->setMapping (a, i);

        QObject::connect (a, &QAction::triggered, mapper, static_cast <void (QSignalMapper::*)()>(&QSignalMapper::map));
    }
}

EXPORT void prefswin_show ()
{
    if (! s_prefswin)
        create_prefs_window ();

    window_bring_to_front (s_prefswin);
}

EXPORT void prefswin_hide ()
{
    delete s_prefswin;
}

EXPORT void prefswin_show_page (int id, bool show)
{
    if (id < 0 || id > CATEGORY_COUNT)
        return;

    if (! s_prefswin)
        create_prefs_window ();

    s_category_notebook->setCurrentIndex (id);

    if (show)
        window_bring_to_front (s_prefswin);
}

EXPORT void prefswin_show_plugin_page (int type)
{
    if (! s_prefswin)
        create_prefs_window ();

    if (type == PLUGIN_TYPE_IFACE)
        return prefswin_show_page (CATEGORY_APPEARANCE);
    else if (type == PLUGIN_TYPE_OUTPUT)
        return prefswin_show_page (CATEGORY_AUDIO);
    else
    {
        prefswin_show_page (CATEGORY_PLUGINS, false);

        for (const PluginCategory & category : plugin_categories)
        {
            if (category.type == type)
                plugin_tabs->setCurrentIndex (& category - plugin_categories);
        }

        window_bring_to_front (s_prefswin);
    }
}

} // namespace audqt
