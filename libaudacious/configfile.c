/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2002  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2002  Haavard Kvaalen
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#if defined(HAVE_CONFIG_H)
#include "../config.h"
#endif

/* bypass the poisoning of the symbols we need */
#define I_AM_A_THIRD_PARTY_DEVELOPER_WHO_NEEDS_TO_BE_KICKED_IN_THE_HEAD_BY_CHUCK_NORRIS

#include "configfile.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include <unistd.h>
#include <sys/stat.h>

typedef gboolean(*XmmsCfgValueReadFunc) (ConfigFile * config_file,
                                         const gchar * section,
                                         const gchar * key,
                                         gpointer * value);

typedef void (*XmmsCfgValueWriteFunc) (ConfigFile * config_file,
                                       const gchar * section,
                                       const gchar * key,
                                       gpointer * value);

struct _XmmsCfgValueTypeInfo {
    XmmsCfgValueReadFunc read;
    XmmsCfgValueWriteFunc write;
};

typedef struct _XmmsCfgValueTypeInfo XmmsCfgValueTypeInfo;


static XmmsCfgValueTypeInfo xmms_cfg_value_type_func[] = {
    {(XmmsCfgValueReadFunc) xmms_cfg_read_int,
     (XmmsCfgValueWriteFunc) xmms_cfg_write_int},
    {(XmmsCfgValueReadFunc) xmms_cfg_read_float,
     (XmmsCfgValueWriteFunc) xmms_cfg_write_float},
    {(XmmsCfgValueReadFunc) xmms_cfg_read_boolean,
     (XmmsCfgValueWriteFunc) xmms_cfg_write_boolean},
    {(XmmsCfgValueReadFunc) xmms_cfg_read_string,
     (XmmsCfgValueWriteFunc) xmms_cfg_write_string}
};


static ConfigSection *xmms_cfg_create_section(ConfigFile * cfg,
                                              const gchar * name);
static ConfigLine *xmms_cfg_create_string(ConfigSection * section,
                                          const gchar * key,
                                          const gchar * value);
static ConfigSection *xmms_cfg_find_section(ConfigFile * cfg,
                                            const gchar * name);
static ConfigLine *xmms_cfg_find_string(ConfigSection * section,
                                        const gchar * key);


ConfigFile *xmms_cfg_new(void)
{
    return g_new0(ConfigFile, 1);
}

gboolean xmms_cfg_read_value(ConfigFile * config_file,
                             const gchar * section, const gchar * key,
                             XmmsCfgValueType value_type, gpointer * value)
{
    return xmms_cfg_value_type_func[value_type].read(config_file,
                                                     section, key, value);
}

void xmms_cfg_write_value(ConfigFile * config_file,
                          const gchar * section, const gchar * key,
                          XmmsCfgValueType value_type, gpointer * value)
{
    xmms_cfg_value_type_func[value_type].read(config_file,
                                              section, key, value);
}

ConfigFile *xmms_cfg_open_file(const gchar * filename)
{
    ConfigFile *cfg;

    gchar *buffer, **lines, *tmp;
    gint i;
    ConfigSection *section = NULL;

    g_return_val_if_fail(filename != NULL, FALSE);

    if (!g_file_get_contents(filename, &buffer, NULL, NULL))
        return NULL;

    cfg = g_malloc0(sizeof(ConfigFile));
    lines = g_strsplit(buffer, "\n", 0);
    g_free(buffer);
    i = 0;
    while (lines[i]) {
        if (lines[i][0] == '[') {
            if ((tmp = strchr(lines[i], ']'))) {
                *tmp = '\0';
                section = xmms_cfg_create_section(cfg, &lines[i][1]);
            }
        } else if (lines[i][0] != '#' && section) {
            if ((tmp = strchr(lines[i], '='))) {
                *tmp = '\0';
                tmp++;
                xmms_cfg_create_string(section, lines[i], tmp);
            }
        }
        i++;
    }
    g_strfreev(lines);
    return cfg;
}

gchar *xmms_cfg_get_default_filename(void)
{
    static gchar *filename = NULL;
    if (!filename)
        filename =
            g_strconcat(g_get_home_dir(), "/", BMP_RCPATH, "/config",
                        NULL);
    return filename;
}

