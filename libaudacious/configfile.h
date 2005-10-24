/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
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
#ifndef XMMS_CONFIGFILE_H
#define XMMS_CONFIGFILE_H

#include <glib.h>


typedef enum {
    XMMS_CFG_INT,
    XMMS_CFG_FLOAT,
    XMMS_CFG_BOOLEAN,
    XMMS_CFG_STRING
} XmmsCfgValueType;


struct _ConfigLine {
    gchar *key;
    gchar *value;
};

struct _ConfigSection {
    gchar *name;
    GList *lines;
};

struct _ConfigFile {
    GList *sections;
};

typedef struct _ConfigLine ConfigLine;
typedef struct _ConfigSection ConfigSection;
typedef struct _ConfigFile ConfigFile;


#ifdef __cplusplus
extern "C" {
#endif

    ConfigFile *xmms_cfg_new(void);
    ConfigFile *xmms_cfg_open_file(const gchar * filename);
    gboolean xmms_cfg_write_file(ConfigFile * cfg, const gchar * filename);
    void xmms_cfg_free(ConfigFile * cfg);
    ConfigFile *xmms_cfg_open_default_file(void);
    gboolean xmms_cfg_write_default_file(ConfigFile * cfg);

    gboolean xmms_cfg_read_value(ConfigFile * config_file,
                                 const gchar * section, const gchar * key,
                                 XmmsCfgValueType value_type,
                                 gpointer * value);

    void xmms_cfg_write_value(ConfigFile * config_file,
                              const gchar * section, const gchar * key,
                              XmmsCfgValueType value_type,
                              gpointer * value);

    gboolean xmms_cfg_read_string(ConfigFile * cfg, const gchar * section,
                                  const gchar * key, gchar ** value);
    gboolean xmms_cfg_read_int(ConfigFile * cfg, const gchar * section,
                               const gchar * key, gint * value);
    gboolean xmms_cfg_read_boolean(ConfigFile * cfg, const gchar * section,
                                   const gchar * key, gboolean * value);
    gboolean xmms_cfg_read_float(ConfigFile * cfg, const gchar * section,
                                 const gchar * key, gfloat * value);
    gboolean xmms_cfg_read_double(ConfigFile * cfg, const gchar * section,
                                  const gchar * key, gdouble * value);

    void xmms_cfg_write_string(ConfigFile * cfg, const gchar * section,
                               const gchar * key, gchar * value);
    void xmms_cfg_write_int(ConfigFile * cfg, const gchar * section,
                            const gchar * key, gint value);
    void xmms_cfg_write_boolean(ConfigFile * cfg, const gchar * section,
                                const gchar * key, gboolean value);
    void xmms_cfg_write_float(ConfigFile * cfg, const gchar * section,
                              const gchar * key, gfloat value);
    void xmms_cfg_write_double(ConfigFile * cfg, const gchar * section,
                               const gchar * key, gdouble value);

    void xmms_cfg_remove_key(ConfigFile * cfg, const gchar * section,
                             const gchar * key);

#ifdef __cplusplus
};
#endif

#endif
