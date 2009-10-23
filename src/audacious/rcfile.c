/*  Audacious
 *  Copyright (c) 2005-2007  Audacious team
 *
 *  BMP
 *  Copyright (c) 2003-2005  BMP team
 *
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
/**
 * @file rcfile.c
 * Functions for handling INI-style sectionized resource files.
 *
 * @warning Many functions of this API are NOT thread-safe due
 * to use of setlocale()!
 */

#include "rcfile.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>


static RcSection *aud_rcfile_create_section(RcFile * file,
                                            const gchar * name);
static RcLine *aud_rcfile_create_string(RcSection * section,
                                        const gchar * key,
                                        const gchar * value);
static RcSection *aud_rcfile_find_section(RcFile * file, const gchar * name);
static RcLine *aud_rcfile_find_string(RcSection * section, const gchar * key);

/**
 * #RcFile object factory.
 *
 * @return A newly allocated empty #RcFile object.
 */
RcFile *
aud_rcfile_new(void)
{
    return g_new0(RcFile, 1);
}

/**
 * #RcFile object destructor. Deallocates all contents
 * of the RcFile object.
 *
 * @param[in] file A #RcFile object to destroy.
 */
void
aud_rcfile_free(RcFile * file)
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
 * Opens an rcfile and returns an #RcFile object representing it.
 *
 * @param[in] filename Path to rcfile to open.
 * @return An #RcFile object representing the rcfile given.
 */
