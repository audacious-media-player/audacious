/*
 * audacious: Cross-platform multimedia player.
 * rcfile.c: Reading and manipulation of .ini-like files.
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

#include "rcfile.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include <unistd.h>
#include <sys/stat.h>


static RcSection *bmp_rcfile_create_section(RcFile * file,
                                            const gchar * name);
static RcLine *bmp_rcfile_create_string(RcSection * section,
                                        const gchar * key,
                                        const gchar * value);
static RcSection *bmp_rcfile_find_section(RcFile * file, const gchar * name);
static RcLine *bmp_rcfile_find_string(RcSection * section, const gchar * key);

/**
 * bmp_rcfile_new:
 *
 * #RcFile object factory.
 *
 * Return value: A #RcFile object.
 **/
RcFile *
bmp_rcfile_new(void)
{
    return g_new0(RcFile, 1);
}

/**
 * bmp_rcfile_free:
 * @file: A #RcFile object to destroy.
 *
 * #RcFile object destructor.
 **/
void
bmp_rcfile_free(RcFile * file)
{
    RcSection *section;
    RcLine *line;
    GList *section_list, *line_list;

    if (file == NULL)
        return;

    section_list = file->sections;
    while (section_list) {
        section = (RcSection *) section_list->data;
        g_free(section->name);

        line_list = section->lines;
        while (line_list) {
            line = (RcLine *) line_list->data;
            g_free(line->key);
            g_free(line->value);
            g_free(line);
            line_list = g_list_next(line_list);
        }
        g_list_free(section->lines);
        g_free(section);

        section_list = g_list_next(section_list);
    }
    g_list_free(file->sections);
    g_free(file);
}

/**
 * bmp_rcfile_open:
 * @filename: Path to rcfile to open.
 *
 * Opens an rcfile and returns an #RcFile object representing it.
 *
 * Return value: An #RcFile object representing the rcfile given.
 **/
RcFile *
bmp_rcfile_open(const gchar * filename)
{
    RcFile *file;

    gchar *buffer, **lines, *tmp;
    gint i;
    RcSection *section = NULL;

    g_return_val_if_fail(filename != NULL, FALSE);
    g_return_val_if_fail(strlen(filename) > 0, FALSE);

    if (!g_file_get_contents(filename, &buffer, NULL, NULL))
        return NULL;

    file = bmp_rcfile_new();
    lines = g_strsplit(buffer, "\n", 0);
    g_free(buffer);
    i = 0;
    while (lines[i]) {
        if (lines[i][0] == '[') {
            if ((tmp = strchr(lines[i], ']'))) {
                *tmp = '\0';
                section = bmp_rcfile_create_section(file, &lines[i][1]);
            }
        }
        else if (lines[i][0] != '#' && section) {
            if ((tmp = strchr(lines[i], '='))) {
                gchar **frags;
                frags = g_strsplit(lines[i], "=", 2);
                if (strlen(frags[1]) > 0) {
                    bmp_rcfile_create_string(section, frags[0], frags[1]);
                };
		g_strfreev(frags);
            }
        }
        i++;
    }
    g_strfreev(lines);
    return file;
}

/**
 * bmp_rcfile_write:
 * @file: A #RcFile object to write to disk.
 * @filename: A path to write the #RcFile object's data to.
 *
 * Writes the contents of a #RcFile object to disk.
 *
 * Return value: TRUE on success, FALSE otherwise.
 **/
gboolean
bmp_rcfile_write(RcFile * file, const gchar * filename)
{
    FILE *fp;
    GList *section_list, *line_list;
    RcSection *section;
    RcLine *line;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);

    if (!(fp = fopen(filename, "w")))
        return FALSE;

    section_list = file->sections;
    while (section_list) {
        section = (RcSection *) section_list->data;
        if (section->lines) {
            fprintf(fp, "[%s]\n", section->name);
            line_list = section->lines;
            while (line_list) {
                line = (RcLine *) line_list->data;
                fprintf(fp, "%s=%s\n", line->key, line->value);
                line_list = g_list_next(line_list);
            }
            fprintf(fp, "\n");
        }
        section_list = g_list_next(section_list);
    }
    fclose(fp);
    return TRUE;
}

