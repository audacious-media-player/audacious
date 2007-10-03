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

#include <stdlib.h>
#include <string.h>

#include <libmcs/mcs.h>


#define RCFILE_DEFAULT_SECTION_NAME "audacious"

static gboolean mcs_initted = FALSE;

struct _ConfigDb
{
    mcs_handle_t *handle;
};


ConfigDb *
cfg_db_open()
{
    ConfigDb *db;

    db = g_new(ConfigDb, 1);

    if (!mcs_initted)
    {
	mcs_init();
        mcs_initted = TRUE;
    }

    db->handle = mcs_new(RCFILE_DEFAULT_SECTION_NAME);

    return db;
}

void
cfg_db_close(ConfigDb * db)
{
    mcs_destroy(db->handle);
    g_free(db);
}

gboolean
cfg_db_get_string(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gchar ** value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_string(db->handle, section, key, value);
}

gboolean
cfg_db_get_int(ConfigDb * db,
                   const gchar * section, const gchar * key, gint * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_int(db->handle, section, key, value);
}

gboolean
cfg_db_get_bool(ConfigDb * db,
                    const gchar * section,
                    const gchar * key,
                    gboolean * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_bool(db->handle, section, key, value);
}

gboolean
cfg_db_get_float(ConfigDb * db,
                     const gchar * section,
                     const gchar * key,
                     gfloat * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_float(db->handle, section, key, value);
}

gboolean
cfg_db_get_double(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gdouble * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_double(db->handle, section, key, value);
}

void
cfg_db_set_string(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      const gchar * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_string(db->handle, section, key, value);
}

void
cfg_db_set_int(ConfigDb * db,
                   const gchar * section,
                   const gchar * key,
                   gint value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_int(db->handle, section, key, value);
}

void
cfg_db_set_bool(ConfigDb * db,
                    const gchar * section,
                    const gchar * key,
                    gboolean value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_bool(db->handle, section, key, value);
}

void
cfg_db_set_float(ConfigDb * db,
                     const gchar * section,
                     const gchar * key,
                     gfloat value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_float(db->handle, section, key, value);
}

void
cfg_db_set_double(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gdouble value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_double(db->handle, section, key, value);
}

void
cfg_db_unset_key(ConfigDb * db,
                     const gchar * section,
                     const gchar * key)
{
    g_return_if_fail(db != NULL);
    g_return_if_fail(key != NULL);

    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_unset_key(db->handle, section, key);
}