ConfigFile *xmms_cfg_open_default_file(void)
{
    ConfigFile *ret;

    ret = xmms_cfg_open_file(xmms_cfg_get_default_filename());
    if (!ret)
        ret = xmms_cfg_new();
    return ret;
}

gboolean xmms_cfg_write_file(ConfigFile * cfg, const gchar * filename)
{
    FILE *file;
    GList *section_list, *line_list;
    ConfigSection *section;
    ConfigLine *line;

    g_return_val_if_fail(cfg != NULL, FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);

    if (!(file = fopen(filename, "w")))
        return FALSE;

    section_list = cfg->sections;
    while (section_list) {
        section = (ConfigSection *) section_list->data;
        if (section->lines) {
            fprintf(file, "[%s]\n", section->name);
            line_list = section->lines;
            while (line_list) {
                line = (ConfigLine *) line_list->data;
                fprintf(file, "%s=%s\n", line->key, line->value);
                line_list = g_list_next(line_list);
            }
            fprintf(file, "\n");
        }
        section_list = g_list_next(section_list);
    }
    fclose(file);
    return TRUE;
}

gboolean xmms_cfg_write_default_file(ConfigFile * cfg)
{
    return xmms_cfg_write_file(cfg, xmms_cfg_get_default_filename());
}

gboolean xmms_cfg_read_string(ConfigFile * cfg, const gchar * section,
                              const gchar * key, gchar ** value)
{
    ConfigSection *sect;
    ConfigLine *line;

    g_return_val_if_fail(cfg != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!(sect = xmms_cfg_find_section(cfg, section)))
        return FALSE;
    if (!(line = xmms_cfg_find_string(sect, key)))
        return FALSE;
    *value = g_strdup(line->value);
    return TRUE;
}

gboolean xmms_cfg_read_int(ConfigFile * cfg, const gchar * section,
                           const gchar * key, gint * value)
{
    gchar *str;

    g_return_val_if_fail(cfg != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!xmms_cfg_read_string(cfg, section, key, &str))
        return FALSE;
    *value = atoi(str);
    g_free(str);

    return TRUE;
}

gboolean xmms_cfg_read_boolean(ConfigFile * cfg,
                               const gchar * section, const gchar * key,
                               gboolean * value)
{
    gchar *str;

    g_return_val_if_fail(cfg != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!xmms_cfg_read_string(cfg, section, key, &str))
        return FALSE;
    if (!strcasecmp(str, "TRUE"))
        *value = TRUE;
    else
        *value = FALSE;
    g_free(str);
    return TRUE;
}

gboolean xmms_cfg_read_float(ConfigFile * cfg,
                             const gchar * section, const gchar * key,
                             gfloat * value)
{
    gchar *str, *locale;

    g_return_val_if_fail(cfg != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!xmms_cfg_read_string(cfg, section, key, &str))
        return FALSE;

    locale = g_strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    *value = strtod(str, NULL);
    setlocale(LC_NUMERIC, locale);
    g_free(locale);
    g_free(str);

    return TRUE;
}

gboolean xmms_cfg_read_double(ConfigFile * cfg,
                              const gchar * section, const gchar * key,
                              gdouble * value)
{
    gchar *str, *locale;

    g_return_val_if_fail(cfg != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!xmms_cfg_read_string(cfg, section, key, &str))
        return FALSE;

    locale = g_strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    *value = strtod(str, NULL);
    setlocale(LC_NUMERIC, locale);
    g_free(locale);
    g_free(str);

    return TRUE;
}

void xmms_cfg_write_string(ConfigFile * cfg,
                           const gchar * section, const gchar * key,
                           gchar * value)
{
    ConfigSection *sect;
    ConfigLine *line;

    g_return_if_fail(cfg != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);
    g_return_if_fail(value != NULL);

    sect = xmms_cfg_find_section(cfg, section);
    if (!sect)
        sect = xmms_cfg_create_section(cfg, section);
    if ((line = xmms_cfg_find_string(sect, key))) {
        g_free(line->value);
        line->value = g_strstrip(g_strdup(value));
    } else
        xmms_cfg_create_string(sect, key, value);
}

