#include <glib.h>
#include "util.h"

/* convert windows time to unix time */
time_t unix_time(guint64 win_time) {
    guint64 t = (guint64) ((win_time / 10000000LL) - 11644473600LL);
    return (time_t) t;
}

guint16 get_year(guint64 win_time) {
    GDate* d = g_date_new();
    g_date_set_time_t(d, unix_time(win_time));
    guint16 year = g_date_get_year(d);
    g_date_free(d);
    return year;
}

Tuple *makeTuple(Tuple *tuple, const gchar* title, const gchar* artist,
        const gchar* comment, const gchar* album,
        const gchar * genre, const gchar* year,
        const gchar* filePath, int tracnr) {

    tuple_associate_string(tuple, FIELD_ARTIST, NULL, artist);
    tuple_associate_string(tuple, FIELD_TITLE, NULL, title);
    tuple_associate_string(tuple, FIELD_COMMENT, NULL, comment);
    tuple_associate_string(tuple, FIELD_ALBUM, NULL, album);
    tuple_associate_string(tuple, FIELD_GENRE, NULL, genre);
    tuple_associate_string(tuple, FIELD_YEAR, NULL, year);
    tuple_associate_int(tuple, FIELD_TRACK_NUMBER, NULL, tracnr);
    tuple_associate_string(tuple, FIELD_FILE_PATH, NULL, filePath);
    return tuple;
}

const gchar* get_complete_filepath(Tuple *tuple) {
    const gchar* filepath;
    const gchar* dir;
    const gchar* file;

    dir = tuple_get_string(tuple, FIELD_FILE_PATH, NULL);
    file = tuple_get_string(tuple, FIELD_FILE_NAME, NULL);
    filepath = g_strdup_printf("%s/%s", dir, file);
    DEBUG("file path = %s\n", filepath);
    return filepath;
}


void printTuple(Tuple *tuple) {
#if WMA_DEBUG
    DEBUG("--------------TUPLE PRINT --------------------\n");
    const gchar* title = tuple_get_string(tuple, FIELD_TITLE, NULL);
    DEBUG("title = %s\n", title);
    /* artist */
    const gchar* artist = tuple_get_string(tuple, FIELD_ARTIST, NULL);
    DEBUG("artist = %s\n", artist);

    /* copyright */
    const gchar* copyright = tuple_get_string(tuple, FIELD_COPYRIGHT, NULL);
    DEBUG("copyright = %s\n", copyright);

    /* comment / description */

    const gchar* comment = tuple_get_string(tuple, FIELD_COMMENT, NULL);
    DEBUG("comment = %s\n", comment);

    /* codec name */
    const gchar* codec_name = tuple_get_string(tuple, FIELD_CODEC, NULL);
    DEBUG("codec = %s\n", codec_name);

    /* album */
    const gchar* album = tuple_get_string(tuple, FIELD_ALBUM, NULL);
    DEBUG("Album = %s\n", album);

    /*track number */
    gint track_nr = tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL);
    DEBUG("Track nr = %d\n", track_nr);

    /* genre */
    const gchar* genre = tuple_get_string(tuple, FIELD_GENRE, NULL);
    DEBUG("Genre = %s \n", genre);

    /* length */
    gint length = tuple_get_int(tuple, FIELD_LENGTH, NULL);
    DEBUG("Length = %d\n", length);

    /* year */
    gint year = tuple_get_int(tuple, FIELD_YEAR, NULL);
    DEBUG("Year = %d\n", year);

    /* quality */
    const gchar* quality = tuple_get_string(tuple, FIELD_QUALITY, NULL);
    DEBUG("quality = %s\n", quality);

    /* path */
    const gchar* path = tuple_get_string(tuple, FIELD_FILE_PATH, NULL);
    DEBUG("path = %s\n", path);

    /* filename */
    const gchar* filename = tuple_get_string(tuple, FIELD_FILE_NAME, NULL);
    DEBUG("filename = %s\n", filename);

    DEBUG("-----------------END---------------------\n");
#endif
}

guint16 fread_16(VFSFile *f) {
    guint16 i;
    if (vfs_fread(&i, 2, 1, f) == 2) {
        return i;
    }
    return -1;
}

guint32 fread_32(VFSFile *f) {
    guint32 i;
    if (vfs_fread(&i, 4, 1, f) == 4) {
        return i;
    }
    return -1;
}

guint64 fread_64(VFSFile *f) {
    guint64 i;
    if (vfs_fread(&i, 8, 1, f) == 8) {
        return i;
    }
    return -1;
}

/*
 * Returns a UTF8 string converted from UTF16(as read from the file)
 * 
 * size is the number of bytes we should read from the file
 * 
 * The string may or may not be NULL-terminated
 * 
 * Valid string representations:
 * "ABC" = [65 00, 66 00, 67 00,00 00], size=8 (NULL-terminated)
 * or                           ^^^^^ 
 * "ABC" = [65 00, 66 00, 67 00], size=6 (not NULL-terminated)
 */
gchar* fread_str(VFSFile *f, int size) {
    gunichar2 *s;
    gchar *res = NULL;

    /*nothing to read*/
    if (size == 0)
        return NULL;

    s = g_new0(gunichar2, size / 2);
    /* if read was successful */
    if (vfs_fread(s, size, 1, f) == size) {

        /* if the string is NULL-terminated */
        if (!(s[size - 2] || s[size - 1])) {
                res = g_utf16_to_utf8(s, -1, NULL, NULL, NULL);
            } else {
            /* we have size defined in bytes, we need count of gunichar2's*/
            res = g_utf16_to_utf8(s, size >> 1, NULL, NULL, NULL);
        }
    } else { /*read failed*/

        g_free(s);
    }
    return res;
}

void seek(VFSFile *f, long pos) {

    vfs_fseek(f, pos, SEEK_SET);
}

void skip(VFSFile *f, int amount) {
    vfs_fseek(f, amount, SEEK_CUR);
}

gchar *read_ASCII(VFSFile *fd, int size)
{
    gchar *value= g_new0(gchar,size);
    vfs_fread(value,size,1,fd);
    return value;
}