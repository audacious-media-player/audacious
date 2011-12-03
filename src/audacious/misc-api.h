/*
 * misc-api.h
 * Copyright 2010-2011 John Lindgren
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

/* CAUTION: Many of these functions are not thread safe. */

/* config.c */

AUD_VFUNC1 (config_clear_section, const gchar *, section)
AUD_VFUNC2 (config_set_defaults, const gchar *, section, const gchar * const *, entries)

AUD_VFUNC3 (set_string, const gchar *, section, const gchar *, name, const gchar *, value)
AUD_FUNC2 (gchar *, get_string, const gchar *, section, const gchar *, name)
AUD_VFUNC3 (set_bool, const gchar *, section, const gchar *, name, gboolean, value)
AUD_FUNC2 (gboolean, get_bool, const gchar *, section, const gchar *, name)
AUD_VFUNC3 (set_int, const gchar *, section, const gchar *, name, gint, value)
AUD_FUNC2 (gint, get_int, const gchar *, section, const gchar *, name)
AUD_VFUNC3 (set_double, const gchar *, section, const gchar *, name, gdouble, value)
AUD_FUNC2 (gdouble, get_double, const gchar *, section, const gchar *, name)

/* credits.c */
AUD_VFUNC3 (get_audacious_credits, const gchar * *, brief,
 const gchar * * *, credits, const gchar * * *, translators)

/* equalizer.c */
AUD_VFUNC1 (eq_set_bands, const gdouble *, values)
AUD_VFUNC1 (eq_get_bands, gdouble *, values)
AUD_VFUNC2 (eq_set_band, gint, band, gdouble, value)
AUD_FUNC1 (gdouble, eq_get_band, gint, band)

/* equalizer_preset.c */
AUD_FUNC1 (GList *, equalizer_read_presets, const gchar *, basename)
AUD_FUNC2 (gboolean, equalizer_write_preset_file, GList *, list, const gchar *,
 basename)
AUD_FUNC1 (EqualizerPreset *, load_preset_file, const gchar *, filename)
AUD_FUNC2 (gboolean, save_preset_file, EqualizerPreset *, preset, const gchar *,
 filename)
AUD_FUNC1 (GList *, import_winamp_eqf, VFSFile *, file)

/* history.c */
AUD_FUNC1 (const gchar *, history_get, gint, entry)
AUD_VFUNC1 (history_add, const gchar *, path)

/* interface.c */
AUD_VFUNC1 (interface_install_toolbar, void *, button)
AUD_VFUNC1 (interface_uninstall_toolbar, void *, button)

/* main.c */
AUD_FUNC1 (const gchar *, get_path, gint, path)

/* probe.c */
AUD_FUNC2 (PluginHandle *, file_find_decoder, const gchar *, filename, gboolean,
 fast)
AUD_FUNC2 (Tuple *, file_read_tuple, const gchar *, filename, PluginHandle *,
 decoder)
AUD_FUNC4 (gboolean, file_read_image, const gchar *, filename, PluginHandle *,
 decoder, void * *, data, gint *, size)
AUD_FUNC2 (gboolean, file_can_write_tuple, const gchar *, filename,
 PluginHandle *, decoder)
AUD_FUNC3 (gboolean, file_write_tuple, const gchar *, filename, PluginHandle *,
 decoder, const Tuple *, tuple)
AUD_FUNC2 (gboolean, custom_infowin, const gchar *, filename, PluginHandle *,
 decoder)

/* ui_albumart.c */
AUD_FUNC1 (gchar *, get_associated_image_file, const gchar *, filename)

/* ui_plugin_menu.c */
AUD_FUNC1 (/* GtkWidget * */ void *, get_plugin_menu, gint, id)
AUD_VFUNC4 (plugin_menu_add, gint, id, MenuFunc, func, const gchar *, name,
 const gchar *, icon)
AUD_VFUNC2 (plugin_menu_remove, gint, id, MenuFunc, func)

/* ui_preferences.c */
AUD_VFUNC4 (create_widgets_with_domain, /* GtkWidget * */ void *, box,
 PreferencesWidget *, widgets, gint, count, const gchar *, domain)
AUD_VFUNC0 (show_prefs_window)

/* util.c */
AUD_FUNC2 (gchar *, construct_uri, const gchar *, base, const gchar *, reference)

/* visualization.c */
AUD_VFUNC2 (vis_func_add, gint, type, VisFunc, func)
AUD_VFUNC1 (vis_func_remove, VisFunc, func)