void xmms_cfg_write_int(ConfigFile * cfg,
                        const gchar * section, const gchar * key,
                        gint value)
{
    gchar *strvalue;

    g_return_if_fail(cfg != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    strvalue = g_strdup_printf("%d", value);
    xmms_cfg_write_string(cfg, section, key, strvalue);
    g_free(strvalue);
}

void xmms_cfg_write_boolean(ConfigFile * cfg,
                            const gchar * section, const gchar * key,
                            gboolean value)
{
    g_return_if_fail(cfg != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    if (value)
        xmms_cfg_write_string(cfg, section, key, "TRUE");
    else
        xmms_cfg_write_string(cfg, section, key, "FALSE");
}

void xmms_cfg_write_float(ConfigFile * cfg,
                          const gchar * section, const gchar * key,
                          gfloat value)
{
    gchar *strvalue, *locale;

    g_return_if_fail(cfg != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    locale = g_strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    strvalue = g_strdup_printf("%g", value);
    setlocale(LC_NUMERIC, locale);
    xmms_cfg_write_string(cfg, section, key, strvalue);
    g_free(locale);
    g_free(strvalue);
}

void xmms_cfg_write_double(ConfigFile * cfg,
                           const gchar * section, const gchar * key,
                           gdouble value)
{
    gchar *strvalue, *locale;

    g_return_if_fail(cfg != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    locale = g_strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    strvalue = g_strdup_printf("%g", value);
    setlocale(LC_NUMERIC, locale);
    xmms_cfg_write_string(cfg, section, key, strvalue);
    g_free(locale);
    g_free(strvalue);
}

void xmms_cfg_remove_key(ConfigFile * cfg,
                         const gchar * section, const gchar * key)
{
    ConfigSection *sect;
    ConfigLine *line;

    g_return_if_fail(cfg != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    if ((sect = xmms_cfg_find_section(cfg, section)) != NULL) {
        if ((line = xmms_cfg_find_string(sect, key)) != NULL) {
            g_free(line->key);
            g_free(line->value);
            g_free(line);
            sect->lines = g_list_remove(sect->lines, line);
        }
    }
}

void xmms_cfg_free(ConfigFile * cfg)
{
    ConfigSection *section;
    ConfigLine *line;
    GList *section_list, *line_list;

    if (cfg == NULL)
        return;

    section_list = cfg->sections;
    while (section_list) {
        section = (ConfigSection *) section_list->data;
        g_free(section->name);

        line_list = section->lines;
        while (line_list) {
            line = (ConfigLine *) line_list->data;
            g_free(line->key);
            g_free(line->value);
            g_free(line);
            line_list = g_list_next(line_list);
        }
        g_list_free(section->lines);
        g_free(section);

        section_list = g_list_next(section_list);
    }
    g_list_free(cfg->sections);
    g_free(cfg);
}

static ConfigSection *xmms_cfg_create_section(ConfigFile * cfg,
                                              const gchar * name)
{
    ConfigSection *section;

    section = g_new0(ConfigSection, 1);
    section->name = g_strdup(name);
    cfg->sections = g_list_append(cfg->sections, section);

    return section;
}

static ConfigLine *xmms_cfg_create_string(ConfigSection * section,
                                          const gchar * key,
                                          const gchar * value)
{
    ConfigLine *line;

    line = g_new0(ConfigLine, 1);
    line->key = g_strstrip(g_strdup(key));
    line->value = g_strstrip(g_strdup(value));
    section->lines = g_list_append(section->lines, line);

    return line;
}

static ConfigSection *xmms_cfg_find_section(ConfigFile * cfg,
                                            const gchar * name)
{
    ConfigSection *section;
    GList *list;

    list = cfg->sections;
    while (list) {
        section = (ConfigSection *) list->data;
        if (!strcasecmp(section->name, name))
            return section;
        list = g_list_next(list);
    }
    return NULL;
}

static ConfigLine *xmms_cfg_find_string(ConfigSection * section,
                                        const gchar * key)
{
    ConfigLine *line;
    GList *list;

    list = section->lines;
    while (list) {
        line = (ConfigLine *) list->data;
        if (!strcasecmp(line->key, key))
            return line;
        list = g_list_next(list);
    }
    return NULL;
}
