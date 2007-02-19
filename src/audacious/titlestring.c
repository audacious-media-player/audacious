/*
 * Copyright (C) 2001,  Espen Skoglund <esk@ira.uka.de>
 * Copyright (C) 2001,  Haavard Kvaalen <havardk@xmms.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define GETTEXT_PACKAGE PACKAGE_NAME

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "titlestring.h"

#define CHECK(input, field) \
	(((gchar *) &input->field - (gchar *) input) < input->__size)

#define VS(input, field) (CHECK(input, field) ? input->field : NULL)
#define VI(input, field) (CHECK(input, field) ? input->field : 0)

/**
 * bmp_title_input_new:
 *
 * #BmpTitleInput tuple factory.
 *
 * Return value: A #BmpTitleInput object.
 **/
BmpTitleInput *
bmp_title_input_new()
{
    BmpTitleInput *input;
    input = g_new0(BmpTitleInput, 1);
    input->__size = XMMS_TITLEINPUT_SIZE;
    input->__version = XMMS_TITLEINPUT_VERSION;
    input->mtime = -1;
    return input;
}

/**
 * bmp_title_input_free:
 * @input: A #BmpTitleInput tuple to destroy.
 *
 * Destroys a #BmpTitleInput tuple.
 **/
void
bmp_title_input_free(BmpTitleInput * input)
{
    if (input == NULL)
        return;

    if (input->performer != NULL)
        g_free(input->performer);

    if (input->album_name != NULL)
        g_free(input->album_name);

    if (input->track_name != NULL)
        g_free(input->track_name);

    if (input->date != NULL)
        g_free(input->date);

    if (input->genre != NULL)
        g_free(input->genre);

    if (input->comment != NULL)
        g_free(input->comment);

    if (input->file_name != NULL)
        g_free(input->file_name);

    if (input->file_path != NULL)
        g_free(input->file_path);

    g_free(input);
}

/**
 * xmms_get_titlestring:
 * @fmt: A format string.
 * @input: A tuple to use for data.
 *
 * Generates a formatted string from a tuple.
 *
 * Return value: A formatted tuple string.
 **/
gchar *
xmms_get_titlestring(const gchar * fmt, TitleInput * input)
{
    GString *outstr;
    const gchar *string;
    gchar c, convert[16];
    gint numdigits, numpr, val, i;
    gint f_left, f_space, f_zero, someflag, width, precision;
    gboolean did_output = FALSE;
    gchar digits[] = "0123456789";

#define PUTCH(ch) g_string_append_c(outstr, ch)

#define LEFTPAD(num)                            \
    G_STMT_START {                              \
        gint cnt = (num);                       \
        if ( ! f_left && cnt > 0 )              \
            while ( cnt-- > 0 )                 \
                PUTCH(f_zero ? '0' : ' ');      \
    } G_STMT_END;

#define RIGHTPAD(num)                           \
    G_STMT_START {                              \
        gint cnt = (num);                       \
        if ( f_left && cnt > 0 )                \
            while ( cnt-- > 0 )                 \
                PUTCH( ' ' );                   \
    } G_STMT_END;

    if (fmt == NULL || input == NULL)
        return NULL;
    outstr = g_string_new("");

    for (;;) {
        /* Copy characters until we encounter '%'. */
        while ((c = *fmt++) != '%') {
            if (c == '\0')
                goto Done;
            g_string_append_c(outstr, c);
        }

        f_left = f_space = f_zero = 0;
        someflag = 1;


        /* Parse flags. */
        while (someflag) {
            switch (*fmt) {
            case '-':
                f_left = 1;
                fmt++;
                break;
            case ' ':
                f_space = 1;
                fmt++;
                break;
            case '0':
                f_zero = 1;
                fmt++;
                break;
            default:
                someflag = 0;
                break;
            }
        }


        /* Parse field width. */
        if ((c = *fmt) >= '0' && c <= '9') {
            width = 0;
            while ((c = *fmt++) >= '0' && c <= '9') {
                width *= 10;
                width += c - '0';
            }
            fmt--;
        }
        else
            width = -1;


        /* Parse precision. */
        if (*fmt == '.') {
            if ((c = *++fmt) >= '0' && c <= '9') {
                precision = 0;
                while ((c = *fmt++) >= '0' && c <= '9') {
                    precision *= 10;
                    precision += c - '0';
                }
                fmt--;
            }
            else
                precision = -1;
        }
        else
            precision = -1;


        /* Parse format conversion. */
        switch (c = *fmt++) {
        case '}':              /* close optional, just ignore */
            continue;

        case '{':{             /* optional entry: %{n:...%} */
                char n = *fmt++;
                if (!((n == 'a' && VS(input, album_name)) ||
                      (n == 'c' && VS(input, comment)) ||
                      (n == 'd' && VS(input, date)) ||
                      (n == 'e' && VS(input, file_ext)) ||
                      (n == 'f' && VS(input, file_name)) ||
                      (n == 'F' && VS(input, file_path)) ||
                      (n == 'g' && VS(input, genre)) ||
                      (n == 'n' && VI(input, track_number)) ||
                      (n == 'p' && VS(input, performer)) ||
                      (n == 't' && VS(input, track_name)) ||
                      (n == 'y' && VI(input, year)))) {
                    int nl = 0;
                    char c;
                    while ((c = *fmt++))    /* until end of string      */
                        if (c == '}')   /* if end of opt            */
                            if (!nl)
                                break;  /* if outmost indent level  */
                            else
                                --nl;   /* else reduce indent       */
                        else if (c == '{')
                            ++nl;   /* increase indent          */
                }
                else
                    ++fmt;
                break;
            }

        case 'a':
            string = VS(input, album_name);
            goto Print_string;
        case 'c':
            string = VS(input, comment);
            goto Print_string;
        case 'd':
            string = VS(input, date);
            goto Print_string;
        case 'e':
            string = VS(input, file_ext);
            goto Print_string;
        case 'f':
            string = VS(input, file_name);
            goto Print_string;
        case 'F':
            string = VS(input, file_path);
            goto Print_string;
        case 'g':
            string = VS(input, genre);
            goto Print_string;
        case 'n':
            val = VI(input, track_number);
            goto Print_number;
        case 'p':
            string = VS(input, performer);
            goto Print_string;
        case 't':
            string = VS(input, track_name);
	    goto Print_string;
        case 'y':
            val = VI(input, year);
	    goto Print_number;

          Print_string:
            if (string == NULL)
                break;
            did_output = TRUE;

            numpr = 0;
            if (width > 0) {
                /* Calculate printed size. */
                numpr = strlen(string);
                if (precision >= 0 && precision < numpr)
                    numpr = precision;

                LEFTPAD(width - numpr);
            }

            /* Insert string. */
            if (precision >= 0) {
                glong offset_max = precision, offset;
                gchar *uptr = NULL;
                const gchar *tmpstring = string;
                while (precision > 0) {
                    offset = offset_max - precision;
                    uptr = g_utf8_offset_to_pointer(tmpstring, offset);
                    if (*uptr == '\0')
                        break;
                    g_string_append_unichar(outstr, g_utf8_get_char(uptr));
                    precision--;
                }
            }
            else {
                while ((c = *string++) != '\0')
                    PUTCH(c);
            }

            RIGHTPAD(width - numpr);
            break;

          Print_number:
            if (val == 0)
                break;
            if (c != 'N')
                did_output = TRUE;

            /* Create reversed number string. */
            numdigits = 0;
            do {
                convert[numdigits++] = digits[val % 10];
                val /= 10;
            }
            while (val > 0);

            numpr = numdigits > precision ? numdigits : precision;

            /* Insert left padding. */
            if (!f_left && width > numpr) {
                if (f_zero)
                    numpr = width;
                else
                    for (i = width - numpr; i-- > 0;)
                        PUTCH(' ');
            }

            /* Insert zero padding. */
            for (i = numpr - numdigits; i-- > 0;)
                PUTCH('0');

            /* Insert number. */
            while (numdigits > 0)
                PUTCH(convert[--numdigits]);

            RIGHTPAD(width - numpr);
            break;

        case '%':
            PUTCH('%');
            break;

        default:
            PUTCH('%');
            PUTCH(c);
            break;
        }
    }

  Done:
    if (did_output)
        return g_string_free(outstr, FALSE);
    else
        return NULL;
}

