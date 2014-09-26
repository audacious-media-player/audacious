/*
 * plugin-declare.h
 * Copyright 2014 John Lindgren
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

template<int N>
constexpr ArrayRef<const char *> to_str_array (const char * const (& array) [N])
    { return {array, N - 1}; }

constexpr ArrayRef<const char *> to_str_array (decltype (nullptr))
    { return {nullptr}; }

#ifndef AUD_PLUGIN_DOMAIN
#define AUD_PLUGIN_DOMAIN PACKAGE
#endif
#ifndef AUD_PLUGIN_ABOUT
#define AUD_PLUGIN_ABOUT nullptr
#endif
#ifndef AUD_PLUGIN_PREFS
#define AUD_PLUGIN_PREFS nullptr
#endif

#ifdef AUD_PLUGIN_ABOUTWIN
#warning AUD_PLUGIN_ABOUTWIN is deprecated!
#endif
#ifdef AUD_PLUGIN_CONFIGWIN
#warning AUD_PLUGIN_CONFIGWIN is deprecated!
#endif

#ifdef AUD_DECLARE_TRANSPORT

class TransportPluginInstance : public TransportPlugin
{
public:
    static constexpr PluginInfo info = {
        AUD_PLUGIN_NAME,
        AUD_PLUGIN_DOMAIN,
        AUD_PLUGIN_ABOUT,
        AUD_PLUGIN_PREFS
    };

    TransportPluginInstance () :
        TransportPlugin (info, to_str_array (AUD_TRANSPORT_SCHEMES), AUD_TRANSPORT_FOPEN) {}

#ifdef AUD_PLUGIN_INIT
    bool init ()
        { return AUD_PLUGIN_INIT (); }
#endif
#ifdef AUD_PLUGIN_CLEANUP
    void cleanup ()
        { AUD_PLUGIN_CLEANUP (); }
#endif
#ifdef AUD_PLUGIN_TAKE_MESSAGE
    int take_message (const char * code, const void * data, int size)
        { return AUD_PLUGIN_TAKE_MESSAGE (code, data, size); }
#endif
};

TransportPluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_TRANSPORT

#ifdef AUD_DECLARE_PLAYLIST

class PlaylistPluginInstance : public PlaylistPlugin
{
public:
    static constexpr PluginInfo info = {
        AUD_PLUGIN_NAME,
        AUD_PLUGIN_DOMAIN,
        AUD_PLUGIN_ABOUT,
        AUD_PLUGIN_PREFS
    };

    PlaylistPluginInstance () :
        PlaylistPlugin (info, to_str_array (AUD_PLAYLIST_EXTS),
#ifdef AUD_PLAYLIST_SAVE
         true) {}
#else
         false) {}
#endif

#ifdef AUD_PLUGIN_INIT
    bool init ()
        { return AUD_PLUGIN_INIT (); }
#endif
#ifdef AUD_PLUGIN_CLEANUP
    void cleanup ()
        { AUD_PLUGIN_CLEANUP (); }
#endif
#ifdef AUD_PLUGIN_TAKE_MESSAGE
    int take_message (const char * code, const void * data, int size)
        { return AUD_PLUGIN_TAKE_MESSAGE (code, data, size); }
#endif

    bool load (const char * path, VFSFile & file, String & title,
     Index<PlaylistAddItem> & items)
        { return AUD_PLAYLIST_LOAD (path, file, title, items); }

#ifdef AUD_PLAYLIST_SAVE
    bool save (const char * path, VFSFile & file, const char * title,
     const Index<PlaylistAddItem> & items)
        { return AUD_PLAYLIST_SAVE (path, file, title, items); }
#endif
};

PlaylistPluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_PLAYLIST

#ifdef AUD_DECLARE_INPUT

#ifndef AUD_INPUT_SUBTUNES
#define AUD_INPUT_SUBTUNES false
#endif
#ifndef AUD_INPUT_EXTS
#define AUD_INPUT_EXTS nullptr
#endif
#ifndef AUD_INPUT_MIMES
#define AUD_INPUT_MIMES nullptr
#endif
#ifndef AUD_INPUT_SCHEMES
#define AUD_INPUT_SCHEMES nullptr
#endif
#ifndef AUD_INPUT_PRIORITY
#define AUD_INPUT_PRIORITY 0
#endif

class InputPluginInstance : public InputPlugin
{
public:
    static constexpr PluginInfo info = {
        AUD_PLUGIN_NAME,
        AUD_PLUGIN_DOMAIN,
        AUD_PLUGIN_ABOUT,
        AUD_PLUGIN_PREFS
    };

    static constexpr InputPluginInfo input_info = {
        AUD_INPUT_PRIORITY,
        AUD_INPUT_SUBTUNES,
#ifdef AUD_INPUT_WRITE_TUPLE
        true,
#else
        false,
#endif
        to_str_array (AUD_INPUT_EXTS),
        to_str_array (AUD_INPUT_MIMES),
        to_str_array (AUD_INPUT_SCHEMES)
    };

    InputPluginInstance () :
        InputPlugin (info, input_info) {}

#ifdef AUD_PLUGIN_INIT
    bool init ()
        { return AUD_PLUGIN_INIT (); }
#endif
#ifdef AUD_PLUGIN_CLEANUP
    void cleanup ()
        { AUD_PLUGIN_CLEANUP (); }
#endif
#ifdef AUD_PLUGIN_TAKE_MESSAGE
    int take_message (const char * code, const void * data, int size)
        { return AUD_PLUGIN_TAKE_MESSAGE (code, data, size); }
#endif

    bool is_our_file (const char * filename, VFSFile & file)
    {
        bool (* ptr) (const char *, VFSFile &) = AUD_INPUT_IS_OUR_FILE;
        return ptr ? ptr (filename, file) : false;
    }

    Tuple read_tuple (const char * filename, VFSFile & file)
        { return AUD_INPUT_READ_TUPLE (filename, file); }
    bool play (const char * filename, VFSFile & file)
        { return AUD_INPUT_PLAY (filename, file); }

#ifdef AUD_INPUT_WRITE_TUPLE
    bool write_tuple (const char * filename, VFSFile & file, const Tuple & tuple)
        { return AUD_INPUT_WRITE_TUPLE (filename, file, tuple); }
#endif
#ifdef AUD_INPUT_READ_IMAGE
    Index<char> read_image (const char * filename, VFSFile & file)
        { return AUD_INPUT_READ_IMAGE (filename, file); }
#endif
#ifdef AUD_INPUT_INFOWIN
    bool file_info_box (const char * filename, VFSFile & file)
        { AUD_INPUT_INFOWIN (filename); return true; }
#endif
};

InputPluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_INPUT

#ifdef AUD_DECLARE_EFFECT

#ifndef AUD_EFFECT_ORDER
#define AUD_EFFECT_ORDER 0
#endif
#ifndef AUD_EFFECT_SAME_FMT
#define AUD_EFFECT_SAME_FMT false
#endif

class EffectPluginInstance : public EffectPlugin
{
public:
    static constexpr PluginInfo info = {
        AUD_PLUGIN_NAME,
        AUD_PLUGIN_DOMAIN,
        AUD_PLUGIN_ABOUT,
        AUD_PLUGIN_PREFS
    };

    EffectPluginInstance () :
        EffectPlugin (info, AUD_EFFECT_ORDER, AUD_EFFECT_SAME_FMT) {}

#ifdef AUD_PLUGIN_INIT
    bool init ()
        { return AUD_PLUGIN_INIT (); }
#endif
#ifdef AUD_PLUGIN_CLEANUP
    void cleanup ()
        { AUD_PLUGIN_CLEANUP (); }
#endif
#ifdef AUD_PLUGIN_TAKE_MESSAGE
    int take_message (const char * code, const void * data, int size)
        { return AUD_PLUGIN_TAKE_MESSAGE (code, data, size); }
#endif

#ifdef AUD_EFFECT_START
    void start (int * channels, int * rate)
        { AUD_EFFECT_START (channels, rate); }
#endif

    void process (float * * data, int * samples)
        { AUD_EFFECT_PROCESS (data, samples); }

#ifdef AUD_EFFECT_FLUSH
    void flush ()
        { AUD_EFFECT_FLUSH (); }
#endif
#ifdef AUD_EFFECT_FINISH
    void finish (float * * data, int * samples)
        { AUD_EFFECT_FINISH (data, samples); }
#endif
#ifdef AUD_EFFECT_ADJ_DELAY
    int adjust_delay (int delay)
        { return AUD_EFFECT_ADJ_DELAY (delay); }
#endif
};

EffectPluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_EFFECT

#ifdef AUD_DECLARE_OUTPUT

#ifndef AUD_OUTPUT_PRIORITY
#define AUD_OUTPUT_PRIORITY 0
#endif
#ifndef AUD_OUTPUT_REOPEN
#define AUD_OUTPUT_REOPEN false
#endif

class OutputPluginInstance : public OutputPlugin
{
public:
    static constexpr PluginInfo info = {
        AUD_PLUGIN_NAME,
        AUD_PLUGIN_DOMAIN,
        AUD_PLUGIN_ABOUT,
        AUD_PLUGIN_PREFS
    };

    OutputPluginInstance () :
        OutputPlugin (info, AUD_OUTPUT_PRIORITY, AUD_OUTPUT_REOPEN) {}

#ifdef AUD_PLUGIN_INIT
    bool init ()
        { return AUD_PLUGIN_INIT (); }
#endif
#ifdef AUD_PLUGIN_CLEANUP
    void cleanup ()
        { AUD_PLUGIN_CLEANUP (); }
#endif
#ifdef AUD_PLUGIN_TAKE_MESSAGE
    int take_message (const char * code, const void * data, int size)
        { return AUD_PLUGIN_TAKE_MESSAGE (code, data, size); }
#endif

#ifdef AUD_OUTPUT_GET_VOLUME
    StereoVolume get_volume ()
    {
        StereoVolume volume = {0, 0};
        AUD_OUTPUT_GET_VOLUME (& volume.left, & volume.right);
        return volume;
    }
#else
    StereoVolume get_volume ()
        { return {0, 0}; }
#endif

#ifdef AUD_OUTPUT_SET_VOLUME
    void set_volume (StereoVolume volume)
        { AUD_OUTPUT_SET_VOLUME (volume.left, volume.right); }
#else
    void set_volume (StereoVolume volume) {}
#endif

    bool open_audio (int format, int rate, int chans)
        { return AUD_OUTPUT_OPEN (format, rate, chans); }
    void close_audio ()
        { AUD_OUTPUT_CLOSE (); }

    int buffer_free ()
    {
        int (* ptr) () = AUD_OUTPUT_GET_FREE;
        return ptr ? ptr () : 44100;
    }

    void period_wait ()
    {
        void (* ptr) () = AUD_OUTPUT_WAIT_FREE;
        if (ptr) ptr ();
    }

    void write_audio (void * data, int size)
        { AUD_OUTPUT_WRITE (data, size); }

    void drain ()
    {
        void (* ptr) () = AUD_OUTPUT_DRAIN;
        if (ptr) ptr ();
    }

    int output_time ()
        { return AUD_OUTPUT_GET_TIME (); }
    void pause (bool p)
        { AUD_OUTPUT_PAUSE (p); }
    void flush (int time)
        { AUD_OUTPUT_FLUSH (time); }
};

OutputPluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_OUTPUT

#ifdef AUD_DECLARE_VIS

class VisPluginInstance : public VisPlugin
{
public:
    static constexpr PluginInfo info = {
        AUD_PLUGIN_NAME,
        AUD_PLUGIN_DOMAIN,
        AUD_PLUGIN_ABOUT,
        AUD_PLUGIN_PREFS
    };

    VisPluginInstance () :
        VisPlugin (info,
#ifdef AUD_VIS_RENDER_MONO
         AUD_VIS_TYPE_MONO_PCM
#endif
#ifdef AUD_VIS_RENDER_MULTI
         AUD_VIS_TYPE_MULTI_PCM
#endif
#ifdef AUD_VIS_RENDER_FREQ
         AUD_VIS_TYPE_FREQ
#endif
         ) {}

#ifdef AUD_PLUGIN_INIT
    bool init ()
        { return AUD_PLUGIN_INIT (); }
#endif
#ifdef AUD_PLUGIN_CLEANUP
    void cleanup ()
        { AUD_PLUGIN_CLEANUP (); }
#endif
#ifdef AUD_PLUGIN_TAKE_MESSAGE
    int take_message (const char * code, const void * data, int size)
        { return AUD_PLUGIN_TAKE_MESSAGE (code, data, size); }
#endif

    void clear ()
    {
        void (* ptr) () = AUD_VIS_CLEAR;
        if (ptr) ptr ();
    }

#ifdef AUD_VIS_RENDER_MONO
    void render_mono_pcm (const float * pcm)
        { AUD_VIS_RENDER_MONO (pcm); }
#endif
#ifdef AUD_VIS_RENDER_MULTI
    void render_multi_pcm (const float * pcm, int channels)
        { AUD_VIS_RENDER_MULTI (pcm, channels); }
#endif
#ifdef AUD_VIS_RENDER_FREQ
    void render_freq (const float * freq)
        { AUD_VIS_RENDER_FREQ (freq); }
#endif

    void * get_gtk_widget ()
        { return AUD_VIS_GET_WIDGET (); }
};

VisPluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_VIS

#ifdef AUD_DECLARE_GENERAL

#ifndef AUD_GENERAL_AUTO_ENABLE
#define AUD_GENERAL_AUTO_ENABLE  false
#endif

class GeneralPluginInstance : public GeneralPlugin
{
public:
    static constexpr PluginInfo info = {
        AUD_PLUGIN_NAME,
        AUD_PLUGIN_DOMAIN,
        AUD_PLUGIN_ABOUT,
        AUD_PLUGIN_PREFS
    };

    GeneralPluginInstance () :
        GeneralPlugin (info, AUD_GENERAL_AUTO_ENABLE) {}

#ifdef AUD_PLUGIN_INIT
    bool init ()
        { return AUD_PLUGIN_INIT (); }
#endif
#ifdef AUD_PLUGIN_CLEANUP
    void cleanup ()
        { AUD_PLUGIN_CLEANUP (); }
#endif
#ifdef AUD_PLUGIN_TAKE_MESSAGE
    int take_message (const char * code, const void * data, int size)
        { return AUD_PLUGIN_TAKE_MESSAGE (code, data, size); }
#endif

#ifdef AUD_GENERAL_GET_WIDGET
    void * get_gtk_widget ()
        { return AUD_GENERAL_GET_WIDGET (); }
#endif
};

GeneralPluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_GENERAL

#ifdef AUD_DECLARE_IFACE

class IfacePluginInstance : public IfacePlugin
{
public:
    static constexpr PluginInfo info = {
        AUD_PLUGIN_NAME,
        AUD_PLUGIN_DOMAIN,
        AUD_PLUGIN_ABOUT,
        AUD_PLUGIN_PREFS
    };

    IfacePluginInstance () :
        IfacePlugin (info) {}

#ifdef AUD_PLUGIN_INIT
    bool init ()
        { return AUD_PLUGIN_INIT (); }
#endif
#ifdef AUD_PLUGIN_CLEANUP
    void cleanup ()
        { AUD_PLUGIN_CLEANUP (); }
#endif
#ifdef AUD_PLUGIN_TAKE_MESSAGE
    int take_message (const char * code, const void * data, int size)
        { return AUD_PLUGIN_TAKE_MESSAGE (code, data, size); }
#endif

    void show (bool show)
        { AUD_IFACE_SHOW (show); }
    void run ()
        { AUD_IFACE_RUN (); }
    void quit ()
        { AUD_IFACE_QUIT (); }

    void show_about_window ()
        { AUD_IFACE_SHOW_ABOUT (); }
    void hide_about_window ()
        { AUD_IFACE_HIDE_ABOUT (); }
    void show_filebrowser (bool open)
        { AUD_IFACE_SHOW_FILEBROWSER (open); }
    void hide_filebrowser ()
        { AUD_IFACE_HIDE_FILEBROWSER (); }
    void show_jump_to_song ()
        { AUD_IFACE_SHOW_JUMP_TO_SONG (); }
    void hide_jump_to_song ()
        { AUD_IFACE_HIDE_JUMP_TO_SONG (); }
    void show_prefs_window ()
        { AUD_IFACE_SHOW_SETTINGS (); }
    void hide_prefs_window ()
        { AUD_IFACE_HIDE_SETTINGS (); }
    void plugin_menu_add (int id, void func (), const char * name, const char * icon)
        { AUD_IFACE_MENU_ADD (id, func, name, icon); }
    void plugin_menu_remove (int id, void func ())
        { AUD_IFACE_MENU_REMOVE (id, func); }
};

IfacePluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_IFACE