/**
 * bmp_rcfile_read_string:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to look in.
 * @key: The name of the identifier to look up.
 * @value: A pointer to a memory location to place the data.
 *
 * Looks up a value in an RcFile and places it in %value.
 *
 * Return value: TRUE on success, FALSE otherwise.
 **/
gboolean
bmp_rcfile_read_string(RcFile * file, const gchar * section,
                       const gchar * key, gchar ** value)
{
    RcSection *sect;
    RcLine *line;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!(sect = bmp_rcfile_find_section(file, section)))
        return FALSE;
    if (!(line = bmp_rcfile_find_string(sect, key)))
        return FALSE;
    *value = g_strdup(line->value);
    return TRUE;
}

/**
 * bmp_rcfile_read_int:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to look in.
 * @key: The name of the identifier to look up.
 * @value: A pointer to a memory location to place the data.
 *
 * Looks up a value in an RcFile and places it in %value.
 *
 * Return value: TRUE on success, FALSE otherwise.
 **/
gboolean
bmp_rcfile_read_int(RcFile * file, const gchar * section,
                    const gchar * key, gint * value)
{
    gchar *str;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!bmp_rcfile_read_string(file, section, key, &str))
        return FALSE;
    *value = atoi(str);
    g_free(str);

    return TRUE;
}

/**
 * bmp_rcfile_read_bool:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to look in.
 * @key: The name of the identifier to look up.
 * @value: A pointer to a memory location to place the data.
 *
 * Looks up a value in an RcFile and places it in %value.
 *
 * Return value: TRUE on success, FALSE otherwise.
 **/
gboolean
bmp_rcfile_read_bool(RcFile * file, const gchar * section,
                     const gchar * key, gboolean * value)
{
    gchar *str;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!bmp_rcfile_read_string(file, section, key, &str))
        return FALSE;
    if (!strcasecmp(str, "TRUE"))
        *value = TRUE;
    else
        *value = FALSE;
    g_free(str);
    return TRUE;
}

/**
 * bmp_rcfile_read_float:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to look in.
 * @key: The name of the identifier to look up.
 * @value: A pointer to a memory location to place the data.
 *
 * Looks up a value in an RcFile and places it in %value.
 *
 * Return value: TRUE on success, FALSE otherwise.
 **/
gboolean
bmp_rcfile_read_float(RcFile * file, const gchar * section,
                      const gchar * key, gfloat * value)
{
    gchar *str, *locale;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!bmp_rcfile_read_string(file, section, key, &str))
        return FALSE;

    locale = g_strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    *value = strtod(str, NULL);
    setlocale(LC_NUMERIC, locale);
    g_free(locale);
    g_free(str);

    return TRUE;
}

/**
 * bmp_rcfile_read_double:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to look in.
 * @key: The name of the identifier to look up.
 * @value: A pointer to a memory location to place the data.
 *
 * Looks up a value in an RcFile and places it in %value.
 *
 * Return value: TRUE on success, FALSE otherwise.
 **/
gboolean
bmp_rcfile_read_double(RcFile * file, const gchar * section,
                       const gchar * key, gdouble * value)
{
    gchar *str, *locale;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!bmp_rcfile_read_string(file, section, key, &str))
        return FALSE;

    locale = g_strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    *value = strtod(str, NULL);
    setlocale(LC_NUMERIC, locale);
    g_free(locale);
    g_free(str);

    return TRUE;
}

/**
 * bmp_rcfile_write_string:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to set the key in.
 * @key: The name of the identifier to set.
 * @value: The value to set for that identifier.
 *
 * Sets a value in an RcFile for %key.
 **/
void
bmp_rcfile_write_string(RcFile * file, const gchar * section,
                        const gchar * key, const gchar * value)
{
    RcSection *sect;
    RcLine *line;

    g_return_if_fail(file != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);
    g_return_if_fail(value != NULL);

    sect = bmp_rcfile_find_section(file, section);
    if (!sect)
        sect = bmp_rcfile_create_section(file, section);
    if ((line = bmp_rcfile_find_string(sect, key))) {
        g_free(line->value);
        line->value = g_strstrip(g_strdup(value));
    }
    else
        bmp_rcfile_create_string(sect, key, value);
}

/**
 * bmp_rcfile_write_int:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to set the key in.
 * @key: The name of the identifier to set.
 * @value: The value to set for that identifier.
 *
 * Sets a value in an RcFile for %key.
 **/
