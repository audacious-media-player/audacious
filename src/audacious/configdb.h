/**
 * @file configdb.h
 * Main API for handling Audacious configuration file/database.
 * Contains functions for getting and setting values in different
 * sections of the configuration.
 *
 * @attention This API is mainly a thin wrapper on top of libmcs API.
 */
#ifndef AUDACIOUS_CONFIGDB_H
#define AUDACIOUS_CONFIGDB_H

#include <glib.h>
#include <libmcs/mcs.h>

G_BEGIN_DECLS

mcs_handle_t *cfg_db_open();
void cfg_db_close(mcs_handle_t *db);

gboolean cfg_db_get_string(mcs_handle_t *db, const gchar *section,
                    const gchar *key, gchar **value);

gboolean cfg_db_get_int(mcs_handle_t *db, const gchar *section,
                    const gchar *key, gint *value);

gboolean cfg_db_get_bool(mcs_handle_t *db, const gchar *section,
                    const gchar *key, gboolean *value);

gboolean cfg_db_get_float(mcs_handle_t *db, const gchar *section,
                    const gchar *key, gfloat *value);

gboolean cfg_db_get_double(mcs_handle_t *db, const gchar *section,
                    const gchar *key, gdouble *value);

void cfg_db_set_string(mcs_handle_t *db, const gchar *section,
                    const gchar *key, const gchar *value);

void cfg_db_set_int(mcs_handle_t *db, const gchar *section,
                    const gchar *key, gint value);

void cfg_db_set_bool(mcs_handle_t *db, const gchar *section,
                    const gchar *key, gboolean value);

void cfg_db_set_float(mcs_handle_t *db, const gchar *section,
                    const gchar *key, gfloat value);

void cfg_db_set_double(mcs_handle_t *db, const gchar *section,
                    const gchar *key, gdouble value);

void cfg_db_unset_key(mcs_handle_t *db, const gchar *section,
                    const gchar *key);

G_END_DECLS

#endif /* AUDACIOUS_CONFIGDB_H */

