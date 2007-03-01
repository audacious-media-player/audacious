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

/**
 * TitleInput:
 * @__size: Private field which describes the version of the TitleInput.
 * @__version: Private field which describes the version of the TitleInput.
 * @performer: The performer of the media that the tuple is describing.
 * @album_name: The name of the album that contains the media.
 * @track_name: The title of the media.
 * @track_number: The track number of the media.
 * @year: The year the media was published.
 * @date: The date the media was published.
 * @genre: The genre of the media.
 * @comment: Any comments attached to the media.
 * @file_name: The filename which refers to the media.
 * @file_ext: The file's extension.
 * @file_path: The path that the media is in.
 * @length: The length of the media.
 * @formatter: The format string that should be used.
 * @custom: A custom field for miscellaneous information.
 * @mtime: The last modified time of the file.
 *
 * Tuple which is passed to xmms_get_titlestring().  An input tuple
 * is allocated and initialized with bmp_title_input_new().  Before
 * passing the struct to xmms_get_titlestring() it should be filled
 * with appropriate field values.
 **/
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
    gint length;                /* not displayable */
    gchar *formatter;           /* not displayable */
    gchar *custom;              /* not displayable, for internal use */
    time_t mtime;
} TitleInput;

/**
 * BmpTitleInput:
 *
 * An alternate name for the #TitleInput object.
 **/
typedef TitleInput BmpTitleInput;


/*
 * Using a __size field helps the library functions detect plugins
 * that use a possibly extended version of the struct.  The __version
 * field helps the library detect possible future incompatibilities in
 * the struct layout.
 */

/**
 * XMMS_TITLEINPUT_SIZE:
 *
 * The size of the TitleInput object compiled into the library.
 **/
#define XMMS_TITLEINPUT_SIZE	sizeof(TitleInput)

/**
 * XMMS_TITLEINPUT_VERSION:
 *
 * The version of the TitleInput object compiled into the library.
 **/
#define XMMS_TITLEINPUT_VERSION	(1)

/**
 * XMMS_NEW_TITLEINPUT:
 * @input: A TitleInput to initialize.
 *
 * Initializes a TitleInput object. Included for XMMS compatibility.
 **/
#define XMMS_NEW_TITLEINPUT(input) input = bmp_title_input_new();


G_BEGIN_DECLS

TitleInput *bmp_title_input_new(void);
void bmp_title_input_free(BmpTitleInput * input);

gchar *xmms_get_titlestring(const gchar * fmt, TitleInput * input);
GtkWidget *xmms_titlestring_descriptions(gchar * tags, gint columns);

G_END_DECLS

#endif                          /* !XMMS_TITLESTRING_H */
