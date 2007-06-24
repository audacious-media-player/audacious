/*
 * audacious: Cross-platform multimedia player.
 * configdb.c: Compatibility API for BMP's ConfigDB system.
 *
 * Copyright (c) 2005-2007 Audacious development team.
 * Copyright (c) 2003-2005 BMP development team.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
bmp_cfg_db_open()
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
bmp_cfg_db_close(ConfigDb * db)
{
    mcs_destroy(db->handle);
    g_free(db);
}

gboolean
bmp_cfg_db_get_string(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gchar ** value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_string(db->handle, section, key, value);
}

gboolean
bmp_cfg_db_get_int(ConfigDb * db,
                   const gchar * section, const gchar * key, gint * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_int(db->handle, section, key, value);
}

gboolean
bmp_cfg_db_get_bool(ConfigDb * db,
                    const gchar * section,
                    const gchar * key,
                    gboolean * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_bool(db->handle, section, key, value);
}

gboolean
bmp_cfg_db_get_float(ConfigDb * db,
                     const gchar * section,
                     const gchar * key,
                     gfloat * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_float(db->handle, section, key, value);
}

gboolean
bmp_cfg_db_get_double(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gdouble * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    return mcs_get_double(db->handle, section, key, value);
}

void
bmp_cfg_db_set_string(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      const gchar * value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_string(db->handle, section, key, value);
}

void
bmp_cfg_db_set_int(ConfigDb * db,
                   const gchar * section,
                   const gchar * key,
                   gint value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_int(db->handle, section, key, value);
}

void
bmp_cfg_db_set_bool(ConfigDb * db,
                    const gchar * section,
                    const gchar * key,
                    gboolean value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_bool(db->handle, section, key, value);
}

void
bmp_cfg_db_set_float(ConfigDb * db,
                     const gchar * section,
                     const gchar * key,
                     gfloat value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_float(db->handle, section, key, value);
}

void
bmp_cfg_db_set_double(ConfigDb * db,
                      const gchar * section,
                      const gchar * key,
                      gdouble value)
{
    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_set_double(db->handle, section, key, value);
}

void
bmp_cfg_db_unset_key(ConfigDb * db,
                     const gchar * section,
                     const gchar * key)
{
    g_return_if_fail(db != NULL);
    g_return_if_fail(key != NULL);

    if (!section)
        section = RCFILE_DEFAULT_SECTION_NAME;

    mcs_unset_key(db->handle, section, key);
}
