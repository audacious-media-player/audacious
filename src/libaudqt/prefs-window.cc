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
#include <QCheckBox>
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
#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/mainloop.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugin.h>
#include <libaudcore/plugins.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include "libguess/libguess.h"

#include "libaudqt.h"
#include "prefs-pluginlist-model.h"

namespace audqt {

class PrefsWindow : public QDialog
{
public:
    static PrefsWindow * get_instance () {
        if (! instance)
            (void) new PrefsWindow;
        return instance;
    }

    static void destroy_instance () {
        if (instance)
            delete instance;
    }

    static ArrayRef<ComboItem> get_output_combo () {
        return {instance->output_combo_elements.begin (),
                instance->output_combo_elements.len ()};
    }

    static int output_combo_selected;
    static void output_combo_changed () { instance->output_change (); }

    static void * get_output_config_button () { return instance->output_config_button; }
    static void * get_output_about_button () { return instance->output_about_button; }

    static void * get_record_checkbox () { return instance->record_checkbox; }
    static void * get_record_config_button () { return instance->record_config_button; }
    static void * get_record_about_button () { return instance->record_about_button; }

private:
    static PrefsWindow * instance;

    PrefsWindow ();
    ~PrefsWindow () { instance = nullptr; }

    Index<ComboItem> output_combo_elements;
    QPushButton * output_config_button, * output_about_button;

    QCheckBox * record_checkbox;
    QPushButton * record_config_button, * record_about_button;

    void output_setup ();
    void output_change ();

    void record_setup ();
    void record_update ();

