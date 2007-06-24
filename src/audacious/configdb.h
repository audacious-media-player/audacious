/*
 * audacious: Cross-platform multimedia player.
 * configdb.h: Compatibility API for BMP's ConfigDB system.
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

#ifndef CONFIGDB_H
#define CONFIGDB_H

#include <glib.h>

/**
 * ConfigDb:
 *
 * A configuration database handle, opened with bmp_cfg_db_open().
 **/
typedef struct _ConfigDb ConfigDb;


G_BEGIN_DECLS

    /**
     * bmp_cfg_db_open:
     *
     * Opens the configuration database.
     *
     * Return value: A configuration database handle.
     **/
    ConfigDb *bmp_cfg_db_open();

    /**
     * bmp_cfg_db_close:
     * @db: A configuration database handle.
     *
     * Closes the configuration database.
     **/
    void bmp_cfg_db_close(ConfigDb *db);

    /**
     * bmp_cfg_db_get_string:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to look up.
     * @value: Pointer to a buffer to put the data in.
     *
     * Searches the configuration database for a value.
     *
     * Return value: TRUE if successful, FALSE otherwise.
     **/
    gboolean bmp_cfg_db_get_string(ConfigDb *db,
                                   const gchar *section,
                                   const gchar *key,
                                   gchar **value);

    /**
     * bmp_cfg_db_get_int:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to look up.
     * @value: Pointer to an integer to put the data in.
     *
     * Searches the configuration database for a value.
     *
     * Return value: TRUE if successful, FALSE otherwise.
     **/
    gboolean bmp_cfg_db_get_int(ConfigDb *db,
                                const gchar *section,
                                const gchar *key,
                                gint *value);

    /**
     * bmp_cfg_db_get_bool:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to look up.
     * @value: Pointer to a boolean to put the data in.
     *
     * Searches the configuration database for a value.
     *
     * Return value: TRUE if successful, FALSE otherwise.
     **/
    gboolean bmp_cfg_db_get_bool(ConfigDb *db,
                                 const gchar *section,
                                 const gchar *key,
                                 gboolean *value);

    /**
     * bmp_cfg_db_get_float:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to look up.
     * @value: Pointer to a floating point integer to put the data in.
     *
     * Searches the configuration database for a value.
     *
     * Return value: TRUE if successful, FALSE otherwise.
     **/
    gboolean bmp_cfg_db_get_float(ConfigDb *db,
                                  const gchar *section,
                                  const gchar *key,
                                  gfloat *value);

    /**
     * bmp_cfg_db_get_double:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to look up.
     * @value: Pointer to a double-precision floating point integer to put the data in.
     *
     * Searches the configuration database for a value.
     *
     * Return value: TRUE if successful, FALSE otherwise.
     **/
    gboolean bmp_cfg_db_get_double(ConfigDb *db,
                                   const gchar *section,
                                   const gchar *key,
                                   gdouble *value);

    /**
     * bmp_cfg_db_set_string:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     * @value: Pointer to a buffer containing the data.
     *
     * Sets a value in the configuration database.
     **/
    void bmp_cfg_db_set_string(ConfigDb *db,
                               const gchar *section,
                               const gchar *key,
                               const gchar *value);

    /**
     * bmp_cfg_db_set_int:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     * @value: Pointer to an integer containing the data.
     *
     * Sets a value in the configuration database.
     **/
    void bmp_cfg_db_set_int(ConfigDb *db,
                            const gchar *section,
                            const gchar *key,
                            gint value);

    /**
     * bmp_cfg_db_set_bool:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     * @value: Pointer to a boolean containing the data.
     *
     * Sets a value in the configuration database.
     **/
    void bmp_cfg_db_set_bool(ConfigDb *db,
                             const gchar *section,
                             const gchar *key,
                             gboolean value);

    /**
     * bmp_cfg_db_set_float:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     * @value: Pointer to a floating point integer containing the data.
     *
     * Sets a value in the configuration database.
     **/
    void bmp_cfg_db_set_float(ConfigDb *db,
                              const gchar *section,
                              const gchar *key,
                              gfloat value);

    /**
     * bmp_cfg_db_set_double:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     * @value: Pointer to a double precision floating point integer containing the data.
     *
     * Sets a value in the configuration database.
     **/
    void bmp_cfg_db_set_double(ConfigDb *db,
                               const gchar *section,
                               const gchar *key,
                               gdouble value);

    /**
     * bmp_cfg_db_unset_key:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     *
     * Removes a value from the configuration database.
     **/
    void bmp_cfg_db_unset_key(ConfigDb *db,
                              const gchar *section,
                              const gchar *key);

G_END_DECLS

#endif /* CONFIGDB_H */

