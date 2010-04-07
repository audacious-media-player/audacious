/*
 * Copyright 2009 Paula Stanciu
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef _WMA_FMT_H
#define _WMA_FMT_H

#include "guid.h"


#define ASF_HEADER_OBJECT_GUID				"75B22630-668E-11CF-A6D900AA0062CE6C"
#define ASF_FILE_PROPERTIES_OBJECT_GUID			"8CABDCA1-A947-11CF-8EE400C00C205365"
#define ASF_STREAM_PROPERTIES_OBJECT_GUID		"B7DC0791-A9B7-11CF-8EE600C00C205365"
#define ASF_HEADER_EXTENSION_OBJECT_GUID		"5FBF03B5-A92E-11CF-8EE300C00C205365"
#define ASF_CODEC_LIST_OBJECT_GUID			"86D15240-311D-11D0-A3A400A0C90348F6"
#define ASF_SCRIPT_COMMAND_OBJECT_GUID			"1EFB1A30-0B62-11D0-A39B00A0C90348F6"
#define ASF_MARKER_OBJECT_GUID				"F487CD01-A951-11CF-8EE600C00C205365"
#define ASF_BITRATE_MUTUAL_EXCLUSION_OBJECT_GUID	"D6E229DC-35DA-11D1-903400A0C90349BE"
#define ASF_ERROR_CORRECTION_OBJECT_GUID		"75B22635-668E-11CF-A6D900AA0062CE6C"
#define ASF_CONTENT_DESCRIPTION_OBJECT_GUID		"75B22633-668E-11CF-A6D900AA0062CE6C"
#define ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT_GUID	"D2D0A440-E307-11D2-97F000A0C95EA850"
#define ASF_CONTENT_BRANDING_OBJECT_GUID		"2211B3FA-BD23-11D2-B4B700A0C955FC6E"
#define ASF_STREAM_BITRATE_PROPERTIES_OBJECT_GUID	"7BF875CE-468D-11D1-8D82006097C9A2B2"
#define ASF_CONTENT_ENCRYPTION_OBJECT_GUID		"2211B3FB-BD23-11D2-B4B700A0C955FC6E"
#define ASF_EXTENDED_CONTENT_ENCRYPTION_OBJECT_GUID	"298AE614-2622-4C17-B935DAE07EE9289C"
#define ASF_DIGITAL_SIGNATURE_OBJECT_GUID		"2211B3FC-BD23-11D2-B4B700A0C955FC6E"
#define ASF_PADDING_OBJECT_GUID				"1806D474-CADF-4509-A4BA9AABCB96AAE8"

typedef enum {
    ASF_HEADER_OBJECT = 0,
    ASF_FILE_PROPERTIES_OBJECT, /*              1 */
    ASF_STREAM_PROPERTIES_OBJECT,
    ASF_HEADER_EXTENSION_OBJECT,
    ASF_CODEC_LIST_OBJECT,
    ASF_SCRIPT_COMMAND_OBJECT, /*               5 */
    ASF_MARKER_OBJECT,
    ASF_BITRATE_MUTUAL_EXCLUSION_OBJECT,
    ASF_ERROR_CORRECTION_OBJECT,
    ASF_CONTENT_DESCRIPTION_OBJECT,
    ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT, /* 10*/
    ASF_CONTENT_BRANDING_OBJECT,
    ASF_STREAM_BITRATE_PROPERTIES_OBJECT,
    ASF_CONTENT_ENCRYPTION_OBJECT,
    ASF_EXTENDED_CONTENT_ENCRYPTION_OBJECT,
    ASF_DIGITAL_SIGNATURE_OBJECT, /*            15 */
    ASF_PADDING_OBJECT,
    ASF_OBJECT_LAST /*                          dummy */
} ObjectType;

#define DESC_ALBUM_STR "WM/AlbumTitle"
#define DESC_YEAR_STR "WM/Year"
#define DESC_GENRE_STR "WM/Genre"
#define DESC_TRACK_STR "WM/TrackNumber"

typedef enum {
    DESC_ALBUM = 0,
    DESC_YEAR,
    DESC_GENRE,
    DESC_TRACK,
    DESC_LAST
} DescrIndexes;

/*
 * this should be fine for all headers whose content is irrelevant,
 * but the size is needed so that we can skip it
 */
typedef struct _generic_header {
    GUID *guid;
    guint64 size;
    gchar *data;
} GenericHeader;

typedef struct _header_object {
    GUID *guid;
    guint64 size;
    guint32 objectsNr;
    guint8 res1;
    guint8 res2;
} HeaderObj;

/*
 * this is special, its size does not include the size of the ext_data
 */
typedef struct _header_extension_object {
    GUID *guid;
    guint64 size;
    guint32 objects_count;
    guint8 res1;
    guint8 res2;
    guint32 ext_data_size;
    gchar *ext_data;
} HeaderExtensionObject;

typedef struct _file_properties_header {
    GUID *guid;
    guint64 size;
    gchar dontcare1[16];
    guint64 duration; //expressed as the count of 100ns intervals
    gchar dontcare2[32];
} FilePropertiesHeader;

typedef struct _content_description_object {
    GUID *guid;
    guint64 size;
    guint16 title_length;
    guint16 author_length;
    guint16 copyright_length; /* dontcare*/
    guint16 desc_length;
    guint16 rating_length; /* dontcare*/
    gunichar2 *title;
    gunichar2 *author;
    gunichar2 *copyright; /* dontcare*/
    gunichar2 *description;
    gunichar2 *rating; /* dontcare*/
} ContentDescrObj;

/* descr_val_type's meaning
 * Value  Type           length
 * 0x0000 Unicode string varies
 * 0x0001 BYTE array     varies
 * 0x0002 BOOL           32
 * 0x0003 DWORD          32
 * 0x0004 QWORD          64
 * 0x0005 WORD           16
 */
typedef struct _content_descriptor {
    guint16 name_len;
    gunichar2 *name;
    guint16 val_type;
    guint16 val_len;
    gchar * val;
} ContentDescriptor;

typedef struct _extended_content_description_object {
    GUID *guid;
    guint64 size;
    guint16 content_desc_count;
    ContentDescriptor **descriptors;
} ExtContentDescrObj;

typedef struct _content_field {
    guint16 size;
    gunichar2 *strValue;
} ContentField;

#endif /* _WMA_FMT_H */