    const HookReceiver<PrefsWindow>
     record_hook {"enable record", this, & PrefsWindow::record_update};
};

/* static data */
PrefsWindow * PrefsWindow::instance = nullptr;
int PrefsWindow::output_combo_selected;

struct Category {
    const char * icon_path;
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

static const TitleFieldTag title_field_tags[] = {
    { N_("Artist")      , "${artist}" },
    { N_("Album")       , "${album}" },
    { N_("Album Artist"), "${album-artist}" },
    { N_("Title")       , "${title}" },
    { N_("Track number"), "${track-number}" },
    { N_("Genre")       , "${genre}" },
    { N_("File name")   , "${file-name}" },
    { N_("File path")   , "${file-path}" },
    { N_("Date")        , "${date}" },
    { N_("Year")        , "${year}" },
    { N_("Comment")     , "${comment}" },
    { N_("Codec")       , "${codec}" },
    { N_("Quality")     , "${quality}" }
};

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

static const ComboItem bitdepth_elements[] = {
    ComboItem (N_("Automatic"), -1),
    ComboItem ("16", 16),
    ComboItem ("24", 24),
    ComboItem ("32", 32),
    ComboItem (N_("Floating point"), 0)
};

static const ComboItem record_elements[] = {
    ComboItem (N_("As decoded"), (int) OutputStream::AsDecoded),
    ComboItem (N_("After applying ReplayGain"), (int) OutputStream::AfterReplayGain),
    ComboItem (N_("After applying effects"), (int) OutputStream::AfterEffects),
    ComboItem (N_("After applying equalization"), (int) OutputStream::AfterEqualizer)
};

static const ComboItem replaygainmode_elements[] = {
    ComboItem (N_("Track"), (int) ReplayGainMode::Track),
    ComboItem (N_("Album"), (int) ReplayGainMode::Album),
    ComboItem (N_("Based on shuffle"), (int) ReplayGainMode::Automatic)
};

static Index<ComboItem> iface_combo_elements;
static int iface_combo_selected;
static QWidget * iface_prefs_box;

static ArrayRef<ComboItem> iface_combo_fill ();
static void iface_combo_changed ();
static void * iface_create_prefs_box ();

static const PreferencesWidget appearance_page_widgets[] = {
    WidgetCombo (N_("Interface:"),
        WidgetInt (iface_combo_selected, iface_combo_changed),
        {0, iface_combo_fill}),
    WidgetSeparator ({true}),
    WidgetCustomQt (iface_create_prefs_box)
};

static void output_bit_depth_changed ();

static const PreferencesWidget output_combo_widgets[] = {
    WidgetCombo (N_("Output plugin:"),
        WidgetInt (PrefsWindow::output_combo_selected,
                   PrefsWindow::output_combo_changed,
                   "audqt update output combo"),
        {0, PrefsWindow::get_output_combo}),
    WidgetCustomQt (PrefsWindow::get_output_config_button),
    WidgetCustomQt (PrefsWindow::get_output_about_button)
};

static const PreferencesWidget record_buttons[] = {
    WidgetCustomQt (PrefsWindow::get_record_config_button),
    WidgetCustomQt (PrefsWindow::get_record_about_button)
};

static const PreferencesWidget gain_table[] = {
    WidgetSpin (N_("Amplify all files:"),
        WidgetFloat (0, "replay_gain_preamp"),
        {-15, 15, 0.1, N_("dB")}),
    WidgetSpin (N_("Amplify untagged files:"),
        WidgetFloat (0, "default_gain"),
        {-15, 15, 0.1, N_("dB")})
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
    WidgetLabel (N_("<b>Recording Settings</b>")),
    WidgetCustomQt (PrefsWindow::get_record_checkbox),
    WidgetBox ({{record_buttons}, true},
        WIDGET_CHILD),
    WidgetCombo (N_("Record stream:"),
        WidgetInt (0, "record_stream"),
        {{record_elements}}),
    WidgetLabel (N_("<b>ReplayGain</b>")),
    WidgetCheck (N_("Enable ReplayGain"),
        WidgetBool (0, "enable_replay_gain")),
    WidgetCombo (N_("Mode:"),
        WidgetInt (0, "replay_gain_mode"),
        {{replaygainmode_elements}},
        WIDGET_CHILD),
    WidgetCheck (N_("Prevent clipping (recommended)"),
        WidgetBool (0, "enable_clipping_prevention"),
        WIDGET_CHILD),
    WidgetTable ({{gain_table}},
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
    WidgetCombo (N_("Auto character encoding detector for:"),
        WidgetString (0, "chardet_detector"),
        {{chardet_detector_presets}}),
    WidgetEntry (N_("Fallback character encodings:"),
        WidgetString (0, "chardet_fallback"))
};

static void send_title_change ();
static void * create_titlestring_table ();

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
    WidgetLabel (N_("<b>Song Display</b>")),
    WidgetCheck (N_("Show song numbers"),
        WidgetBool (0, "show_numbers_in_pl", send_title_change)),
    WidgetCheck (N_("Show leading zeroes (02:00 vs. 2:00)"),
        WidgetBool (0, "leading_zero", send_title_change)),
    WidgetCheck (N_("Show hours separately (1:30:00 vs. 90:00)"),
        WidgetBool (0, "show_hours", send_title_change)),
    WidgetCustomQt (create_titlestring_table),
    WidgetLabel (N_("<b>Compatibility</b>")),
    WidgetCheck (N_("Interpret \\ (backward slash) as a folder delimiter"),
        WidgetBool (0, "convert_backslash")),
    WidgetTable ({{chardet_elements}})
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
        WIDGET_CHILD),
    WidgetLabel (N_("<b>Advanced</b>")),
    WidgetCheck (N_("Guess missing metadata from file path"),
        WidgetBool (0, "metadata_fallbacks")),
    WidgetCheck (N_("Do not load metadata for songs until played"),
        WidgetBool (0, "metadata_on_play")),
    WidgetCheck (N_("Probe content of files with no recognized file name extension"),
        WidgetBool (0, "slow_probe"))
};

#define TITLESTRING_NPRESETS 8

static const char * const titlestring_presets[TITLESTRING_NPRESETS] = {
    "${title}",
    "${title}${?artist: - ${artist}}",
    "${title}${?artist: - ${artist}}${?album: - ${album}}",
    "${?artist:${artist} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${?track-number:${track-number}. }${title}",
    "${?artist:${artist} }${?album:[ ${album} ] }${?artist:- }${?track-number:${track-number}. }${title}",
    "${?album:${album} - }${title}"
};

static const char * const titlestring_preset_names[TITLESTRING_NPRESETS] = {
    N_("TITLE"),
    N_("TITLE - ARTIST"),
    N_("TITLE - ARTIST - ALBUM"),
    N_("ARTIST - TITLE"),
    N_("ARTIST - ALBUM - TITLE"),
    N_("ARTIST - ALBUM - TRACK. TITLE"),
    N_("ARTIST [ ALBUM ] - TRACK. TITLE"),
    N_("ALBUM - TITLE")
};

static void * create_titlestring_table ()
{
    QWidget * w = new QWidget;
    QGridLayout * l = new QGridLayout (w);
    l->setContentsMargins (0, 0, 0, 0);
    l->setSpacing (sizes.TwoPt);

    QLabel * lbl = new QLabel (_("Title format:"), w);
    l->addWidget (lbl, 0, 0);

    QComboBox * cbox = new QComboBox (w);
    l->addWidget (cbox, 0, 1);

    for (int i = 0; i < TITLESTRING_NPRESETS; i ++)
        cbox->addItem (translate_str (titlestring_preset_names [i]), i);
    cbox->addItem (_("Custom"), TITLESTRING_NPRESETS);
    cbox->setCurrentIndex (TITLESTRING_NPRESETS);

    lbl = new QLabel (_("Custom string:"), w);
    l->addWidget (lbl, 1, 0);

    QLineEdit * le = new QLineEdit (w);
    l->addWidget (le, 1, 1);

    String format = aud_get_str (nullptr, "generic_title_format");
    le->setText ((const char *) format);
    for (int i = 0; i < TITLESTRING_NPRESETS; i ++)
    {
        if (! strcmp (titlestring_presets [i], format))
            cbox->setCurrentIndex (i);
    }

    QObject::connect (le, & QLineEdit::textChanged, [] (const QString & text) {
        aud_set_str (nullptr, "generic_title_format", text.toUtf8 ().data ());
    });

    void (QComboBox::* signal) (int) = & QComboBox::currentIndexChanged;
    QObject::connect (cbox, signal, [le] (int idx) {
        if (idx < TITLESTRING_NPRESETS)
            le->setText (titlestring_presets [idx]);
    });

    /* build menu */
    QPushButton * btn_mnu = new QPushButton (w);
    btn_mnu->setFixedWidth (btn_mnu->sizeHint ().height ());
    btn_mnu->setIcon (QIcon::fromTheme ("list-add"));
    l->addWidget (btn_mnu, 1, 2);

    QMenu * mnu_fields = new QMenu (w);

    for (auto & t : title_field_tags)
    {
        QAction * a = mnu_fields->addAction (_(t.name));
        QObject::connect (a, & QAction::triggered, [=] () {
            le->insert (t.tag);
        });
    }

    QObject::connect (btn_mnu, & QAbstractButton::clicked, [=] () {
        mnu_fields->popup (btn_mnu->mapToGlobal (QPoint (0, 0)));
    });

    return w;
}

static Index<ComboItem> fill_plugin_combo (PluginType type)
{
    Index<ComboItem> elems;
    int i = 0;

    for (PluginHandle * plugin : aud_plugin_list (type))
        elems.append (aud_plugin_get_name (plugin), i ++);

    return elems;
}

static void send_title_change ()
{
    if (aud_drct_get_ready ())
        hook_call ("title change", nullptr);
}

static void iface_fill_prefs_box ()
{
    Plugin * header = (Plugin *) aud_plugin_get_header (aud_plugin_get_current (PluginType::Iface));
    if (header && header->info.prefs)
    {
        auto vbox = make_vbox (iface_prefs_box, sizes.TwoPt);
        prefs_populate (vbox, header->info.prefs->widgets, header->info.domain);
    }
}

static void iface_combo_changed ()
{
    /* prevent audqt from being shut down during the switch */
    init ();

    if (QLayout * layout = iface_prefs_box->layout ())
    {
        clear_layout (layout);
        delete layout;
    }

    aud_plugin_enable (aud_plugin_list (PluginType::Iface)[iface_combo_selected], true);

    iface_fill_prefs_box ();
    cleanup ();
}

static ArrayRef<ComboItem> iface_combo_fill ()
{
    if (! iface_combo_elements.len ())
    {
        iface_combo_elements = fill_plugin_combo (PluginType::Iface);
        iface_combo_selected = aud_plugin_list (PluginType::Iface).
         find (aud_plugin_get_current (PluginType::Iface));
    }

    return {iface_combo_elements.begin (), iface_combo_elements.len ()};
}

static void * iface_create_prefs_box ()
{
    iface_prefs_box = new QWidget;
    iface_fill_prefs_box ();
    return iface_prefs_box;
}

static void output_bit_depth_changed ()
{
    aud_output_reset (OutputReset::ReopenStream);
}

static void create_category (QStackedWidget * notebook, ArrayRef<PreferencesWidget> widgets)
{
    QWidget * w = new QWidget;
    auto vbox = make_vbox (w, sizes.TwoPt);
    prefs_populate (vbox, widgets, nullptr);
    vbox->addStretch (1);

    notebook->addWidget (w);
}

static QTreeView * s_plugin_view;
static PluginListModel * s_plugin_model;

static void create_plugin_category (QStackedWidget * parent)
{
    s_plugin_view = new QTreeView (parent);
    s_plugin_model = new PluginListModel (s_plugin_view);

    s_plugin_view->setModel (s_plugin_model);
    s_plugin_view->setSelectionMode (QTreeView::NoSelection);

    auto header = s_plugin_view->header ();

    header->hide ();
    header->setSectionResizeMode (header->ResizeToContents);
    header->setStretchLastSection (false);

    parent->addWidget (s_plugin_view);

    QObject::connect (s_plugin_view, & QAbstractItemView::clicked, [] (const QModelIndex & index)
    {
        auto p = s_plugin_model->pluginForIndex (index);
        if (! p)
            return;

        switch (index.column ())
        {
        case PluginListModel::AboutColumn:
            plugin_about (p);
            break;
        case PluginListModel::SettingsColumn:
            plugin_prefs (p);
            break;
        }
    });
}

static QStackedWidget * s_category_notebook = nullptr;

PrefsWindow::PrefsWindow () :
    output_combo_elements (fill_plugin_combo (PluginType::Output)),
    output_config_button (new QPushButton (translate_str (N_("_Settings")))),
    output_about_button (new QPushButton (translate_str (N_("_About")))),
    record_checkbox (new QCheckBox),
    record_config_button (new QPushButton (translate_str (N_("_Settings")))),
    record_about_button (new QPushButton (translate_str (N_("_About"))))
{
    /* initialize static data */
    instance = this;
    output_combo_selected = aud_plugin_list (PluginType::Output)
     .find (aud_plugin_get_current (PluginType::Output));

    setAttribute (Qt::WA_DeleteOnClose);
    setWindowTitle (_("Audacious Settings"));
    setContentsMargins (0, 0, 0, 0);

    QToolBar * toolbar = new QToolBar;
    toolbar->setToolButtonStyle (Qt::ToolButtonTextUnderIcon);

    QWidget * child = new QWidget;
    child->setContentsMargins (margins.FourPt);

    auto vbox_parent = make_vbox (this);
    vbox_parent->addWidget (toolbar);
    vbox_parent->addWidget (child);

    auto child_vbox = make_vbox (child);

    s_category_notebook = new QStackedWidget;
    child_vbox->addWidget (s_category_notebook);

    create_category (s_category_notebook, appearance_page_widgets);
    create_category (s_category_notebook, audio_page_widgets);
    create_category (s_category_notebook, connectivity_page_widgets);
    create_category (s_category_notebook, playlist_page_widgets);
    create_category (s_category_notebook, song_info_page_widgets);
    create_plugin_category (s_category_notebook);

    QDialogButtonBox * bbox = new QDialogButtonBox (QDialogButtonBox::Close);
    bbox->button (QDialogButtonBox::Close)->setText (translate_str (N_("_Close")));
    child_vbox->addWidget (bbox);

    QObject::connect (bbox, & QDialogButtonBox::rejected, this, & QObject::deleteLater);

    QSignalMapper * mapper = new QSignalMapper (this);
    const char * data_dir = aud_get_path (AudPath::DataDir);

    QObject::connect (mapper, static_cast <void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
                      s_category_notebook, static_cast <void (QStackedWidget::*)(int)>(&QStackedWidget::setCurrentIndex));

    for (int i = 0; i < CATEGORY_COUNT; i ++)
    {
        QIcon ico (QString (filename_build ({data_dir, "images", categories[i].icon_path})));
        QAction * a = new QAction (ico, translate_str (categories[i].name), toolbar);

        toolbar->addAction (a);
        mapper->setMapping (a, i);

        void (QSignalMapper::* slot) () = & QSignalMapper::map;
        QObject::connect (a, & QAction::triggered, mapper, slot);
    }

    output_setup ();
    record_setup ();
    record_update ();
}

void PrefsWindow::output_setup ()
{
    auto p = aud_plugin_get_current (PluginType::Output);

    output_config_button->setEnabled (aud_plugin_has_configure (p));
    output_about_button->setEnabled (aud_plugin_has_about (p));

    QObject::connect (output_config_button, & QPushButton::clicked, [] (bool) {
        plugin_prefs (aud_plugin_get_current (PluginType::Output));
    });

    QObject::connect (output_about_button, & QPushButton::clicked, [] (bool) {
        plugin_about (aud_plugin_get_current (PluginType::Output));
    });
}

void PrefsWindow::output_change ()
{
    auto & list = aud_plugin_list (PluginType::Output);
    auto p = list[output_combo_selected];

    if (aud_plugin_enable (p, true))
    {
        output_config_button->setEnabled (aud_plugin_has_configure (p));
        output_about_button->setEnabled (aud_plugin_has_about (p));
    }
    else
    {
        /* set combo box back to current output */
        output_combo_selected = list.find (aud_plugin_get_current (PluginType::Output));
        hook_call ("audqt update output combo", nullptr);
    }
}

void PrefsWindow::record_setup ()
{
    QObject::connect (record_checkbox, & QCheckBox::clicked, [] (bool checked) {
        aud_drct_enable_record (checked);
    });

    QObject::connect (record_config_button, & QPushButton::clicked, [] (bool) {
        if (aud_drct_get_record_enabled ())
            plugin_prefs (aud_drct_get_record_plugin ());
    });

    QObject::connect (record_about_button, & QPushButton::clicked, [] (bool) {
        if (aud_drct_get_record_enabled ())
            plugin_about (aud_drct_get_record_plugin ());
    });
}

void PrefsWindow::record_update ()
{
    auto p = aud_drct_get_record_plugin ();

    if (p)
    {
        bool enabled = aud_drct_get_record_enabled ();
        auto text = str_printf (_("Enable audio stream recording with %s"), aud_plugin_get_name (p));

        record_checkbox->setEnabled (true);
        record_checkbox->setText ((const char *) text);
        record_checkbox->setChecked (enabled);
        record_config_button->setEnabled (enabled && aud_plugin_has_configure (p));
        record_about_button->setEnabled (enabled && aud_plugin_has_about (p));
    }
    else
    {
        record_checkbox->setEnabled (false);
        record_checkbox->setText (_("No audio recording plugin available"));
        record_checkbox->setChecked (false);
        record_config_button->setEnabled (false);
        record_about_button->setEnabled (false);
    }
}

EXPORT void prefswin_show ()
{
    window_bring_to_front (PrefsWindow::get_instance ());
}

EXPORT void prefswin_hide ()
{
    PrefsWindow::destroy_instance ();
}

EXPORT void prefswin_show_page (int id, bool show)
{
    if (id < 0 || id > CATEGORY_COUNT)
        return;

    auto win = PrefsWindow::get_instance ();
    s_category_notebook->setCurrentIndex (id);

    if (show)
        window_bring_to_front (win);
}

EXPORT void prefswin_show_plugin_page (PluginType type)
{
    if (type == PluginType::Iface)
        prefswin_show_page (CATEGORY_APPEARANCE);
    else if (type == PluginType::Output)
        prefswin_show_page (CATEGORY_AUDIO);
    else
    {
        prefswin_show_page (CATEGORY_PLUGINS, false);

        s_plugin_view->collapseAll ();

        auto index = s_plugin_model->indexForType (type);
        if (index.isValid ())
        {
            s_plugin_view->expand (index);
            s_plugin_view->scrollTo (index, QTreeView::PositionAtTop);
            s_plugin_view->setCurrentIndex (index);
        }

        window_bring_to_front (PrefsWindow::get_instance ());
    }
}

} // namespace audqt
