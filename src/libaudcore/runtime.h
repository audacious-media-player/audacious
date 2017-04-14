/*
 * runtime.h
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

#ifndef LIBAUDCORE_RUNTIME_H
#define LIBAUDCORE_RUNTIME_H

#include <libaudcore/objects.h>

enum class AudPath {
    BinDir,
    DataDir,
    PluginDir,
    LocaleDir,
    DesktopFile,
    IconFile,
    UserDir,
    PlaylistDir,
    count
};

enum class MainloopType {
    GLib,
    Qt
};

enum class OutputReset {
    EffectsOnly,
    ReopenStream,
    ResetPlugin
};

enum class OutputStream {
    AsDecoded,
    AfterReplayGain,
    AfterEffects,
    AfterEqualizer
};

enum class ReplayGainMode {
    Track,
    Album,
    Automatic
};

namespace audlog
{
    enum Level {
        Debug,
        Info,
        Warning,
        Error
    };

    typedef void (* Handler) (Level level, const char * file, int line,
     const char * func, const char * message);

    void set_stderr_level (Level level);
    void subscribe (Handler handler, Level level);
    void unsubscribe (Handler handler);

#ifdef _WIN32
    void log (Level level, const char * file, int line, const char * func,
     const char * format, ...) __attribute__ ((__format__ (gnu_printf, 5, 6)));
#else
    void log (Level level, const char * file, int line, const char * func,
     const char * format, ...) __attribute__ ((__format__ (__printf__, 5, 6)));
#endif

    const char * get_level_name (Level level);
}

#define AUDERR(...) do { audlog::log (audlog::Error, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); } while (0)
#define AUDWARN(...) do { audlog::log (audlog::Warning, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); } while (0)
#define AUDINFO(...) do { audlog::log (audlog::Info, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); } while (0)
#define AUDDBG(...) do { audlog::log (audlog::Debug, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); } while (0)

const char * aud_get_path (AudPath id);

void aud_set_headless_mode (bool headless);
bool aud_get_headless_mode ();

// Note that the UserDir and PlaylistDir paths vary depending on the instance
// number.  Therefore, calling aud_set_instance() after these paths have been
// referenced, or after aud_init(), is an error.
void aud_set_instance (int instance);
int aud_get_instance ();

void aud_set_mainloop_type (MainloopType type);
MainloopType aud_get_mainloop_type ();

void aud_init_i18n ();

void aud_config_set_defaults (const char * section, const char * const * entries);

void aud_set_str (const char * section, const char * name, const char * value);
String aud_get_str (const char * section, const char * name);
void aud_set_bool (const char * section, const char * name, bool value);
bool aud_get_bool (const char * section, const char * name);
void aud_toggle_bool (const char * section, const char * name);
void aud_set_int (const char * section, const char * name, int value);
int aud_get_int (const char * section, const char * name);
void aud_set_double (const char * section, const char * name, double value);
double aud_get_double (const char * section, const char * name);

void aud_init ();
void aud_resume ();
void aud_run ();
void aud_quit ();
void aud_cleanup ();

void aud_leak_check ();

String aud_history_get (int entry);
void aud_history_add (const char * path);

void aud_output_reset (OutputReset type);

#endif
