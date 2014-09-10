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

#include <stdio.h>

#include "objects.h"

enum class AudPath {
    BinDir,
    DataDir,
    PluginDir,
    LocaleDir,
    DesktopFile,
    IconFile,
    UserDir,
    PlaylistDir,
    n_paths
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

void aud_init_paths ();
void aud_cleanup_paths ();

const char * aud_get_path (AudPath id);

void aud_set_headless_mode (bool headless);
bool aud_get_headless_mode ();

void aud_set_verbose_mode (bool verbose);
bool aud_get_verbose_mode ();

void aud_set_mainloop_type (MainloopType type);
MainloopType aud_get_mainloop_type ();

/* logger.cc */
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

typedef void (* aud_log_handler_t) (LogLevel level, const char * file, unsigned int line, const char * function, const char * message);

void aud_logger_subscribe (aud_log_handler_t hdl);
void aud_logger_log (LogLevel level, const char * file, unsigned int line, const char * function, const char * format, ...);
void aud_logger_stdio (LogLevel level, const char * filename, unsigned int line, const char * function, const char * message);
const char * aud_logger_get_level_name (LogLevel level);

#define AUDDBG(...) do { \
    aud_logger_log (LogLevel::Debug, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); \
} while (0)

/* Requires: aud_init_paths() */
void aud_init_i18n ();

void aud_config_set_defaults (const char * section, const char * const * entries);

void aud_set_str (const char * section, const char * name, const char * value);
String aud_get_str (const char * section, const char * name);
void aud_set_bool (const char * section, const char * name, bool value);
bool aud_get_bool (const char * section, const char * name);
void aud_set_int (const char * section, const char * name, int value);
int aud_get_int (const char * section, const char * name);
void aud_set_double (const char * section, const char * name, double value);
double aud_get_double (const char * section, const char * name);

void aud_init ();
void aud_resume ();
void aud_run ();
void aud_quit ();
void aud_cleanup ();

String aud_history_get (int entry);
void aud_history_add (const char * path);

void aud_output_reset (OutputReset type);

#endif
