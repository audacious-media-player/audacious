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

    constexpr InputPluginInstance () :
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

EXPORT InputPluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_INPUT

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

    constexpr OutputPluginInstance () :
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

    void write_audio (const void * data, int size)
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

EXPORT OutputPluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_OUTPUT

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

    constexpr GeneralPluginInstance () :
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

EXPORT GeneralPluginInstance aud_plugin_instance;

#endif // AUD_DECLARE_GENERAL
