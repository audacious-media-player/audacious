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

#ifndef AUD_PLUGIN_DOMAIN
#define AUD_PLUGIN_DOMAIN PACKAGE
#endif
#ifndef AUD_PLUGIN_ABOUT
#define AUD_PLUGIN_ABOUT nullptr
#endif
#ifndef AUD_PLUGIN_PREFS
#define AUD_PLUGIN_PREFS nullptr
#endif

#ifdef AUD_DECLARE_INPUT

class InputPluginInstance : public InputPlugin
{
public:
    static constexpr PluginInfo info = {
        AUD_PLUGIN_NAME,
        AUD_PLUGIN_DOMAIN,
        AUD_PLUGIN_ABOUT,
        AUD_PLUGIN_PREFS
    };

    static constexpr auto input_info = InputInfo (
#ifdef AUD_INPUT_SUBTUNES
     FlagSubtunes |
#endif
#ifdef AUD_INPUT_WRITE_TUPLE
     FlagWritesTag |
#endif
     0)
#ifdef AUD_INPUT_PRIORITY
     .with_priority (AUD_INPUT_PRIORITY)
#endif
#ifdef AUD_INPUT_EXTS
     .with_exts (to_str_array (AUD_INPUT_EXTS))
#endif
#ifdef AUD_INPUT_MIMES
     .with_mimes (to_str_array (AUD_INPUT_MIMES))
#endif
#ifdef AUD_INPUT_SCHEMES
     .with_schemes (to_str_array (AUD_INPUT_SCHEMES))
#endif
     ;

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
