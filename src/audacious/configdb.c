/*  This program is free software; you can redistribute it and/or modify
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


mcs_handle_t *
cfg_db_open()
{
    if (!mcs_initted)
    {
	mcs_init();
        mcs_initted = TRUE;
    }

    return mcs_new(RCFILE_DEFAULT_SECTION_NAME);
}

void
cfg_db_close(mcs_handle_t * db)
{
    mcs_destroy(db);
}

gboolean
cfg_db_get_string(mcs_handle_t * db,
                      const gchar * section,
                      const gchar * key,
                      gchar ** value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_string(db, section, key, value);
}

gboolean
cfg_db_get_int(mcs_handle_t * db,
                   const gchar * section, const gchar * key, gint * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_int(db, section, key, value);
}

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

void
cfg_db_set_string(mcs_handle_t * db,
                      const gchar * section,
                      const gchar * key,
                      const gchar * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_string(db, section, key, value);
}

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
