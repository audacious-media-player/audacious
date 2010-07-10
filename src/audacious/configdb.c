/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "configdb.h"
#include <libmcs/mcs.h>
#include <stdlib.h>
#include <string.h>


#define RCFILE_DEFAULT_SECTION_NAME "audacious"

static gboolean mcs_initted = FALSE;
static mcs_handle_t * config_handle = NULL;
static gint config_refcount = 0;

/**
 * Opens the configuration database.
 *
 * @return A configuration database handle pointer.
 */
mcs_handle_t *
cfg_db_open()
{
    if (!mcs_initted)
    {
        mcs_init();
        mcs_initted = TRUE;
    }

    if (! config_handle)
        config_handle = mcs_new (RCFILE_DEFAULT_SECTION_NAME);

    config_refcount ++;
    return config_handle;
}

/**
 * Closes the configuration database.
 * @param[in] db A configuration database handle pointer.
 */
void cfg_db_close (mcs_handle_t * handle)
{
    g_return_if_fail (handle == config_handle);
    g_return_if_fail (config_refcount > 0);
    config_refcount --;
}

void cfg_db_flush (void)
{
    if (! config_handle)
        return; /* nothing to do */

    g_return_if_fail (! config_refcount);
    mcs_destroy (config_handle);
    config_handle = NULL;
}

/**
 * Fetches a string from the configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database to search.
 * @param[in] key The name of the field in the configuration database to look up.
 * @param[out] value Pointer to a buffer to put the data in.
 * @return TRUE if successful, FALSE otherwise.
 */
gboolean
cfg_db_get_string(mcs_handle_t * db,
                  const gchar * section,
                  const gchar * key,
                  gchar ** value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    if (! mcs_get_string (db, section, key, value))
        return FALSE;

    /* Prior to 2.3, NULL values were saved as "(null)". -jlindgren */
    if (! strcmp (* value, "(null)"))
    {
        * value = NULL;
        return FALSE;
    }

    return TRUE;
}

/**
 * Fetches a integer value from the configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database to search.
 * @param[in] key The name of the field in the configuration database to look up.
 * @param[out] value Pointer to a buffer to put the data in.
 * @return TRUE if successful, FALSE otherwise.
 */
gboolean
cfg_db_get_int(mcs_handle_t * db,
               const gchar * section, const gchar * key, gint * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_int(db, section, key, value);
}

/**
 * Fetches a boolean value from the configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database to search.
 * @param[in] key The name of the field in the configuration database to look up.
 * @param[out] value Pointer to a buffer to put the data in.
 * @return TRUE if successful, FALSE otherwise.
 */
gboolean
cfg_db_get_bool(mcs_handle_t * db,
                    const gchar * section,
                    const gchar * key,
                    gboolean * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_bool(db, section, key, value);
}

/**
 * Fetches a single precision floating point value from the
 * configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database to search.
 * @param[in] key The name of the field in the configuration database to look up.
 * @param[out] value Pointer to a buffer to put the data in.
 * @return TRUE if successful, FALSE otherwise.
 */
gboolean
cfg_db_get_float(mcs_handle_t * db,
                     const gchar * section,
                     const gchar * key,
                     gfloat * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_float(db, section, key, value);
}

/**
 * Fetches a double precision floating point value from the
 * configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database to search.
 * @param[in] key The name of the field in the configuration database to look up.
 * @param[out] value Pointer to a buffer to put the data in.
 * @return TRUE if successful, FALSE otherwise.
 */
gboolean
cfg_db_get_double(mcs_handle_t * db,
                      const gchar * section,
                      const gchar * key,
                      gdouble * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_double(db, section, key, value);
}

/**
 * Sets string value in given key of given section in
 * the configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database.
 * @param[in] key The name of the field in the configuration database to set.
 * @param[in] value A double precision floating point value.
 */
void
cfg_db_set_string(mcs_handle_t * db,
                      const gchar * section,
                      const gchar * key,
                      const gchar * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    if (value == NULL)
        mcs_unset_key (db, section, key);
    else
        mcs_set_string (db, section, key, value);
}

/**
 * Sets integer value in given key of given section in
 * the configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database.
 * @param[in] key The name of the field in the configuration database to set.
 * @param[in] value A double precision floating point value.
 */
void
cfg_db_set_int(mcs_handle_t * db,
                   const gchar * section,
                   const gchar * key,
                   gint value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_int(db, section, key, value);
}

/**
 * Sets boolean value in given key of given section in
 * the configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database.
 * @param[in] key The name of the field in the configuration database to set.
 * @param[in] value A double precision floating point value.
 */
void
cfg_db_set_bool(mcs_handle_t * db,
                    const gchar * section,
                    const gchar * key,
                    gboolean value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_bool(db, section, key, value);
}

/**
 * Sets single precision floating point value in given key
 * of given section in the configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database.
 * @param[in] key The name of the field in the configuration database to set.
 * @param[in] value A double precision floating point value.
 */
void
cfg_db_set_float(mcs_handle_t * db,
                     const gchar * section,
                     const gchar * key,
                     gfloat value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_float(db, section, key, value);
}

/**
 * Sets double precision floating point value in given key
 * of given section in the configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database.
 * @param[in] key The name of the field in the configuration database to set.
 * @param[in] value A double precision floating point value.
 */
void
cfg_db_set_double(mcs_handle_t * db,
                      const gchar * section,
                      const gchar * key,
                      gdouble value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_double(db, section, key, value);
}

/**
 * Removes a value from the configuration database.
 *
 * @param[in] db A configuration database handle pointer.
 * @param[in] section The section of the configuration database to search.
 * @param[in] key The name of the field in the configuration database to set.
 */
void
cfg_db_unset_key(mcs_handle_t * db,
                     const gchar * section,
                     const gchar * key)
{
    g_return_if_fail(db != NULL);
    g_return_if_fail(key != NULL);

    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_unset_key(db, section, key);
}
