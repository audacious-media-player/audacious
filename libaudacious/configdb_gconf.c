/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "configdb.h"

#include <string.h>
#include <gconf/gconf-client.h>


#define BMP_CONFIG_BASE_PATH "/apps/audacious"


struct _ConfigDb {
    GConfClient *client;
};


ConfigDb *
bmp_cfg_db_open()
{
    ConfigDb *db;

    db = g_new(ConfigDb, 1);

    db->client = gconf_client_get_default();

    return db;
}

void
bmp_cfg_db_close(ConfigDb * db)
{
    g_object_unref(db->client);
}

static gchar *
build_keypath(const gchar * section,
              const gchar * key)
{
    if (section == NULL) {
        return g_strconcat(BMP_CONFIG_BASE_PATH, "/", key, NULL);
    }
    else {
        return g_strconcat(BMP_CONFIG_BASE_PATH, "/", section, "/",
                           key, NULL);
    }
}

static gboolean
bmp_cfg_db_get_value(ConfigDb * db,
                     const gchar * section,
                     const gchar * key,
                     GConfValue ** value)
{
    gchar *path;

    g_return_val_if_fail(db != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    path = build_keypath(section, key);
    *value = gconf_client_get(db->client, path, NULL);
    g_free(path);

    return (*value != NULL) ? TRUE : FALSE;
}

static void
bmp_cfg_db_set_value(ConfigDb * db,
                     const gchar * section,
                     const gchar * key,
                     GConfValue * value)
{
    gchar *path;

    g_return_if_fail(db != NULL);
    g_return_if_fail(key != NULL);
    g_return_if_fail(value != NULL);

    path = build_keypath(section, key);
    gconf_client_set(db->client, path, value, NULL);
    g_free(path);
}

gboolean
bmp_cfg_db_get_string(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gchar ** value)
{
    GConfValue *cval;

    if (!bmp_cfg_db_get_value(db, section, key, &cval))
        return FALSE;
    if (cval->type != GCONF_VALUE_STRING)
        return FALSE;
    *value = strdup(gconf_value_get_string(cval));
    gconf_value_free(cval);

    return TRUE;
}

gboolean
bmp_cfg_db_get_int(ConfigDb * db,
                   const gchar * section,
                   const gchar * key,
                   gint * value)
{
    GConfValue *cval;

    if (!bmp_cfg_db_get_value(db, section, key, &cval))
        return FALSE;
    if (cval->type != GCONF_VALUE_INT)
        return FALSE;
    *value = gconf_value_get_int(cval);
    gconf_value_free(cval);

    return TRUE;
}

gboolean
bmp_cfg_db_get_bool(ConfigDb * db,
                    const gchar * section,
                    const gchar * key,
                    gboolean * value)
{
    GConfValue *cval;

    if (!bmp_cfg_db_get_value(db, section, key, &cval))
        return FALSE;
    if (cval->type != GCONF_VALUE_BOOL)
        return FALSE;
    *value = gconf_value_get_bool(cval);
    gconf_value_free(cval);

    return TRUE;
}

gboolean
bmp_cfg_db_get_float(ConfigDb * db,
                     const gchar * section,
                     const gchar * key,
                     gfloat * value)
{
    GConfValue *cval;

    if (!bmp_cfg_db_get_value(db, section, key, &cval))
        return FALSE;
    if (cval->type != GCONF_VALUE_FLOAT)
        return FALSE;
    *value = gconf_value_get_float(cval);
    gconf_value_free(cval);

    return TRUE;
}

gboolean
bmp_cfg_db_get_double(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gdouble * value)
{
    GConfValue *cval;

    if (!bmp_cfg_db_get_value(db, section, key, &cval))
        return FALSE;
    if (cval->type != GCONF_VALUE_FLOAT)
        return FALSE;
    *value = gconf_value_get_float(cval);
    gconf_value_free(cval);

    return TRUE;
}

void
bmp_cfg_db_set_string(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gchar * value)
{
    GConfValue *cval;

    cval = gconf_value_new(GCONF_VALUE_STRING);
    gconf_value_set_string(cval, value);
    bmp_cfg_db_set_value(db, section, key, cval);
    gconf_value_free(cval);
}

void
bmp_cfg_db_set_int(ConfigDb * db,
                   const gchar * section,
                   const gchar * key,
                   gint value)
{
    GConfValue *cval;

    cval = gconf_value_new(GCONF_VALUE_INT);
    gconf_value_set_int(cval, value);
    bmp_cfg_db_set_value(db, section, key, cval);
    gconf_value_free(cval);
}

void
bmp_cfg_db_set_bool(ConfigDb * db,
                    const gchar * section,
                    const gchar * key,
                    gboolean value)
{
    GConfValue *cval;

    cval = gconf_value_new(GCONF_VALUE_BOOL);
    gconf_value_set_bool(cval, value);
    bmp_cfg_db_set_value(db, section, key, cval);
    gconf_value_free(cval);
}

void
bmp_cfg_db_set_float(ConfigDb * db,
                     const gchar * section,
                     const gchar * key,
                     gfloat value)
{
    GConfValue *cval;

    cval = gconf_value_new(GCONF_VALUE_FLOAT);
    gconf_value_set_float(cval, value);
    bmp_cfg_db_set_value(db, section, key, cval);
    gconf_value_free(cval);
}

void
bmp_cfg_db_set_double(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gdouble value)
{
    GConfValue *cval;

    cval = gconf_value_new(GCONF_VALUE_FLOAT);
    gconf_value_set_float(cval, value);
    bmp_cfg_db_set_value(db, section, key, cval);
    gconf_value_free(cval);
}

void
bmp_cfg_db_unset_key(ConfigDb * db,
                     const gchar * section,
                     const gchar * key)
{
    gchar *path;

    g_return_if_fail(db != NULL);
    g_return_if_fail(key != NULL);

    path = build_keypath(section, key);
    gconf_client_unset(db->client, path, NULL);
    g_free(path);
}
