/*
 * plugins-api.h
 * Copyright 2010-2012 John Lindgren
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

/* CAUTION: These functions are not thread safe. */

/* plugin-init.c */
AUD_FUNC1 (PluginHandle *, plugin_get_current, int, type)
AUD_FUNC2 (bool_t, plugin_enable, PluginHandle *, plugin, bool_t, enable)
AUD_FUNC1 (PluginHandle *, plugin_by_widget, void /* GtkWidget */ *, widget)
AUD_FUNC4 (int, plugin_send_message, PluginHandle *, plugin,
 const char *, code, const void *, data, int, size)

/* plugin-registry.c */
AUD_FUNC1 (int, plugin_get_type, PluginHandle *, plugin)
AUD_FUNC1 (const char *, plugin_get_filename, PluginHandle *, plugin)
AUD_FUNC1 (PluginHandle *, plugin_lookup, const char *, filename)
AUD_FUNC1 (PluginHandle *, plugin_lookup_basename, const char *, basename)

AUD_FUNC1 (const void *, plugin_get_header, PluginHandle *, plugin)
AUD_FUNC1 (PluginHandle *, plugin_by_header, const void *, header)

AUD_FUNC1 (int, plugin_count, int, type)
AUD_FUNC1 (int, plugin_get_index, PluginHandle *, plugin)
AUD_FUNC2 (PluginHandle *, plugin_by_index, int, type, int, index)

AUD_FUNC2 (int, plugin_compare, PluginHandle *, a, PluginHandle *, b)
AUD_VFUNC3 (plugin_for_each, int, type, PluginForEachFunc, func, void *, data)

AUD_FUNC1 (bool_t, plugin_get_enabled, PluginHandle *, plugin)
AUD_VFUNC3 (plugin_for_enabled, int, type, PluginForEachFunc, func,
 void *, data)

AUD_FUNC1 (const char *, plugin_get_name, PluginHandle *, plugin)
AUD_FUNC1 (bool_t, plugin_has_about, PluginHandle *, plugin)
AUD_FUNC1 (bool_t, plugin_has_configure, PluginHandle *, plugin)
AUD_VFUNC1 (plugin_do_about, PluginHandle *, plugin)
AUD_VFUNC1 (plugin_do_configure, PluginHandle *, plugin)

AUD_VFUNC3 (plugin_add_watch, PluginHandle *, plugin, PluginForEachFunc,
 func, void *, data)
AUD_VFUNC3 (plugin_remove_watch, PluginHandle *, plugin, PluginForEachFunc,
 func, void *, data)