RcFile *
aud_rcfile_open(const gchar * filename)
{
    RcFile *file;
    gchar *buffer, **lines, *tmp;
    gint i;
    RcSection *section = NULL;

    g_return_val_if_fail(filename != NULL, FALSE);
    g_return_val_if_fail(strlen(filename) > 0, FALSE);

    if (!g_file_get_contents(filename, &buffer, NULL, NULL))
        return NULL;

    file = aud_rcfile_new();
    lines = g_strsplit(buffer, "\n", 0);
    g_free(buffer);
    i = 0;
    while (lines[i]) {
        if (lines[i][0] == '[') {
            if ((tmp = strrchr(lines[i], ']'))) {
                *tmp = '\0';
                section = aud_rcfile_create_section(file, &lines[i][1]);
            }
        }
        else if (lines[i][0] != '#' && section) {
            if ((tmp = strchr(lines[i], '='))) {
                gchar **frags;
                frags = g_strsplit(lines[i], "=", 2);
                if (strlen(frags[1]) > 0) {
                    aud_rcfile_create_string(section, frags[0], frags[1]);
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
 * Writes the contents of a #RcFile object to given file.
 *
 * @param[in] file A #RcFile object to write to disk.
 * @param[in] filename A path to write the #RcFile object's data to.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean
aud_rcfile_write(RcFile * file, const gchar * filename)
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
 * Looks up a string in an RcFile and places it in %value.
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to look in.
 * @param[in] key The name of the identifier to look up.
 * @param[out] value A pointer to a string pointer (gchar *) variable to place the data.
 * @return value: TRUE on success, FALSE otherwise.
 */
gboolean
aud_rcfile_read_string(RcFile * file, const gchar * section,
                       const gchar * key, gchar ** value)
{
    RcSection *sect;
    RcLine *line;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!(sect = aud_rcfile_find_section(file, section)))
        return FALSE;
    if (!(line = aud_rcfile_find_string(sect, key)))
        return FALSE;
    *value = g_strdup(line->value);
    return TRUE;
}

/**
 * Looks up a integer value in an RcFile and places it in %value.
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to look in.
 * @param[in] key The name of the identifier to look up.
 * @param[out] value A pointer to a integer variable to place the data.
 * @return value: TRUE on success, FALSE otherwise.
 */
gboolean
aud_rcfile_read_int(RcFile * file, const gchar * section,
                    const gchar * key, gint * value)
{
    gchar *str;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!aud_rcfile_read_string(file, section, key, &str))
        return FALSE;
    *value = atoi(str);
    g_free(str);

    return TRUE;
}

/**
 * Looks up a boolean value in an RcFile and places it in %value.
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to look in.
 * @param[in] key The name of the identifier to look up.
 * @param[out] value A pointer to a boolean variable to place the data.
 * @return TRUE on success, FALSE otherwise.
 */
gboolean
aud_rcfile_read_bool(RcFile * file, const gchar * section,
                     const gchar * key, gboolean * value)
{
    gchar *str;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!aud_rcfile_read_string(file, section, key, &str))
        return FALSE;
    if (!strcasecmp(str, "TRUE"))
        *value = TRUE;
    else
        *value = FALSE;
    g_free(str);
    return TRUE;
}

/**
 * Looks up a single precision floating point value in
 * an RcFile and places it in %value.
 *
 * @bug This function is not thread-safe due to use of setlocale().
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to look in.
 * @param[in] key The name of the identifier to look up.
 * @param[out] value A pointer to a variable of type gfloat to place the data.
 * @return value: TRUE on success, FALSE otherwise.
 */
gboolean
aud_rcfile_read_float(RcFile * file, const gchar * section,
                      const gchar * key, gfloat * value)
{
    gchar *str, *locale;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!aud_rcfile_read_string(file, section, key, &str))
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
 * Looks up a double precision floating point value in
 * an RcFile and places it in %value.
 *
 * @bug This function is not thread-safe due to use of setlocale().
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to look in.
 * @param[in] key The name of the identifier to look up.
 * @param[out] value A pointer to a variable of type gdouble to place the data.
 * @return value: TRUE on success, FALSE otherwise.
 */
gboolean
aud_rcfile_read_double(RcFile * file, const gchar * section,
                       const gchar * key, gdouble * value)
{
    gchar *str, *locale;

    g_return_val_if_fail(file != NULL, FALSE);
    g_return_val_if_fail(section != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);

    if (!aud_rcfile_read_string(file, section, key, &str))
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
 * Sets a string value in an RcFile for %key.
 *
 * @bug This function is not thread-safe due to use of setlocale().
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to set the key in.
 * @param[in] key The name of the identifier to set.
 * @param[in] value The value to set for that identifier.
 */
void
aud_rcfile_write_string(RcFile * file, const gchar * section,
                        const gchar * key, const gchar * value)
{
    RcSection *sect;
    RcLine *line;

    g_return_if_fail(file != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);
    g_return_if_fail(value != NULL);

    sect = aud_rcfile_find_section(file, section);
    if (!sect)
        sect = aud_rcfile_create_section(file, section);
    if ((line = aud_rcfile_find_string(sect, key))) {
        g_free(line->value);
        line->value = g_strstrip(g_strdup(value));
    }
    else
        aud_rcfile_create_string(sect, key, value);
}

/**
 * Sets a integer value in an RcFile for %key.
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to set the key in.
 * @param[in] key The name of the identifier to set.
 * @param[in] value The value to set for that identifier.
 */
void
aud_rcfile_write_int(RcFile * file, const gchar * section,
                     const gchar * key, gint value)
{
    gchar *strvalue;

    g_return_if_fail(file != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    strvalue = g_strdup_printf("%d", value);
    aud_rcfile_write_string(file, section, key, strvalue);
    g_free(strvalue);
}

/**
 * Sets a boolean value in an RcFile for %key.
 *
 * @bug This function is not thread-safe due to use of setlocale().
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to set the key in.
 * @param[in] key The name of the identifier to set.
 * @param[in] value The value to set for that identifier.
 */
void
aud_rcfile_write_boolean(RcFile * file, const gchar * section,
                         const gchar * key, gboolean value)
{
    g_return_if_fail(file != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    if (value)
        aud_rcfile_write_string(file, section, key, "TRUE");
    else
        aud_rcfile_write_string(file, section, key, "FALSE");
}

/**
 * Sets a single precision floating point value in an RcFile for %key.
 *
 * @bug This function is not thread-safe due to use of setlocale().
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to set the key in.
 * @param[in] key The name of the identifier to set.
 * @param[in] value The value to set for that identifier.
 */
void
aud_rcfile_write_float(RcFile * file, const gchar * section,
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
    aud_rcfile_write_string(file, section, key, strvalue);
    g_free(locale);
    g_free(strvalue);
}

/**
 * Sets a double precision floating point value in an RcFile for %key.
 *
 * @bug This function is not thread-safe due to use of setlocale().
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to set the key in.
 * @param[in] key The name of the identifier to set.
 * @param[in] value The value to set for that identifier.
 */
void
aud_rcfile_write_double(RcFile * file, const gchar * section,
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
    aud_rcfile_write_string(file, section, key, strvalue);
    g_free(locale);
    g_free(strvalue);
}

/**
 * Removes %key from an #RcFile object.
 *
 * @param[in] file A #RcFile object.
 * @param[in] section The section of the RcFile to set the key in.
 * @param[in] key The name of the identifier to remove.
 */
void
aud_rcfile_remove_key(RcFile * file, const gchar * section, const gchar * key)
{
    RcSection *sect;
    RcLine *line;

    g_return_if_fail(file != NULL);
    g_return_if_fail(section != NULL);
    g_return_if_fail(key != NULL);

    if ((sect = aud_rcfile_find_section(file, section)) != NULL) {
        if ((line = aud_rcfile_find_string(sect, key)) != NULL) {
            g_free(line->key);
            g_free(line->value);
            g_free(line);
            sect->lines = g_list_remove(sect->lines, line);
        }
    }
}

static RcSection *
aud_rcfile_create_section(RcFile * file, const gchar * name)
{
    RcSection *section;

    section = g_new0(RcSection, 1);
    section->name = g_strdup(name);
    file->sections = g_list_append(file->sections, section);

    return section;
}

static RcLine *
aud_rcfile_create_string(RcSection * section,
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
aud_rcfile_find_section(RcFile * file, const gchar * name)
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
aud_rcfile_find_string(RcSection * section, const gchar * key)
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