void
bmp_rcfile_write_int(RcFile * file, const gchar * section,
                     const gchar * key, gint value)
{
    gchar *strvalue;

    g_return_if_fail(file != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    strvalue = g_strdup_printf("%d", value);
    bmp_rcfile_write_string(file, section, key, strvalue);
    g_free(strvalue);
}

/**
 * bmp_rcfile_write_boolean:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to set the key in.
 * @key: The name of the identifier to set.
 * @value: The value to set for that identifier.
 *
 * Sets a value in an RcFile for %key.
 **/
void
bmp_rcfile_write_boolean(RcFile * file, const gchar * section,
                         const gchar * key, gboolean value)
{
    g_return_if_fail(file != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    if (value)
        bmp_rcfile_write_string(file, section, key, "TRUE");
    else
        bmp_rcfile_write_string(file, section, key, "FALSE");
}

/**
 * bmp_rcfile_write_float:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to set the key in.
 * @key: The name of the identifier to set.
 * @value: The value to set for that identifier.
 *
 * Sets a value in an RcFile for %key.
 **/
void
bmp_rcfile_write_float(RcFile * file, const gchar * section,
                       const gchar * key, gfloat value)
{
    gchar *strvalue, *locale;

    g_return_if_fail(file != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    locale = g_strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    strvalue = g_strdup_printf("%g", value);
    setlocale(LC_NUMERIC, locale);
    bmp_rcfile_write_string(file, section, key, strvalue);
    g_free(locale);
    g_free(strvalue);
}

/**
 * bmp_rcfile_write_double:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to set the key in.
 * @key: The name of the identifier to set.
 * @value: The value to set for that identifier.
 *
 * Sets a value in an RcFile for %key.
 **/
void
bmp_rcfile_write_double(RcFile * file, const gchar * section,
                        const gchar * key, gdouble value)
{
    gchar *strvalue, *locale;

    g_return_if_fail(file != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    locale = g_strdup(setlocale(LC_NUMERIC, NULL));
    setlocale(LC_NUMERIC, "C");
    strvalue = g_strdup_printf("%g", value);
    setlocale(LC_NUMERIC, locale);
    bmp_rcfile_write_string(file, section, key, strvalue);
    g_free(locale);
    g_free(strvalue);
}

/**
 * bmp_rcfile_remove_key:
 * @file: A #RcFile object to write to disk.
 * @section: The section of the RcFile to set the key in.
 * @key: The name of the identifier to remove.
 *
 * Removes %key from an #RcFile object.
 **/
void
bmp_rcfile_remove_key(RcFile * file, const gchar * section, const gchar * key)
{
    RcSection *sect;
    RcLine *line;

    g_return_if_fail(file != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    if ((sect = bmp_rcfile_find_section(file, section)) != NULL) {
        if ((line = bmp_rcfile_find_string(sect, key)) != NULL) {
            g_free(line->key);
            g_free(line->value);
            g_free(line);
            sect->lines = g_list_remove(sect->lines, line);
        }
    }
}

static RcSection *
bmp_rcfile_create_section(RcFile * file, const gchar * name)
{
    RcSection *section;

    section = g_new0(RcSection, 1);
    section->name = g_strdup(name);
    file->sections = g_list_append(file->sections, section);

    return section;
}

static RcLine *
bmp_rcfile_create_string(RcSection * section,
                         const gchar * key, const gchar * value)
{
    RcLine *line;

    line = g_new0(RcLine, 1);
    line->key = g_strstrip(g_strdup(key));
    line->value = g_strstrip(g_strdup(value));
    section->lines = g_list_append(section->lines, line);

    return line;
}

static RcSection *
bmp_rcfile_find_section(RcFile * file, const gchar * name)
{
    RcSection *section;
    GList *list;

    list = file->sections;
    while (list) {
        section = (RcSection *) list->data;
        if (!strcasecmp(section->name, name))
            return section;
        list = g_list_next(list);
    }
    return NULL;
}

static RcLine *
bmp_rcfile_find_string(RcSection * section, const gchar * key)
{
    RcLine *line;
    GList *list;

    list = section->lines;
    while (list) {
        line = (RcLine *) list->data;
        if (!strcasecmp(line->key, key))
            return line;
        list = g_list_next(list);
    }
    return NULL;
}
