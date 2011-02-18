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
AUD_FUNC1 (PluginHandle *, plugin_get_current, gint, type)
AUD_FUNC2 (gboolean, plugin_enable, PluginHandle *, plugin, gboolean, enable)

/* plugin-registry.c */
AUD_FUNC1 (gint, plugin_get_type, PluginHandle *, plugin)
AUD_FUNC1 (const gchar *, plugin_get_filename, PluginHandle *, plugin)
AUD_FUNC1 (gint, plugin_get_number, PluginHandle *, plugin)
AUD_FUNC3 (PluginHandle *, plugin_lookup, gint, type, const gchar *, filename,
 gint, number)

AUD_FUNC1 (const void *, plugin_get_header, PluginHandle *, plugin)
AUD_FUNC1 (PluginHandle *, plugin_by_header, const void *, header)

AUD_FUNC2 (gint, plugin_compare, PluginHandle *, a, PluginHandle *, b)
AUD_FUNC3 (void, plugin_for_each, gint, type, PluginForEachFunc, func, void *,
 data)

AUD_FUNC1 (gboolean, plugin_get_enabled, PluginHandle *, plugin)
AUD_FUNC3 (void, plugin_for_enabled, gint, type, PluginForEachFunc, func,
 void *, data)

AUD_FUNC1 (const gchar *, plugin_get_name, PluginHandle *, plugin)
AUD_FUNC1 (gboolean, plugin_has_about, PluginHandle *, plugin)
AUD_FUNC1 (gboolean, plugin_has_configure, PluginHandle *, plugin)

AUD_FUNC3 (void, plugin_add_watch, PluginHandle *, plugin, PluginForEachFunc,
 func, void *, data)
AUD_FUNC3 (void, plugin_remove_watch, PluginHandle *, plugin, PluginForEachFunc,
 func, void *, data)

/* New in 2.5-alpha2 */
AUD_FUNC1 (PluginHandle *, plugin_by_widget, /* GtkWidget * */ void *, widget)
