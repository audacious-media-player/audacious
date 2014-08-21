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

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <string.h>

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

static QWidget * m_prefswin = nullptr;

struct PluginCategory {
    int type;
    const char * name;
};

struct TitleFieldTag {
    const char * name;
    const char * tag;
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

#define INT_TO_POINTER(i) ((void *) ((intptr_t) i))

static const ComboBoxElements bitdepth_elements[] = {
    { INT_TO_POINTER (16), "16" },
    { INT_TO_POINTER (24), "24" },
    { INT_TO_POINTER (32), "32" },
    { INT_TO_POINTER  (0), N_("Floating point") }
};

#ifdef XXX_NOTYET
static Index<ComboBoxElements> output_combo_elements;
static int output_combo_selected;
static QWidget * output_config_button;
static QWidget * output_about_button;

static ArrayRef<const ComboBoxElements> output_combo_fill ();
static void output_combo_changed (void);
static void * output_create_config_button (void);
static void * output_create_about_button (void);
#endif
static void output_bit_depth_changed (void);

static const PreferencesWidget audio_page_widgets[] = {
    WidgetLabel (N_("<b>Output Settings</b>")),
#ifdef XXX_NOTYET
    WidgetBox ({{output_combo_widgets}, true}),
#endif
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
#ifdef XXX_NOTYET
    WidgetCustom (create_titlestring_table)
#endif
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
        elems[i].value = INT_TO_POINTER (i);
    }

    return elems;
}

static void send_title_change (void)
{
    hook_call ("title change", nullptr);
}

static void output_bit_depth_changed (void)
{
    aud_output_reset (OutputReset::ReopenStream);
}

static void create_audio_category (QTabWidget * category_notebook)
{
    QWidget * audio_page = new QWidget;
    QVBoxLayout * audio_page_vbox = new QVBoxLayout;

    audio_page->setLayout (audio_page_vbox);
    prefs_populate (audio_page_vbox, audio_page_widgets, nullptr);

    category_notebook->addTab (audio_page, "Audio");
}

static void create_connectivity_category (QTabWidget * category_notebook)
{
    QWidget * connectivity_page = new QWidget;
    QVBoxLayout * connectivity_page_vbox = new QVBoxLayout;

    connectivity_page->setLayout (connectivity_page_vbox);

    prefs_populate (connectivity_page_vbox, connectivity_page_widgets, nullptr);

    category_notebook->addTab (connectivity_page, "Connectivity");
}

static void create_playlist_category (QTabWidget * category_notebook)
{
    QWidget * playlist_page = new QWidget;
    QVBoxLayout * playlist_page_vbox = new QVBoxLayout;

    playlist_page->setLayout (playlist_page_vbox);

    prefs_populate (playlist_page_vbox, playlist_page_widgets, nullptr);

    category_notebook->addTab (playlist_page, "Playlist");
}

static void create_song_info_category (QTabWidget * category_notebook)
{
    QWidget * song_info_page = new QWidget;
    QVBoxLayout * song_info_page_vbox = new QVBoxLayout;

    song_info_page->setLayout (song_info_page_vbox);

    prefs_populate (song_info_page_vbox, song_info_page_widgets, nullptr);

    category_notebook->addTab (song_info_page, "Song Info");
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

    view->setModel (plm);
    view->header ()->hide ();

    vbox->addWidget (view);

    QDialogButtonBox * bbox = new QDialogButtonBox;
    vbox->addWidget (bbox);

    QPushButton * about_btn = new QPushButton (_("About"));
    about_btn->setEnabled (false);

    QPushButton * settings_btn = new QPushButton (_("Settings"));
    settings_btn->setEnabled (false);

    bbox->addButton (about_btn, QDialogButtonBox::ActionRole);
    bbox->addButton (settings_btn, QDialogButtonBox::ActionRole);

    QItemSelectionModel * model = view->selectionModel ();
    QObject::connect (model, &QItemSelectionModel::selectionChanged, [=] (const QItemSelection & selected, const QItemSelection & deselected) {
        if (selected.length () < 1)
            return;

        int idx = selected.indexes () [0].row ();
        PluginHandle * ph = aud_plugin_by_index (category_id, idx);

        if (! ph)
            return;

        AUDDBG ("plugin %s selected\n", aud_plugin_get_name (ph));

        about_btn_watch (about_btn, ph);
        settings_btn_watch (settings_btn, ph);
    });

    QObject::connect (about_btn, &QAbstractButton::clicked, [=] (bool) {
        const QItemSelection & selected = model->selection ();
        if (selected.length () < 1)
            return;

        int idx = selected.indexes () [0].row ();
        PluginHandle * ph = aud_plugin_by_index (category_id, idx);

        if (! ph)
            return;

        AUDDBG ("plugin %s: about\n", aud_plugin_get_name (ph));

        plugin_about (ph);
    });

    QObject::connect (settings_btn, &QAbstractButton::clicked, [=] (bool) {
        const QItemSelection & selected = model->selection ();
        if (selected.length () < 1)
            return;

        int idx = selected.indexes () [0].row ();
        PluginHandle * ph = aud_plugin_by_index (category_id, idx);

        if (! ph)
            return;

        AUDDBG ("plugin %s: settings\n", aud_plugin_get_name (ph));
    });
}

static void create_plugin_category (QTabWidget * parent)
{
    QTabWidget * child = new QTabWidget;

    for (const PluginCategory & w : plugin_categories)
    {
        create_plugin_category_page (w.type, w.name, child);
    }

    parent->addTab (child, "Plugins");
}

static void create_prefs_window ()
{
    static QVBoxLayout m_vbox;
    static QTabWidget category_notebook;

    m_prefswin = new QWidget;
    m_prefswin->setWindowTitle (_("Audacious Settings"));
    m_prefswin->setLayout (& m_vbox);

    m_vbox.addWidget (& category_notebook);

    create_audio_category (& category_notebook);
    create_connectivity_category (& category_notebook);
    create_playlist_category (& category_notebook);
    create_song_info_category (& category_notebook);
    create_plugin_category (& category_notebook);
}

EXPORT void prefswin_show ()
{
    if (! m_prefswin)
        create_prefs_window ();

    window_bring_to_front (m_prefswin);
}

EXPORT void prefswin_hide ()
{
    if (! m_prefswin)
        return;

    m_prefswin->hide ();
}

};
