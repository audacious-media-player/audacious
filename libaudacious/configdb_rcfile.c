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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "configdb.h"

#include <string.h>
#include "rcfile.h"


#define RCFILE_DEFAULT_SECTION_NAME "audacious"


struct _ConfigDb
{
    RcFile *file;
    gchar *filename;
    gboolean dirty;
};


ConfigDb *
bmp_cfg_db_open()
{
    ConfigDb *db;

    db = g_new(ConfigDb, 1);
    db->filename = g_build_filename(g_get_home_dir(), BMP_RCPATH, 
                                    "config", NULL);
    db->file = bmp_rcfile_open(db->filename);
    if (!db->file)
        db->file = bmp_rcfile_new();

    db->dirty = FALSE;

    return db;
}

void
bmp_cfg_db_close(ConfigDb * db)
{
    if (db->dirty)
        bmp_rcfile_write(db->file, db->filename);
    bmp_rcfile_free(db->file);
    g_free(db->filename);
}

gboolean
bmp_cfg_db_get_string(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gchar ** value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return bmp_rcfile_read_string(db->file, section, key, value);
}

gboolean
bmp_cfg_db_get_int(ConfigDb * db,
                   const gchar * section, const gchar * key, gint * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return bmp_rcfile_read_int(db->file, section, key, value);
}

gboolean
bmp_cfg_db_get_bool(ConfigDb * db,
                    const gchar * section,
                    const gchar * key,
                    gboolean * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return bmp_rcfile_read_bool(db->file, section, key, value);
}

gboolean
bmp_cfg_db_get_float(ConfigDb * db,
                     const gchar * section,
                     const gchar * key,
                     gfloat * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return bmp_rcfile_read_float(db->file, section, key, value);
}

gboolean
bmp_cfg_db_get_double(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gdouble * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return bmp_rcfile_read_double(db->file, section, key, value);
}

void
bmp_cfg_db_set_string(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gchar * value)
{
    db->dirty = TRUE;

    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    bmp_rcfile_write_string(db->file, section, key, value);
}

void
bmp_cfg_db_set_int(ConfigDb * db,
                   const gchar * section,
                   const gchar * key,
                   gint value)
{
    db->dirty = TRUE;

    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    bmp_rcfile_write_int(db->file, section, key, value);
}

void
bmp_cfg_db_set_bool(ConfigDb * db,
                    const gchar * section,
                    const gchar * key,
                    gboolean value)
{
    db->dirty = TRUE;

    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    bmp_rcfile_write_boolean(db->file, section, key, value);
}

void
bmp_cfg_db_set_float(ConfigDb * db,
                     const gchar * section,
                     const gchar * key,
                     gfloat value)
{
    db->dirty = TRUE;

    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    bmp_rcfile_write_float(db->file, section, key, value);
}

void
bmp_cfg_db_set_double(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gdouble value)
{
    db->dirty = TRUE;

    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    bmp_rcfile_write_double(db->file, section, key, value);
}

void
bmp_cfg_db_unset_key(ConfigDb * db,
                     const gchar * section,
                     const gchar * key)
{
    db->dirty = TRUE;

    g_return_if_fail(db != NULL);
    g_return_if_fail(key != NULL);

    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    bmp_rcfile_remove_key(db->file, section, key);
}
