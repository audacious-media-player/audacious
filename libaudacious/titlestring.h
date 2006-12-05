/*
 * Copyright (C) 2001,  Espen Skoglund <esk@ira.uka.de>
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
#ifndef XMMS_TITLESTRING_H
#define XMMS_TITLESTRING_H

#include <glib.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <time.h>

/*
 * Struct which is passed to xmms_get_titlestring().  An input struct
 * is allocated and initialized with XMMS_NEW_TITLEINPUT().  Before
 * passing the struct to xmms_get_titlestring() it should be filled
 * with appropriate field values.
 */

typedef struct {
    gint __size;                /* Set by bmp_title_input_new() */
    gint __version;             /* Ditto */

    gchar *performer;           /* %p */
    gchar *album_name;          /* %a */
    gchar *track_name;          /* %t */
    gint track_number;          /* %n */
    gint year;                  /* %y */
    gchar *date;                /* %d */
    gchar *genre;               /* %g */
    gchar *comment;             /* %c */
    gchar *file_name;           /* %f */
    const gchar *file_ext;      /* %e *//* is not always strdup'ed, see xmms_input_get_song_info and plugins! */
    gchar *file_path;           /* %F */
    gint length;		/* not displayable */
    gchar *formatter;		/* not displayable */
    time_t mtime;
} TitleInput;

typedef TitleInput BmpTitleInput;


/*
 * Using a __size field helps the library functions detect plugins
 * that use a possibly extended version of the struct.  The __version
 * field helps the library detect possible future incompatibilities in
 * the struct layout.
 */

#define XMMS_TITLEINPUT_SIZE	sizeof(TitleInput)
#define XMMS_TITLEINPUT_VERSION	(1)

#define XMMS_NEW_TITLEINPUT(input) G_STMT_START { \
    input = g_new0(TitleInput, 1);                \
    input->__size = XMMS_TITLEINPUT_SIZE;         \
    input->__version = XMMS_TITLEINPUT_VERSION;   \
} G_STMT_END


G_BEGIN_DECLS

TitleInput *bmp_title_input_new(void);
void bmp_title_input_free(BmpTitleInput * input);

gchar *xmms_get_titlestring(const gchar * fmt, TitleInput * input);
GtkWidget *xmms_titlestring_descriptions(gchar * tags, gint columns);

G_END_DECLS

#endif                          /* !XMMS_TITLESTRING_H */
