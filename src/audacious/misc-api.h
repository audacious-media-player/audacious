/*
 * misc-api.h
 * Copyright 2010-2013 John Lindgren
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

/* Do not include this file directly; use misc.h instead. */

/* all (char *) return values must be freed with str_unref() */

/* art.c (thread-safe) */

/* Gets album art for <file> (the URI of a song file) as JPEG or PNG data.  If
 * the album art is not yet loaded, sets <data> to NULL and begins to load the
 * album art in the background.  On completion, the "art ready" hook is called,
 * with <file> as a parameter.  The "current art ready" hook is also called if
 * <file> is the currently playing song. */
AUD_VFUNC3 (art_request_data, const char *, file, const void * *, data, int64_t *, len)

/* Similar to art_request_data() but returns the URI of an image file.
 * (A temporary file will be created if necessary.) */
AUD_FUNC1 (const char *, art_request_file, const char *, file)

/* Releases album art returned by art_request_data() or art_request_file(). */
AUD_VFUNC1 (art_unref, const char *, file)

/* config.c (thread-safe) */

AUD_VFUNC2 (config_set_defaults, const char *, section, const char * const *, entries)

AUD_VFUNC3 (set_str, const char *, section, const char *, name, const char *, value)
AUD_FUNC2 (char *, get_str, const char *, section, const char *, name)
AUD_VFUNC3 (set_bool, const char *, section, const char *, name, bool_t, value)
AUD_FUNC2 (bool_t, get_bool, const char *, section, const char *, name)
AUD_VFUNC3 (set_int, const char *, section, const char *, name, int, value)
AUD_FUNC2 (int, get_int, const char *, section, const char *, name)
AUD_VFUNC3 (set_double, const char *, section, const char *, name, double, value)
AUD_FUNC2 (double, get_double, const char *, section, const char *, name)

/* equalizer.c */
AUD_VFUNC1 (eq_set_bands, const double *, values)
AUD_VFUNC1 (eq_get_bands, double *, values)
AUD_VFUNC2 (eq_set_band, int, band, double, value)
AUD_FUNC1 (double, eq_get_band, int, band)

/* equalizer_preset.c */
AUD_FUNC1 (EqualizerPreset *, equalizer_preset_new, const char *, name)
AUD_VFUNC1 (equalizer_preset_free, EqualizerPreset *, preset)
AUD_FUNC1 (Index *, equalizer_read_presets, const char *, basename)
AUD_FUNC2 (bool_t, equalizer_write_presets, Index *, list, const char *, basename)

/* note: legacy code! these are local filenames, not URIs */
AUD_FUNC1 (EqualizerPreset *, load_preset_file, const char *, filename)
AUD_FUNC2 (bool_t, save_preset_file, EqualizerPreset *, preset, const char *, filename)

AUD_FUNC1 (Index *, import_winamp_presets, VFSFile *, file)
AUD_FUNC2 (bool_t, export_winamp_preset, EqualizerPreset *, preset, VFSFile *, file)

/* history.c */
AUD_FUNC1 (const char *, history_get, int, entry)
AUD_VFUNC1 (history_add, const char *, path)

/* interface.c */
AUD_VFUNC1 (interface_show, bool_t, show)
AUD_FUNC0 (bool_t, interface_is_shown)

/* interface_show_error() is safe to call from any thread */
AUD_VFUNC1 (interface_show_error, const char *, message)

/* main.c */
AUD_FUNC1 (const char *, get_path, int, path)
AUD_FUNC0 (bool_t, headless_mode)

/* output.c */
AUD_VFUNC1 (output_reset, int, type)

/* probe.c */
AUD_FUNC2 (PluginHandle *, file_find_decoder, const char *, filename, bool_t,
 fast)
AUD_FUNC2 (Tuple *, file_read_tuple, const char *, filename, PluginHandle *,
 decoder)
AUD_FUNC4 (bool_t, file_read_image, const char *, filename, PluginHandle *,
 decoder, void * *, data, int64_t *, size)
AUD_FUNC2 (bool_t, file_can_write_tuple, const char *, filename,
 PluginHandle *, decoder)
AUD_FUNC3 (bool_t, file_write_tuple, const char *, filename, PluginHandle *,
 decoder, const Tuple *, tuple)
AUD_FUNC2 (bool_t, custom_infowin, const char *, filename, PluginHandle *,
 decoder)

/* ui_plugin_menu.c */
AUD_FUNC1 (/* GtkWidget * */ void *, get_plugin_menu, int, id)
AUD_VFUNC4 (plugin_menu_add, int, id, MenuFunc, func, const char *, name,
 const char *, icon)
AUD_VFUNC2 (plugin_menu_remove, int, id, MenuFunc, func)

/* ui_preferences.c */
AUD_VFUNC4 (create_widgets_with_domain, /* GtkWidget * */ void *, box,
 const PreferencesWidget *, widgets, int, n_widgets, const char *, domain)
AUD_VFUNC0 (show_prefs_window)
AUD_VFUNC1 (show_prefs_for_plugin_type, int, type)

/* util.c */

/* Constructs a full URI given:
 *   1. path: one of the following:
 *     a. a full URI (returned unchanged)
 *     b. an absolute filename (in the system locale)
 *     c. a relative path (character set detected according to user settings)
 *   2. reference: the full URI of the playlist containing <path> */
AUD_FUNC2 (char *, construct_uri, const char *, path, const char *, reference)

/* visualization.c */
AUD_VFUNC2 (vis_func_add, int, type, VisFunc, func)
AUD_VFUNC1 (vis_func_remove, VisFunc, func)
