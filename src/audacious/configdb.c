/* THIS FILE IS DEPRECATED.  Use the new aud_set/get functions instead. */

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
#include <stdlib.h>
#include <string.h>

#include "misc.h"

gboolean xxx_config_is_set (const gchar * section, const gchar * name);

/**
 * Opens the configuration database.
 *
 * @return A configuration database handle pointer.
 */
mcs_handle_t *
cfg_db_open()
{
    return (void *) 0xdeadbeef;
}

/**
 * Closes the configuration database.
 * @param[in] db A configuration database handle pointer.
 */
void cfg_db_close (mcs_handle_t * handle)
{
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
    if (! xxx_config_is_set (section, key))
        return FALSE;

    * value = get_string (section, key);
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
    if (! xxx_config_is_set (section, key))
        return FALSE;

    * value = get_int (section, key);
    return TRUE;
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
    if (! xxx_config_is_set (section, key))
        return FALSE;

    * value = get_bool (section, key);
    return TRUE;
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
    if (! xxx_config_is_set (section, key))
        return FALSE;

    * value = get_double (section, key);
    return TRUE;
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
    if (! xxx_config_is_set (section, key))
        return FALSE;

    * value = get_double (section, key);
    return TRUE;
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
    set_string (section, key, value ? value : "");
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
    set_int (section, key, value);
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
    set_bool (section, key, value);
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
    set_double (section, key, value);
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
    set_double (section, key, value);
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
}
