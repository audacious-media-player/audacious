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

/* effect.c */
AUD_FUNC2 (void, effect_plugin_enable, PluginHandle *, plugin, gboolean, enable)

/* general.c */
AUD_FUNC2 (void, general_plugin_enable, PluginHandle *, plugin, gboolean, enable)

/* main.c */
AUD_FUNC0 (PluginHandle *, iface_plugin_get_active)
AUD_FUNC1 (void, iface_plugin_set_active, PluginHandle *, plugin)

/* plugin-registry.c */
AUD_FUNC4 (void, plugin_get_path, PluginHandle *, plugin, const gchar * *, path,
 gint *, type, gint *, number)
AUD_FUNC3 (PluginHandle *, plugin_by_path, const gchar *, path, gint, type,
 gint, number)
AUD_FUNC1 (void *, plugin_get_header, PluginHandle *, plugin)
AUD_FUNC1 (PluginHandle *, plugin_by_header, void *, header)
AUD_FUNC2 (gint, plugin_compare, PluginHandle *, a, PluginHandle *, b)
AUD_FUNC3 (void, plugin_for_each, gint, type, PluginForEachFunc, func, void *,
 data)
AUD_FUNC1 (const gchar *, plugin_get_name, PluginHandle *, plugin)
AUD_FUNC1 (gboolean, plugin_has_about, PluginHandle *, plugin)
AUD_FUNC1 (gboolean, plugin_has_configure, PluginHandle *, plugin)
AUD_FUNC1 (gboolean, plugin_get_enabled, PluginHandle *, plugin)
AUD_FUNC2 (void, plugin_set_enabled, PluginHandle *, plugin, gboolean, enabled)
AUD_FUNC3 (void, plugin_for_enabled, gint, type, PluginForEachFunc, func,
 void *, data)

/* visualization.c */
AUD_FUNC2 (void, vis_plugin_enable, PluginHandle *, plugin, gboolean, enable)
