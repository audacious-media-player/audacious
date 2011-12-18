/*
 * plugins-api.h
 * Copyright 2010 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
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

AUD_FUNC2 (int, plugin_compare, PluginHandle *, a, PluginHandle *, b)
AUD_VFUNC3 (plugin_for_each, int, type, PluginForEachFunc, func, void *,
 data)

AUD_FUNC1 (bool_t, plugin_get_enabled, PluginHandle *, plugin)
AUD_VFUNC3 (plugin_for_enabled, int, type, PluginForEachFunc, func,
 void *, data)

AUD_FUNC1 (const char *, plugin_get_name, PluginHandle *, plugin)
AUD_FUNC1 (bool_t, plugin_has_about, PluginHandle *, plugin)
AUD_FUNC1 (bool_t, plugin_has_configure, PluginHandle *, plugin)

AUD_VFUNC3 (plugin_add_watch, PluginHandle *, plugin, PluginForEachFunc,
 func, void *, data)
AUD_VFUNC3 (plugin_remove_watch, PluginHandle *, plugin, PluginForEachFunc,
 func, void *, data)