struct _TagDescription {
    gchar tag;
    gchar *description;
};

typedef struct _TagDescription TagDescription;

static TagDescription tag_descriptions[] = {
    {'p', N_("Performer/Artist")},
    {'a', N_("Album")},
    {'g', N_("Genre")},
    {'f', N_("File name")},
    {'F', N_("File path")},
    {'e', N_("File extension")},
    {'t', N_("Track name")},
    {'n', N_("Track number")},
    {'d', N_("Date")},
    {'y', N_("Year")},
    {'c', N_("Comment")}
};

gint tag_descriptions_length =
    sizeof(tag_descriptions) / sizeof(TagDescription);

/**
 * xmms_titlestring_descriptions:
 * @tags: A list of formatters to provide.
 * @columns: A number of columns to arrange them in.
 *
 * Generates a box explaining how to use the formatters.
 *
 * Return value: A GtkWidget containing the table.
 **/
GtkWidget *
xmms_titlestring_descriptions(gchar * tags, gint columns)
{
    GtkWidget *table, *label;
    gchar tag_str[5];
    gint num = strlen(tags);
    gint r = 0, c, i;

    g_return_val_if_fail(tags != NULL, NULL);
    g_return_val_if_fail(columns <= num, NULL);

    table = gtk_table_new((num + columns - 1) / columns, columns * 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 2);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);

    for (c = 0; c < columns; c++) {
        for (r = 0; r < (num + columns - 1 - c) / columns; r++) {
            g_snprintf(tag_str, sizeof(tag_str), "%%%c:", *tags);
            label = gtk_label_new(tag_str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
            gtk_table_attach(GTK_TABLE(table), label, 2 * c, 2 * c + 1, r,
                             r + 1, GTK_FILL, GTK_FILL, 0, 0);
            gtk_widget_show(label);

            for (i = 0; i < tag_descriptions_length; i++) {
                if (*tags == tag_descriptions[i].tag) {
                    label = gtk_label_new(_(tag_descriptions[i].description));
                    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
                    gtk_table_attach(GTK_TABLE(table), label, 2 * c + 1,
                                     2 * c + 2, r, r + 1,
                                     GTK_EXPAND | GTK_FILL,
                                     GTK_EXPAND | GTK_FILL, 0, 0);
                    gtk_widget_show(label);
                    break;
                }
            }

            if (i == tag_descriptions_length)
                g_warning("Invalid tag: %c", *tags);

            tags++;
        }

    }

    label = gtk_label_new(_("%{n:...%}: Display \"...\" only if element "
                            "%n is present"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, r + 1,
                     r + 1, r + 2, GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show(label);

    return table;
}
