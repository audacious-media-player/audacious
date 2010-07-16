/*
 * playlist-api.h
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

/* Do not include this file directly; use playlist.h instead. */

/* CAUTION: These functions are not thread safe. */

AUD_FUNC0 (mcs_handle_t *, cfg_db_open)
AUD_FUNC1 (void, cfg_db_close, mcs_handle_t *, db)

AUD_FUNC4 (gboolean, cfg_db_get_string, mcs_handle_t *, db, const gchar *,
 section, const gchar *, key, gchar * *, value)
AUD_FUNC4 (gboolean, cfg_db_get_int, mcs_handle_t *, db, const gchar *, section,
 const gchar *, key, gint *, value)
AUD_FUNC4 (gboolean, cfg_db_get_bool, mcs_handle_t *, db, const gchar *,
 section, const gchar *, key, gboolean *, value)
AUD_FUNC4 (gboolean, cfg_db_get_float, mcs_handle_t *, db, const gchar *,
 section, const gchar *, key, gfloat *, value)
AUD_FUNC4 (gboolean, cfg_db_get_double, mcs_handle_t *, db, const gchar *,
 section, const gchar *, key, gdouble *, value)

AUD_FUNC4 (void, cfg_db_set_string, mcs_handle_t *, db, const gchar *, section,
 const gchar *, key, const gchar *, value)
AUD_FUNC4 (void, cfg_db_set_int, mcs_handle_t *, db, const gchar *, section,
 const gchar *, key, gint, value)
AUD_FUNC4 (void, cfg_db_set_bool, mcs_handle_t *, db, const gchar *, section,
 const gchar *, key, gboolean, value)
AUD_FUNC4 (void, cfg_db_set_float, mcs_handle_t *, db, const gchar *, section,
 const gchar *, key, gfloat, value)
AUD_FUNC4 (void, cfg_db_set_double, mcs_handle_t *, db, const gchar *, section,
 const gchar *, key, gdouble, value)

AUD_FUNC3 (void, cfg_db_unset_key, mcs_handle_t *, db, const gchar *, section,
 const gchar *, key)
